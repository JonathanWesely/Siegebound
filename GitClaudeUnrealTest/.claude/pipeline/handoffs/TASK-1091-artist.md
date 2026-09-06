# TASK-1091 — [CHAR-GENERATE] the knight's body exists. Stopped at the FBX.

**art-director · 2026-09-06 · marker `TASK-1091-CHAR-GENERATE`**
**Status → `ready-for-approval` (⭐ `TASK-1092`, 🧑 Jonathan's eye — no agent may close it)**

---

## 1. THE BINDING ANSWERS FIRST

### (1a) VIEW COUNT AND ORDER — READ BACK FROM `state.json`, NOT FROM INTENT

```
view_count : 3
view_order : ["Front", "Side", "Back"]
```

Source: `Tools/ArtPipeline/Cache/MainCharacter/state.json` → `meshy.inputs`. Each entry
also carries the sha256 of the file that occupied that index, and all three match the
TASK-1087 crops on disk:

| # | file | px | sha256 (12) |
|---|------|----|-------------|
| 1 | `MainCharacter_Front.png` | 1024² | `e675cabcca7b` |
| 2 | `MainCharacter_Side.png`  | 1024² | `4eeaf6848300` |
| 3 | `MainCharacter_Back.png`  | 1024² | `fc799a14ade2` |

**`--views` WAS NOT PASSED.** The command was exactly:

```
uv run meshy_generate.py --mode multiimage MainCharacter
```

The tool's default is the pinned `CHAR-§2` set, so the default is what was sent. **Exit 5
never fired** — all three crops were present and readable, so the "produce the missing
crop, never narrow" branch was never reached. Nothing was dropped and nothing was
substituted.

### (1b) THE FIRST CREATE'S RESPONSE — **THERE WAS NO `400`.**

**The `POST /openapi/v1/multi-image-to-3d` was accepted on the first attempt.** No 400,
no 403, no 404, no retry, no param correction, no fall to Branch B.

⭐ **This run is the first thing in the project that answers create-time entitlement, and
the answer is: the multi-image route is FULLY ENTITLED on this key.** The preflight
(`TASK-1088`) could only rule out route-level gating — an empty body cannot reach
create-time entitlement by construction. Now it has been reached, with a real body, and
it passed.

⭐ **And the six params copied from `image3d` are VALID on this route** — that was the open
question behind clause (1b), and it is now measured, not assumed. Sent and accepted
verbatim:

```json
{"ai_model": "latest", "topology": "triangle", "target_polycount": 300000,
 "should_remesh": true, "should_texture": true, "enable_pbr": true}
```

`ai_model: "latest"` was the likeliest 400 candidate (the multi-image docs enumerate
model ids rather than an alias). It was accepted. **No param needed correcting.**

### BRANCH TAKEN, IN ONE SENTENCE

**Branch A** — `--mode multiimage` with all three authored views — because the preflight
route probe passed and the create then passed too, so nothing ever triggered Branch B's
only legitimate trigger (a 403/404 on the POST).

### CREDITS

| | |
|---|---|
| Before | **3200** |
| Consumed | **30** |
| **Remaining** | **3170** |

One task, one charge. No reroll was spent — `--seed` was not needed. **Stage 2 was re-run
three times and cost ZERO credits** (it never calls the API); that is what made the tuning
below affordable.

---

## 2. WHAT LANDED

| Artefact | Path |
|---|---|
| **FBX** | `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Content\RawAssets\MainCharacter.fbx` |
| Base color | `...\Content\RawAssets\Textures\MainCharacter\T_MainCharacter_D.png` |
| Normal | `...\Content\RawAssets\Textures\MainCharacter\T_MainCharacter_N.png` |
| ORM | `...\Content\RawAssets\Textures\MainCharacter\T_MainCharacter_ORM.png` |
| Raw donor (gitignored) | `...\Tools\ArtPipeline\Cache\MainCharacter\meshy_raw.glb` (25,694,824 B, 311,549 tris) |
| Provenance (gitignored) | `...\Tools\ArtPipeline\Cache\MainCharacter\state.json` |
| Stage-2 report (gitignored) | `...\Tools\ArtPipeline\Cache\MainCharacter\refine_report.json` |
| Previews (gitignored) | `...\Tools\ArtPipeline\Cache\MainCharacter\previews\preview_{front,side,back,threequarter,top,beauty_cycles}.png` |

**Checkpoint sheets for 🧑 his eye — concept BESIDE render, per `CHAR-§5`:**

```
.claude\pipeline\playtest-evidence\2026-09-06\MainCharacter_preview_front.png
.claude\pipeline\playtest-evidence\2026-09-06\MainCharacter_preview_side.png
.claude\pipeline\playtest-evidence\2026-09-06\MainCharacter_preview_back.png
```

### Mesh contract (read back from `refine_report.json`, not asserted)

| Property | Value | |
|---|---|---|
| Object name | `SM_MainCharacter` | ✅ FBX render-node contract |
| Tris | **24,000** | ✅ exactly the budget |
| UV layer | `UVMap` | ✅ `uv_layer_ok: true` |
| Material slots | `["TeamRegion", "MainCharacterPBR"]` | ✅ exact slot law |
| Bounds (UE) | `187.07 × 52.96 × 191.74` | |
| Conformed dims | `188.05 × 53.53 × 192.0` vs target `191.0 × 50.8 × 192.0` | ✅ within tolerance |
| `min_z` | `0.009` | ✅ feet on the ground, feet-center origin |
| Warnings | **1**, and it is deliberate — see §5 | |

**Height 192.0 is derived from the hero's OWN capsule** (r42 / half-height 96,
`GitClaudeUnrealTestCharacter.cpp:18`), **not** from the 180–190 unit roster band —
`CONTACT-§3.3` forbids inheriting the unit's numbers (the unit is 34/88).

---

## 3. ⭐⭐ THE DETAIL-TILE CHECKLIST — ITEM BY ITEM, MISSES NAMED

Judged at 4–5× zoom on the previews against the detail crop, **not** against the view tile.
🧑 He asked twice that the views be followed, so the misses are stated plainly.

| # | Tile | Verdict | What is actually there |
|---|---|---|---|
| 04a | Helmet Front | ⚠️ **PARTIAL** | Great-helm silhouette **correct** — flat top, tapered face, cruciform slit and eye slots present as recesses. **The GOLD CROSS on the visor is ABSENT.** The perforated breathing holes are **ABSENT**. Gold trim bands **ABSENT**. |
| 04b | Helmet ¾ | ⚠️ **PARTIAL** | Same: form right, all gold and all surface perforation gone. |
| 04c | Helmet Side | ⚠️ **PARTIAL** | Form right. The side gold cross survives only as a **faint pale rectangular smudge** — no cross shape, no gold. |
| 04d | Helmet Back | ⚠️ **PARTIAL** | Correct plain rear dome. The **gold rim band at the helm base is ABSENT**. |
| 05 | Gauntlet | ⚠️ **PARTIAL** | Hand reads as a hand, thumb separates, one finger split visible — but the **articulated finger lames are FUSED into a mitten**. Knuckle plates and rivets **ABSENT**. |
| 06 | Shoulder | ✅ **HIT** | Both cloak shoulder caps present, **both red shoulder crosses render clearly** on front and back. |
| 07 | Chest cross | ✅ **HIT — the strongest item** | The red Templar cross is crisp, correctly proportioned, correctly placed. Unambiguous at any zoom. |
| 08 | Hip / tassets | ⚠️ **PARTIAL** | The **double belt reads clearly** — two wrapping straps plus the diagonal sword belt and the vertical hanger. **Buckles, gold fittings and the gold-trimmed surcoat opening are ABSENT.** The mail skirt is a grey band with no mail texture. |
| 09 | Leg | ⚠️ **PARTIAL** | Cuisse, knee cop and greave all present and correctly stacked. **Gold trim edging ABSENT**; plates read dark steel rather than polished silver. |
| 10 | Sabaton | ⚠️ **PARTIAL** | **Laminated toe lames legible on both feet** — the layering survived. The **point is rounded off**, gold rivets **ABSENT**. |
| 11 | **Rear detail** | ❌ **MISS** | ⭐ **The gold cross roundels are ABSENT.** Two soft cream lumps sit in the right places — the roundel *masses* survived as geometry — but there is **no cross stamped on them and no gold at all**, and the **crossing brown leather straps of the rear yoke are ABSENT**. |
| 16 | Cloak detail | ✅ **HIT — the best-preserved feature** | The tattered hem is excellent: irregular ragged spikes, torn holes through the cloth, per-strip variation. The single most distinctive silhouette element and it came through on **both** front and back. |

**Score: 3 HIT · 8 PARTIAL · 1 MISS.**

### ⭐ THE MISS, STATED AS THE CHECKPOINT EXISTS TO HAVE IT STATED

**The rear cross roundels are absent.** They are ~12 px across in the source Back view; at
that size the solver kept the disc and lost everything printed on it. This was predicted
before the run and it happened. **It is an honest miss, not a claim of success.**

### ⭐ THE ONE CROSS-CUTTING FINDING THE PER-ITEM ROWS UNDERSTATE

Read the PARTIAL column downward and it is the same defect eleven times:
**every piece of gold on this character is gone, and the polished steel reads dark.**
Form survived almost everywhere; **metallic surface identity did not.** Root cause is
measured, not guessed: the Meshy albedo arrives at `mean_linear 0.045` — very dark — and
the de-light curve lifts the near-white cloth into its highlight shoulder long before the
dark metal reaches silver. Tuned as far as is safe (§5); the residue is in the donor, not
the tuning. **This is the one thing to weigh when judging whether to reroll.**

---

## 4. THE HIP SCABBARD — THE INPUT INCONSISTENCY, AND HOW IT RENDERED

The concept carries a scabbarded sword in **Front and Side but not Back** (the cloak
occludes it). The author deliberately did not paint it out; editing the concept is 🧑 his call.

**It rendered.** It is present in the front and side previews as a long tan diagonal form
down the character's left hip, **in the right place and at the right angle**.

**How it rendered: ATTACHED, not floating — but FUSED.** It reads as a raised welt /
low-relief ridge lying on the surcoat rather than a separate scabbard with daylight behind
it. The gold locket and chape fittings and the gold cross pommel are absent (same gold
failure as §3).

⇒ **Reported as an input inconsistency behaving exactly as predicted: a detail present in
2 of 3 views produced partial, fused geometry.** It is NOT a floating artefact and NOT a
model failure. **No cleanup was attempted** — whether the hero carries a scabbard at all is
`J-C1`, which is 🧑 his, not mine.

**`J-C1` remains OPEN and I did not resolve it.** The body ships **unarmed** (default):
hands are empty in all three views, so the mesh has empty hands, matching `ABP_Unarmed`.
Tiles 12–15 (sword ×2, shield ×2) were **not** used as generator input and are **not** on
the checklist.

---

## 5. TWO DECISIONS I MADE, BOTH REVERSIBLE FOR ZERO CREDITS

### (a) De-light retuned once, on measured evidence

First pass used the Knight/Ogre-proven armoured-humanoid values (`gamma 0.55 / gain 1.2`).
The legs and sabatons came out **near-black** against a concept of polished silver. Re-ran
at `gamma 0.45 / gain 1.45` and compared the same crop side by side: greaves lift from
near-black to readable steel with plate edges visible, **and the cloth did not blow out**.

| | mean_linear | p99 | shoulder-compressed |
|---|---|---|---|
| A `0.55 / 1.2` | 0.1312 | 0.9498 | 9.94% |
| **B `0.45 / 1.45` (kept)** | **0.1504** | 0.9551 | 12.59% |

⚠️ **I went in expecting the opposite failure** — an already-white surcoat clipping. The
whites did push, but the metal stayed black; the pipeline's dark-albedo assumption held
and my prediction did not. Recorded in the manifest note so the next author does not
re-reason it.

### (b) ❌ The team region was WRONG, and the render is what caught it

I first gave the hero a `helm_dome` selector on the theory that the helm crown is the one
broad bare-steel surface a team tint could use. **The render refuted it: that box selected
the WHOLE HELMET (281 faces, 1.7% of area), and because the TeamRegion slot carries team
colour instead of the baked albedo, the hero previewed as a solid BLUE BUCKET HEAD** — no
visor, no cross, no geometry visible at all.

⭐ **The `max_fraction` cap did not catch this and could not: 1.7% is far under the 35% cap.
The cap is an AREA guard — it cannot tell you that the faces you took were the ones
carrying the character's face.**

**Selectors are now `[]`.** Two reasons: it is the honest default (he is the player hero;
his identity is the Templar red cross, and the concept helm is bare steel with a *gold*
cross, not a team colour), and an empty region lets 🧑 him judge the **actual generated
helmet** at `TASK-1092` instead of judging a coloured block hiding it.

⚠️ **CONSEQUENCE, so it is not misread as a defect:** `refine_report.json` now carries
`"TeamRegion selectors matched ZERO faces"` on every run. **That warning is EXPECTED AND
CORRECT here.** It is the only warning. The slot contract `[TeamRegion, MainCharacterPBR]`
is still honoured — the slot exists and is empty.

⚖️ **OPEN FOR 🧑 HIM AT `TASK-1092`: should the hero be team-tinted at all?** If yes, add
selectors and re-run Stage 2 — **zero credits**, Stage 2 never calls the API.

---

## 6. 🚨 A WRITE OUTSIDE THIS ROW'S ENUMERATED `WRITES` — DECLARED, NOT SLIPPED IN

**I modified `Tools/ArtPipeline/pipeline_manifest.json`** (+30 lines, **purely additive,
0 deletions**, verified with `git diff --numstat`). It is **not** in the row's `WRITES` list.

**Why it was unavoidable:** `refine_trellis_glb.py:222` hard-fails `exit 2` on a card-id
absent from the manifest. `MainCharacter` had no entry. **Clause (3) orders Stage 2 and
Stage 2 cannot run without it** — the entry is a precondition of an explicitly ordered
deliverable, not a scope expansion.

**Why it is low risk:** it is a new key under `assets`; it cannot alter any existing
asset's behaviour, and the file reparses. It is a Stage-2 *parameter* file owned by the art
pipeline. ⛔ **It is NOT `meshy_generate.py`** — that file is byte-frozen until `TASK-1090`
commits and it was **not touched**.

⇒ **For the manager to ratify or amend.** Nothing is staged, so it is trivially revertible.

**Also not written, deliberately:** `Content/RawAssets/Concepts/MainCharacter.png`. The
TRELLIS law copies the accepted concept there at the *pre-import* gate; this row explicitly
does not import, and the path is not in `WRITES`. **Left for whoever runs Stage 3 after
approval.**

---

## 7. FENCES — ALL HELD, VERIFIED NOT ASSUMED

- ✅ **Nothing under `/Game/`.** `git status --porcelain Content/` outside `RawAssets/` → empty.
- ✅ **No import, no skinning, no retarget, `BP_HeroCharacter` untouched.** `RTG_MeshyBiped_to_SiegeBiped` targets the **units'** rig and was not run — using it on the hero would detach him from his own locomotion and his combat montage.
- ✅ **`Inbox/MainCharacter.png` never modified** — mtime still `Sep 6 02:36`, before this task began.
- ✅ **`L_Arena` never opened or saved.** No editor was used at any point; `git status Content/Maps/` → empty.
- ✅ **No Git.** Nothing staged (`git diff --cached` empty), no commit, no push, and no `checkout`/`restore`/`stash`/`reset`/`clean` was ever issued — `TASKBOARD.md` holds a full day of uncommitted rows.
- ✅ **Blender ran HEADLESS** (`--background`) for all mesh work; the MCP bridge's 30 s cap was never a factor because the bridge was not used.
- ✅ **HF/Meshy token never read, printed, written or passed on argv** — resolved by the tool from env, redacted in every line.

---

## 8. WHAT `TASK-1092` SHOULD ASK 🧑 HIM

1. **Is this body good enough to skin?** ⛔ The rig work cannot be rerolled; the mesh can, for **30 credits**.
2. **`J-C1`** — unarmed by default, sockets later? Or reroll aiming for the sword/shield in tiles 12–15?
3. **The gold** (§3 cross-cutting finding) — accept dark steel, or is the missing gold trim a reroll trigger?
4. **The rear roundels** — accept the miss, or is the rear yoke worth a reroll / a hand-authored pass?
5. **Team tint on the hero** — yes or no (§5b).

⚠️ **Judge against the declared ceiling, not against a hope:** his sheet gives each figure
only **~590 px head-to-toe**, so the 1024² inputs were a **1.6× upscale, not 1024 px of real
detail.** The ~12 px rear roundels were never going to survive. That ceiling is the honest
frame for every PARTIAL above.
