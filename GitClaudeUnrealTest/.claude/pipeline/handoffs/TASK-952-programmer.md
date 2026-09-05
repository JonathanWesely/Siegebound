# TASK-952 — Two oddities in shipped units — DIAGNOSIS (no diff)

**Agent:** gameplay-programmer · **Date:** 2026-09-03 · **Status:** ready-for-qa
**Deliverable:** this document. ⛔ **There is no diff.** Zero writes to `Source/**`, zero writes to `Content/**`, no compile, no Git write, no board edit beyond this row's `status`.
**Input:** `handoffs/TASK-946-artist.md` §3.1 / §3.4 / §8 · board row `TASK-952`
**Law cited (each located by grep, not by the citation I was handed — `SC-§38`):** `SC-§37` (CONVENTIONS:3540) · `SC-§39` (:3596) · `SC-§40` (:3694) · `SC-§49` (:3963) · `SC-§15` (no heading; used at :1823/:2030/:2617) · the **"Unit mesh facing"** bullet (:4383) and its static-`VisualMesh` clause (:4392–4397) · `WITCH-§6` (:8056–8093)

---

## 0. THE TWO VERDICTS, UP FRONT

| subject | verdict | one line |
|---|---|---|
| **A — `BP_Unit_Sorcerer` parents `ASorcererUnit`** | ✅ **DESIGN** | A deliberate C++ subclass on the shipped `AMinerUnit` precedent. **Every class gate in the project is `IsChildOf`; `GetClass(` appears ZERO times in the entire non-test `Source/` tree.** Nothing can distinguish a subclass. |
| **B — `SkeletalVisualMesh` hand-authored rotation + Z** | ⚠️ **INERT — and STRUCTURALLY so, not by coincidence of data** | Both halves are overwritten before the component can ever render, on both branches. ⛔ **But two things in the relayed premise are wrong, and one of them is the exact live-grounding hypothesis item (2)(b) told me to test — see §3.2. The authored Z is WRONG on 8 of 9 units, by up to 15.5 uu. Only the C++ derivation stands between it and a visible fleet-wide grounding error.** |

⛔ **Neither finding earns a repair task on its own.** Subject B earns a **cleanup** and a **law correction**; both are named in §5 as recommendations, not built.

---

## 1. THE PREMISE I WAS HANDED, RE-MEASURED (`SC-§40` cl. 3 + cl. 9 — reported even where it confirms)

| relayed claim | source | measured today | verdict on the claim |
|---|---|---|---|
| "`BP_Unit_Sorcerer` has the WRONG PARENT" | TASK-946 §3.5 (as relayed) | parent **is** `ASorcererUnit` | ✅ **the fact is right, the label is wrong** — see §2 |
| "the Sorcerer resolves — 21 of 22 rows" | `TASK-950` census, via the board | `BP_Unit_Sorcerer` loads; `IsChildOf(ASummonedUnit)` at every gate | ✅ **confirmed** |
| "Cleric + Footman carry a hand-authored `SkeletalVisualMesh` rotation" | TASK-946 §3.4 | ⛔ **it is NINE units, not two** | ⚠️ **INCOMPLETE — §3.1** |
| "the nine measure `(0,−90,0)` **with a hand-authored `−CapsuleHalfHeight` Z**" | CONVENTIONS:4386 | ⛔ **FALSE. The Z is a flat `−90` on all nine; it equals `−CapsuleHalfHeight` on exactly ONE (Footman).** | ⛔ **FALSE against the tree — §3.2** |
| "the stale Wizard sentence lives in `WITCH-§6`" (artist) / "there is no `Wizard` token in `WITCH-§6` at all" (manager, relayed to me) | TASK-946 §8 / my dispatch | ⛔ **BOTH are wrong today** — see §6 | ⛔ **both corrected by grep** |
| fleet rule `VisualMesh.Z == −CapsuleHalfHeight`; yaw `−90` | TASK-946 §3.2 (13/13) | ✅ **14/14 exact, re-measured, no rounding slack** | ✅ **confirmed, not inherited** |

---

## 2. SUBJECT A — the Sorcerer's parent. **VERDICT: DESIGN.**

### (a) Does `ASorcererUnit` exist and derive from `ASummonedUnit`?
✅ **Yes.** `Source/GitClaudeUnrealTest/Siegebound/SorcererUnit.h:66` — `class GITCLAUDEUNREALTEST_API ASorcererUnit : public ASummonedUnit`. Editor read-back of the BP's parent: **`/Script/GitClaudeUnrealTest.SorcererUnit`**.

### (b) Is it intentional and correct — the Miner's shape?
✅ **Yes, and the header says so in its own words** (`SorcererUnit.h:20–28`, the *"WHY A CLASS AND NOT A CSV FLAG"* block): both behaviours are **class identity**, so they ride two base virtuals rather than new `DT_Cards` columns. `SorcererUnit.cpp:7` names the precedent explicitly — *"Per-card class (the AMinerUnit precedent)"*. The parent census is **11 `ASummonedUnit` + `ASorcererUnit` + `AMinerUnit` + the new Witch = 14**; the two exceptions are the two units whose mechanic is a class, and `AMinerUnit` has shipped that way since M4.

⭐ **And the reparent did not clobber the seal — I checked, because a reparented BP keeps CDO overrides authored under its old parent.** Read off the CDOs:

| unit | `AggroRadius` | `DefendRadius` | `StateCheckInterval` | `CardID` |
|---|---|---|---|---|
| **Sorcerer** | **0** | **0** | **0.25** | `Sorcerer` |
| Miner (precedent) | 0 | 1281 | **0** | `Miner` |
| Footman (control) | **600** | **1281** | 0.25 | `Footman` |

⇒ the C++ constructor's `AggroRadius = 0` / `DefendRadius = 0` **survive** into `BP_Unit_Sorcerer`'s CDO; `StateCheckInterval` correctly stays at the base `0.25` (the header forbids copying the Miner's `0` onto a *commandable* unit, and it was not copied). ✅ **`SC-§39`: the same reader returns three distinct answers, so it is not simply echoing one value.**

### (c) What it adds or overrides, by symbol
- `virtual bool CanEverAttack() const override { return false; }` — `SorcererUnit.h:78`. Consumed at the shipped guard points in `SummonedUnit.cpp` (`:925` grouped-acquisition, `:2053`, `:3639`, `:3926`), all through `FSiegeLadderClimbStatics::IsAttackAllowed(CanEverAttack(), IsClimbing())`.
- `virtual bool IsAncientGroundEmpowerer() const override { return true; }` — `SorcererUnit.h:81`. **One consumer:** `AncientGround.cpp:234`.
- Constructor (`SorcererUnit.cpp:5–48`): `CardID = "Sorcerer"`, `AggroRadius = 0`, `DefendRadius = 0`.
- ⛔ **It adds no replicated state, no new component, no new `UPROPERTY`.**

### (d) ⭐⭐ Is there ANY path that assumes a spawned unit is *exactly* `ASummonedUnit`? — **NO, and this is a structural answer.**

| needle | result | control that proves the needle works |
|---|---|---|
| `GetClass() ==` / `GetClass() !=` | **0** in all of `Source/` | ✅ the same needle finds `SiegeKeyboardLayoutTest.cpp:1139` |
| `== A…::StaticClass()` | 0 in gameplay code (4 hits, all in `Tests/`, all `PropertyClass`/`MetaClass` reflection asserts) | ✅ returns the 4 |
| `ExactCast` | **0** repo-wide | — |
| ⭐ **`GetClass(` at all, non-test `Source/`** | ⛔ **ZERO — the whole tree** | ✅ the same needle returns 4 hits once `Tests/` is included |
| `TSubclassOf<ASummonedUnit>` | **0** declarations (the only unit `TSubclassOf`s are `ProjectileClass` and `LocomotionAnimClass`) | — |

**Every class gate in the project is `IsChildOf`, and there are exactly four:**
`SiegePlayerController.cpp:4619/4633` (the player's `ResolveCardActorClass`) · `SiegeBotController.cpp:1649/1652` · `Barracks.cpp:205` · `SiegeCheatManager.cpp:261`. The `TASK-947` roster gate uses the same shape (`SiegeCardRosterTest.cpp:273` + `:456`).

**Data side, checked too:** `get_referencers` on `BP_Unit_Sorcerer` / `Cleric` / `Footman` / `Witch` returns **`[]` for all four** — **no asset anywhere pins a unit class**; every spawn goes through the composed string. `FCardRow` carries no class column.

⇒ ⭐ **`GetClass(` being absent from the entire non-test tree is *shape*, not data.** A subclass is indistinguishable from its base at every site that can see it. **DESIGN.**

---

## 3. SUBJECT B — the hand-authored `SkeletalVisualMesh`. **VERDICT: INERT, structurally.** With two corrections that make the finding bigger than reported.

### 3.1 ⛔ CORRECTION 1 — it is **NINE** units, not two. I found this only by censusing all 14.

Read off every `Default__BP_Unit_<X>_C.SkeletalVisualMesh`:

| authored `(0,0,−90)` / `(0,−90,0)` — **9** | all-zeros (law-compliant) — **5** |
|---|---|
| Footman · Knight · Pikeman · Cavalry · Longbowman · MilitiaMob · Sapper · **Cleric** · Miner | Archer · Ogre · Wizard · Sorcerer · Witch |

**Origin, settled by read-only `git log` — this is legacy, not recent, and the chronology is exact:**

| commit | date | what |
|---|---|---|
| `e586699` | 2026-07-04 | the unit BPs are created |
| **`150c3c3`** | **2026-07-17** | **TASK-165 M7 rigged-unit integration — authors the transform on 8 units, named in its own subject line: Knight/Cavalry/Pikeman/MilitiaMob/Sapper/Cleric/Longbowman/Miner** |
| `0d717c0` | 2026-07-26 | TASK-307 lands the **capsule-derived Z** (`GroundedLoc.Z = -HalfHeight - MeshMinZ`) |
| `7bedf58` | 2026-07-27 | TASK-327 lands the **absolute yaw** (`FacingRot.Yaw = SkeletalVisualYawOffset`) |
| `80c47e8` | 2026-08-02 | `BP_Unit_Sorcerer` authored — **compliant**, all-zeros |

`git merge-base --is-ancestor` confirms `150c3c3` precedes **both** fixes. ⇒ **the authored values are pre-fix legacy that was never swept; every BP authored after 2026-07-27 is clean.** Negative control on the `git log -S` instrument: a nonsense needle returns empty.

### 3.2 ⛔⛔ CORRECTION 2 — **item (2)(b)'s hypothesis is CONFIRMED on the value, and refuted on the consequence.**

The dispatch predicted: *"a hand-authored `Z −90` on a unit whose capsule is NOT 90 is a LIVE GROUNDING ERROR."* **The first half is true and worse than stated.** The authored Z is a **flat `−90` on all nine** — it is **not** `−CapsuleHalfHeight`, and CONVENTIONS:4386's claim that it is, is false.

Derived Z computed exactly as `ResolveSkeletalVisual` computes it — `−CapsuleHalfHeight − SkBounds.MinZ`, with the ref-pose bounds read off each `SK_<CardID>`:

| unit | capsule | SK local min-Z | **derived Z (what C++ writes)** | **authored Z** | error if it were NOT overwritten |
|---|---|---|---|---|---|
| **MilitiaMob** | 74.5 | +0.012 | −74.51 | −90 | ⛔ **sinks 15.49 uu** |
| **Cavalry** | 104 | −0.018 | −103.98 | −90 | ⛔ **floats 13.98 uu** |
| Sapper | 84.5 | +0.020 | −84.52 | −90 | sinks 5.48 |
| Knight | 95 | −0.006 | −94.99 | −90 | floats 4.99 |
| Pikeman | 95 | −0.061 | −94.94 | −90 | floats 4.94 |
| Miner | 86.5 | +0.041 | −86.54 | −90 | sinks 3.46 |
| Longbowman | 92 | −0.004 | −92.00 | −90 | floats 2.00 |
| **Cleric** | 91 | +0.005 | −91.01 | −90 | floats 1.01 |
| Footman | 90 | +0.008 | −90.01 | −90 | **0.008 — the only one that matches** |

⇒ ⭐ **The authored value is not "redundant" as `SummonedUnit.h:998` calls it. It is WRONG on 8 of 9 units, spanning a 29.5 uu range.** The header's *"at best redundant"* is itself an under-statement that would let a future reader treat these as harmless.

### 3.3 (a) Is the yaw absolutely assigned at every path that reaches the component? ✅ **YES.**

`SummonedUnit.cpp:460–462` — reads the rotator, writes **only** `.Yaw`, with `=`:
```
FRotator FacingRot = SkeletalVisualMesh->GetRelativeRotation(); // keep authored pitch/roll
FacingRot.Yaw = SkeletalVisualYawOffset;
SkeletalVisualMesh->SetRelativeRotation(FacingRot);
```
`SkeletalVisualYawOffset` has **exactly one** read site (`:461`) and one declaration (`SummonedUnit.h:1005`, `= -90.f`). Repo-wide there is **no `.Yaw +=` on any unit path** — the only `.Yaw +=` in the tree is `ThirdPerson/FloatingCastle.cpp:41`, which is the control proving the needle fires.

### 3.4 (b) ⭐⭐ **THE SHARP EDGE: is the authored Z also re-derived? — ✅ YES. Yaw AND Z.**

`SummonedUnit.cpp:426–434`:
```
const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
const FBoxSphereBounds SkBounds = SkeletalAsset->GetBounds();
const float MeshMinZ = (SkBounds.Origin.Z - SkBounds.BoxExtent.Z) * SkeletalVisualMesh->GetRelativeScale3D().Z;
FVector GroundedLoc = SkeletalVisualMesh->GetRelativeLocation(); // keep authored X/Y
GroundedLoc.Z = -HalfHeight - MeshMinZ;
SkeletalVisualMesh->SetRelativeLocation(GroundedLoc);
```
**Absolute assignment, derived from the capsule.** So the answer to (2)(b) is **yaw-and-Z**, not yaw-only — the grounding error the dispatch feared is real in the *authored data* and is killed in the *code*.

⛔ **What is NOT re-derived, stated explicitly because it is the only remaining live surface:** **X, Y, pitch and roll.** The header states this as intent (`SummonedUnit.h:997` — *"Pitch and roll are preserved exactly, as the grounding fix preserves the authored X/Y"*). **Measured on all 14: X, Y, pitch and roll are `0` on every unit.** ⇒ no lateral or tilt error exists today. **This is the one leg that is coincidence-of-data, and it is the leg a future donor-duplicate could break.**

### 3.5 (c) Is the value reachable at all? — measured on both branches.

`SK_<CardID>` **loads** for **13/13** rigged units (`AssetTools.load_asset`, not existence-on-disk). ⛔ Negative controls: `SK_Witch` → *"Unable to load"*, `SK_Nonexistentxyz` → *"Unable to load"*. So the swap **does** take today for the Cleric, the Footman and the other seven.

**Branch 1 — SK resolves (today, all nine).** `SetSkeletalMeshAsset` (`:396`) → Z re-derived (`:433`) → yaw re-derived (`:462`) → `SetVisibility(true)` (`:509`). **There is no early return anywhere between `:396` and `:509`.** The authored values are dead before the first frame the component can be drawn.

**Branch 2 — SK does not resolve (the Witch today, and every future un-rigged card).** Early return at `:390`. The component then has **no mesh asset** *and* is still hidden from the constructor's `SetVisibility(false)` (`:200`).

### 3.6 ⚖️ `SC-§37`: **structurally covered**, not *"inert by coincidence of today's data"* — and here is the measurement that separates them.

The distinction turns on: **can anything else make that component visible, or bind it a mesh?**

- **C++ side — shape.** Every write to `SkeletalVisualMesh`'s transform or visibility, repo-wide: `:200` (hide, ctor) · `:433` (Z) · `:462` (yaw) · `:509` (show). **Four sites, all in `SummonedUnit.cpp`, and the only one that shows the component is unconditionally downstream of both re-derivations in the same synchronous call.** Needle proven by its own hits.
- **Blueprint side — measured, all 14.** Every `BP_Unit_*` graph is byte-identical: an empty `ConstructionScript` plus three **disconnected** stock stubs (`EventBeginPlay`, `EventActorBeginOverlap`, `EventTick`), 139 characters, **zero variables**. `SetVisibility`: 0. `SkeletalVisualMesh`: 0. `SetRelative*`: 0.
  ✅ **`SC-§39` control — 14 identical rows prove nothing until one disagrees:** the same `read_graph_dsl` reader returns a **329-character graph with real nodes** for `ABP_Footman` (`TryGetPawnOwner` → `GetVelocity` → `SetGroundSpeed`). The instrument can return the other answer.
- **Gameplay consumers — none read this transform.** `FireProjectileAt` takes its muzzle from `GetActorLocation()`, not the mesh. `GetActiveVisualMesh()`'s four consumers (`ApplyTeamMaterial :336`, juice `:1333`, veil `:2956`/`:3011`) touch materials and scale, never the authored Z.
- **And branch 2 needs TWO rogue authorings, not one:** a hidden `USkeletalMeshComponent` **with no mesh asset** renders nothing even if something made it visible. A BP would have to *both* bind a skeletal mesh *and* unhide it — at which point it has left the contract entirely and the authored Z is the least of it.

⇒ ⭐ **VERDICT: INERT, and inert because of how the code is shaped.** It is not the *"covered only because today's data happens not to hit it"* case that `SC-§37` warns about. **The next card cannot resurrect it by existing.**

### 3.7 ⚠️ ONE FRAGILITY I AM NAMING ANYWAY, BECAUSE IT IS ORDER-DEPENDENT AND NOTHING GUARDS THE ORDER

`SiegeMeshJuiceComponent::SetTargetMesh` **caches** `BaseRelativeLocation = InMesh->GetRelativeLocation()` (`SiegeMeshJuiceComponent.cpp:25`) and **restores the mesh to that cached value** after every recoil (`:118`, `:126`).

It is called at `SummonedUnit.cpp:1333` — **ten lines after** `ResolveSkeletalVisual()` at `:1323`. So it caches the **corrected** Z, and is correct today.

⛔ **But its correctness rests entirely on those two statements' order, and nothing enforces it — no comment at the call site says the order is load-bearing, and no test reads it.** If a future edit hoisted `SetTargetMesh` above `ResolveSkeletalVisual`, the authored `−90` would be latched as the juice base and **every ranged unit's recoil would settle the mesh at the wrong Z, permanently, on 8 of 9 units, by up to 15.49 uu** — through a clean compile, a green suite and two gates. ⛔ **Not a defect today. Named because it is the one place where deleting the stale authored values (§5) is worth more than the tidiness.**

---

## 4. ⭐⭐ ITEM (4) — SAID BACK: **THESE WERE FOUND BY AN AGENT CHECKING WHETHER IT WAS SAFE TO COPY SOMETHING.**

The artist did not set out to audit the fleet. It set out to author one Blueprint, asked *"may I duplicate a donor?"*, read two candidates before answering, and refused both with measured reasons. **That single question produced three board rows** (`TASK-952`, `953`, `955`) **and two corrections to standing law.**

⛔ **I measured the gap it exposed, rather than asserting it:**

- **`SiegeCardRosterTest` is the only test that loads a `BP_Unit_*` asset at all.** It asserts the composed class path resolves and `IsChildOf(ASummonedUnit)` — **it never reads a single component property.**
- **`grep -rl "SkeletalVisualMesh" Source/GitClaudeUnrealTest/Siegebound/Tests/` returns NOTHING.** Not one test in the suite reads this component.
- **No test asserts `VisualMesh.Z == −CapsuleHalfHeight`, the yaw, the capsule rule, or the slot-0 override** — the four properties every new unit is expected to copy.

⇒ ⭐ **A `BP_Unit_*` can ship with any component transform whatsoever and the entire 300+ test suite stays green.** `WITCH-§6`'s new fresh-create clause already boards the *discipline*; there is still **no detector**.

### 🔧 RECOMMENDED MECHANISM — ⛔ named for the manager to board, ⛔ NOT built, ⛔ not self-served

**A fleet-parity automation test, hosted in the file that already loads every unit class** (`SiegeCardRosterTest.cpp` — it already walks `DT_Cards`, composes `BP_Unit_<CardID>_C` and loads it, so this adds **no new asset-loading machinery**). For every spawnable unit row, read the CDO's components and assert, **`SC-§37` cl. 2 — the derived property, never the literal**:

1. `VisualMesh.RelativeLocation.Z == −CollisionCylinder.CapsuleHalfHeight` — **re-derived per unit from that unit's own capsule.** ⛔ Never `== −90`; the "half-height == 90" assumption is already banned by name.
2. `VisualMesh.RelativeRotation.Yaw == ASummonedUnit::SkeletalVisualYawOffset` — **read the C++ default through reflection**, so the test tracks the constant instead of restating it.
3. `SkeletalVisualMesh.RelativeLocation.{X,Y} == 0` and `.RelativeRotation.{Pitch,Roll} == 0` — ⛔ **the four values `ResolveSkeletalVisual` does NOT re-derive** (§3.4). This is the assertion that would have caught a donor-duplicated lateral offset, and it is the *only* one of the four whose failure is live rather than cosmetic.
4. `OverrideMaterials[0]` present — ⛔ **`TASK-953`'s subject; I did not measure or reason about it.** Listed only so the boarding manager sees the whole surface in one place.

⚠️ **Deliberately NOT proposed: asserting `SkeletalVisualMesh.Z`.** It is overwritten at runtime, so a test on it would assert authored tidiness rather than behaviour — and a test that fails for a value nothing reads trains people to edit the test.

---

## 5. RECOMMENDED REPAIRS — ⛔ NAMED, ⛔ NOT PERFORMED, ⛔ NOT BOARDED

1. 📌 **`CONVENTIONS:4386` carries a third false claim in the same bullet that was already amended twice today.** *"The other nine measure exactly `(0,-90,0)` **with a hand-authored `-CapsuleHalfHeight` Z**"* — the Z is a flat `−90` and matches `−CapsuleHalfHeight` on **1 of 9**. ⛔ **This is the sentence that would tell a future reader the authored values are harmless legacy.** They are wrong values that the code happens to overwrite. **Manager's edit.**
2. 📌 **`SummonedUnit.h:998`'s *"at best redundant"* under-states it** — say *"wrong on 8 of 9 shipped units, and overwritten"*. **A `Source/**` edit; outside my fence on this row.**
3. 🔧 **A `Content/`-only sweep zeroing `SkeletalVisualMesh` on the nine** would make the assets say what the code does. ⛔ **Low value on its own — nothing renders differently.** Its real payoff is §3.7: it removes the wrong number that an order-inversion could latch. **A future row if the manager wants it; it is nine asset saves for zero visible change, so it should be weighed, not assumed.**
4. 🔧 **The fleet-parity test in §4.** This is the one with real value.
5. ⛔ **Explicitly NOT recommended: touching `BP_Unit_Sorcerer`.** It is correct as shipped.

---

## 6. ⚠️ THE STALE ATTRIBUTION — I RE-GREPPED IT AND **BOTH** VERSIONS I WAS GIVEN ARE WRONG TODAY

- The artist said the stale *"Wizard sits at yaw 0 / Z 0"* sentence lives in `WITCH-§6`.
- My dispatch relayed the manager's correction: *"there is no `Wizard` token in `WITCH-§6` at all."*

**Measured.** `WITCH-§6` spans **CONVENTIONS:8056–8093** (next heading `WITCH-§7` at `:8094`). `Wizard` appears at **`:8079` and `:8080`** — **inside `WITCH-§6`.** Same `awk` needle returns 6 hits at `:4390–4397` where the sentence really lived, so the reader is not blind.

⚖️ **The nuance that matters:** what sits in `WITCH-§6` today is the **retraction**, not the stale claim — `:8080` reads *"THE `BP_Unit_Wizard` MISSED THE RITUAL, yaw 0 / Z 0 CLAIM … IS FALSE."* The stale sentence itself lived in the **"Unit mesh facing"** static-`VisualMesh` bullet (`:4392`) and is now struck at `:4393`.

⇒ ⭐ **So the manager's grep was almost certainly correct when it ran and was overtaken by its own amendment.** I am recording the measurement rather than the conclusion, exactly because that is the failure mode this whole chain is about: **a claim was relayed to me three times, each relay confident, and the tree had moved under all three.** `SC-§40` cl. 3.

---

## 7. ⛔ WHAT I COULD NOT DETERMINE

1. ⛔ **No PIE, no pixels.** Every claim here is static — source, git and editor-CDO reads. **I did not watch a Cleric walk.** The grounding conclusion is derived from the code path, not observed on a frame. A pixel confirmation would need `TASK-954`'s sitting or a build-master run.
2. ⚠️ **The SK bounds are the MCP reader's, not the compiled `USkeletalMesh::GetBounds()`.** `get_bounds` documents itself as ref-pose local bounds, which is what the C++ reads, but **extended-bounds settings could in principle make them differ.** Every delta I measured is < 0.07 uu, so no verdict moves — but the authoritative read is compiled-side and I did not have one.
3. ⛔ **Whether TASK-165 authored those transforms deliberately or by copy-paste.** The commit subject names the 8 units; I did not read its handoff, and the distinction does not change any verdict.
4. ⛔ **Cooked/packaged behaviour.** Editor CDOs only.
5. ⛔ **The Sorcerer's seal in live play.** I read the CDO and the four `CanEverAttack()` guard points; **I did not run a match to watch a Sorcerer decline to attack.**
6. ⛔ **`BP_Unit_Wizard`'s `OverrideMaterials`** — `TASK-953`'s row. I read the property incidentally in the census and **deliberately drew no conclusion from it.**

---

## 8. FENCE COMPLIANCE

- ⛔ **Zero writes** to `Source/**` and `Content/**`. No asset opened for edit, no BP compiled, no save issued, **no `save_assets` call of any kind**. Every MCP call was `get_*` / `list_*` / `read_*` / `load_asset`.
- ⛔ **`L_Arena` never touched.** No save prompt and no restore prompt was presented; none was accepted.
- ⛔ **`is_dirty` was never called** — the standing warning is that it returns `true` for every extant asset, so nothing here is gated on it.
- ⛔ **No compile. No Git write.** Git use was `log`, `log -S`, `log -1 --format` and `merge-base --is-ancestor` — read-only.
- ⛔ **No task boarded.** The only board edit is this row's own `status`.
- ⚠️ **`TASK-953` overlap:** none. I read `OverrideMaterials` on no unit and reasoned about `ApplyTeamMaterial` only where `GetActiveVisualMesh()` bears on the *transform* question.

## 9. FOR QA — WHAT TO SCRUTINISE

1. ⭐ **§3.6 is the load-bearing claim.** If you can name a fifth site that makes `SkeletalVisualMesh` visible, or a BP surface my `read_graph_dsl` census cannot see (a component-level property override that fires without a graph node, a level-instance override), then subject B drops from **INERT/structural** to **INERT by coincidence** and the §5(3) sweep becomes worth boarding.
2. ⚠️ **§3.7 is a judgement call I made inside a diagnose-only fence.** I called an order-dependent cache a *fragility*, not a defect, because the order is correct today. Rule me wrong if you think an unguarded ordering with a 15.49 uu failure mode deserves the third verdict.
3. ⚠️ **§2(d) rests on a repo-wide zero.** `GetClass(` = 0 in non-test `Source/`. I showed the needle returns 4 hits when `Tests/` is included, so it is not a dead needle — but check whether a class comparison could hide behind a form I did not enumerate.
4. ⛔ **§3.2's derived-Z table** is the numeric core. The arithmetic is `−CapsuleHalfHeight − MinZ`; capsules and bounds are both read values. Re-run one row if you want it independently.
