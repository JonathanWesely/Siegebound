# TASK-800 — [KE2] THE AXIS MEASUREMENT — handoff (gameplay-programmer)

**Status:** SOURCE HALF **COMPLETE**. PIE HALF **DEFERRED — needs PIE** (rides TASK-802; see §7, which also *changes* the row TASK-802 should run).
**Files written:** this file, and **nothing else**. ⛔ Zero edits to `Source/`, `Tools/`, `Content/`, `CONVENTIONS.md`, `TASKBOARD.md`. ⛔ No compile, no editor, no MCP, no Git. Suite stays **279**.

---

## 0. THE VERDICT, IN FOUR LINES

1. **The offset is on `Y`.** ⭐ The manager is **RIGHT**, and it is now derived from the shipped geometry and the shipped driver rather than from the video's pixels.
2. **The true clear width is `132.0 uu`** — and it is not merely derived from the two constants, it is **measured as-built off the mesh's own 248 ladder vertices**. The manager's `STILE_CY 76` / `STILE_HY 10` / inner faces `|y| = 66` reading is **CONFIRMED at source, exactly**.
3. **A real lateral offset exists, it is STRUCTURAL, and its source is NOT the ladder, the sockets, the lift, the line or the clip.** It is that **the contact trigger admits the pawn from anywhere in a 2D disc and NOTHING in either driver ever puts it on the line.** The pawn's tower-local `Y` at `Begin` is carried, unchanged, to the top.
4. ⚖️ **The shipped `10 uu` move does EXACTLY NOTHING for it.** ⛔ Not "not enough" — **zero**. The question *"is 10 uu enough?"* is **MALFORMED** for this defect. ⭐ The move is **still required and still correct** for the thing it was actually ruled for (`K-1` / the `TOWER-§8.5a` standoff), and that half is measured-good.

---

## 1. CITE HYGIENE — everything below was re-grepped live, nothing inherited

`SummonedUnit.cpp` is now **4,325** lines and the delegate does live in `LadderClimber.h`. Every line number in this handoff was read from the file on disk on 2026-09-02, ⛔ not carried from an earlier handoff.

| Claim | Live cite |
|---|---|
| socket literals, actor space | `ClimbableTower.cpp:101-102` — `LadderFootDefaultRelative(-460, 0, 0)` / `LadderTopDefaultRelative(-160, 0, 1200)` |
| the `10 uu` move | `build_watchtower.py:117` `LADDER_OUTWARD_SHIFT = 10.0`, applied `:126-127` |
| stile constants | `build_watchtower.py:214-215` |
| clear width, as-built | `Cache/WatchTower/watchtower_report.json` → `ladder_contract/as_built/ladder_clear_width_uu = 132.0` |
| hero capsule | `GitClaudeUnrealTestCharacter.cpp:18` `InitCapsuleSize(42.f, 96.0f)` |
| hero climb driver | `HeroCharacter.cpp:1802-1899` (`TickLadderClimb`), move at **`:1867`**, arrival snap at **`:1844`** |
| unit climb driver | `SummonedUnit.cpp:3944-4019` (`TickLadderClimb`), move at **`:3993`**, arrival snap at **`:3964`** |
| the pure rules | `SiegeLadderClimbStatics.cpp:46-190` |
| contact trigger | `SiegeLadderClimbStatics.cpp:226-315`; tunables `ClimbableTower.h:653 / :669 / :685` |
| contact start | `ClimbableTower.cpp:769-889`; link path `:444-556` |

⚠️ **`VIS-§1` observed, ⛔ not assumed.** I did **not** stop at the leaf. `AHeroCharacter`'s base `AGitClaudeUnrealTestCharacter` was read: it declares **no `Tick`** (`GitClaudeUnrealTestCharacter.h:74` is the only virtual in play, `DoMove`) and its **only** movement is `AddMovementInput` at `GitClaudeUnrealTestCharacter.cpp:108-109`, inside `DoMove` — which the hero's override **returns before calling** while `LadderClimb.bActive` (`HeroCharacter.cpp:1949-1969`). ⇒ **there is no inherited lateral driver.**

---

## 2. (Q3) THE CLEAR WIDTH — the manager's reading CONFIRMED, and improved on

**CONFIRMED, character for character.** `build_watchtower.py:214-215`:

```
STILE_CY = 76.0                 # stile centre |y|
STILE_HY = 10.0                 # => inner faces at |y| = 66, clear width 132
```

⭐ **And it is better than a derivation — it is a MEASUREMENT.** `build_watchtower.py:1249-1277` sweeps the shipped ladder vertices and the report reads back:

| quantity | as-built | pinned | source |
|---|---|---|---|
| **clear width between the stiles** | **`132.0 uu`** | `LADDER_CLEAR_W_MIN = 120.0` ✅ | `ladder_contract/as_built/ladder_clear_width_uu` |
| stile inner face | `\|v\| = 66.0` | 66.0 ✅ | `rung_plane/stile_inner_abs_v_uu` (248 ladder vertices) |
| stile outer face | `\|v\| = 86.0` | 86.0 ✅ | `rung_plane/stile_outer_abs_v_uu` |
| overall ladder width | `172.0 uu` | — | `ladder_overall_width_uu` |

⇒ **THE TRUE CLEAR WIDTH IS `132.0 uu`.** Half-opening `66.0`.

**The margins that fall out of it, and they are the whole story:**

| pawn | capsule r | centred side margin |
|---|---|---|
| `AHeroCharacter` | **42** | **`66 − 42 =` 24.0 uu** |
| `ASummonedUnit` | 34 | `66 − 34 =` 32.0 uu |

⛔ **The rungs do NOT narrow it.** `build_watchtower.py:482` builds each rung with a `Y` half-extent of `STILE_CY + STILE_HY = 86`, i.e. the rungs span the **full** `|y| ≤ 86` from stile outer face to stile outer face. They are the tread, ⛔ not a constriction.

---

## 3. (Q1) WHICH AXIS — derived from the geometry and the driver, ⛔ not from a pixel

### 3a. The ladder's frame, at source

`build_watchtower.py:166-180`, from the **two sockets only**:

```
_D      = LADDER_TOP - LADDER_FOOT = (300, 0, 1200)
CLIMB_LEN = 1236.931688
U_AXIS  = ( 0.24253563, 0.0, 0.97014250 )   # along the line
W_AXIS  = (-0.97014250, 0.0, 0.24253563 )   # in-plane normal — the STANDOFF axis
V_AXIS  = ( 0.0,        1.0, 0.0        )   # across the clear opening — PURE Y
```

⭐ **The art pipeline's own readback says it in words**, and this single sentence settles the disagreement (`watchtower_report.json`, `rung_plane/frame`):

> *"u along the climb line from LadderFoot; **v = +Y**; **w = in-plane normal pointing AWAY from the tower**. The ladder sits at NEGATIVE w… which is the **22 uu** the F5 defect was made of."*

⇒ **`v` (= Y) is the clear-opening axis. `w` (in the XZ plane) is the standoff axis. They are orthogonal and they are different defects.**

### 3b. The line's `Y` is identically zero, for its whole length

`LADDER_FOOT.y = 0`, `LADDER_TOP.y = 0`, `_D.y = 0` ⇒ `U_AXIS.y = 0`. **The climb line lies exactly on the clear opening's centre-plane at every `t`.** There is no drift to find in the line itself.

### 3c. Nothing in the climb can change a pawn's `Y` — five terms, each checked

| term | cite | `Y` contribution |
|---|---|---|
| `Begin` stores `Start`/`End` | `SiegeLadderClimbStatics.cpp:68-69` | `FromWorld`/`ToWorld` are the **socket** points ⇒ `y = 0` |
| the capsule-centre lift | `:65` `const FVector Lift(0.f, 0.f, HalfHeight);` | **pure Z. Zero.** |
| `ClimbDirection` | `:107` `(End − Start).GetSafeNormal()` | `Y` component **exactly 0** |
| the swept move | `HeroCharacter.cpp:1867` / `SummonedUnit.cpp:3993` — `AddMovementInput(Direction, 1.f, true)` | along `Direction` only ⇒ **0** |
| the non-swept deck-breach drive | `HeroCharacter.cpp:1898` / `SummonedUnit.cpp:4019` — `SetActorLocation(Here + Direction * StepUU, false)` | along `Direction` only ⇒ **0** |
| the player's steer | `HeroCharacter.cpp:1947-1972` — `DoMove` **captures and does NOT forward**, ⛔ no `Super::DoMove` | **0** |

⇒ ⭐⭐ **The climb is a rigid translation along a constant vector whose `Y` is zero. `Y(top) == Y(entry)`, exactly.**

### 3d. …and nothing puts the pawn ON the line

- `Begin` (`SiegeLadderClimbStatics.cpp:46-100`) **never reads and never writes the pawn's location.** It is pure; it takes two world points and stores them.
- `TryBeginContactClimb` (`ClimbableTower.cpp:769-889`) reads `Climber->GetActorLocation()` at `:783` **for the three terms only** and never writes it back. The file says so itself at `:859-861`: *"the tower still drives nothing itself — it does not set a movement mode, does not interpolate, does not tick and **does not teleport**. It contributes exactly two facts… WHO climbs, and BETWEEN WHICH TWO WORLD POINTS."*
- `AHeroCharacter::BeginLadderClimb` (`:1585-1703`) and `ASummonedUnit::BeginLadderClimb` (`:3792-…`): **neither contains a `SetActorLocation`, a `TeleportTo` or any snap.**

⇒ ⭐⭐ **CONCLUSION (Q1): THE OFFSET IS ON `Y`, AND IT IS THE PAWN'S ENTRY `Y`, CARRIED UNCHANGED TO THE TOP.**

---

## 4. (Q2) IS THE OFFSET REAL, AND WHERE DOES IT COME FROM

### 4a. The four boarded candidates — three REFUTED at source

| candidate | verdict | why |
|---|---|---|
| **the sockets** `(−460,0,0)` / `(−160,0,1200)` | ⛔ **REFUTED** | both `y = 0`, measured; the line is exactly on the opening's centre-plane. ⛔ Nothing to fix. |
| **the capsule-centre lift** | ⛔ **REFUTED** | `FVector Lift(0,0,H)` — `SiegeLadderClimbStatics.cpp:65`. Pure Z, identical at both ends. |
| **the climb line's own frame** | ⛔ **REFUTED** | `U_AXIS.y = 0`; `_D = (300,0,1200)`. |
| **the clip's 24 uu / −22 uu root translation** | ⛔ **REFUTED — TWICE OVER** | (a) **wrong axis**: `author_climb_anim.py:116` `RUNG_PLANE_OFFSET_M = 0.22` is applied along **`n`**, defined at `:291` as *"perpendicular, toward the rungs"*, i.e. the **standoff** axis `w`. The clip's **lateral** axis is `e` (`:292`, *"character LEFT"*), and the only thing on it is `HAND_LATERAL_M = 0.18`, which is **symmetric ±** ⇒ net zero. (b) **not in play at all**: `SiegeHeroLadderClimbTest.cpp:1271-1272` **asserts** the hero's climb region contains neither `PlayAnimMontage(` nor `A_SiegeBiped_Climb` — Jonathan's `CONTACT-§5` waiver is enforced by the suite. ⇒ ⭐ the boarded suspicion that it *"can even be in play"* is answered: **it cannot.** |

### 4b. THE ACTUAL SOURCE — the contact trigger's admission set, times a driver with no cross-track term

The trigger (`SiegeLadderClimbStatics.cpp:226-315`, tunables `ClimbableTower.h:653/669/685`) admits a pawn on **proximity (a 2D DISC, `R = 350`) + intent (`IntentCos = 0.5`, a 60° half-cone) + dwell (`0.35 s`)**. ⛔ **A disc is not a point.** For a pawn walking a straight line at perpendicular offset `d` from the foot, the dwell it can bank is `(√(R²−d²) − d·cot 60°) / v`, so the admissible band is:

| approach speed | admissible lateral band `±d` | vs the hero's **24.0 uu** margin |
|---|---|---|
| 150 uu/s (slow) | **±277.8 uu** | **11.58×** |
| **300 uu/s (walk)** | **±247.2 uu** | **10.30×** |
| 500 uu/s | ±197.4 uu | 8.23× |
| 750 uu/s (sprint) | ±116.8 uu | 4.87× |
| **937.5 uu/s (sprint + Swift Boots — the FASTEST shipped)** | **±34.9 uu** | **1.45×** |

⭐⭐ **THE FINDING: at EVERY ONE of the hero's four shipped speeds the admissible lateral band EXCEEDS the 24.0 uu the capsule has. At no speed does the trigger guarantee the hero enters the clear opening at all.** Even the fastest possible approach — where the dwell has least time to accumulate and the band is tightest — overshoots by 45 %.

⚠️ **AND THE `±247` IS NOT A NEW NUMBER — IT IS THE BOARD'S OWN.** `TASK-798` calls `±247 uu` the *"abduction"* window. It reproduces here to three figures (`247.234`) from the shipped tunables. ⇒ ⭐⭐ **`TASK-798` and `VID-004`'s lateral offset are the SAME DEFECT seen from two angles.** The abduction window is not merely *"a pawn is grabbed from too far away"* — it is *"a pawn is grabbed from too far away **and carries that offset up the ladder**, because nothing ever puts it on the line."* ⛔ Nobody has connected those two rows before; they should be fixed as one.

### 4c. Why it is INVISIBLE — the reconciliation the analyst could not make

**The ladder has ZERO collision** — `watchtower_report.json`, `collision/ladder_hulls = 0`, deliberate. ⇒ a capsule overlapping a stile is **not depenetrated, not blocked and not slid**; there is literally nothing to push back. That is why a real overlap can coexist with the analyst's *"no frame shows any body-clip"* finding, and neither observation is wrong.

### 4d. ⚠️ A SECOND, UNBOARDED CONSEQUENCE — THE ARRIVAL POP

`HeroCharacter.cpp:1844` / `SummonedUnit.cpp:3964`:

```cpp
SetActorLocation(FSiegeLadderClimbStatics::ArrivalTarget(LadderClimb), /*bSweep=*/ false);
```

`ArrivalTarget` returns `State.End` — a point with `y = 0`. ⇒ ⭐ **on the arrival frame the pawn is teleported laterally by its entire accumulated `Y` offset — up to ~247 uu — in one frame, unswept.** The ascent is off-line; the *landing* is snapped back. ⛔ This has not been looked for and it is not in `VID-004` (the analyst sampled at 0.4–0.5 s; a one-frame lateral pop is exactly what that cadence cannot see). **Recommend it as a named PIE row.**

---

## 5. (Q4) IS `10 uu` ENOUGH — the question the analyst said was not derivable

⚖️ **THE QUESTION IS MALFORMED, AND I am saying so plainly as the spec asked.**

`δ = (−10, 0, 0)` (`build_watchtower.py:117`, `:126-127`) is **X-only**. The offending quantity is the pawn's **`Y`** relative to a line whose `Y` is identically `0`. A pure `X` translation moves **both** the line and the trigger disc by the same `−10` in `X` and changes the cross-track `Y` error by **exactly `0.000` uu**. ⛔ **Not "insufficient" — inert.** There is **no value** of a pure-`X` translation that answers this: `4.6`, `10`, `100` and `1000` all move it by zero.

⭐⭐ **AND THE MOVE IS STILL RIGHT, WHICH IS THE PART THAT MUST NOT BE LOST.** It was ruled for a *different* axis and a *different* defect, and on that axis it is **measured good** (`watchtower_report.json`, `ladder_contract/standoff`):

| | before | after `δ` |
|---|---|---|
| min spine→body distance | `93.61875` | **`103.3201`** |
| hero clearance (`r 42`) | `51.61875` — ⛔ **4.38125 SHORT of 56** | **`61.32015`** |
| `hero_margin_over_56_uu` | ⛔ void | **`+5.32015`** ✅ |
| unit clearance (`r 34`) | `59.61875` | `69.32015` ✅ |
| `body_only_HERO_pass_56` | ⛔ false | **`true`** |

⇒ ✅ **`TOWER-§8.5a`'s licence is RE-EARNED for the hero's 42-radius capsule. `TASK-783`'s art work is CORRECT and should NOT be reverted, revisited or re-scoped.**

⛔⛔ **BUT NOBODY MAY BELIEVE IT FIXED WHAT JONATHAN SAW.** Two orthogonal defects on one ladder; one is shipped-fixed, the other is untouched.

---

## 6. (Q5) IF A REAL `Y` OFFSET EXISTS — WHAT WOULD FIX IT

⛔ **PROPOSALS ONLY. ⛔ Nothing implemented. ⛔ `TOWER-§8.3`'s pinned geometry and `CONTACT-§7a`'s `δ` are not mine to touch, and a second translation is a manager decision made from this number.**

### ⭐ FIX A — **CODE**, and it is the recommended one

**Shape:** give the driver a **cross-track term**. Add ONE new pure function to `FSiegeLadderClimbStatics` — e.g. `SteerDirection(State, CurrentWorld)` — returning the unit vector from `CurrentWorld` toward a look-ahead point **on the line**, so the pawn converges onto the line as it climbs. Both drivers swap `ClimbDirection` → `SteerDirection` at their **one** movement call each.

⚠️ **The trap a naive version falls into, named now so the implementer does not:** `ClimbDirection` is *also* read by `Advance`'s arrival dot test (`:158`), by the deck-breach step (`:1898`/`:4019`) and by `IsLadderClimbInputHeld`'s sign test (`:1927`). ⛔ The new function must be **ADDITIVE** — `ClimbDirection` keeps its current meaning at all three of those sites, or arrival and sustain silently change semantics.

⭐ **Variant A2, if the pop matters more than the purity:** carry the entry cross-track error in `FSiegeLadderClimbState` and lerp it to zero over the first ~150 uu of line. No pop at entry, no pop at arrival (§4d disappears for free), one extra float of state.

- **Cost:** ~15 lines of pure function + ~4 headless tests + **two one-line driver edits**. ⛔ No art, ⛔ no mesh, ⛔ no rebake, ⛔ no reimport, ⛔ no asset of any kind.
- **Risk:** LOW, and it is *testable headlessly by construction* because the pair takes no engine type (`SiegeLadderClimbStatics.cpp:5-9`).
- ⚠️ **`SC-§36.1`:** the pure function and its two call sites must be **one task**, not two.

### ⚠️ FIX B — **TUNABLE ONLY** (`LadderContactIntentCos`): a MITIGATION, ⛔ not a fix

Tightening the cone narrows the band — but **never to zero**, and not far enough:

| `IntentCos` | half-angle | band at v=300 | vs 24.0 |
|---|---|---|---|
| **0.5 (shipped)** | 60.0° | ±247.2 | 10.30× |
| 0.8 | 36.9° | ±156.2 | 6.51× |
| 0.9 | 25.8° | ±110.1 | 4.59× |
| 0.98 | 11.5° | ±49.1 | 2.04× |

⇒ ⛔ **even an 11.5° cone leaves the band at 2× the margin, and would make the ladder feel unusable.** It also cannot be paid for by shrinking `R`: `CONTACT-§7b`'s `K-6.1` raised `R` to `350` on a measurement (sprint + Swift Boots needs `937.5 × 0.35 = 328.125`), so lowering it re-opens a ruled row. **Cost: zero code, and it does not work.** ⛔ Not the fix.

### ⛔ FIX C — **SOCKET CHANGE: REFUSED, and there is nothing to change**

Both sockets are already **exactly** on the clear opening's centre-plane (`y = 0`, measured). A socket `Y` offset would move the line *off* centre and make it **worse**. **Cost: zero, because it is void.**

### ⛔ FIX D — **MESH CHANGE: REFUSED — it cannot work at any cost**

To cover even the 300 uu/s case the clear width would have to be `2 × (247 + 42) = 578 uu`. **The tower's west bay is `2 × 130 = 260 uu` wide** (`BAY_HALF_Y = 130`) and the ladder's overall outer width is `172`. ⇒ **structurally impossible.** Widening to "fix" the analyst's eyeballed `26.4` would mean `132 → 136.8` — a change that repairs one sampled frame and nothing else. And `CONTACT-§13`: **any** mesh edit repacks the entire UV atlas ⇒ full rebake + a coupled mesh + three-texture reimport. ⇒ **highest cost, lowest coverage, and it still fails.** ⛔ Refuse.

**⇒ RECOMMENDATION TO THE MANAGER: FIX A (or A2), boarded as ONE gameplay-programmer task owning the pure function AND both call sites, jointly with `TASK-798` — they are the same defect.**

---

## 7. THE PIE HALF — **DEFERRED**, and the row TASK-802 should run is ⛔ NOT the one on the board

⛔ **I made no editor contact. The number is deferred, ⛔ not guessed** (`AS-§6 A(e)`; a guessed number is worse than a deferred one).

⚠️⚠️ **BUT THE BOARDED ROW WOULD WASTE THE SESSION, AND THE SOURCE HALF IS WHY.** The board asks for *"the pawn's tower-local `Y` at 3–4 points up the line."* §3c proves **all four samples return the identical number** — nothing in the climb can change `Y`. ⇒ 4 samples, 1 bit of information.

⭐ **The row that is actually worth a PIE session:**

1. **`Y` at entry** — log `Tower->GetActorTransform().InverseTransformPosition(Hero->GetActorLocation()).Y` **inside `BeginLadderClimb`**. ⭐ *This is the whole measurement.* Compare against **±24.0**.
2. **`Y` one frame before arrival** — confirms §3c empirically (expect: equal to (1) within sweep noise). One sample, ⛔ not four.
3. ⭐ **The arrival pop (§4d)** — log `|ArrivalTarget.Y − Here.Y|` at `HeroCharacter.cpp:1844`. **This has never been looked for.**
4. **Repeat over ~6 approaches** at different offsets/speeds — the finding is a *distribution*, ⛔ not one number.

⛔ **A property readback is not an answer** — but note (1)–(3) are *log lines from the live climb*, not editor property reads, which is what `AS-§6 A(e)` actually requires.

---

## 8. (Item 5) THE ANALYST'S TWO CAVEATS — CARRIED, ⛔ not dropped

- **(a) Sampling was 0.4–0.5 s with no `--run` burst** ⇒ a single-frame interpenetration could hide between samples. ⛔ **Still true and still unresolved.** ⚠️ And §4d **raises its stakes**: the arrival pop is a *one-frame* event and is exactly what that cadence cannot resolve.
- **(b) One ascent, one pawn class, one approach angle**; the six unit ascents' handovers are largely off-frame. ⛔ **Still true**, and §4b explains why it matters far more than it looks: the offset is a **function of approach angle and speed**, so one ascent samples one point of a two-parameter family. ⚠️ Units are also a *different* case — they arrive via the nav link (`ClimbableTower.cpp:444-556`), whose path-following puts them near the link start, ⛔ **not** via the 350 uu contact disc. ⇒ **the contact path (the hero's) is the wide-open one; the unit evidence does not transfer.**

### ⭐ On the `--run` burst at 29.3 fps across 76.8–77.2 s — **RECOMMEND YES**, with the reason corrected

⛔ **It is NOT needed to settle the axis** — that is settled at source, and no pixel count could have settled it (*"~180 px" is not "~180 uu"*, and no conclusion here rests on one). ✅ **It IS worth it for caveat (a) and for §4d**: ~12 frames across the handover is the only instrument that can catch a one-frame lateral pop or a one-frame interpenetration, and it costs one flag on a capture that is being taken anyway.

---

## 9. WHAT QA / THE BUILD-MASTER SHOULD SCRUTINISE

⛔ **There is no diff to review.** This task is terminal, read-only, **no QA gate** (`CONTACT-§10.2`, the `TASK-775` precedent). ⛔ Do not fail it for having no diff. Board status is the orchestrator's — this task's `names:` fence permitted **this file only**, so `TASKBOARD.md` was deliberately **not** written.

**The three things to check if anyone wants to audit this instead:**
1. ⭐ **The 5-row table in §3c is the whole argument.** If any one of those five terms *does* have a `Y` component, the conclusion falls. Re-grep them; they are cheap.
2. ⚠️ **§4b's band arithmetic** is my derivation of the trigger's own documented model (`SiegeLadderClimbStatics.h:430-446`). It **reproduces `TASK-798`'s independently-boarded `±247`**, which is the strongest check available — but it is arithmetic, ⛔ not a playtest.
3. ⛔ **I did NOT measure the observed offset's magnitude, and nothing here should be read as certifying `26.4 uu`.** The manager's `20 % × 132 = 26.4 > 24` arithmetic is **correct as arithmetic** — but it rests on an eyeballed `~20 %` and stays a **hypothesis** (`SC-§20`, `CONTACT-§13`). ⭐ What I add is that `26.4` sits at the **low end** of a band the mechanism permits up to `±277`, so the observation is not merely possible — **it is what the mechanism predicts.**
