# QA Report — TASK-854 (gate over TASK-853)

**Verdict: PASS** — **0 BLOCKER** · 4 WARN · 3 NIT
**Date:** 2026-09-02 · **Reviewer:** qa-reviewer · **Subject diff:** `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeLadderClimbTest.cpp` (sole)

> ⛔ **ACCEPTANCE IS NOT THIS DOCUMENT.** This gate says the diff is *safe to compile and correct as reasoned*. ✅ **The acceptance evidence for the fix is `TASK-855`'s EXECUTED `Result={Success}` on `Siegebound.LadderClimb.ClimbDirectionIsByteIdenticalAndItsThreeOtherReadersStillReadTheLine`** — ⛔ not my reading, ⛔ not the programmer's arithmetic, ⛔ not a census. Until that line exists in a log, the row is *unverified and expected-green* (`TL-§5c`).

---

## SC-§29 COVERAGE LEDGER

| Task | Assignee | Covered by this gate | Verdict |
|---|---|---|---|
| **TASK-853** | gameplay-programmer | ✅ **YES — the only task this gate names** | ✅ **PASS** |
| TASK-855 | build-master | ⛔ no — not yet run; this gate unblocks it | — |
| TASK-828 / 829 / 838-840 / 851 / 852 | — | ⛔ **NO** — other gates (`848`, `849`, `850`) own them; their files were dirty **on arrival** and are ⛔ not this diff's leakage | — |

---

## 0. ⛔ INSTRUMENT DECLARATION — read this before trusting any absence claim below (`SC-§39`)

⚠️ **This session exposed ⛔ NO shell/Bash tool ⇒ ⛔ I could not run `git diff`, `git status` or `git show`.** Stating it plainly rather than implying a git-backed verification I did not perform — which is the exact discipline `TL-§5c` was bought with.

**What I actually used, and the positive control for each:**

| claim | instrument | positive control (proof the instrument could have SEEN a change) |
|---|---|---|
| `Advance`'s plane not relaxed | **full read** of `SiegeLadderClimbStatics.cpp` (384 lines) + token grep | ✅ the grep **did** return `UE_KINDA_SMALL_NUMBER` at `:88`, `<= 0.f` at `:158`, `<=` at `:119`/`:159` ⇒ ⛔ the instrument reads this file and ⛔ would have surfaced an added epsilon |
| `SiegeLadderClimbStatics.{h,cpp}` not written by TASK-853 | **mtime ordinal** (Glob sorts oldest→newest) | ✅ `Tests/SiegeLadderClimbTest.cpp` (the file TASK-853 **did** write) sorts in the ⛔ newest cluster, alongside the other live lanes' files (`SiegeControlsHelpTest.cpp`, `SiegeAcquisitionFunnelTest.cpp`, `SiegeInvisibilityTest.cpp`, `SummonedUnit.cpp`, `HeroCharacter.cpp`). ⛔ `SiegeLadderClimbStatics.cpp` sorts **~24 files earlier**, and `SiegeLadderClimbStatics.h` before `SiegeCombatStatics.h`/`SummonedUnit.h`. ⇒ it was ⛔ not written in this window |
| the arithmetic | **re-derived from source myself** (fixture constants + engine `Vector.h` read at `C:\Program Files\Epic Games\UE_5.8\...`) | the engine source is quoted verbatim in WARN-1 |

⛔ **What this canNOT prove: BYTE-identity against `HEAD`.** A write-then-revert is invisible to mtime ordering. ⇒ **carried forward as a named check for `TASK-855`'s host — see §7.** ⚖️ *Semantically, the plane is intact and unrelaxed; that is what the automatic-fail condition is about, and it is verified.*

---

## 1. (a) ⭐⭐ `Advance` IS UNTOUCHED AND THE PLANE IS UNRELAXED — ✅ PASS

`SiegeLadderClimbStatics.cpp:158-160`, read verbatim:

```cpp
const bool bPassedTheTop = FVector::DotProduct(ToTop, Direction) <= 0.f;
const bool bWithinTolerance = ToTop.SizeSquared() <= FMath::Square(ArrivalToleranceUU);
if (bPassedTheTop || bWithinTolerance)
```

- ✅ **`<= 0.f` — no epsilon, no `KINDA_SMALL_NUMBER`, no `UE_SMALL_NUMBER`, no tolerance term, no sign flip.**
- ✅ **The OR is in its original order**, both arms present, arrival still tested **before** the watchdog (`:155-157`).
- ✅ The file's ⛔ only near-zero constant is `UE_KINDA_SMALL_NUMBER` at `:88`, inside `Begin`'s **rise** guard — pre-existing, unrelated to arrival, and it is what proves the grep was not blind.
- ✅ `ArrivalToleranceUU = 16.f` (`SiegeLadderClimbStatics.h:188`) unchanged; `SteerLookAheadUU = 150.f` (`:371`) unchanged; `float LengthUU` (`:116`) unchanged.

⇒ ⛔ **The automatic-fail condition did NOT fire.**

## 2. (b) ⭐ THE MARGIN IS LITERALLY PRESENT AND THE LENGTH IS RE-DERIVED IN DOUBLE — ✅ PASS

`Tests/SiegeLadderClimbTest.cpp:2225` (📌 *dated hint; located by symbol per `SC-§38`*):

```cpp
const FVector JustPastTopOffLine = PointOffLine(State, (State.End - State.Start).Size() + 1.0, 200.0);
```

- ✅ `(State.End - State.Start).Size()` — `FVector` is double ⇒ **double**. ⛔ **Zero `static_cast<double>(State.LengthUU)` survives at this site** (verified by grep: the cast appears at six OTHER lines and ⛔ not here).
- ✅ `+ 1.0` present **literally**, as a double literal.

### ⭐ THE SHIPPED LINE'S ACTUAL VALUE — RE-DERIVED, NOT QUOTED: **`dot = -1.000000e+00`**

Fixture at source: `LadderFoot(-450,0,0)`, `LadderTop(-150,0,1200)` (`:78-79`), hero half-height **96** (`:1975`). ⛔ **The lift is equal at both ends** (`Begin`, `:68-69`) ⇒ `End - Start = (300, 0, 1200)` **whatever the capsule is** — so the half-height (88 vs 96) cannot move any figure here.

```
Dir   = (300,0,1200).GetSafeNormal() = (0.2425356..., 0, 0.9701425...)   ⇒ Dir.Y is EXACTLY 0
Cross = (Up × Dir).GetSafeNormal()   = (0, 1, 0)  EXACTLY               ⇒ dot(Cross, Dir) = 0 EXACTLY
ToTop = End − Probe = −Dir·1.0 − Cross·200
dot(ToTop, Dir) = −1.0 − 200·0 = −1.000000e+00        (residual ≲ 1e-13 from normalisation only)
```

✅ **CONFIRMED: the shipped line yields `-1.000000e+00`, ⛔ NOT the `-9.999529e-01` the diagnosis tabled and the dispatch relayed.** `-9.999529e-01` is what `L_float + 1.0` gives; the diagnosis's **prose disagreed with the snippet beside it and the snippet was right**. Immaterial to every verdict (same sign, 4 orders of magnitude off the knife edge). ✅ **The programmer found this itself and recorded it rather than quietly adopting the number — credited, and it is the correct handling.**

### ⭐⭐ AND THE ANTI-TAUTOLOGY CASE IS **STRONGER** THAN EITHER DOCUMENT ARGUED

The diagnosis rejected option A (true `.Size()`, no margin) on the ground that it lands on **exactly `+0.0`** and passes only through the `=` of `<=`. ⛔ **That "exactly" is itself an idealisation.** `Advance` computes `dot(End − Probe, ClimbDirection(State))`, in which `Size()` and `(End−Start)·Dir` are **two separately-rounded quantities**: `Dir` comes back through `InvSqrt`, so `Dir·Dir` is `1 ± 1 ULP`. ⇒ under option A the dot is **within a few ULP of zero with EITHER SIGN (~±3e-13)** — ⛔ not provably `+0.0`, and ⛔ not provably `<= 0`.

⇒ ✅ **The `+ 1.0` margin is REQUIRED, not preferred, and for a reason ⛔ stronger than "a socket retune could flip it": on today's geometry option A's sign is ⛔ already not guaranteed by the arithmetic.** The board's item (2) ruling is upheld on independent grounds.

## 3. (c) ⭐⭐ THE ROW IS STILL A TRIPWIRE — ✅ PASS, VERIFIED THREE WAYS MYSELF

⛔ I did not adopt the handoff's table. Re-derived:

| the defect the row exists to catch | what the row computes | goes RED? |
|---|---|---|
| `Advance` re-pointed at `SteerDirection` | `Steer ≈ (Aim−Probe).GetSafeNormal()`, `dot(ToTop, Steer) = +200.0025` ⇒ `bPassedTheTop` false | ✅ **YES** (`TestFalse` at the `Advance` call fails) |
| the arrival **PLANE** deleted, sphere kept | `ToTop.SizeSquared = 40001.0` vs `16² = 256` ⇒ `bWithinTolerance` **false** | ✅ **YES** |
| `SteerDirection` collapsed into `ClimbDirection` | self-check becomes `dot(ToTop, Line) = −1.0`, which is ⛔ not `> 1.0` | ✅ **YES** (the self-check fails first) |

- ✅ **Item (3)'s required number confirmed independently: the `(c)` self-check reads `200.002500`** (`|ToTop| = sqrt(1 + 40000) = 200.00249999…`), against a `> 1.0` bar. It moved by 0.0025 and did ⛔ not become vacuous.
- ⭐ **And the 200 uu offset does real work:** `40001 > 256` means the **sphere path is excluded**, so this row measures the **plane and nothing else** — exactly what its comment claims.
- ✅ ⛔ **No assertion weakened, deleted, renamed or re-ordered.** The two ORed arrival paths are untouched. The `(c)` block still carries 1 self-check + 3 assertions, in order.

## 4. (d) ⛔ THE SIX SIBLING SITES — ✅ UNCHANGED, ZERO EDITS

Grep for `static_cast<double>((Unit)?State\.LengthUU` in the file returns **exactly six** hits: `:2274` · `:2321` · `:2387` · `:2435` · `:2462` · `:2530` — the same six the handoff enumerated, all present, none rewritten, and ⛔ none at the fixed site. ✅ No scope creep.

### ⛔⛔ WARN-1 — **THE HANDOFF'S "INVERTED FINDING" DOES NOT REPRODUCE. ⛔ DO NOT PUT IT IN LAW AS MEASURED.**

The handoff's §4 headline — *"sweeping `:2435` to a true `.Size()` would turn a CURRENTLY-GREEN row RED; the steer flips to exactly `−Line`"* — is ⛔ **not what the shipped code does.** I re-derived it at source, and the claim omits one branch of the very function it models.

**The site (`:2435-2437`), and the shipped clamp it plays against (`SiegeLadderClimbStatics.cpp:235-246`):**

```cpp
const double AimUU = FMath::Clamp(AlongUU + (double)SteerLookAheadUU, 0.0, (double)State.LengthUU);
const FVector Aim  = State.Start + Along * AimUU;
const FVector Steer = (Aim - CurrentWorld).GetSafeNormal();
return Steer.IsZero() ? Along : Steer;
```

✅ **The half the handoff got right:** the clamp really is at `State.LengthUU`, **the float** — so today's float-derived top sample lands exactly on it, `Aim − Probe` is exactly zero, `Steer.IsZero()` fires and the function returns `Along` ⇒ the sample agrees. **Confirmed.**

⛔ **The half that is wrong:** under the counterfactual sweep, `Aim − Probe = −Dir × 4.706e-05`. It never reaches the normalisation. **UE 5.8 `Vector.h:2058-2073`, read verbatim:**

```cpp
const T SquareSum = X*X + Y*Y + Z*Z;
if (SquareSum == 1.f) { return *this; }
else if (SquareSum < Tolerance) { return ResultIfZero; }     // Tolerance = UE_SMALL_NUMBER
```

`UE_SMALL_NUMBER = 1.e-8f` (`UnrealMathUtility.h:130`). `SquareSum = (4.706e-05)² = 2.2147e-09` ⇒ **`2.2147e-09 < 1e-08` ⇒ `GetSafeNormal` returns ZeroVector ⇒ `Steer.IsZero()` ⇒ the function returns `Along` = `Line` ⇒ `Equals(Line, 1e-5)` is TRUE ⇒ the sample AGREES ⇒ row (a) stays GREEN.**

⇒ ⛔ **Sweeping `:2435` would be a NO-OP at all 26 samples on this geometry, ⛔ not a red row.** The printed `Steer = (−0.242535625, 0, −0.9701425)` is what an **unguarded** normalise returns; the model applied the `IsZero` fallback in the *unswept* column and an exact normalise in the *swept* column — ⛔ the same branch, ⛔ two different treatments. The same omission voids the companion claim about `:2530` (its one `Cross = 0` grid cell keeps its `IsZero()` coverage under a sweep, for the identical reason).

### ⭐⭐ THE CORRECTED FINDING — sharper than the one it replaces, and it is the part worth keeping

The flip the handoff described is **real in kind and geometry-gated**, and the gate is measurable:

```
flip requires |L_true − L_float| ≥ 1e-4              (so that SquareSum ≥ UE_SMALL_NUMBER)
max float32 error at L ∈ [1024, 2048) = ½ ULP = 2^10·2^-24 = 6.1035e-05   <  1e-4   ⇒ ⛔ CANNOT flip
our L = 1236.93 ⇒ actual error 4.706e-05, i.e. 2.1× inside the guard
next binade   L ∈ [2048, 4096): ½ ULP = 1.2207e-04   >  1e-4   ⇒ ⚠️ CAN flip, for ~18% of lengths
```

⇒ ⛔ **A longer ladder — anything over 2,048 uu — turns the handoff's fiction into fact.** ⭐ **That is a genuine latent trap and it belongs on the record; the `−Line` measurement does not.**

**RULINGS:**
1. ✅ **The VERDICT stands: ⛔ change none of the six.** Board item (5) forbids it outright, and my measurement gives an *additional* reason (a sweep there buys nothing).
2. ⛔ **The EVIDENCE must not be enshrined.** ⛔ Do not write *"`:2435` must stay float-derived or row (a) goes red"* into `CONVENTIONS.md`, a task spec or a handoff citation — it is ⛔ false as stated, and a future task honouring it would be obeying a measurement that never happened (⚖️ **`SC-§38` clause 4's shape exactly: a law describing BEHAVIOUR rots silently and instructs the next task**).
3. ✅ **The GENERALISATION is sound and I endorse it, on its own merits rather than on that number:** *derive a probe in the precision of the thing it will be COMPARED AGAINST.* `:2225` is compared against `State.End`/`ClimbDirection` (**doubles**) ⇒ derive in double. `:2435`/`:2530` are compared against a clamp at `State.LengthUU` (**a float**) ⇒ float-derived is the intent-preserving choice. ⛔ *"Fix the float casts"* remains the wrong generalisation. ⇒ **manager: phrase the clause from the PRINCIPLE, ⛔ never from the `−Line` table.**
4. ⚠️ ⛔ **This changes nothing about the applied fix** — it concerns sites the task correctly did ⛔ not touch. It is ⛔ not a blocker; it is a ⛔ record correction, and it is filed under the same law the whole batch exists for.

---

## 5. (e) FENCES + SUITE DELTA — ✅ PASS

- ✅ **One file.** `Tests/SiegeLadderClimbTest.cpp` only. `SummonedUnit.*`, `SiegeCombatStatics.*`, `HeroCharacter.*`, `Tests/SiegeHeroLadderClimbTest.cpp`, `ClimbableTower.*` — ⛔ not this diff's (dirty on arrival, other gates').
- ✅ **The rename is COMPLETE.** `LevelButOffLine` survives at **exactly one place in all of `Source/`**: `:2221`, inside the explanatory comment, as prose about the rename. ⛔ Zero identifier occurrences. `JustPastTopOffLine` appears at `:2225`, `:2226`, `:2229`, `:2236` — 4 occurrences, all inside the one block.
- ✅ **THE RENAME IS THE RIGHT CALL AND I UPHOLD IT.** `LevelButOffLine` asserted the exact falsehood that authored the bug; keeping the name would have made the margin look like a wart on a correct name and invited its removal. ⭐ **Recording the OLD name in the comment is better than either option alone — it leaves a trail for anyone who greps `git log -S"LevelButOffLine"`, which is how the diagnosis found the birth commit in the first place.**
- ✅ **SUITE DELTA: `0`.** `Tests/SiegeLadderClimbTest.cpp` = **22 declared** in the working tree; the changed region contains ⛔ no `IMPLEMENT_` macro, and the diff is one expression + one comment block + a 4-occurrence local rename. ⇒ delta 0 is consistent with everything observable. (⛔ HEAD's 22 is the programmer's measurement, ⛔ not mine — no git here.)
- ✅ **FRESH CENSUS, MINE, AT MY INSTANT** (`TL-§5b`'s pattern and scope, ⛔ not a bare `^IMPLEMENT_`):

  > **`389 declared across 29 files in Source/GitClaudeUnrealTest/Siegebound/Tests/*.cpp`** (pattern `^IMPLEMENT_SIMPLE_AUTOMATION_TEST`).

  ⚠️ **THE DRIFT, IN ONE EVENING, ON ONE TREE: `376` (the dispatch) → `378` (TASK-853's census) → `389` (mine).** ⛔ **Not one of them was wrong when it was taken.** ⛔ Reconcile to ⛔ none of them, ⛔ including this one. `SiegeLadderClimbTest.cpp` = 22 of the 389.
- ⛔ **I did NOT execute the suite. ⛔ NO pass count is written anywhere in this report** (`TL-§5c` part 1). `389 declared` ⛔ is not `389/389`.
- ✅ No compile, no editor, no MCP, no Git write, no `CONVENTIONS.md` edit by me.

## 6. (f) ⛔ RECORDED SO A FUTURE READER CANNOT MISREAD IT

1. ⛔⛔ **THIS ROW'S REDNESS WAS ⛔ NEVER A GAMEPLAY DEFECT. ⭐ THE LADDER JONATHAN PLAYTESTED IS FINE.** The failing row never described a reachable state, and if reached it clears in **one frame (16.7 ms)**. What was fixed is a **test that measured itself wrong**.
2. ⛔⛔ **THE UNTESTED-DESCENT DEBT IS ⛔ UNCHANGED — ⛔ NEITHER WORSENED NOR DISCHARGED.** ✅ **VERIFIED: `handoffs/TASK-853-programmer.md` §5 states this correctly and explicitly** (*"nothing in this handoff may be cited to close it… It still owes a real PIE run"*), and ⛔ **nothing anywhere in that handoff implies otherwise.** Arrival is the **same plane test at `End`** whichever way the line is armed ⇒ ⛔ nothing in this diagnosis or this fix is descent-specific. ⛔ **A `Result={Success}` on this row in TASK-855 does ⛔ NOT close the descent row either.**

---

## FINDINGS

- **[WARN-1]** `handoffs/TASK-853-programmer.md` §4 — the *"sweeping `:2435` turns a green row red / steer flips to exactly `−Line`"* measurement ⛔ **does not reproduce**: `|Aim − Probe| = 4.706e-05` ⇒ `SquareSum = 2.21e-09 < UE_SMALL_NUMBER (1e-08)` ⇒ `GetSafeNormal` returns ZeroVector ⇒ the `Steer.IsZero()` fallback returns `Along` ⇒ the sample still agrees. The model omitted the tolerance branch it relied on in the adjacent column. **Fix:** ⛔ do not carry that table into law or into any spec; carry the **principle** (probe precision follows the comparand) plus the **corrected, geometry-gated** finding (a flip needs `|ΔL| ≥ 1e-4`, impossible below a 2,048 uu line, possible above it). ⛔ Not a blocker — it concerns sites the task correctly left alone.
- **[WARN-2]** `Tests/SiegeLadderClimbTest.cpp:2235` — the `(c) READER 1` assertion message still reads *"a pawn **level with the top** but 200 uu OFF the line"* while the probe is now **1 uu past** it. ⚖️ **RULING: the programmer's judgement is UPHELD and this is ⛔ not a blocker** — item (3) fences assertions, a message string is part of one, and the block comment at `:2212-2224` carries the accurate account. ⛔ **But the message IS wrong, and this row's entire history is a test that lied about its own state.** **Fix (rider, ⛔ do not re-open this diff for it):** the **next task that legitimately edits this file** corrects *"level with the top"* → *"1 uu past the top"*, one word, no assertion logic touched. **Mitigation until then, and it is directed at `TASK-855`:** if this row reports red, ⛔ **read the block comment at the probe, ⛔ not the assertion string** — the string is known-stale.
- **[WARN-3]** `TASKBOARD.md` TASK-854 `spec:`/`names:` and TASK-855 `names:` both point at **`qa/TASK-853.md`** ⛔ — which contradicts `SC-§29`'s own gate-naming precedent (`qa/TASK-849.md`, `qa/TASK-850.md`, `qa/TASK-550.md`: **the gate file is named for the GATE task**) and would have left `TASK-855`'s declared input resolving to nothing. **Fix applied within my own territory:** the authoritative verdict is **this file, `qa/TASK-854.md`**; a ⛔ non-authoritative one-line pointer sits at `qa/TASK-853.md` so the downstream reference resolves. **Manager:** correct `TASK-855`'s input pointer to `qa/TASK-854.md`.
- **[WARN-4]** ⛔ **THE BOARD ROW EDITS ARE OWED AND I COULD NOT MAKE THEM.** This session exposed ⛔ no line-editing tool, and `TASKBOARD.md` is ~18,400 lines — a whole-file rewrite is ⛔ exactly the write-race hazard the board has been bitten by before. ⇒ ⛔ **I did not touch the board.** The two exact edits are in §7; **orchestrator to apply.**
- **[NIT-1]** `:2232` — `ArmPinnedHeroClimb(Arriving, 350.f)`'s return value is ⛔ unchecked, while every other arm in the file is wrapped in a `TestTrue` self-check. **Pre-existing, ⛔ not this diff** (the identical arm is checked at `:2160`, so a failure would already have returned). ⛔ Do not open the diff for it.
- **[NIT-2]** `:2442`'s message claims agreement *"from the foot to the top — the end-clamp included"*. Measured: at the top sample the steer comes from the **`IsZero()` fallback**, ⛔ not from a normalised aim — and it would ⛔ also agree if the upper clamp were **deleted** (the aim would sit 150 uu ahead, still on the line). ⇒ that sample does ⛔ not discriminate the clamp it advertises. **Pre-existing test-design observation** (`SC-§37`), recorded because WARN-1 forced the measurement; ⛔ ⛔ **not TASK-853's**, and ⛔ not to be swept into this batch.
- **[NIT-3]** `:2228`'s self-check message says *"strictly positive"* while the assertion requires `> 1.0`. Pre-existing, harmless, ⛔ unchanged by this diff.

---

## 7. NOTES FOR BUILD-MASTER (TASK-855) — and the two board edits

**Carried-forward checks (⛔ mine could not close them; ⛔ yours can, you have git):**

1. ⛔⛔ **RE-VERIFY THE ZERO DIFF WITH GIT, BECAUSE I COULD NOT.** Before committing:
   `git diff --numstat -- Source/GitClaudeUnrealTest/Siegebound/SiegeLadderClimbStatics.h Source/GitClaudeUnrealTest/Siegebound/SiegeLadderClimbStatics.cpp` ⇒ ⛔ **must return NOTHING.** ⛔ Any output = ⛔ STOP and re-gate; the `<= 0` arrival plane is an ⛔ automatic-fail surface.
2. ✅ **Commit pathspec is `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeLadderClimbTest.cpp` and ⛔ nothing else from this row.** If `SiegeLadderClimbStatics.*` shows dirty in `git status`, that is finding #1, ⛔ not a file to sweep in.
3. ✅ **Confirm the delta with git:** `IMPLEMENT_SIMPLE_AUTOMATION_TEST` count in that file, `HEAD` vs working ⇒ **22 → 22, delta 0.** My working-tree half reads 22.
4. ⛔⛔ **THE ACCEPTANCE ROW, AND ⛔ ONLY AN EXECUTION SATISFIES IT:** report `Result={Success}` **verbatim** for `Siegebound.LadderClimb.ClimbDirectionIsByteIdenticalAndItsThreeOtherReadersStillReadTheLine`, **plus both totals** (`Result={Success}` and `Result={Fail}`). ⛔ **A census may ⛔ not be reported as `N/N`** (`TL-§5c`). ⭐ `SC-§39`'s positive control here = a `Result={Fail}` line ⛔ reading `0`; an ⛔ ABSENT `Result` line is an ⛔ unreadable instrument, ⛔ not a green suite.
5. ⚠️ **Take a FRESH census at YOUR instant.** Mine reads **389 declared across 29 files** and was already the third different number of the evening (`376 → 378 → 389`). ⛔ Do ⛔ not reconcile to it.
6. ⚠️ **If the named row goes RED:** its assertion message says *"level with the top"* and is **known-stale** (WARN-2) — the probe is **1 uu past**. Read the block comment at `:2212-2224`. ⛔ Do not re-derive the probe from the message.
7. ⛔ **`TASK-855` item (4)'s annotation of `handoffs/TASK-805-buildmaster.md` §3 is ⛔ still owed** and is ⛔ not discharged by this gate.
8. ⛔ **Neither this gate nor a green row closes Jonathan's untested-descent debt.** §6.2.

**BOARD EDITS OWED (⛔ orchestrator to apply — see WARN-4):**

- **TASK-853** — `- status: qa-passed` (keep the existing summary text; append: `✅ GATE TASK-854 PASS 2026-09-02, 0 BLOCKER / 4 WARN — report qa/TASK-854.md. Acceptance remains TASK-855's EXECUTED Result={Success} on the named row.`)
- **TASK-854** — `- status: done — 2026-09-02: PASS, 0 BLOCKER / 4 WARN / 3 NIT over TASK-853 (SC-§29 ledger: TASK-853 only). Advance's `<= 0` plane verified UNRELAXED at source; margin + double re-derivation confirmed; dot = -1.000000e+00 (⛔ not -9.999529e-01); self-check 200.002500; row still goes RED three ways; six siblings unchanged; delta 0; census 389 declared across 29 files (⛔ declared, ⛔ not a pass count — no suite run). ⭐ WARN-1: TASK-853's "sweeping :2435 turns a green row red" measurement DOES NOT REPRODUCE (GetSafeNormal's UE_SMALL_NUMBER branch) — verdict upheld, evidence must NOT enter law. Report qa/TASK-854.md. TASK-855 UNBLOCKED.`
- **TASK-855** — `blocked-by` is satisfied on the QA side; correct its `names:` input from `qa/TASK-853.md` to **`qa/TASK-854.md`** (WARN-3).

## 8. ⭐ ONE LAW RECOMMENDATION, FOR THE MANAGER TO PHRASE OR REFUSE

The dispatch asked whether the `-9.999529e-01` incident is worth a clause beyond `TL-§5c`. ✅ **Yes, and it is ⛔ not a new law — it is `SC-§38` applied to NUMBERS instead of to line numbers, which is the same defect wearing a different coat:**

> **A FIGURE quoted in prose is a ⛔ DATED ANNOTATION. ⭐ The ⛔ EXPRESSION it was computed from is the key.** ⛔ When a report's prose and the code snippet beside it disagree, the ⛔ **SNIPPET IS NORMATIVE** and the number is the thing that rotted. ⇒ a recommendation that quotes a value ⛔ states the expression that produces it, and a consumer ⛔ re-derives rather than relays.

⚖️ **Three data points in one evening, ⛔ all the same shape:** (1) `-9.999529e-01` travelled report → dispatch → nearly into a handoff, corrected only because the implementer re-derived instead of quoting; (2) `306/306` travelled into three records **unexecuted** (`TL-§5c`); (3) **WARN-1 above** — a figure that was ⛔ never derivable from the shipped code at all reached a board row and a `names:` block within the hour. ⭐ **The dividend is already proven twice: the ⛔ only two figures that survived contact this evening were the ⛔ two that somebody re-computed** (TASK-853's, and this gate's).

⚠️ **And the honest scoring of this batch: ⛔ every one of those three defects was caught by the ⛔ NEXT agent re-deriving, ⛔ never by the agent that wrote it, and ⛔ never by a gate that read for style.** That is the pipeline working — and it is ⛔ only working because each stage refused to relay.
