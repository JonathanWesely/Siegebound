# TASK-1167 — [FOGGREY-COMPONENT-SHIP] — build-master handoff

⛔ **THIS FILE IS BORN OUTSIDE ITS OWN COMMIT — and in this case outside *any* commit, because there is none.** Nothing was committed. Say so plainly to whoever reads this next.

---

## 0. 🚨 THE HEADLINE

**Every gate I was told to run, ran, and passed.** The compile is green, the executed suite is 556/556/0, and all 13 mutations red.

**And ask (C) is not closed. The fog is still beige, measured on pixels.** I did not commit.

The reason is a **third** cause, and it is neither of the two the dispatch anticipated:

- ⛔ **NOT** *"the vendor never sources its material from `BoxMaterials["Base"]`"* — **it does.** I proved it three ways.
- ⛔ **NOT** *"the write did not land"* — **it would land**, and `Error #7` would stay **silent**.
- ⭐ **BUT** the vendor's construction script, *after* building its MID from our material, **overwrites `Base Color` and `Emissive Color` on that MID from its own `generalData` struct** — and **those two parameters are the entire authored content of `MI_SiegeFog_Grey`.**

⇒ The swap is **correct, complete, verified — and inert.** Every readback in the diff reads green, the suite is green, the compile is green, `Error #7` stays silent, and **the colour never arrives.** This is exactly the shape `TASK-1166` §7(1) warned was consistent with a green gate; it simply arrived through a mechanism nobody had named.

⚖️ **The code is not defective.** It does precisely what the row told it to do, and does it well. **The row's premise was wrong.**

---

## 1. COMPILE

```
"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" GitClaudeUnrealTestEditor Win64 Development -project=".../GitClaudeUnrealTest.uproject" -waitmutex
```

**`Result: Succeeded`** — parsed out of the log. ⛔ `$LASTEXITCODE` was **0** and is **not** what I trusted (`ue-build-bat-exit-code-lies`). 13 actions, `FogVolume.cpp` + `SiegeFogVisualTest.cpp` compiled out of unity, link + `WriteMetadata` clean.

🚨 **"COOKED TARGET NOT COMPILED"** — in those words, per `SC-§111` cl. 3(d). `GitClaudeUnrealTestEditor` is the only target this project builds. A green `Result:` here is **not** evidence about the packaged game. **M9 is the only proxy that exists**, and it ran first (below).

Editor was closed for the compile (PID 13180, Jonathan's standing grant, not asked) and reopened for the pixels (PID 10084). `L_Arena` hash verified unmoved across the close.

⚠️ **One trap worth recording.** After the last mutation restored the source, the rebuild reported `Result: Succeeded` in **5.07 s** — fast enough to look like a no-op that would have left a **mutant binary** under the pixel lane. It was real: `FogVolume.cpp.obj` mtime `21:09:45` and the DLL `21:09:46` both **post-date** the source restore at `21:09:19`. Checked rather than assumed, because a green suite would **not** have caught it — the Lane-B probes read the *file from disk*, not the binary.

---

## 2. SUITE — EXECUTED

Runner: `UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests Siegebound;Quit" -nullrhi -unattended -nopause -nosplash -NoLiveCoding -log -abslog=…`, wrapped in `timeout --signal=KILL 1500`. ⛔ No run was killed. Wall clock ≈ 40 s.

| | baseline (this tree) | FINAL (post-mutation restore) |
|---|---|---|
| `Test Started` | **556** | **556** |
| `Test Completed` | **556** | **556** |
| `Result={Success}` | **556** | **556** |
| `Result={Fail}` | **0** | **0** |
| log lines | 7,018 | 7,017 |

⛔ `N` read first, non-zero, and `Started == Completed == Success + Fail` (`SC-§95` cl. 1).

**Reconciliation — residual ZERO.** Prior is `554 / 0`, measured by `TASK-1149` at `60dca54`. `TASK-1165` declared **+2** tests (10 = Lane A, 11 = Lane B) in the existing `SiegeFogVisualTest.cpp`; `20c1bea` was art and could not move the count. `554 + 2 = 556`. ✅

⛔ The programmer's **65/65 needles** was a **text simulation, not a run** (his words, and correct). This is the executed figure that was owed.

---

## 3. MUTATIONS — M1…M13, ALL RED

Harness: pristine snapshot → mutant written from pristine (never edited in place) → **own compile** → suite → **`RESTORE_BYTE_EXACT`** verified by sha256 against the snapshot. Every anchor asserted to appear **exactly once** before substitution; a dry run confirmed all 13 resolve and **none is a no-op**.

Pristine sha256 — `FogVolume.cpp` `567cffb0fcb7abc5…f58225b1` · `FogVolume.h` `8ffa4de6830b94b3…de0d2997` · `SiegeFogVisualTest.cpp` `5554a475a0222f00…dd1dc3cf`. All three verified byte-identical at the end; `git diff --stat` returned to the exact pre-mutation surface.

⛔ **Every mutation reds `TheColourGoesThroughTheVendorsOwnPipelineSoTheUniformityScalarsSurviveIt` (test 11) unless noted.** The "must red at" column named *assertion rows*, not distinct tests, so `Fail=1` with N failed rows is the expected shape.

| # | mutation | compile | observed failure set |
|---|---|---|---|
| **M9** ⭐ | re-introduce `Visual->RerunConstructionScripts();` | ✅ | **RED, 1 row** — *"FogVolume.cpp reaches NO editor-only construction API"* (0⇒1). ⛔ **Ran first.** The only row that fails a defect the editor compile passes. |
| M3 ⭐ | delete `SpawnParams.bDeferConstruction = true;` | ✅ | RED, 1 row — *"CONSTRUCTION IS DEFERRED…"* |
| M1 | delete `ApplyFogVisualMaterial(Spawned);` | ✅ | RED, **3 rows** + stale-probe marker (*ONE application site and ONE caller* · *THE COLOUR HAS A CALLER* · *WRITTEN BEFORE CONSTRUCTION*). **Broader than predicted.** |
| M2 | move that call **after** `FinishSpawning` | ✅ | RED, 1 row — *"WRITTEN **BEFORE** CONSTRUCTION FINISHES"* |
| M4 | move `VerifyFogVisualMaterial` **above** `FinishSpawning` | ✅ | RED, 1 row — *"…the MEASUREMENT is NOT"* (0⇒1) |
| M5 | delete the post-write readback block | ✅ | RED, 2 rows — *"THE WRITE IS READ BACK"* + the six-Error-site count |
| M6 | drop `.MI_SiegeFog_Grey` from the path literal | ✅ | RED, **2 tests / 4 rows** — also reds `TheGreyMaterialIsOnDiskAndTheDetector…` (MI_ naming · object-vs-package · one-literal · package-only) |
| M7 | `FogVisualMaterialMatches` → `return true;` | ✅ | RED, 5 rows in the detector test (all three empty-path rows · `_C` variant · *SAYS NO to the VENDOR material*) |
| M8 | add a `SetScalarParameterValue` to `FogVolume.cpp` | ✅ | RED, 1 row — the forbidden-shortcut ban (uniformity non-regression in executable form) |
| **M10** ⚠️ | bare `return;` between the colour and `FinishSpawning` | 🚨 **FAILED** | ⛔ **NEVER REACHED THE SUITE.** MSVC `error C4702: unreachable code` ×5. Caught by the **compiler**, **not** by the assertion it targets — so the predicted row was **never exercised**. Reported, not rounded up. |
| **M10b** ⭐ | same defect in the only form that compiles — a **reachable** early return in the same gap | ✅ | RED, **2 tests** — *"the function has NO OTHER return anywhere"* (3⇒4) **and** test 7's independent pin. ⛔ This is the row M10 was meant to prove; the pair-assertion works exactly as designed. |
| M11 | flip `/*bIsDefaultTransform=*/ false` → `true` | ✅ | RED, 1 row — *"…is SPELLED rather than defaulted"* |
| M12 | `GetMaterial(SlotIndex)` → `GetMaterial(0)` | ✅ | RED, 2 rows — *"the hardcoded element 0 is GONE"* + *"the slot index it does read is the loop's"* |
| **M13** ⭐ | (QA NIT-5) move the scale force **above** `FinishSpawning` | ✅ | RED, **2 rows** — *"NOTHING touches the transform in that window"* (0⇒1) **and** *"THE SCALE IS FORCED **AFTER** CONSTRUCTION"* (1⇒0) |

### ⭐ M13 — which instruments can and cannot see it

QA asked for this explicitly. **Measured:**

- ✅ **CAN see it:** the **source-text** rows above (`SiegeFogVisualTest.cpp:1638-1639` and `:1670-1671`). Nothing else.
- ⛔ **CANNOT see it:** every **runtime** instrument. The achieved-scale readback reads **green** either way, because on the deferred path the SCS already delivers the correct scale, and `SetActorScale3D` before `FinishSpawning` early-outs on `if (RootComponent)` with no root in that window. The Error never fires; the log never mentions it.

⇒ NIT-5's claim is **confirmed**: this defect is caught **only** by source text.

---

## 4. 🚨 THE PROOF — PIXELS

### 4.1 Method and instrument control (`SHIP-§9`)

Metric is the pair cl. (3) uses: far-field **B/G** and **HSV saturation**, over band `y ∈ [0.42, 0.48]`, `x ∈ [0.30, 0.70]`.

⭐ **The band is not invented — it is calibrated against the published number.** On `VID-007-t00m27s-reference-standard.png` (a frame of Jonathan's own recording) that band reads **(190.2, 180.5, 160.7)** against `TASK-1152`'s reported far-field **(190.6, 181.1, 162.2)** — agreeing to **≤1.5 per channel**. I am measuring what the artist measured.

🚨 **Two instrument controls, one of which failed and was discarded:**

1. ⛔ **GDI screen capture of the editor window returned an ALL-BLACK image** — `mean (0,0,0)`, `max 0`. Had I trusted it, every subsequent "colour" would have been a fabrication. Discarded; captures were taken through MCP `CaptureViewport` instead (2764×828, FOV 90, camera echoed back in every result).
2. ✅ **POSITIVE CONTROL — the probe reads beige as beige.** It returns `B/G 0.891 / sat 15.5 %` on Jonathan's recorded beige fog and `B/G 0.548 / sat 45.2 %` on fog-free grass. It does **not** answer *grey* to everything.

`SC-§88a`: all match frames taken in **Simulate-In-Editor** (`bSimulate=true`, `PlayMode_Simulate`, 8 s warmup), world `/Game/Maps/UEDPIE_0_L_Arena`, `BP_BattlefieldScatter_C_0` present. Camera `(-20000, 0, 176)`, pitch `-11.5°`, yaw 0 — `TASK-1152`'s vantage, never moved.

### 4.2 THE NUMBERS

| # | lane | B/G | sat % | evidence |
|---|---|---|---|---|
| — | cl. (3) **target** | `~0.963` | `5.9` | — |
| P | 🧑 **his own beige fog**, `VID-007` (**positive control**) | **0.891** | **15.5** | archived |
| A | Simulate, **no fog** (negative control) | **0.548** | **45.2** | `TASK-1167-A-…png` |
| **B** | Simulate, fog up, **Dynamic + `BoxMaterials[Base]` = vendor — the SHIPPED transport** | **0.845** | **20.2** | `TASK-1167-B-…png` |
| **C** | Simulate, fog up, **`MI_SiegeFog_Grey` rendered directly** (Static) | **0.947** | **7.0** | `TASK-1167-C-…png` |
| **D** | Simulate, fog up, **shipped Dynamic transport + grey colours pushed via `generalData`** | **0.911** | **10.7** | `TASK-1167-D-…png` |

⇒ **C proves the material is genuinely grey and very close to target.** ⇒ **B is what ships, and it is beige.** ⇒ **D shows where the fix actually lives.**

### 4.3 THE MECHANISM — measured, not reasoned

**(i) The vendor DOES source its MID from `BoxMaterials[Mode]`.** Varying the writable `mode` enum and reading back the material on slot 0:

| `mode` | `BoxMaterials[mode]` | MID built, and assigned to slot 0 |
|---|---|---|
| `Base` | `MI_FogArea_Box` | `MID_MI_FogArea_Box_0` |
| `Shadows` | `MI_FogArea_Box_Shadows` | `MID_MI_FogArea_Box_Shadows_0` |
| `LightShafts` | `MI_FogArea_Box_Shafts` | `MID_MI_FogArea_Box_Shafts_0` |
| `Base` (restored) | `MI_FogArea_Box` | `MID_MI_FogArea_Box_0` |

⇒ `TASK-1166`'s largest residual is **answered, in the row's favour**: the write would land, and **`Error #7` would stay silent.**

**(ii) …and then it overwrites the colour.** Driving the actor's `generalData` and reading the MID's vector parameters:

| `generalData` | MID `Base Color` | MID `Emissive Color` |
|---|---|---|
| shipped `(1,1,1)` / `(0.05,0.05,0.05)` | `(1,1,1)` | `(0.05,0.05,0.05)` |
| **driven to `(1,0,0)` / `(0,0.9,0)`** | **`(1,0,0)`** | **`(0,0.9,0)`** |
| restored | `(1,1,1)` | `(0.05,0.05,0.05)` |

⇒ **Causal, not circumstantial.** The construction script writes both colour parameters onto the MID from `generalData`, on every construction.

**(iii) …and those two parameters are all our material has.**

- `MI_SiegeFog_Grey` — `VectorParameterValues`: **`Base Color (0.88, 1, 1)`** and **`Emissive Color (0.05, 0.08, 0.26)`**. `ScalarParameterValues`: **empty**. Parent: `MI_FogArea_Box`.
- `MI_FogArea_Box` (vendor) — `VectorParameterValues`: **empty**. One scalar, `RefractionDepthBias = 0`.

⇒ The two materials differ **only** in the two parameters the vendor overwrites. **A MID built from grey is parameter-identical to one built from the vendor material.** The swap cannot change one pixel.

### 4.4 🚨 A LOAD-BEARING BOARD SENTENCE IS REFUTED

`TASKBOARD.md:958` states: *"The vendor Blueprint pushes every **scalar** into its material and **no colour at all**, so the field the row was told to set is inert on pixels."*

**The second half is exactly backwards.** It pushes **both colours**. That sentence is the premise on which a `BoxMaterials["Base"]` swap looked like the fix — and it is also why the **~30-second hand edit offered to Jonathan** (`BP_SiegeFog` → Class Defaults → `Box Materials["Base"]` → Save) would have done **nothing** either. Routed to the manager; ⛔ not edited by me beyond the status lines I own.

---

## 5. ⚠️ WHAT I COULD NOT MEASURE, AND WHY

🚨 **`Error #7` was never observed — in either direction — and its silence is NOT evidence.** `ApplyFogVisualMaterial` **never executed**: `grep` over the editor log returns **0** lines matching `AFogVolume|FOG VISUAL|ApplyFogVisualMaterial|VerifyFogVisualMaterial`.

**There is no reachable trigger for `RaiseFog()` from this seat**, and this is a measured dead end, not a shortcut:

- `AFogVolume` exposes **no `UFUNCTION` at all** (the header says so at `:306`, and grep confirms it) ⇒ nothing on it is reflection-callable.
- The only runtime caller is `USpellLibrary::ResolveSpell` (`SpellLibrary.cpp:690`), which is a **plain static, not a `UFUNCTION`**.
- `SiegePlayerController::PlayHandSlot` **is** `BlueprintCallable` — but **MCP `ObjectTools` has no function-invocation tool at all**, only get/set/list properties. Nothing in the toolset can call a `UFUNCTION`.
- `FogActiveUntilTimeSeconds` **is** a writable `UPROPERTY`, but nothing polls it: `RefreshFogVisual()` is reached **only** from `RaiseFog` / `ApplyBrightSun` / `ResetFog`, and there is no `BeginPlay`/`Tick` hook.
- `add_to_scene_from_*` is **refused while PIE is active**, so the visual had to be staged in the editor world and carried into Simulate.

⇒ **Ask (B) was NOT re-measured** (whiteout %, median visibility ≈647 uu) for the same reason — there is no way from here to raise fog through the production path, and lane B/C/D above are single frames, not the 5-position × 3-time sample ask (B) was established on. **Stated, not quietly skipped.** ⛔ Nothing in this diff names density, sharpness or wind, and QA confirmed the shortcuts are banned file-wide at 0 occurrences, so ask (B) is **structurally** safe — but it is **not** re-measured, and I am not claiming it is.

⛔ **`TASK-1166` WARN-10 (does a fog raise dirty a package?) is likewise UNSETTLED** — the production `TryLoad()` never ran.

⚠️ Also unverified: the editor-scripting surface **cannot write the `BoxMaterials` TMap at all** (`set_properties` refused every payload shape — bare strings and `refPath` objects, whole-map and single-entry). That is the same surface `TASK-1154` failed on, and it is why lane B/C/D use `mode` and `generalData` (both writable) rather than the map itself. So lane B is the **vendor** material under the shipped transport; the claim *"grey under the shipped transport is pixel-identical to B"* rests on §4.3(iii)'s parameter measurement, not on a direct capture. **I flag that as inference, and it is the one link in the chain that is.**

---

## 6. FENCES

| Fence | Result |
|---|---|
| `Content/Maps/L_Arena.umap` | ✅ sha256 **`1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622`** — **identical** before the editor close, after it, and after all engine work. **Never saved.** |
| Probe actor | `BP_SiegeFog_C_0` spawned in the editor world, used, and **removed**; `find_actors` returns **none** remaining. Map left dirty in memory, **discarded, never written**. |
| `Content/**` · `Config/**` | ✅ `git status` **clean** on both. |
| Git index | ✅ **empty** — the UE Git plugin auto-staged nothing (`ue-git-plugin-autostages-index`). |
| `Content/FogArea/**` | ⛔ untouched, and **not written** — only read through reflection on a spawned instance. |
| `Tools/**` | ⛔ untouched. The mutation harness lives in the session scratchpad, **not** in the repo. |
| Push | ⛔ **never.** |

---

## 7. COMMIT — ⛔ WITHHELD, DELIBERATELY

**Nothing was committed. `git log` is unchanged at `20c1bea`; `main` is still 6 ahead of origin, unpushed.**

The dispatch required the message to *"state ask (C) closed **with the measured colour numbers**"*. **The measured colour numbers say ask (C) is open.** Writing that message would have made the commit its own counter-evidence, and the board would have recorded a colour fix that changes nothing on screen — which is precisely the failure `TASK-1154` already produced once and the reason `TASK-1165` exists at all.

⚖️ So this is a **finding, reported**, not a gate quietly failed. The diff is sound, gated, compiled and fully mutation-covered; it is waiting on a **premise fix**, not on a code fix.

**Verified-present pathspec, ready for whoever hosts the eventual commit** (all confirmed to exist on disk, with a `MISSING` negative control run against a deliberate bad path, `SC-§106`) — anchored at the **git root, one level above the project dir** (`SC-§102`):

```
GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp        ✅ exists
GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/FogVolume.h          ✅ exists
GitClaudeUnrealTest/Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogVisualTest.cpp  ✅ exists
GitClaudeUnrealTest/.claude/pipeline/qa/TASK-1166-report.md                    ✅ exists  ⛔ NOT qa/TASK-1165.md, which has NEVER existed
GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1165-programmer.md          ✅ exists
GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-1158-buildmaster.md         ✅ exists  ⭐ known orphan, born outside its own commit
GitClaudeUnrealTest/.claude/pipeline/TASKBOARD.md                              ✅ exists
GitClaudeUnrealTest/.claude/pipeline/CONVENTIONS.md                            ✅ exists (dirty)
```

⛔ **LFS was NOT verified by oid-vs-sha256, because nothing was staged** — the check belongs to the commit that never happened. Recorded as **owed**, not as done. (`MI_SiegeFog_Grey.uasset` is **already committed and materialised** — it is a real 25 KB asset the engine loaded, rendered and reported parameters from in §4.3, which is stronger than a pointer check, but it is *not* the staged-blob comparison that was asked for.)

---

## 8. WHAT STAYED DIRTY, AND WHOSE IT IS

| path | whose |
|---|---|
| `Source/.../FogVolume.{h,cpp}`, `Tests/SiegeFogVisualTest.cpp` | ⭐ `TASK-1165`'s — **byte-exact as he left them**, restored and hash-verified after 14 mutations |
| `.claude/pipeline/qa/TASK-1166-report.md` (untracked) | ⭐ `TASK-1166`'s |
| `.claude/pipeline/handoffs/TASK-1165-programmer.md` (untracked) | ⭐ `TASK-1165`'s |
| `.claude/pipeline/handoffs/TASK-1158-buildmaster.md` (untracked) | ⭐ orphan from `TASK-1158` |
| `.claude/pipeline/TASKBOARD.md` | **mine** — three `- status:` lines (`TASK-1165`, `1166`, `1167`), `Edit` tool only |
| `.claude/pipeline/CONVENTIONS.md` | ⛔ **not mine** — arrived dirty, I did not touch it |
| `playtest-evidence/2026-09-08/TASK-1167-{A,B,C,D}-*.png` (untracked) | **mine** — the four measured frames, **uncommitted** |
| `handoffs/TASK-1167-buildmaster.md` (this file) | **mine**, untracked |

---

## 9. 🙋 FOR THE MANAGER

1. 🚨 **`TASK-1165`'s premise is falsified; the row needs re-specifying, not re-coding.** The colour channel the vendor honours is **`generalData.baseColor` / `generalData.emissiveColor`**, not `BoxMaterials[Mode]`. Lane D measures the shipped transport reaching `B/G 0.911 / sat 10.7 %` through that channel.
2. 🚨 **Correct `TASKBOARD.md:958`** — *"pushes … no colour at all"* is refuted. It is also the sentence that made the hand-edit offer to Jonathan look like a fix; **that offer should be withdrawn**, because clicking it would have changed nothing.
3. ⚠️ **`MI_SiegeFog_Grey` may be the wrong artefact entirely.** Its whole content is two vector parameters that the vendor overwrites. If the fix moves to `generalData`, the material asset has no job — decide whether it stays as the *source of the two numbers* (`0.88,1,1` / `0.05,0.08,0.26`) or is retired.
4. ⚠️ **`TASK-1166`'s WARN-10 and the ask-(B) re-measurement are still owed** and cannot be discharged from the build-master seat — they need a lane that can actually raise fog. Worth boarding a **reflected test hook or an automation test with a real world**, because right now *no agent in this pipeline can trigger the fog path at all*.
5. ⚖️ Carried forward untouched: `TASK-1166`'s WARN-7 (fence's second false reason), WARN-8 (ANY-vs-ALL), WARN-9 (`FText` culture), NIT-8 (`Wind Speed` `0.005` vs `0.5` — **two documents still disagree**).

---

## 10. ⭐ THE TRANSFERABLE LESSON

**A green compile, a green suite, 13 red mutations and a silent error log are all fully consistent with a feature that does nothing.** Every instrument in this row's gate was pointed at *"did we write the value?"* — and the answer was yes, at every layer, truthfully. Not one of them asked *"does anything downstream still read it when we're done?"*

The only instrument that could tell the difference was a **camera**, and the only reason it could is that it was **controlled against a known-beige frame first**. A colour probe that had answered *"grey"* to everything would have closed this row with a commit, a hash, and a number.

⇒ ⭐ **Verify the CONSUMER, not just the WRITE** — `SC-§94`'s shape one layer further out: not *"the log printed the request"*, but *"the field we wrote is read by someone who then overwrites the part that mattered."*

---
---

# ⭐ REV-2 — the revert, the recompile, and the RE-MEASUREMENT that refused to close ask (C)

⛔ **Rev-1 above is untouched.** This section is appended, not a rewrite. Rev-1 stays the record of the withheld commit; this is a second, later run under a **different binary**.

🚨 **HEADLINE: NOTHING WAS COMMITTED. `HEAD` is still `20c1bea`, `main` 6 ahead, tree exactly as inherited.**
The band **did not reproduce** on my instrument, so committing "ask (C) closed" would have been a false claim.

---

## R1. `TASK-1165` — preserved, then reverted, then RESTORED

| step | evidence |
|---|---|
| patch written | `handoffs/TASK-1165-reverted.patch` — **94,101 B / 1,251 lines**, exactly 3 files, `1097 insertions(+), 19 deletions(-)` |
| patch is faithful | `git apply --check --reverse` **OK** against the live tree |
| ⭐ reader controlled | a patch corrupted at one **context** line (`#include "Siegebound/FogVolume.h"`) was **REFUSED** — *"patch does not apply"*. My first control attempt was **worthless** (its temp write failed and the mutation landed on a non-context line, so it re-tested a pristine patch and "passed"). Recorded because a control that cannot fail is not a control. |
| revert | `git checkout --` the 3 paths |
| ⭐ revert proven | blob oid per file **identical** to `20c1bea`: `7e5cce4d…` / `d16aeb25…` / `fe861614…`; `git diff 20c1bea -- Source` **empty** |
| restore proven | `git apply` forward **OK**, round-tripped, then re-reverted |
| final state | patch **re-applied** (see R5); `git diff` of the 3 files is now **byte-identical to the preserved patch** (`cmp` clean) |

⚠️ **A raw `sha256` of the reverted files does NOT match `git cat-file blob`** — the repo stores **LF** (blob CR count = **0**) and checkout materialises **CRLF** (working CR count = **1305**). Identical after stripping CR. ⛔ My first line-ending census used a `grep` CR pattern and reported **1305 CR lines on a blob containing zero CR bytes** — a lying instrument; the raw byte census (`tr -cd`) is the honest one. **The authoritative revert proof is the blob oid, not a file hash.**

---

## R2. Compile — ⚠️ **"COOKED TARGET NOT COMPILED"**

Editor killed first (standing grant; the kill **discards** the dirty `L_Arena`, which is what the never-save law wants).

| build | parsed `Result:` | note |
|---|---|---|
| after the revert | **`Result: Succeeded`** | `UnrealEditor-GitClaudeUnrealTest.dll` relinked **22:47:30** — the binary Jonathan playtests now matches the reverted tree |
| after re-applying the patch (R5) | **`Result: Succeeded`** | binary again matches the tree it sits on |

⛔ Exit code **ignored** on both (`Build.bat` returns 0 on a failed build). `Result: Failed` count = **0**, compiler-error lines = **0**.
⚠️ **"COOKED TARGET NOT COMPILED"** — editor target only (`SC-§111` cl. 3(d)).

## R3. Suite — **554 / 0 MEASURED**

`Automation Test Queue Empty **554 tests performed**` · per-test census: **554 × `Result={Success}`**, **0** Failed/Error.
Reader control: a bogus pattern (`Result={ThisValueCannotExist}`) returned **0**; `Result={Success}` returned **554**.
⇒ exactly the predicted 554 (down from 556 — `TASK-1165`'s two tests went with the revert).

---

## R4. 🚨 THE RE-MEASUREMENT — and it does **NOT** clear the band

### R4a. The instruments, controlled BEFORE anything was scored

**Analyser** (`SC-§112` cl. 2, archived files) — an independent reimplementation reproducing **every** published figure:

| archived frame | published | mine |
|---|---|---|
| `VID-007-t00m27s` — 🧑 his beige | 0.891 / 15.5 | **0.8906 / 15.51** |
| `TASK-1167-A` no fog | 0.548 / 45.2 | **0.5481 / 45.19** |
| `TASK-1167-B` shipped beige | 0.845 / 20.2 | **0.8451 / 20.19** |
| `TASK-1167-C` grey direct | 0.947 / 7.0 | **0.9470 / 7.02** |
| `TASK-1167-D` generalData | 0.911 / 10.7 | **0.9104 / 10.69** |
| `TASK-1172` ship-path frame | 0.978 / 3.9 | **0.9776 / 3.87** |

⭐ It reads 🧑 **his beige AS BEIGE** and passes only the grey ⇒ it does not answer *grey* to everything.

**Blank-frame control** (cl. 3(b)), measured not asserted: synthetic 2764×828 black ⇒ `sat` **0.0 %** — ⛔ **INSIDE the pass band, the best possible score** — and `B/G = nan`, refused only because `nan >= 0.945` is `False`. My `ACQ_live` gate (`max > 0 and ncolours > 16`) **refuses it before scoring**; proven to fire.

**Acquirer**: the editor writes every PNG itself via `HighResShot 2764x828` over the **Python remote-execution lane** (`bRemoteExecution=True`), so no base64 ever entered my context. ⛔ The MCP `CaptureViewport` route was unavailable to a second HTTP session (see R6).

⭐ **Acquisition control — the acquirer tracks LIVE STATE**, four mutually-distinguishable driven states, each moving as predicted, spanning B/G 0.66 → 1.12. A frozen frame returns the same numbers four times. It did not:

| driven state | theirs | **mine** |
|---|---|---|
| no fog | 0.5600 / 44.00 | **0.5660 / 43.40** |
| beige `(1,1,1)/(.05,.05,.05)` | 0.8940 / 15.84 | **0.8452 / 19.90** |
| `emissive = (0,0,1)` | 1.1376 / 12.10 | **1.1198 / 10.70** |
| `base = (1,0,0)` | 0.6908 / 72.56 | **0.6596 / 70.89** |

⭐ **A fifth, accidental control:** spawning without forcing the scale reproduced the **`TASK-1074` substitution** — the engine gave `(20,20,5)` instead of `(640,360,260)` — and the frame then scored **0.5569 / 44.31**, i.e. **the no-fog numbers**. The fog was *there* and rendered *nothing*. My rig detects real fog.

### R4b. ⛔ THE NUMBER — the shipped asset does **not** reach `B/G ≥ 0.945`

Pinned vantage, character for character: camera `(-20000, 0, 176)` · **pitch `-11.5`** · yaw 0 · Simulate-In-Editor (`SC-§88a`) · 8 s warmup · 2764×828, camera **echoed back by the engine on every frame**. Fog spawned at the **code-derived shipped transform** — `(0,0,7000)`, scale `(640,360,260)`, re-derived from `FogVisualTransform` + `ArenaHalfExtent(26000,12000)` + the margins, and **read back achieved**.

| build | frames | **mean B/G** | mean sat | band `≥0.945` / `≤8.0` |
|---|---|---|---|---|
| **REVERTED** (what ships) | 0.9358 · 0.9370 · 0.9387 | **0.9372** | **7.79 %** | ⛔ **B/G FAILS** · sat ok |
| **PATCHED** (`TASK-1165` restored) | 0.9387 · 0.9409 · 0.9424 | **0.9406** | **6.66 %** | ⛔ **B/G FAILS** · sat ok |
| C3 driven **in place** (reverted) | — | **0.9445** | 6.96 % | ⛔ **B/G FAILS** |

`TASK-1172` reported **0.9772 / 3.90**, replicated twice to ΔB/G 0.0001. **I cannot reproduce it. I measure 0.937–0.941 for the same asset values at the same vantage.** The gap is **~0.040**, and the band's entire margin above the floor was **0.032** — so the verdict flips on it.

### R4c. ⚖️ THE DISPATCH'S HYPOTHESIS IS **REFUTED** — the code route was **NOT** load-bearing

> *"If the colour does NOT survive the revert… it would mean the code route was load-bearing after all."*

**Measured, A/B, same rig, one variable (the patch plus a recompile between them): `0.9372` reverted vs `0.9406` patched ⇒ the code is worth `+0.0034 B/G`.** That is ~1/12 of the gap, and **both fail the band**. ⛔ The colour **did** survive the revert; what it never had, on my instrument, was the band.

⭐ **Structural corroboration, and it explains why:** `TASK-1165`'s write lives inside `AFogVolume::SpawnFogVisual`. My probe — like `TASK-1172`'s `BP_SiegeFog_C_0..C_4` — is **hand-spawned** and never passes through that C++ site. ⇒ **neither row's instrument could ever have exercised the code route**, which is exactly why removing it changes almost nothing. ⚖️ **The manager's branch (a) stands on the code question, and stands for a better reason than the one it was decided on.**

### R4d. ⚠️ Where the ~0.040 seems to live — **a lead, not a finding**

⛔ I did not diagnose it and will not invent a mechanism. What is **measured**:

- My beige reads **0.8452**. `TASK-1167`'s own archived lane-B frame of the shipped beige reads **0.845**. ⇒ **my acquirer and `TASK-1167`'s `CaptureViewport` agree to 0.0002 on the same state** — so an *acquirer difference* does **not** explain the gap.
- `TASK-1172` §3 records its live capture of the **shipped** fog at **0.8461** — matching lane B — while its **BEFORE** frame, *"driven back to `20c1bea`'s values in-session"*, reads **0.8940**. Those are the **same nominal state 0.048 apart, inside one session**, and 0.048 is the size of the whole discrepancy.
- ⇒ **Lead:** an **in-place drive** appears to read ~0.05 high versus a state that arrived by construction. `TASK-1172` ranked its candidates on the in-place rig; if that offset rides along, the reported 0.9772 would sit near 0.93 corrected — which is where I land.
- ⛔ **Routed to the manager. `FOG-§12.4b` / `TASK-1172` §5's band verdict needs re-adjudicating, not by me.**

⭐ **The transferable shape, and it is rev-1's lesson one layer out:** `TASK-1172` proved its *acquirer* live and its *analyser* calibrated, and named the **driver** as the third instrument. There is a **fourth: the STATE-ARRIVAL PATH.** *Driven-to* a value and *constructed-with* it are not the same measurement — its own §4 said so (*"the rig was sound for RANKING; the ship path is the number that ships"*), priced the difference at +0.021, and still published the higher number as the pass.

---

## R5. Tree left EXACTLY as inherited — ⛔ nothing committed

Per the dispatch's failure branch (*"stop, do not commit ask (C) as closed, restore the patch, and report"*):

| | |
|---|---|
| `HEAD` | **`20c1bea`**, `main` **6 ahead / 0 behind**, ⛔ **not pushed** |
| commit | ⛔ **NONE.** No staging, no `git add`, index untouched |
| `TASK-1165`'s 3 files | **restored** — `git diff` byte-identical to the preserved patch (`cmp` clean) |
| binary | recompiled **with** the patch ⇒ source and binary agree |
| `Content/Blueprints/BP_SiegeFog.uasset` | `411a8bc0…d1c19271` · **38,595 B** — unchanged, uncommitted |
| `Content/Materials/Instances/MI_SiegeFog_Grey.uasset` | `554154c4…1df33709` · **7,288 B** — unchanged, uncommitted |
| ⭐ `Content/Maps/L_Arena.umap` | `1f78419d…0af15622` · 605,098 B — **IDENTICAL at every checkpoint**: before the probes, after each editor kill, and at the end. ⛔ **Never saved; `save_assets([])` never called.** Probe actors discarded by killing the editor. |
| `Config/**` · `Tools/**` · `Content/FogArea/**` | ⛔ untouched, clean |
| `CONVENTIONS.md` | ⛔ **not mine** — arrived dirty (the manager's), not edited, not staged |

⚠️ **The LFS oid-vs-sha256 verification is still OWED** and passes to whoever commits: `MI_SiegeFog_Grey` changed **content at an identical 7,288 B**, so ⛔ a size check passes on a stale blob (`SC-§68`). I did not stage, so I did not perform it.

**New, untracked, mine (8 rev-2 evidence frames)** in `.claude/pipeline/playtest-evidence/2026-09-08/` — `TASK-1167rev2-*`, including the reverted/patched ship-path pair, all four acquisition controls, and the `(20,20,5)`-renders-nothing frame.

## R6. ⚠️ Instrument debt worth recording

- ⛔ **A second MCP HTTP session cannot execute tools.** `initialize` and `tools/list` answer inline, but **every `tools/call` returns HTTP 200 with a zero-byte body** — on all three protocol versions, all `Accept` variants, with and without `MCP-Protocol-Version`; `GET /mcp` is **405**, so there is no stream to collect from. Claude Code's own bridge (the primary session) works. ⇒ **the PNG-to-disk route is the Python remote-exec lane, not a second HTTP client.**
- ⛔ The MCP **sandbox** (`execute_tool_script`) is **read-only for files** (`Mode 'w' is not permitted`) and its import allowlist is checked **statically**, so it can neither write a capture nor decode one.
- ⭐ `unreal.Rotator`'s **positional** order is `(roll, pitch, yaw)`. `Rotator(-11.5, 0, 0)` put the tilt in **roll** and left pitch at **0**. It was caught **only** because the negative control disagreed with its published value — the frame was perfectly acquired, perfectly analysed, and of the wrong view. **Use keywords.**
- `RaiseFog()` is not a `UFUNCTION` ⇒ the game's **real** consumer path cannot be driven from Python. Every fog number in this lane — mine and `TASK-1172`'s — comes from a **hand-spawned probe**, not from playing the Fog card.

## R7. 🙋 FOR THE MANAGER

1. 🚨 **Ask (C) is NOT closed.** Independent re-measurement: **0.9372** (reverted) / **0.9406** (patched) against the **0.945** floor. ⛔ I did not commit.
2. ⚖️ **Branch (a)'s code ruling STANDS, measured:** `TASK-1165` is worth **+0.0034 B/G**, and its write site is unreachable by either row's instrument. It buys nothing observable — exactly as ruled, for a firmer reason than the one used.
3. ⚠️ **`TASK-1172`'s 0.9772 needs re-adjudicating.** Its own §3 carries the discriminator: the shipped fog at **0.8461** and "driven back to shipped values" at **0.8940** — one session, same state, 0.048 apart. **Lead: in-place drives read high.**
4. 🙋 **The colour may still be RIGHT for his eye** — `sat` passes comfortably in every run of mine (6.6–7.9 % against the 8.0 ceiling) and the fog is plainly no longer yellow. ⛔ **Only 🧑 he closes it on taste**; what I cannot certify is the **`B/G ≥ 0.945`** number.
5. ⚠️ The **LFS oid-vs-sha256** duty is unspent and still owed by whoever commits.
