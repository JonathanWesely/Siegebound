# TASK-790 — the camera stops collapsing into the hero at the ladder (`V2`)

**Assignee:** gameplay-programmer · **Status:** `ready-for-qa` · **Gate:** TASK-801 · **Compile:** TASK-802 (⛔ not run here)
**Law:** `VIS-§3` (`VIS-R2`) · `SHIP-§9c` · `SC-§36.1`
**Files touched — and ⛔ nothing else:**

- `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.h`
- `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp`
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeHeroCameraTest.cpp` (**new**)

⭐ **THE DIFF IS PURELY ADDITIVE: `git diff` reports ZERO deleted lines across `HeroCharacter.{h,cpp}`** — 8 insertion points in the `.cpp`, 4 in the `.h`, not one existing line modified or removed. That is stated up front for **TASK-803**, which edits this file next (the additive `SteerDirection` fix): it is merging against pure insertions in the constructor, `BeginPlay`, `Tick`, and one new block after `ApplyTerrainMovementTuning` — ⛔ nothing in the climb driver, the ten exits, or the contact poll moved.

---

## 1. THE MECHANISM, DIAGNOSED AT `file:line` — ⛔ AND IT IS ⛔ NOT THE LADDER

I re-derived this at the engine rather than accepting the symptom, because the dispatch explicitly warned the ladder carries **zero collision hulls**.

| Fact | Where |
|---|---|
| `CameraBoom` is built with `TargetArmLength = 400.0f` and `bUsePawnControlRotation = true` and ⛔ **nothing else** | `GitClaudeUnrealTestCharacter.cpp:39-42` |
| `bDoCollisionTest` / `ProbeSize` / `ProbeChannel` have **ZERO occurrences in all of `Source/`** — I re-ran the grep, it holds | (verified) |
| ⇒ they sit at engine defaults `true` / `12` / `ECC_Camera` | `SpringArmComponent.cpp:84-86` |
| The boom sphere-sweeps `ArmOrigin → DesiredLoc` **every frame** | **`SpringArmComponent.cpp:197`** |
| …and on a blocking hit `BlendLocations` hands the socket **straight to `Result.Location`** — ⛔ no minimum, ⛔ no blend, ⛔ no damping | **`SpringArmComponent.cpp:201` → `:227-231`** |
| `AHeroCharacter : public AGitClaudeUnrealTestCharacter` inherits it unmodified | `HeroCharacter.h:398` |

⭐ **THE COLLAPSE IS THEREFORE ARITHMETIC, NOT A BUG:** a 400 uu boom whose view direction points into a 1200 uu tower face resolves the arm to the standoff distance — tens of uu — and the camera lands inside the pawn. Back and hips at 60–70 % of frame for ≈2 s is exactly that number.

⛔ **AND IT IS ⛔ NOT THE LADDER'S GEOMETRY, WHICH THE DISPATCH WAS RIGHT TO WARN ABOUT.** The probe hits **whatever blocks `ECC_Camera` first** — the tower's own hulls, the deck, the plinth or the ground, depending on where the player's pitch points. My fix is deliberately written to be **surface-agnostic**: it never asks *what surface*, only *is the resulting arm too short, and is the actor an `AClimbableTower`*. ⛔ Nothing in it depends on the ladder having a hull, and the ladder having none changes nothing.

### ⭐⭐ CLIMB-SPECIFIC OR GENERIC? — **GENERIC, AND I SAY SO PLAINLY**

`AHeroCharacter` contained **ZERO camera code** before this task (grepped: no `CameraBoom`, no `SpringArm`, no `FollowCamera` anywhere in the 2 192-line `.cpp` or the 1 351-line `.h`). ⇒ ⛔ **no climb code was ever involved.** Walking up to *any* tall blocking geometry — the castle wall, a second tower — with the view pointed into it produces the identical collapse. **The ladder is merely the one place in the game the player is REQUIRED to stand flush against a 1200 uu wall while committing to an action.**

⚠️ **AND THIS CHANGED THE SCOPE, WHICH IS WHY THE DISPATCH ASKED:** VID-004's window (**01:12.0–01:13.5**) is the **APPROACH**, ⛔ not the ascent — the hero mounts at **01:17.0**. A fix scoped to "during a climb" would ⛔ **not have covered the frames Jonathan actually captured.** That alone rules out the climb-scoped shape.

---

## 2. ⛔⛔ THE BOARD'S SUGGESTED IMPLEMENTATION IS ⛔ NOT IMPLEMENTABLE — ARGUED, ⛔ NOT SILENTLY REPLACED

Spec §4 called *"adding `AClimbableTower` to the boom's ignore set"* the obvious right answer. **It is measured-false, and here is the line:**

> `SpringArmComponent.cpp:194`
> `FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SpringArm), false, GetOwner());`

⛔ **The spring arm ignores its OWNER and nothing else. There is no ignore-actor array, no accessor, and no hook to extend the query.** `BlendLocations` is `virtual` but reaching it needs a `USpringArmComponent` **subclass**, and the component is created by `CreateDefaultSubobject` in a base constructor that takes **no `FObjectInitializer`** — so `SetDefaultSubobjectClass` cannot be threaded through without editing `GitClaudeUnrealTestCharacter.{h,cpp}`, which `VIS-R2` puts ⛔ out of bounds.

The two ways to fake an ignore were both refused, and for the reason the dispatch itself named:

- ⛔ **Mutating the tower's collision responses at runtime** = cross-actor state that **ten climb exits** would owe a revert — *exactly the class of bug this wave has been fighting all day.* **REFUSED.**
- ⛔ **Blanket `bDoCollisionTest = false`** = the camera inside every wall in the map. **Already refused by the spec; I did not touch the flag.**

⚖️ **THE THIRD OPTION, ARGUED HERE AS §4 REQUIRES: a MINIMUM-ARM FLOOR under the engine's own answer, scoped to climbable geometry.** The probe stays **ON**, on its **default channel**, at a slightly **smaller radius**. The engine keeps deciding; we only refuse to let its answer go below a floor, and only when the blocker is an `AClimbableTower`.

---

## 3. THE FIX, AND ⛔ WHAT UNWINDS IT (⭐ NOTHING)

**Two halves, layered, answering the RULE-AND-DECLARE both ways:**

| Half | What | Scope |
|---|---|---|
| **generic** | `HeroCameraProbeSize` → `CameraBoom->ProbeSize` (`12` → **`8`**) | helps **everywhere**, costs **nothing** per frame |
| **targeted** | `MinCameraArmLengthUU` (**`150`**) enforced as a **floor**, only when the blocker `IsA(AClimbableTower)` and `bIgnoreClimbableGeometryForCamera` is on | **only** the reported defect |

⭐ **Why not the floor everywhere?** Because a floor puts the camera up to that many uu **inside** the blocker. Applying it map-wide re-creates, in bounded form, the exact failure the spec refused a blanket `bDoCollisionTest = false` for. Narrow is honest here: **every other blocker in the map keeps the engine's answer byte-for-byte**, so this change ⛔ cannot regress a camera situation that was not the defect.

### ⭐⭐ STATE TO UNWIND: **NONE. THERE IS NO EXIT COVERAGE TO REPORT, BY CONSTRUCTION.**

`TickHeroCameraCollision()` holds **one local** that starts at `0.f` every frame and is written to the camera **on every frame, through exactly ONE `SetRelativeLocation` call at a single exit.** The "not pushing" state is therefore re-established unconditionally the instant any gate stops holding. ⇒

- ⛔ **`EndLadderClimb` gained NOTHING** — asserted, not claimed (test 10: zero `Camera`/`Boom` tokens in the teardown body).
- ⛔ There is **no flag, no ignore list, no saved value** that a missed exit could strand.
- ⛔ A camera cannot be left ignoring the tower forever, because **nothing ever starts ignoring it** — the decision is recomputed from this frame's geometry, every frame.

The one value that persists is `HeroCameraBaseRelativeLocation`, and it is ⛔ **not runtime state**: it is a **read-once capture of the follow camera's AUTHORED relative location** at `BeginPlay`, taken before this feature has ever written. It exists so a `BP_HeroCharacter` over-the-shoulder framing is not silently clobbered, and so the off-switch returns the camera to **exactly** where the Blueprint put it rather than to an assumed zero.

### THE FOUR GATES (cheapest first — an ordinary frame is ⛔ ONE bool read)

1. `bIgnoreClimbableGeometryForCamera` — **the off-switch**; false ⇒ today's exact behaviour, backed out mid-playtest with no build.
2. `Boom->IsCollisionFixApplied()` — the boom's own report that its probe displaced the camera. **False in open ground.**
3. `ComputeCameraPushOutLocalX(...) != 0` — is the collapse even bad enough to matter? A trim from 400 to 260 is the spring arm working and is left alone.
4. **Only then** one sphere sweep, reproducing `SpringArmComponent.cpp:197` exactly, to learn the blocker's identity.

⇒ ⭐ **the trace runs ZERO times per frame in ordinary play**, which is why it is allowed to live in `Tick` at all. (This is one pawn — the hero — not the 20+ roster `TASK-791`'s perf clause is about.)

---

## 4. 🔍 WHAT QA SHOULD SCRUTINISE — the honest list, including what I am least sure of

1. ⚠️⚠️ **THE ONE-FRAME LATENCY, DECLARED.** `USpringArmComponent` ticks in **`TG_PostPhysics`** (`SpringArmComponent.cpp:22`); the actor ticks in `TG_PrePhysics`. So `IsCollisionFixApplied()` / `GetUnfixedCameraPosition()` / `GetSocketLocation()` read **last frame's** resolved arm. I judged this acceptable — the defect lasted ≈2 s and the offset is re-based onto the *current* socket by the boom's own update — but it is a real property and it is stated in the code, not hidden. ⭐ **If QA disagrees, the alternative is a component subclass, which `VIS-R2` fences out.**
2. ⚠️ **THE FLOOR PUTS THE CAMERA INSIDE THE TOWER**, up to `MinCameraArmLengthUU`. That is the deliberate trade — a near-clip into stone beats a blind screen — and `150` is chosen **low on purpose**. 🧑 **The number is Jonathan's feel pass, and `TASK-802`'s PIE row is what judges it, ⛔ not this handoff.**
3. **The blocker-identity sweep duplicates the engine's sweep.** I reproduce origin, endpoint, shape, channel and the single ignored actor from the component's own fields rather than re-deriving them, so it cannot drift from `SpringArmComponent.cpp:190-197`. Worth a second pair of eyes.
4. **`ECC_Camera` on the tower.** The floor only engages if the tower blocks the camera channel — which is precisely what makes the arm collapse in the first place, so the two are consistent by construction. ⛔ But it means the fix is **inert** for any tower that does not block `ECC_Camera`, which would also mean the defect cannot occur there.
5. **Defaults are proceeding, ⛔ not final** — all three are `EditDefaultsOnly` and commented as flagged for the feel pass, per `VIS-§3`.

### ⛔ PRESERVED — re-verified on disk AFTER my edit, not assumed

- `SetDefaultMovementMode()` — **exactly 1** · `SetMovementMode(MOVE_Flying)` — **exactly 1** · `FSiegeLadderClimbStatics::End(` — **exactly 1** *(asserted by test 10, not just checked)*
- **Ten exits through one teardown** · **`H-5 UnPossessed`** · **`H-7` before the recall broadcast** · **the 0.25 s watchdog** · **the four capsule re-derivations** · **the contact poll duplicating no tunable** · **`IsRecalling()` as the fifth refusal** — ⛔ **all untouched: zero deleted lines, zero modified lines.**
- ⛔⛔ **`LADDER_OUTWARD_SHIFT` AND ALL LADDER GEOMETRY: NOT TOUCHED.** It lives in `Tools/ArtPipeline/build_watchtower.py:137`, a file outside my fence that I never opened for edit. `CONTACT-§14.4` intact.

### ⛔ FENCES HELD

⛔ `GitClaudeUnrealTestCharacter.{h,cpp}` **UNTOUCHED** (and **test 7 asserts it, with a positive control**) · ⛔ no `ClimbableTower.*` (its header was **already** included at `HeroCharacter.cpp:29` — I added no include for it and edited no line of it) · ⛔ no `SummonedUnit.*` · ⛔ no `SiegeLadderClimbStatics.*` · ⛔ no mesh · ⛔ no material · ⛔ no compile · ⛔ no editor · ⛔ no MCP · ⛔ no Git · ⛔ no `CONVENTIONS.md` edit.

---

## 5. THE SUITE — ⭐ **DELTA: +10** (baseline **279** ⇒ **289** from this task alone)

`Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeHeroCameraTest.cpp`, all headless, all `Siegebound.HeroCamera.*`.
⭐ **Every row can FAIL, and the `⭐ CONTROL` rows are there because FIVE assertions stopped discriminating this week.**

| # | Test | ⭐ What makes it able to fail |
|---|---|---|
| 1 | PushOut restores a collapsed arm and passes healthy arms through | expectation **re-derived** (`Fixed + \|PushOut\| == Floor`), ⛔ not transcribed; a 260 uu trim must return **exactly 0** |
| 2 | PushOut is clamped to the natural arm | a floor **above** 400 must land at **400, not 500**; ⭐ control proves the floor is read at all |
| 3 | PushOut is inert for a non-positive floor/arm | ⭐ control: the **same** inputs with a valid floor **must** push — without it, "always return 0" passes |
| 4 | `HeroCameraProbeSize` survives construction | ⭐⭐ CDO equality **plus** an independent source probe, because equality alone goes vacuous if the tunable ever equals the engine default; ⭐ asserts the **ctor + BeginPlay** re-apply (3 call-sites) |
| 5 | The probe stays **enabled** on the default channel | `bDoCollisionTest` still true, `ProbeChannel` still `ECC_Camera`, `ProbeSize > 0` — catches a disguised blanket-off |
| 6 | The three `VIS-§3` pinned names | reflection, character-for-character; `EditDefaultsOnly` flags; ⭐ controls that the floor ships **positive** and **below** the natural arm |
| 7 | ⛔ **The template base is untouched** | `VIS-R2`'s automatic-fail gate made **executable**; ⭐⭐ control proves the file was really read (`CameraBoom` present), so it cannot pass on an empty string or a moved path |
| 8 | Serviced **outside** any climb guard | ⭐⭐ the *generic-not-climb-specific* ruling as code: zero `LadderClimb.bActive`, zero `IsClimbing()` in `Tick` |
| 9 | ⭐⭐ **Stateless — one write, one exit** | exactly **1** `SetRelativeLocation`, exactly **1** `return`; a later early-return that skips the write **fails this** |
| 10 | The ten-exit teardown gained nothing | zero `Camera`/`Boom` in `EndLadderClimb`, **plus** the one-of-each census (`SetDefaultMovementMode` / `MOVE_Flying` / `End(`) |

⚠️ **Baseline re-measured on disk before declaring:** 21 test files, macro census sums to **279 exactly** — matches `qa/TASK-779.md`.

⛔⛔ **WHAT THIS SUITE DOES ⛔ NOT CLAIM, SAID OUT LOUD: it does ⛔ NOT prove the camera LOOKS right.** That is a pixel question and it is `TASK-802`'s PIE row (`SC-§35`, `AS-§6 A(e)`): **walk the hero into the ladder and WATCH.** These tests prove the arithmetic, the configuration path, the fence, and the absence of anything to unwind.
