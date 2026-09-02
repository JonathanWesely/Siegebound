# TASK-747 (`IA_Recall` → **B**) + TASK-757 (the recall tell) — art-director

**Status:** both COMPLETE, all proofs PASS → `ready-for-integration`
**Date:** 2026-09-01
**Law applied:** `RECALL-§2` · `RECALL-§4` R-3 · `HELP-§4` · `KBD-§2a` (the editor-time authoring carve) · `KBD-§1`/`§2`/`§4` · CONVENTIONS naming row `NS_ → Content/VFX/` · template-donor rule
**Route:** in-editor Python over the PythonScriptPlugin remote-execution lane (`bRemoteExecution=True`) + Unreal MCP. **No editor bounce · no compile · no `Source/` edit · no Git · no `.umap` save.**

---

# PART 1 — TASK-747: `IA_Recall` on **B**

## ⚠️ H1 — THE B-CONFLICT VERDICT: **B IS FREE. NO CONFLICT. NOTHING WAS STOMPED.**

Proven **in the asset** before any mutation, not by a `Source/` grep (`RECALL-§2` / `HELP-§4` are explicit that the grep is necessary and **not** sufficient because `IMC_Hero` is binary).

- `IMC_Hero.DefaultKeyMappings.mappings` read **before** the append: **26 rows, zero carrying key `B`.**
- The authoring script re-checks the gate **internally** and `bail()`s before touching anything if a `B` row exists — a conflict could only ever have been flagged, never overwritten.
- ⇒ **No ruling is owed from Jonathan.** B was unclaimed; it is now `IA_Recall`'s.

Full pre-state key list, in array order:

`SpaceBar · W · S · A · D · Mouse2D · LeftShift · LeftMouseButton · One · RightMouseButton · Escape · Two · Three · Four · Five · Six · LeftAlt · Q · T · R · E · F · C · Enter · M · Tab`

## Assets

| Asset | /Game/ path | Disk path |
|---|---|---|
| `IA_Recall` (NEW) | `/Game/Input/Actions/IA_Recall` | `Content/Input/Actions/IA_Recall.uasset` |
| `IMC_Hero` (append only) | `/Game/Input/IMC_Hero` | `Content/Input/IMC_Hero.uasset` |

⛔ No other asset was created, modified or saved in this part. Saves were explicit and per-path (`save_asset` × 2). **There is no `save_assets([])` anywhere in the script.**

### `IA_Recall` read-back — matches the shipped digital template exactly

Confirmed by **two independent instruments** (remote-exec Python, and an MCP `get_properties` call on a different code path):

```
ValueType            = Boolean      (Digital/bool, per spec)
bConsumeInput        = true
bTriggerWhenPaused   = false
bReserveAllMappings  = false
Triggers             = []           Modifiers = []
```

Byte-for-byte the same property set as the shipped `IA_AssistantConsole` and `IA_ControlsHelp`, which were read first as the template rather than guessed.

## `KBD-§2a` — THE FOUR CONDITIONS, EACH DISCHARGED

**1. The `MapKey` call lives in a SCRATCHPAD script, never in `Source/`.** ✅
`<scratchpad>/t747_author.py`. `git status` shows zero `Source/` paths from this task.

**2. ONE call, ONE appended row; the removal APIs stay banned.** ✅
Exactly one `imc.map_key(ia, key)`. The driver **mechanically refuses to send** any payload containing `unmap_key` / `unmap_all` / `unmap_all_keys_from_action` / the PascalCase forms — it scanned every payload and printed `banned-token scan: CLEAN` on each send. The ban is enforced by the tool, not by my care.
⛔ The mappings array was never rewritten — that is the TASK-445 defect and the one operation MCP *would* have accepted.

**3. SURVIVORS PROVEN BY NAMING THE MODIFIER OBJECTS, NOT THE KEYS.** ✅

| row | modifier objects AFTER the append |
|---|---|
| `IA_Move` / `W` | `InputModifierSwizzleAxis_0` |
| `IA_Move` / `S` | `InputModifierSwizzleAxis_1`, `InputModifierNegate_0` |
| `IA_Move` / `A` | `InputModifierNegate_1` |
| `IA_Move` / `D` | *(none — and none before)* |
| `IA_Look` / `Mouse2D` | `InputModifierNegate_2` |

- `proof_survivors_identical: true` (before-dict == after-dict)
- All five required objects present **by name**: `true ×5`
- MCP cross-check confirms the same objects by **refPath**, e.g. `/Game/Input/IMC_Hero.IMC_Hero:InputModifierSwizzleAxis_0`
- **⭐ COUNT: 26 → 27** (`proof_count_n_to_n_plus_1: true`) — `N` measured **in the asset, in the same session as the append**, per `KBD-§2a`'s repaired RELATIVE invariant
- **First 26 keys identical IN ARRAY ORDER** ✅ — and the first 26 *actions* identical in array order too (an extra check)

**4. The appended row carries EMPTY `Triggers` and EMPTY `Modifiers`.** ✅

```
index 26 → action=IA_Recall  key=B
          triggers=[]  modifiers=[]
          settingBehavior=InheritSettingsFromAction
          playerMappableKeySettings=None
```

Identical in shape to the shipped `IA_AssistantConsole` / `IA_ControlsHelp` rows. Its emptiness is the specification, which is why the append cannot express the ban's failure mechanism.

`ALL_PROOFS_PASS: true`

## ✅ THE BINDING RESOLVES THROUGH THE LAYOUT SYSTEM — ZERO EXTRA CODE

`IA_Recall` rides `IMC_Hero`, and `USiegeKeyboardLayoutSubsystem` retargets **only the `.Key` field, index-by-index, on a transient duplicate of the whole context** (`KBD-§2a`'s 5.7-deprecation note: `GetMappings()`/`GetMapping(i)` both already return the new `DefaultKeyMappings.Mappings` array). `KBD-§4` tables **all 26 letters**, so **`B` inherits Dvorak with zero extra code**, and the append landing at the **END** of the array is exactly why the index-by-index retarget is unaffected. ⛔ No `EKeys::B` literal exists on any shipped path.

## ⚠️ WHAT THE TAB CONTROLS MENU (`HELP-§`) NEEDS — FOR THE MANAGER TO BOARD

⛔ **Not mine** — `FSiegeControlsHelpRegistry` is C++ in `SiegeControlsHelpWidget.{h,cpp}`, which is **TASK-751's** sole-owned surface. Reporting rather than leaving a key that appears nowhere in the menu (`HELP-§2` mechanism 2: a new action is made *surface-able*, not automatically documented, and an undocumented row renders as **(undocumented — TODO)**).

The row needs:

- **Lane:** **A (mapped)** — `IA_Recall` is an **IMC-context** action, so its label comes from the **already-remapped context** via `QueryKeysMappedToAction`. ⛔ **Not** `GetPositionalKey` — that is the raw-polled-key API and would DOUBLE-TRANSLATE (TASK-748 flags the same trap; `SiegePlayerController.h:1223-1225`).
- **ActionId:** `Recall` (or the registry's existing convention for an `IA_`-backed row).
- **One-liner:** *"Channel a recall — teleport home and refill health."*
- **Detail page**, every sentence traceable to a file:line the author reads for themselves (`HELP-§2` mechanism 3). The derivation surface TASK-748 exposes for exactly this: `IsRecalling()` · **`GetRecallChannelSeconds()` — ⭐ derive the duration from this, ⛔ do not type "10 seconds"** · `GetRecallProgress01()` · `GetRecallRemainingSeconds()` · `OnRecallStateChanged(bChannelling, ChannelSeconds)`.
- **Content the page must carry** (all from `RECALL-§4`, cite don't restate): channel duration is derived, ⛔ never typed · **cancel is re-pressing the same key, or MOVING** (`R-2`) · ⛔ **`Escape` is NOT a cancel route** (`AS-§6` A-2 / `RECALL-§3`) · **damage that LANDS interrupts** — a miss/blocked/fully-mitigated hit does not (`R-1`) · **the hero cannot attack while channelling** (`R-5`), but **movement is not restricted — moving CANCELS**, and the page must not conflate the two · a re-press restarts **from zero**, no partial credit (`R-4`).
- **`RelatedActionIds`:** the attack row (disarmed during the channel) — one level deep, per TASK-707's shape.

---

# PART 2 — TASK-757: the recall tell

## The asset

| Asset | /Game/ path | Disk path | sha256 |
|---|---|---|---|
| `NS_RecallChannel` (NEW) | `/Game/VFX/NS_RecallChannel` | `Content/VFX/NS_RecallChannel.uasset` | `a082d7978fdcb4c618e66db1ef3dfd77375941b3c0a7f0c303c45766fdd90cdc` (1,545,446 B) |

**Donor:** `/Game/Fire_Magic/VFX_Niagara/NS_Fire_Magic_Orb` — duplicated into a project folder per the template-donor rule; the donor was **never edited in place**. Naming per the CONVENTIONS row `NS_ → Content/VFX/`, matching the shipped `NS_ChainZap` / `NS_Spell_*` family.

**What it looks like:** a bright orange-red glowing orb, roughly torso-width, sitting at the hero's hip. Radially symmetric — an enemy reads it from any angle. **The hero's silhouette stays fully visible and targetable**, which is load-bearing (see the ice-orb rejection below).

## ⭐⭐ THE MEASUREMENT THAT PICKED IT — AND IT REJECTED MY FIRST TWO CHOICES

**Reuse before authoring was followed, and it required real evidence.** All **117** NiagaraSystems reachable to this project were enumerated; **34** were staged in-engine and scrubbed to an **exact system age of 10.0 s** using `UNiagaraComponent::SetDesiredAge` + `SeekToDesiredAge` — deterministic, ⛔ not wall-clock timing (my earlier wall-clock samples contradicted each other, which is *why* I switched instruments).

> ### ⛔ **ONLY THREE OF THE 34 WERE STILL ALIVE AT AGE 10.0:** `NS_Fire_Magic_Orb`, `NS_Ice_Magic_Orb`, `FountainLightweight`.

**Every "aura / buff / circle / casting" system in the Fire_Magic, Ice_Magic and StylizedWizardSet packs finishes well before a 10-second channel ends.** This project owns essentially no effect designed to run for the duration of a channelled ability. That is the single most important art fact in this task and it is why FAB-008 now exists.

**Why the other two survivors lost:**
- ⛔ **`NS_Ice_Magic_Orb` — REJECTED ON GAMEPLAY GROUNDS, not taste.** It is prettier, larger and the better colour (cyan reads as *teleport*, not *damage*) — but at the real camera distance it **completely swallows the hero**: no part of the mannequin is visible. **Damage that lands interrupts the channel (`R-1`), so the enemy must be able to see and hit the recalling hero.** An opaque ball around them removes the counterplay the tell exists to create. Captured and confirmed at 400 uu.
- ⛔ **`FountainLightweight`** — scattered white sprites, generic, low contrast, reads as nothing.

**Why I first picked `NS_Casting_Lv2` and was wrong:** it is the best *thematic* fit (a looping mage circle, and its dependency list even names `ENiagara_InfiniteLoopDuration`). Measured at age 10.0 it is **dead**. ⚠️ **The lesson worth keeping: a Niagara system's dependency on the infinite-loop enum does NOT mean the system loops** — the enum is referenced by the module whether or not that option is selected. Only the age scrub settles it.

## ✅ IT READS AT GAMEPLAY CAMERA DISTANCE — MEASURED, NOT ASSUMED

- The camera distance was **read from the shipped Blueprint**, not assumed: `BP_HeroCharacter.CameraBoom.TargetArmLength = **400.0**`, `SocketOffset` and `TargetOffset` both zero.
- The preview rig is **code-faithful**: `StartRecallChannelEffect` calls `SpawnSystemAttached(..., GetRootComponent(), NAME_None, ZeroVector, ..., SnapToTarget)`, so the system origin sits at the **capsule origin** — `CapsuleHalfHeight` read from the BP CDO = **96.0** ⇒ the tell spawns at **feet + 96 uu**, at the hero's hip, ⛔ **not at the feet**. The rig placed it at exactly that offset with a `BP_HeroCharacter` beside it for scale.
- Captured in the live level under Simulate-In-Editor at **400 uu** (the shipped camera distance) and again at ~700 / ~1200 / ~2400 uu. It reads clearly at all of them; at 400 uu it is unmistakable.

## ✅ IT SURVIVES THE FULL CHANNEL

`/Game/VFX/NS_RecallChannel` scrubbed to **exact system age 10.0** ⇒ `active: true`. The tell is still running at the moment the recall completes.

## ✅ IT TEARS DOWN CLEANLY — PROVEN AGAINST A CONTROL, ON THE CODE'S OWN EXIT PATH

The test calls exactly what `StopRecallChannelEffect()` calls — `DestroyComponent()` — mid-channel, with the untouched donor standing beside it **in the same frame** as a control:

```
components_before_destroy : [ NiagaraComponent0, active=true ]
components_after_destroy  : [ ]
teardown_left_zero_components : true
```

Pixels agree: the destroyed side is **completely gone** — no residue, no lingering particles, hero unchanged — while the control beside it is **still burning**, proving the frame was live and the disappearance was caused by the destroy, not by the effect ending or the sim stopping. ⇒ **An interrupted channel tears the tell down through the single exit path, exactly as `EndRecall` intends.**

---

## ⚠️⚠️ THE ENGINE FINDING THE INTEGRATOR AND EVERY FUTURE NIAGARA TASK MUST READ

> ### ⛔ **A NIAGARA SYSTEM DUPLICATED VIA MCP / `EditorAssetLibrary` IS *SILENTLY INERT*. It loads, reports the correct class, resolves the correct dependencies, spawns a component that reports NO error — and RENDERS NOTHING.**

**Proven by an A/B in one frame:** the duplicate at `/Game/VFX/NS_RecallChannel` and its donor `NS_Fire_Magic_Orb`, same lineage, same age, same capture — **duplicate blank, donor burning.**

**The repair: open the duplicate in the Niagara asset editor once (this triggers the compile), then save.** Re-ran the identical A/B afterwards — **both sides render identically.**

⚠️ **And the part that must not be skimmed: the `.uasset` bytes are UNCHANGED by the repair** (sha `a082d797…` before and after). The compiled data is therefore **DDC-side, not package-side**. ⇒ ✅ **Verification owed at integration: confirm the tell actually renders in PIE on the integrator's machine.** If it is blank there, the fix is the same one line of manual work — open `/Game/VFX/NS_RecallChannel` in the Niagara editor once. ⛔ **This is a declared uncertainty, not a claim that it is fine.**

📌 **SIDE FINDING, FOR A SEPARATE CHECK (⛔ not actioned by me — out of my task):** `/Game/VFX/NS_CastleDebris` was created by exactly this un-opened-duplicate idiom (`handoffs/TASK-157-artist.md` — a duplicate of `NS_Damage`, and that handoff records no Niagara-editor open). **It may be inert on disk.** Worth one PIE look at a castle crumble threshold.

## 🙋 WIRING IS NOT MINE — AND WITHOUT IT THE TELL IS INERT

⛔ Per my lane I do **not** attach assets to gameplay actors or Blueprints. Stated precisely so it is one call for whoever owns it:

- `AHeroCharacter::RecallChannelEffect` is a **hard `TObjectPtr<UNiagaraSystem>`**, `UPROPERTY(EditAnywhere, Category="Siegebound|Recall")`, shipped **unset** (`HeroCharacter.h:899-900`) — ⛔ **not** a composed soft path, so creating the asset at a conventional path does **not** wire it.
- **The one edit needed:** set `RecallChannelEffect = /Game/VFX/NS_RecallChannel` on the CDO of **`/Game/Blueprints/BP_HeroCharacter`** (the real hero pawn — `SiegeGameMode.cpp:47` resolves `BP_HeroCharacter_C`; the raw `AHeroCharacter` is only the meshless fallback). Then compile + save that one Blueprint.
- Until then `B` starts and cancels a channel with **no tell** — null-safe by design, ⛔ never a crash.

## 🚩 FOR JONATHAN'S EYE (art judgement, ⛔ not correctness — a one-property swap either way)

**The tell is ORANGE-RED**, because it is the only effect in the project that both lasts the channel and leaves the hero visible. Two honest caveats:
1. **Fire reads as *damage*.** A player may read "I am burning" rather than "I am channelling". League's recall is cool-toned for exactly this reason.
2. **Red is the enemy banner colour in this arena**, so a red glow on a friendly hero is a mild miscue.

The cyan `NS_Ice_Magic_Orb` fixes both and looks better — it was rejected **only** because it hides the hero. If Jonathan prefers the look over the visibility, the swap is one duplicate at the same path (**plus the Niagara-editor open above**). The real fix is **FAB-008**.

## 📋 FAB-008 RAISED

`.claude/pipeline/fab/FAB-REQUESTS.md` → **FAB-008 — Looping channel / recall / buff-aura VFX pack**, carrying the 34-system measurement as its justification, the four hard requirements (loops indefinitely · does not hide the character · reads at 400 uu · radially symmetric), and the duplication finding above so a future integrator cannot ship an inert asset.

---

# Node identity, blast radius, and the never-save law

- **Port-8000 / editor identity bound BEFORE any editor-shaped call:** exactly one `UnrealEditor.exe`, PID **36192**, matching the dispatch. The remote-exec node announcement carries no `process_id`, so identity was bound two independent ways: (a) exactly one node, whose `project_root` is this project; (b) **every payload asserts `os.getpid() == 36192` and aborts before touching an asset otherwise.** Every run printed `pid 36192 confirmed`.
- ⛔ **The editor was NOT closed, restarted or bounced.** PIE/Simulate was started and stopped cleanly; `StopPIE` was called after every session and no session is left running.
- ✅ **`L_Arena` NEVER-SAVE LAW HELD, AND IT IS MEASURED:** `Content/Maps/L_Arena.umap` sha256 = `9ccd54efeb0459df9ed15204fd7e5274797e5f6093f5504a5730c3c9d5ea0e58` — an **exact match to the `ROT-§2` ledger `9ccd54ef…0e58`**, mtime **unchanged at 2026-08-27 15:05**. ⛔ No `.umap` was saved and no `save_assets([])` was ever called.
- ✅ Every temporary preview actor was destroyed and the removal **read back**: `remaining_temp_actors: []`, `all_niagara_actors_in_level: []`. No `__ExternalActors__` path appears in `git status`.
- ⛔ Fenced surfaces untouched, verified in `git status`: no `Source/`, no `HeroCharacter.*`, no `SiegeGameMode`/`SiegePlayerController`, no `SM_WatchTower`/`WatchTower.fbx`, no `ABP_Footman` (⚠️ left dirty-in-memory by someone else and **not** touched or saved by me), no `A_SiegeBiped_Climb`. No compile, no Git.

## ⚠️⚠️ TWO THINGS THE INTEGRATOR MUST NOT SKIM

**(1) `L_Arena` IS DIRTY IN MEMORY.** It was **clean** when I started; the preview rig (place → capture → delete) dirtied it. **On disk it is pristine and hash-verified above**, and the actors are gone — but the editor still flags the level unsaved. ⛔ **Do NOT save it, and do NOT let a "save all" run.** A restart or a discard returns it to the ledger state. This is the one side effect I left behind and it is declared rather than hidden.

**(2) `§25b` — THE LFS/INDEX TRAP FIRED AGAIN, EXACTLY AS TASK-705 RECORDED.** All three paths are LFS-tracked, and the editor's SCC **auto-staged the two new assets by itself** (I ran no Git command):

| path (repo-relative) | worktree sha256 | index state | verdict |
|---|---|---|---|
| `Content/Input/Actions/IA_Recall.uasset` | `4982c1585c13c2b751d51a14f5dfeb4b3f24f723abe343270206299780057a32` | `A ` (auto-staged) | treat as **unproven until re-`add`ed** |
| `Content/VFX/NS_RecallChannel.uasset` | `a082d7978fdcb4c618e66db1ef3dfd77375941b3c0a7f0c303c45766fdd90cdc` | `A ` (auto-staged) | treat as **unproven until re-`add`ed** |
| `Content/Input/IMC_Hero.uasset` | `88f7f6e882e23f03fb154b63e2ce93cfab2531384bbaac0ac2a2be1c01332fa1` (14,552 B) | ` M` — index still holds the **PRE-append** blob `9ba4aeb0…46ab` (14,099 B) | ⛔ **STALE — re-`add` REQUIRED** |

⛔ **Re-`git add` all three and verify each by oid-vs-sha256, ⛔ never by byte size** — a size check "notices" the change while certifying the wrong blob, which is precisely why §25b bans it. **A commit as-found would ship `B` UNBOUND while looking fully staged** — the exact failure TASK-709 caught last time.

---

## ⚠️ DEVIATIONS / FINDINGS (`SC-§15`)

**D1 — `KBD-§2a` condition 3's literal counts are stale AGAIN.** The law text was repaired once (from `24 → 25`) and now reads the general `N → N+1`, but the surrounding prose still carries the old absolute figures in its struck-through history. Live measurement this session: **26 → 27**. ⇒ I satisfied the RELATIVE invariant with `N` measured in-session, which is what the law now actually requires. Flagging only so a reviewer who reads the struck-through text does not raise a false FAIL — **no amendment is owed; the repaired law is correct.**

**D2 — `is_active()` on a `UNiagaraComponent` is NOT a reliable liveness signal under wall-clock sampling**, and it cost me several wrong conclusions before I noticed. Wall-clock PIE warmups gave contradictory readings (an effect "alive" at 12 s and "dead" at 11 s across sessions), because the elapsed time between `StartPIE` returning and the capture is not controlled. **`SetDesiredAge` + `SeekToDesiredAge` is the sound instrument** and it is what every duration claim in this handoff rests on. Recorded because the next agent measuring a VFX duration will otherwise repeat the mistake.

**D3 — `CaptureAssetImage` does not support NiagaraSystem** (*"Asset type does not support image capture"*), and a Niagara system placed in the level **does not simulate in a static editor-viewport capture** — the capture shows only the editor billboard icon. **Simulate-In-Editor is required** to see any Niagara effect at all. Recorded so the next agent does not conclude an effect is broken when it is merely not ticking.

**D4 — three engine-API signatures differ from the obvious guess; all were probed, not assumed.** `NiagaraComponent.seek_to_desired_age(age)` takes the age (not zero args) · `ActorComponent.destroy_component(self)` takes the calling object · `HitResult` exposes no `.impact_point` in Python and `GameplayStatics.break_hit_result` does not exist (the ground trace was dropped and a known flat Z used instead — it affected only preview placement, nothing shipped).

**D5 — BOARD NOT UPDATED FOR TASK-757, DELIBERATELY.** TASK-747's row was flipped. **There is no `#### TASK-757` row on the board** — the recall tell was dispatched to me directly and never boarded. ⇒ **The orchestrator/manager should board it (or fold it into 747's row) so the gate can see it.** Flagging rather than inventing a row in a file with a known write-race hazard.

**D6 — the tell's donor is a *repurposed* effect, and I am not pretending otherwise.** `NS_Fire_Magic_Orb` is a fire-magic orb doing duty as a channel tell. It satisfies every hard requirement (duration · visibility · symmetry · clean teardown) and it is the best the project owns, but it is **not** a purpose-built recall effect. FAB-008 is the real answer.

---

## Reproduction artifacts (scratchpad, not repo)

- `ue_remote_driver.py` — remote-exec driver; enforces the banned-token scan, the sole-node + project-root check, and the `__EXPECT_PID__` payload self-assertion.
- `t747_preflight.py` (read-only baseline + factory probe) · `t747_author.py` (**the `KBD-§2a` scratchpad authoring script — the one `map_key`**)
- `t757_enum.py` · `t757_place_probe.py` · `t757_probe_live.py` · `t757_scrub.py` (the age-scrub instrument) · `t757_ab.py` (the duplicate-vs-donor A/B) · `t757_teardown.py` · `t757_cleanup.py`
- Captures: `age10_small.png` (the 5-way shootout at exact age 10) · `ab_crop.png` / `ab2_crop.png` (the duplication defect, before/after repair) · `teardown_crop.png` (destroy vs control) · `ice_close_full.png` (the ice-orb occlusion that disqualified it)
