# TASK-922 — [HOST-ART] the asset host for `TASK-832` — build-master handoff

**Commit: `a8b97b3`** — `TASK-922: the veil material lands with ZERO CALLERS, knowingly - MI_Unit_Invisible + the M_HeroSpirit master edit (TASK-832)`
**Status:** ✅ **done.** ⛔ **NOT PUSHED** — `main` is now **12 ahead** of `origin/main`.
**Law:** `TL-§5e` · `WITCH-§5` (incl. the new application clause) · `GHOST-§5` · `§17a` · `§25b` cl. R + cl. S · `SC-§29` · `SC-§36.1` · `SC-§39.1`.
⛔ **No compile** (assets only) · **zero `Source/` diff** · **suite delta `0`** · ⛔ no `.umap` written · ⛔ no editor save.

---

## 1. ⛔ I COMMITTED AN ASSET WITH ZERO CALLERS, KNOWINGLY — AND THE COMMIT MESSAGE SAYS SO

⛔ **`get_referencers(/Game/Materials/MI_Unit_Invisible)` = `[]`, measured by me in-editor at commit time**, not relayed from the artist. The wiring seam is named but deliberately empty — `SummonedUnit.cpp:2879`, on `BreakInvisibility`'s true edge.

⇒ ⛔ **Nothing renders veiled in play as a result of this commit. No player-visible behaviour changes at all.** That is the boarded decision (`SC-§36.1`'s shape — a built, tested surface shipping ahead of its caller), and the commit message states it in its **second heading** rather than burying it. ⚠️ A message implying the veil is visible in play would have been exactly the defect the help-screen law exists to prevent.

⭐ **Corroboration worth recording: `1aa0fee`'s own message already confessed this gap** — *"TASK-832 (MI_Unit_Invisible) was never dispatched and is still `backlog`; the material does not exist."* ⛔ The host that spent itself **wrote down what it was leaving behind**, and the board still lost it for a day. ⚖️ *A confession in a commit body is not a tracking mechanism; `TL-§5e` cl. 5's count is.*

---

## 2. PATHSPEC vs THE `names:` LINE — reconciled against `git status --porcelain`, ⛔ never the line

| | |
|---|---|
| `names:` line said | `MI_Unit_Invisible` (NEW) + `M_HeroSpirit` (MODIFIED) |
| **`git status` said** | ✅ **the same two, and only those two** |
| **committed** | ✅ **exactly 2 files, by explicit pathspec** |

✅ **They agree — but they were verified independently, and the verification is what found §4's fire.** ⭐ Two independent instruments closed the asset set: my own `git status` sweep of all changed binaries, and the artist's disk-mtime sweep of every `Content/**/*.uasset|umap` written in 75 minutes (**exactly two files, both its own**).

⛔ **`Source/` diff for this commit: ZERO lines**, as spec (1) required. ⚠️ The tree *does* carry 7 modified `Source/` files — ⛔ **all seven are claimed by `TASK-919`** (its pathspec is 6 paths across `SiegeControlsHelpWidget.{h,cpp}` / `SiegePlayerController.{h,cpp}` / `Tests/SiegeControlsHelpTest.cpp` / `Tests/SiegePlacementTest.cpp`, plus conditionally `Tests/SiegeAssistantSelectionTest.cpp`). ⇒ ⛔ **not a finding.**

---

## 3. ⭐⭐ THE GHOST DID NOT REGRESS — RE-MEASURED BY ME ON **THREE** AXES, AND THE THIRD IS THE ONE THAT MATTERS

| # | instrument | result |
|---|---|---|
| 1 | ⭐ **worktree sha256 vs `HEAD`'s OWN LFS POINTER OID** | `fbeb1f5f413851a77139f7208615253f9d6a12dc461bc807ca27869841a22044` **on both sides — full 64-hex, byte-identical** ⇒ ⛔ never written. ⚠️ **By oid, ⛔ NEVER by size.** |
| 2 | its own params, read in-editor | `CoreOpacity` **0.16** · `PulseAmount` **0.12** = ⭐ exactly `TASK-756`'s recorded values |
| 3 | ⭐⭐ **the INHERITED new parameter** | `VeilRefractionStrength` = **0** ⇒ the `+1.0` bias node maps it to **IOR exactly 1.0 = zero bend** |
| 4 | 👁️ **pixels — I opened `TASK-832-D` myself** | the leftmost (ghost) figure renders **crisp and cold-blue with the paving band running straight behind it**; the veiled figures beside it visibly scramble the background |

### ⛔⛔ WHY AXIS 3 IS NOT REDUNDANT WITH AXIS 1 — the sentence to keep

⛔ **Byte-identity proves the ghost's FILE was not written. It proves ⛔ NOTHING about the ghost's APPEARANCE, because the ghost INHERITS from the master that WAS edited.**
⇒ ⭐ **A shared-master edit can regress an instance whose own bytes never move.** `§17a`/`§25b`'s memory-vs-disk-vs-index framing does not reach this at all — it is a **fourth** layer: **parent-vs-child**.

⚠️ **And that is exactly the near-miss that nearly shipped:** `VeilRefractionStrength` was first defaulted to `0` *specifically so existing instances would be untouched* — but in `RM_IndexOfRefraction` the input is an **index of refraction**, whose neutral is **`1.0`**. ⛔ `0` is violently refractive. ⇒ ⛔ the ghost would have silently gained a refraction nobody asked for, with **every success return green**. ✅ Fixed by the artist with the `+1.0` bias node and proven on pixels (`TASK-832-C`).

⚖️ ***A defaulted parameter is not automatically a neutral parameter. Neutrality is a property of the CONSUMING MODE and must be measured there.***

---

## 4. ⛔⛔ `§25b` CLAUSE S PAID AGAIN — AND THE UNPREDICTED FILE WAS THE DANGEROUS ONE, FOR THE FOURTH TIME

⚠️ **Both assets were staged by the editor's revision-control integration, by no task.** I swept **every** changed binary rather than only the one with a recorded prior:

| file | index oid as found | worktree sha256 | verdict |
|---|---|---|---|
| `MI_Unit_Invisible.uasset` | `fea6e0d4…8485` | `fea6e0d4…8485` | ✅ **auto-stage CLEAN** — the law working, ⛔ not a fire |
| ⛔⛔ **`M_HeroSpirit.uasset`** | ⛔ **`0201a7e3…133b`** (= the **PRE-EDIT** master, identical to `HEAD`) | **`4879a679…f348`** | ⛔⛔ **STALE — the modified file did not stage** |

⇒ ⛔⛔ **A bare `git commit` of the index as found would have shipped `MI_Unit_Invisible` — whose ONLY dependency is `M_HeroSpirit` — against the OLD master: `RefractionMethod = RM_None`, no `MP_Refraction` input, and ⛔ none of the three `03 Veil` parameters its own overrides name.** ⭐ **The veil would have existed, read back correctly, and refracted nothing** — ⛔ the *exact* silent-defect species this lane already paid for twice (`RM_2DOffset`; `GHOST-§5`'s `bUsedWithSkeletalMesh`).

⭐ **This is fire #4's shape reproduced precisely: ONE SCC operation, the NEW file staged correctly and the MODIFIED file did not.** ⛔ Sizes also differ (23142 → 28800) — ⛔ **but size was not the instrument; the digest was.**

✅ Fixed by `git add` on the two explicit paths, then **re-verified**: post-stage index oid == worktree sha256 on both, and post-commit **`HEAD` oid == worktree sha256** on all three materials.

---

## 5. ⛔ `§25b` cl. R — THE STANDING RECONCILIATION. ⛔ REPORTED, ⛔ NEVER `git reset`

⛔ **No `git reset` was run. Nothing was swept in.** Committed by explicit pathspec so nothing *could* be. Everything below is **left exactly as found**, reported to the manager.

### 🚨 (a) ⛔⛔ A **THIRD ORPHAN OF THE SAME SHAPE**, AND IT IS THE STOP-THE-LINE ITEM

| path | claimed by | reality |
|---|---|---|
| `Tools/ArtPipeline/concept_prompts.json` (modified) | ⛔ **NO open row** | the **`Witch` entry, seed 71035** |
| `Tools/ArtPipeline/pipeline_manifest.json` (modified) | ⛔ **no row for COMMIT** — `TASK-899` names it only as a *remedy fence* | the **`Witch` block**, every `_source` stamped **"TASK-833 (2026-09-02)"** |

⇒ ⛔⛔ **Both diffs are pure `TASK-833` Witch data. Their lane host `TASK-835` is `done` — `1aa0fee` carried `Content/RawAssets/Concepts/Witch.png` but ⛔ NEITHER JSON.** ⛔ **Identical shape to my own row's origin: the host was spent and the deliverable pointed nowhere.**

⛔ **`TASK-899` CANNOT ABSORB THEM** — it is assigned to **Jonathan** for a ruling, its item (6) fence ends *"⛔ no Git"*, and it may never be dispatched.
⚠️⚠️ **AND IT HAS NOW BEEN REPORTED TWICE AND STILL HAS NO HOST** — `TASK-918`'s item (5) reconciliation already named both JSONs, the same night. ⇒ 📋 **MANAGER: this needs a host row boarded in this sitting** (`TL-§5e` cl. 5: *board the host in the SAME sitting as the rider, never later*). ⛔ **I deliberately did not adopt them** — they are not `Content/`, they are not gated by my row, and improvising a pathspec at the terminal is precisely what cl. R's boarding-time bullet forbids.

### ⚠️ (b) `Content/FogArea/` — **27 untracked files, ⛔ claimed by NO row, and the decision is genuinely unmade**

⛔ A **vendor/marketplace pack Jonathan imported himself** (`FOG-§3`: *"I added a 'FogArea' asset to the game. Use that one."*). Three rows name it **READ-ONLY** (`TASK-836` done/zero-writes-attested, `TASK-841` not-dispatchable, `TASK-859` backlog). ⛔ **`FOG-§6` forbids EDITING it; ⛔ no law anywhere addresses TRACKING it.**

⚠️ **Two live hazards in leaving it as-is:**
1. ⛔ **It is not gitignored** ⇒ it will fire clause R on **every future commit row**, forever, as 27 unattributed paths.
2. ⛔ **The pack dirties its own package on load** (`TASK-836` observed the editor autosave `BP_FogArea_Auto3`) ⇒ **any agent that clicks Save writes a vendor pack.** ⚠️ This is also the `BP_FogArea` entry that `TASK-832` was offered in the `Restore Packages` dialog and **correctly declined**.

⇒ 📋 **MANAGER: commit-or-ignore is yours.** ⭐ My recommendation: **`.gitignore` it** — it is a vendor donor nobody may edit, and ignoring it removes 27 permanent false positives from every future cl. R run.

### ✅ (c) The pipeline docs + the 4 evidence renders — ⛔ correctly NOT mine, and it is law, not preference

⛔ `.claude/pipeline/CONVENTIONS.md`, `TASKBOARD.md`, ~50 untracked `handoffs/`+`qa/` `.md`, and the **4 `TASK-832-*.png` arena renders**.

✅ **CONVENTIONS' "PIPELINE-DOCS COMMITS ARE ALWAYS PERMITTED" (ratified 2026-07-29) settles it:** docs *"may be committed **docs-only**, at any time, by any agent that holds Git… A docs-only commit is NEVER a substitute for a gated integration commit, and **never carries code/asset paths**."*
⇒ ⛔ **Their carrier is their OWN docs-only commit, ⛔ never a host.** All three open hosts fence them out by construction (`TASK-918` = `Tools/**`, `TASK-919` = `Source/**` "NOTHING ELSE", `TASK-922` = `Content/` only). ⚠️ Rider: **explicit file paths, never a directory pathspec** (GIT HAZARD LAW (d), which fired live at `TASK-481`).

⚠️ **Flagging the 4 PNGs specifically:** they are `TASK-922`'s `names:` **input**, ⛔ not its pathspec — but they are **the only evidence that the ghost did not regress and the only record of what the veil looks like.** ⛔ **They should not be allowed to rot untracked.** 📌 Whoever cuts the docs-only commit: take them.

---

## 6. 📌 WHAT NOTHING DOES YET — the application law, recorded because it is not implemented

⛔ **Nothing applies `MI_Unit_Invisible`, and I did NOT write that code** (spec (5) forbids it here). The binding constraint on whoever eventually wires it is now **`WITCH-§5`'s application clause**, promoted into CONVENTIONS 2026-09-03:

- ⛔⛔ **APPLY TO EVERY MATERIAL SLOT, ⛔ NEVER SLOT 1.** `BeginPlay` writes `MI_TeamColor_<Team>` to **slot 0** (the two-slot `[TeamRegion, <CardID>PBR]` contract).
- ⛔ A slot-1-only swap leaves slot 0 **fully opaque**: a 1.7–4.1 % speck on a normal unit — ⛔⛔ **but 18 % on the WITCH, and it is her HAT BRIM** ⇒ ⛔ *an opaque chrome hat floating over a ghostly body.* ⚖️ *The unit whose whole card is invisibility is the one the naive implementation breaks worst.*
- ✅ **A whole-body MID, iterating `GetNumMaterials()`** — ⛔ never a hard-coded slot index.
- ⛔ **Restoring on break must re-apply `MI_TeamColor_<Team>` to slot 0**, or the unit returns untinted.

---

## 7. ⚠️ OPEN GATE CARRIED FORWARD **BY NAME** TO `TASK-907`

⛔⛔ **THE VEIL IS NOT PROVEN ON A SKELETAL MESH IN PIE, AND SAYING SO IS THE DELIVERABLE.**
⛔ `SK_Witch` does not exist and units spawn **static** ⇒ it is proven on **static actors only**. ⛔ I did not attempt a skeletal pixel gate — **it is structurally unavailable, not merely skipped.**
⭐ Mitigation: `bUsedWithSkeletalMesh = true` **re-read by me at commit time**, and the ghost already exercises this master on `SKM_Quinn_Simple`. ⛔ **That is MITIGATION, ⛔ NOT PROOF** — `GHOST-§5`'s defect is invisible in-editor and only appears in a **packaged build**.
⇒ 📌 **`TASK-907` (the witch rig integration) is the first row at which a skeletal veil can exist. It inherits this gate.**

---

## 8. ✅ WHAT I READ BACK IN-EDITOR (the master, at commit time)

`RefractionMethod` = **`RM_IndexOfRefraction`** ⭐ (the working mode — ⛔ **not** `RM_2DOffset`, which compiles, saves, reads back correctly and refracts **nothing**)
`bUsedWithSkeletalMesh` = **`true`** · `ShadingModel` = **`MSM_Unlit`** · `BlendMode` = **`BLEND_Translucent`** · `TwoSided` = **`false`** · `TranslucencyPass` = **`MTP_AfterDOF`** · `MaterialDomain` = **`MD_Surface`**
⇒ ⭐ **`RefractionMethod` is the only property that moved.** Confirmed independently of the artist's claim.

`MI_Unit_Invisible`: class **`MaterialInstanceConstant`** · dependencies **exactly `[/Game/Materials/M_HeroSpirit]`** · referencers **`[]`** · `VeilRefractionStrength` **0.25**.
`M_HeroSpirit` referencers = **4** — `MI_Ghost_Translucent`, `MI_HeroSpirit_Blue`, `MI_HeroSpirit_Red`, `MI_Unit_Invisible`. ⭐ **That fan-out is precisely why §3's regression check was owed.**

---

## 9. 🪤 ENVIRONMENT

- ⭐ **MCP verified live by a REAL REQUEST, ⛔ not a port check** — `list_toolsets` answered. ⚠️ `TASK-832` found the editor wedged on a modal `Restore Packages` dialog while `:8000` **accepted connections but never answered**: ⛔ **a listening port is not liveness.** ⛔ No restore dialog was offered to me; ⛔ nothing was restored.
- ✅ **Editor left UP (PID 7076), deliberately** — `TASK-919`'s compile needs it **DOWN** and the orchestrator is sequencing that next. ⛔ **I did not close it.**
- ✅ **`L_Arena` untouched.** ⛔ No save of any kind was issued; I made **read-only** MCP calls exclusively. ⛔ Its dirtiness is in memory only and `L_Arena.umap` remains untouched since 2026-08-27.
- ⛔ **`main` is 12 ahead of `origin/main` and was NOT pushed.** ⚠️ The dispatch said 10; it was **11** before my commit — `TASK-918`'s `44a8710` landed in between.

---

## 10. 🧑 STILL OWED TO JONATHAN (⛔ not mine to answer — carried, not dropped)

| # | question |
|---|---|
| 1 | 👁️ **`TASK-832-A`: can you tell the veiled unit at a glance, and does it read as *hidden* rather than *dead*?** ⭐ the one question that decides the feature |
| 2 | ⚠️ **The veil barely shows on the WITCH** (95.2 % metallic, `TASK-899`) — should be ruled **together with `J-W15`**, not separately |
| 3 | ⚠️ **True background BLUR is reachable** (`r.Refraction.Blur = 1`, Substrate on) but needs Roughness ⇒ a **lit** master ⇒ an amendment to `WITCH-§5`'s *"never a new master"*. ⛔ Deliberately not done unilaterally |
| 4 | ⚠️ **Veil and the hero's GHOST read as visual siblings** — contexts never overlap; every lever is an instance parameter |
| 5 | 📋 **manager:** ratify the `M_HeroSpirit` extension · board a host for the **two orphaned Witch JSONs** · rule **commit-or-ignore on `Content/FogArea/`** |
