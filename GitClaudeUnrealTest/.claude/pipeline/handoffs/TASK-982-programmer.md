# TASK-982 — [BS-1] `BrightSun`: the prevention window + the height-scaled timer

**Author:** gameplay-programmer · **Date:** 2026-09-04 · **Status:** `ready-for-qa` · **Gate:** `TASK-986` · **Ship host:** `TASK-987`
**Law:** `FOG-§10` (all of `§10.1`/`§10.2`/`§10.3`/`§10.6`/`§10.7`) · `FOG-§9.4` · `FOG-§9.5` · `FOG-§6` · `HIGH-§1` · `SC-§37` · `SC-§33` · `SC-§40` cl. 2 · `SC-§53` cl. 3 · `SC-§62` · `SC-§65`
**Rulings built to (⛔ not re-derived):** ✅ `J-F13` · `J-F14` · `J-F15` · `J-F16` · `J-F17` · `J-F18` · `J-F19`

---

## §0 — ⛔ READ FIRST: THE THREE THINGS THE GATE MUST NOT TAKE ON TRUST

1. **⛔ `qa/TASK-1011.md` NIT-2 is SUPERSEDED and I did NOT treat it as a clearance.** It graded `FogVolume.h`'s *"cut to 609.6 from `2000` — a 69.5% reduction … every RANGED unit"* as *"true today"*. All three parts were false. The repair cites **`qa/TASK-1014.md`** in the header text itself and names NIT-2 as superseded **inside the comment**, so the stale verdict cannot travel forward from that file. **Measured at source, not taken from the report:** `ASummonedUnit::UnitEngagementRadiusUU = 5000.f` (`SummonedUnit.h:895`), `FSiegeFogTuning::FogVisionCeilingUU = 609.6f` (`SiegeFogStatics.h:268`), `1 − 609.6/5000 = 0.87808` ⇒ **87.8%**, and the cut is applied in `GatherHostileAgents` to whatever carries a `Vision` query — **every class, melee included**.
2. **⛔ SUITE DELTA ONLY: +8 registered automation tests**, all in the new `Tests/SiegeBrightSunTest.cpp` (counted at source: 8 `IMPLEMENT_SIMPLE_AUTOMATION_TEST` macros, 8 `"Siegebound.BrightSun.…"` names). Plus **new rows inside an existing registration** — `Tests/SiegeFogVolumeTest.cpp` test 1 gained the `FogClear == 7` assertion — which is **not** a new registration and is **not** counted in the delta. **⛔ NO absolute is claimed — nothing in this batch has been executed, and I ran no compile and no suite.**
3. **⛔ I moved 3 pins and renamed 1 registered test.** Every one is listed in §6 with old value, new value and why. Two of them **had to** move (a changed signature and a changed enum tail); the file's own comments instructed one of them in advance.

---

## §1 — WHAT SHIPPED

### `Source/GitClaudeUnrealTest/Siegebound/FogVolume.h` + `.cpp` — ⛔ EXTENDED, ⛔ never duplicated

**The second scalar** (`FOG-§10.1`): `double FogPreventedUntilTimeSeconds = 0.0;` — `Transient`, `VisibleInstanceOnly`, private, beside `FogActiveUntilTimeSeconds` on the **same one actor**. ⛔ No second state actor, ⛔ no second `ReadFogState`, ⛔ no per-actor flag, ⛔ no companion `bool` (`qa/TASK-1011.md` NIT-5's explicit warning, followed).

**Four new `EditDefaultsOnly` tunables**, each with its `HIGH-§1` consequence written beside it:

| symbol | value | derivation written in the comment |
|---|---|---|
| `BrightSunBaseDurationSeconds` | `120.f` | his *"base … is 2 minutes"* ⇒ 2 × 60 |
| `BrightSunBonusSecondsPerStep` | `60.f` | his *"increases by 1 minute"* |
| `BrightSunHeightStepUU` | `1524.f` | ⭐ **50 ft × 30.48 cm/ft** — his 2026-09-04 amendment. ⛔ NOT `609.6`, and ⛔ NOT a reference to `FogVisionCeilingUU` |
| `ArenaGroundReferenceZUU` | `0.f` | ✅ `J-F13` — the FLAT grass datum. The comment quotes `CONVENTIONS:131` verbatim so a bare `0.f` cannot read as a placeholder and get "tidied" |

**Seven functions** (all public):

| signature | role |
|---|---|
| `bool RaiseFog()` | ⚠️ **`void` ⇒ `bool`.** Now REFUSES (writing nothing) while the window is up — `J-F19` |
| `bool ApplyBrightSun(ETeamId CasterTeam)` | the cast: `J-F18`'s conditional branch + the one-way door |
| `void ResetFog()` | now zeroes **BOTH** deadlines |
| `bool IsFogPrevented() const` | the `SHIELDED` predicate — exact sibling of `IsFogActive()` |
| `float GetFogPreventionSecondsRemaining() const` | ⭐ **accessor (i)** — live remainder, `0` when not `SHIELDED`, ⛔ never cached |
| `float GetBrightSunWindowSeconds(ETeamId CasterTeam) const` | ⭐⭐ **accessor (ii)** — the DURATION accessor, item (5a) |
| `static float BrightSunWindowSeconds(HeroZUU, GroundReferenceZUU, BaseSeconds, BonusSecondsPerStep, HeightStepUU)` | the pure formula; ⛔ no defaults (`SC-§33`) |

### `Source/GitClaudeUnrealTest/Siegebound/CardRow.h`
- **Item (0) rider — LANDED.** See §2.
- **`ESpellEffect::FogClear` APPENDED LAST** (== 7). See §3.

### `Source/GitClaudeUnrealTest/Siegebound/SpellLibrary.cpp`
- `FogCover` arm: now checks `RaiseFog()`'s return and `return false;` on refusal (⛔ before the VFX tail, so a refused fog spawns no `NS_Spell_Fog`).
- **New `case ESpellEffect::FogClear:` arm** → `FindOrSpawn` + `ApplyBrightSun(CasterTeam)`, same refuse-on-false shape.

### Tests
- **NEW `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeBrightSunTest.cpp`** — 7 tests. See §5.
- **`Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogVolumeTest.cpp`** — minimum forced edits only. See §6.

---

## §2 — ⛔ ITEM (0): THE ONE-SENTENCE RIDER. **IT LANDED.**

`CardRow.h`'s `FogCover` doc used to open: *"Fog: raises the WORLD-GLOBAL fog **for EffectDuration**, then it lifts."*

It now opens *"raises the WORLD-GLOBAL fog, which then lifts on its own"*, followed by a correction paragraph stating, in the code's own terms:
- the duration is **`AFogVolume::FogDurationSeconds`**, read off the CDO by `RaiseFog()`, **which takes no argument**;
- the row's `EffectDuration` cell **is not read by the fog arm at all**;
- the old sentence was true **only by the coincidence that both values are `300`**;
- ⭐ **`TASK-1016` is the row that makes the cell authoritative**, and until it lands the cell is inert.

⛔ **ZERO behaviour changed by this rider. I did not implement `TASK-1016`.**

**Item (0a) also landed** (`FogVolume.h`'s tunable doc): `2000` ⇒ `5000` · `69.5%` ⇒ `87.8%` · *"ranged"* ⇒ **every** unit, with the struck numbers **named rather than deleted** (`SC-§53` cl. 3) so a reader who remembers them finds the replacement.

⚠️ **The fourth false claim in that block is DELIBERATELY LEFT STANDING and flagged in place:** `FogVolume.h`'s *"until `TASK-840` lands there is nothing on the other side of it"*. `qa/TASK-1014.md` routed exactly that one to ⭐ **`TASK-1016` item (3)**, not to me, because `TASK-1016` is what makes the cell authoritative — repairing the prose now would describe a wiring that still does not exist. The comment says so explicitly, in place. **⛔ Reported, ⛔ not swept.**

---

## §3 — THE ENUM: `FogClear`, APPENDED **LAST**

`ESpellEffect::FogClear = 7`, after `FogCover = 6`. ⛔ **Not beside `FogCover`, not alphabetical, not grouped** — every one of those is a mid-enum insert that renumbers `FogCover` and silently re-reads every saved `Fog` cell in `/Game/Data/DT_Cards` as BrightSun, with **no compile error, no log line and no red**. I read the append-only rule in the enum's own doc before typing, exactly as the spec instructed.

**⛔ THE NAME IS NARROWER THAN THE EFFECT, and I chose it deliberately.** `FogClear` names the visible half; the *prevention window* is the half that decides matches. I picked it because **four shipped documents already predict it by name** — `handoffs/TASK-839-programmer.md:45`, `handoffs/TASK-998-programmer.md:161`, `qa/TASK-1011.md:144`, `qa/TASK-1015.md:29/48/72/149` — plus `Tests/SiegeFogVolumeTest.cpp`'s own comment. Any other spelling makes five documents false. The enum's doc comment states the narrowness in plain words so nobody "fixes" it, and notes that renaming it now needs a `CoreRedirect` exactly like a retirement.

⭐ **`qa/TASK-1015.md` NIT-2 asked that `CardRow.h:93-95`'s sibling paragraph NOT be tidied away when this landed. It was kept.** Only its **tense** moved (from *"gets its OWN new value"* to *"has its OWN value — `FogClear`, appended below"*), and the comment says so, because leaving it in the future tense would have been the `SC-§65` drift the note exists to prevent. Its job — stopping the two effects being overloaded onto one value — is intact.

**⛔ COMPILE-SAFETY OF THE NEW VALUE, MEASURED AT ALL THREE SWITCH SITES:** `SpellLibrary.cpp:600` (mine, arm added), `DeckBuilderWidget.cpp:1342` (`default:` at 1395), `SpellLineSweep.cpp:177` (`default:` at 256). ⇒ **no unhandled-enum error anywhere.** The two `ESpellEffect::` uses in `SiegePlayerController.cpp` are `== GoldSteal` comparisons, not switches.

---

## §4 — THE MECHANISM, AND WHY EACH RULING IS A PROPERTY RATHER THAN A PROMISE

### ⭐⭐ THE THREE-STATE MACHINE HAS **EXACTLY THREE** STATES

The two deadlines could in principle both be live — *"fogged AND shielded"*, `FOG-§10.3`'s forbidden fourth state. **It is unreachable by construction, and there are only two ways in:**

| way in | what stops it |
|---|---|
| shield arrives during fog | `ApplyBrightSun` **ZEROES** the fog deadline in the same block that stamps the shield ⇒ entering `SHIELDED` always exits `FOGGED` |
| fog arrives during a shield | `RaiseFog` **REFUSES** and returns false ⇒ `FOGGED` can never be re-entered from `SHIELDED` |

Those two functions plus `ResetFog` (which zeroes both) are the **only writers of either deadline in the project** — censused at **3** fog-deadline write sites and **2** prevention write sites by test 6, so a fourth writer goes red.

⭐ **The `J-F19` guard lives inside `RaiseFog`, not at the call site.** That is the same reasoning `TASK-998` used for the `=`: put the rule where the state is, so a second caller cannot get it backwards. It also means the fourth state is closed by the *state object*, not by a spell resolver.

### ⭐⭐⭐ THE ONE-WAY DOOR (item 6): `SHIELDED` ⇒ `CLEAR`, ⛔ NEVER ⇒ `FOGGED`

**There is no expiry handler, and there is nothing to handle.** `ApplyBrightSun` sets `FogActiveUntilTimeSeconds = 0.0` — it does **not** stash a remainder. When the shield lapses, `IsFogActive()` compares the clock against `0.0` and answers false forever. ⛔ **No suspended fog, no paused timer, no remembered remainder exists anywhere in the class** (test 4 asserts the zeroing, bans a second assignment, and sweeps the file for a stash).

⭐ It **zeroes** rather than adding a `bool bFogCleared` because `qa/TASK-1011.md` NIT-5 is right: an *expired* deadline is non-zero and also clear, so a second representation would disagree with the first on day one.

### ⛔ `J-F18` IS A BRANCH (item 7a) — and this row owns the LONGER half + the predicate

```
NewWindow = GetBrightSunWindowSeconds(CasterTeam);   // sampled ONCE, here, at the cast (J-F15)
if (NewWindow < GetFogPreventionSecondsRemaining())  // ⛔ STRICT — EQUAL resets (declared default)
    return false;                                    // ⛔ nothing written; caller refunds net-zero
FogActiveUntilTimeSeconds    = 0.0;                  // the one-way door
FogPreventedUntilTimeSeconds = now + NewWindow;      // ⛔ `=`, never `+=`
```

⛔ **I did not build the two-value message.** That is `TASK-991`, in `SiegePlayerController.cpp`.
⭐ **The boundary is the strict `<`** — he wrote *"LESS than"*, so EQUAL resets. Declared as a default, not as his word.
⛔ **`FMath::Max` appears ZERO times in `ApplyBrightSun`, and test 5(a) is the only assertion in this diff that can see the ruling at all**: `max` and *"reset if longer"* produce the identical remaining time in the longer case, so every duration test passes either way. They diverge only in the shorter case — where `max` would keep the timer while billing 60 gold and eating the card.

### ⛔ `J-F19` — THE TWO PROPERTIES I OWN, AND **THEY ARE TWO**

I own (1) **zero gold moves** and (2) **the card is not consumed**. Both are delivered by returning `false` — and **that is the shipped `RefuseCardPlay` net-zero doctrine reused, not a refusal path invented here.** Measured at both live call sites:

| path | property 1 (gold) | property 2 (card) |
|---|---|---|
| `ResolveSpellInstant` (`SiegePlayerController.cpp:4529`) | `SiegeState.AddGold(Row.Cost)` on the false branch | `ConfirmInstantDraw(Slot, CardID)` sits **after** the refusal's `return` ⇒ unreachable on a refusal |
| `TryConfirmSpellTarget` (`:3203`) | `SiegeState->AddGold(TargetingCost)` on the false branch | `DeckComponent->ConfirmPlayFromHand(TargetingHandSlot)` sits **after** the refusal's `return` |

Test 6 asserts both halves **separately**, and proves property 2 by **index order** rather than by prose — a build that refunds the gold but eats the card satisfies exactly half his ruling, and a test written against either half alone passes the broken build.

⛔ **My refusals `return` rather than falling through to `bResolved = true`, so a refused cast also spawns NO VFX.** `NS_Spell_Fog` blooming over a battlefield that never fogged is the "resolved" lie in visual form.

### ✅ `J-F17` — BrightSun with no fog is LEGAL, and the way that dies is the READ door

The `FogClear` arm uses **`FindOrSpawn`**, not `Find`. It looks like a card that only *removes* state, so `Find` is the tempting call — but a pre-emptive `BrightSun` may be the **first cast of the match**, with no `AFogVolume` in the world yet, and `Find` would make a 60-gold cast a silent no-op. Test 5(d) pins `AFogVolume::Find(` at **0** inside `ResolveSpell`.

### ✅ `J-F13` — THE DATUM IS FLAT. ⛔ THERE IS NO TRACE.

`Height = FMath::Max(0.f, HeroZ − ArenaGroundReferenceZUU)`, `ArenaGroundReferenceZUU = 0.f`. **⛔ No trace, no trace channel, no ignore list, no missed-trace degrade path exists anywhere in the class** — test 2(a) asserts six trace symbols at **zero** in `FogVolume.cpp`. A hero on a hill **earns** the height.

### ✅ `J-F14` — UNCAPPED, asserted at an absurd height on purpose

Test 1's cap row uses **100 steps**, because the tallest *reachable* perch (a ×2 Watch Tower, ~2,400 uu) is exactly **one** step at the 50-ft figure — so a cap set anywhere above 1 step would be invisible to every realistic assertion.

### ⛔ `FOG-§9.5` — the step is DECOUPLED from the vision ceiling

`FogVisionCeilingUU` appears **zero times on code lines** in `FogVolume.{h,cpp}` (test 2(b), both files), and `FSiegeFogTuning` appears zero times (test 2(c) — the indirect spelling of the same coupling). The two numbers were both `609.6` under the retired 20-ft reading; his 50-ft amendment separated the *values* but not the *hazard*, because `J-F12`'s named remedy for over-strong fog is still *"lower the ceiling"*.

### ⭐⭐ ITEM (5a) — THE SEAM, AND WHY IT IS A REQUIREMENT

`float GetBrightSunWindowSeconds(ETeamId CasterTeam) const` is **public, `const`, side-effect-free, and callable outside the cast path**. `ApplyBrightSun` **calls it** rather than computing inline — so there is exactly **one** copy of the formula (test 1 pins `FMath::FloorToFloat(` at **1** file-wide; test 5 pins it at **0** inside `ApplyBrightSun`).

⭐ **`TASK-991` is its second caller and it is boarded ⇒ `SC-§40` cl. 2 is satisfied. `TASK-989` is `GetFogPreventionSecondsRemaining()`'s caller and it is boarded.** Neither is dead surface; this row ships both with no caller of its own, on purpose, because the seam is the one thing a later row cannot add for itself.

**⛔ PINNED SIGNATURES for `TASK-989` / `TASK-991` — test 8 asserts these character-for-character:**
```cpp
float AFogVolume::GetFogPreventionSecondsRemaining() const;                 // X — live remainder, 0 when not SHIELDED
float AFogVolume::GetBrightSunWindowSeconds(ETeamId CasterTeam) const;      // Y — "what window WOULD this cast produce right now?"
```
Both **re-sample live on every call** (test 8 pins `GetTimeSeconds()` at 1 in the first, `TActorIterator<AHeroCharacter>` at 1 in the second). `ApplyBrightSun` is the **only** place either answer is ever frozen, and it freezes it once, at the cast.

### ⭐⭐ `SC-§62` — THE GAME MODE LEARNS **NO** FOG POLICY, AND THE CONDITION IS NOW EXECUTABLE

`ASiegeGameMode::PlayAgain`'s call site is **byte-unchanged**: still `It->ResetFog();`, once. The second zero happens **inside** `ResetFog`, which is the whole reason it is there rather than in a second loop. Test 7 asserts **five policy symbols at zero** across the entire `SiegeGameMode.cpp` (`BrightSun`, `FogPrevent`, `FogDurationSeconds`, `FogVisionCeilingUU`, `FSiegeFogTuning`) — so the exception's *condition*, not just its *grant*, now goes red if a future row leaks a duration, a ceiling, a density or a window up there.

### ⛔ ITEM (4b/4c/4d) — THE DATUM TIE IS AN **ASSERTION**, NEVER A CALL

Test 3 pins `ASummonedUnit::HeightAdvantageMultiplier(ArenaGroundReferenceZUU, ArenaGroundReferenceZUU, Step, Bonus) == 1.0f` and `(+ HeightBonusStepUU, …) == 1.10f`, reading `ArenaGroundReferenceZUU` off `AFogVolume`'s CDO and `HeightBonusStepUU`/`HeightBonusPerStep` off `ASummonedUnit`'s CDO **by reflection** (they are `protected`; ⛔ **I did not widen their access**).

**⛔ THE DAMAGE FORMULA IS UNCHANGED — NOT ONE LINE.** `ComputeOutputDamage` still passes the **target's own Z** as its zero. Test 3 additionally pins **`ArenaGroundReferenceZUU` at ZERO occurrences in `SummonedUnit.cpp`**, which is the fence executed rather than described.

**⛔ `HeightBonusStepUU = 152.4f` is CORRECT and I did not touch it** (`FOG-§10.7` (E) — he confirmed the 10-ft misspeak himself). ⛔ `HeightAdvantageMultiplier`, `HeightBonusStepUU` and `HeightBonusPerStep` were **read only**.

**⚠️ REPORTED, ⛔ NOT SWEPT (`FOG-§10.7` (D) rule 5):** `FSiegeAssistantSnapshot::MarkPlaceGroundZ = 0.f` (`SiegeAssistantSnapshot.h:831`) is a **third site holding this same datum today**. It is not referenced from the fog lane and is **not asserted equal** anywhere in my tests — its own comment licenses it to be retuned for hill-accurate marks, i.e. it may legitimately stop being the flat-grass datum, and pinning it would convert a documented divergence into a false coupling. The `ArenaGroundReferenceZUU` doc names it in place.

---

## §5 — THE NEW TESTS (**+8**) — `Tests/SiegeBrightSunTest.cpp`

| # | registered name | what it can catch |
|---|---|---|
| 1 | `…TheWindowFormulaFloorsAtEveryStepBoundary` | the formula at **0 / 1523 / 1524 / 3047 / 3048** uu **+ a hero on a hill at +1600 ⇒ 1 step** (item 10a ii — the row that would have gone RED against the struck trace default) **+ −500 clamps to 0 steps** + uncapped at 100 steps + four totality rows (zero/negative/NaN step, NaN hero Z) + one-formula-one-copy. ⭐ Every constant is **re-derived from his words** (`2 × 60`, `1 × 60`, `50 × 30.48`) and read off the **CDO**, so the test and the code cannot agree by both being wrong the same way |
| 2 | `…TheHeightStepIsItsOwnConstantAndTheGroundDatumIsNeverTraced` | six trace symbols at 0; `FogVisionCeilingUU` at 0 in **both** `.h` and `.cpp`; `FSiegeFogTuning` at 0 |
| 3 | `…TheGroundDatumIsTheSameHeightWhereElevationDamageIsExactlyOneTimes` | ⭐⭐⭐ **the datum tie** — `1.0f` exactly and `1.10f`; plus `152.4` and `0.10` re-derived; plus the no-runtime-coupling fence over `SummonedUnit.cpp` |
| 4 | `…TheShieldExpiresToClearAndTheFogCanNeverComeBack` | ⭐⭐⭐ **the one-way door** — the zeroing at 1, a **second** fog-deadline assignment banned, no stash file-wide, no `+=` on the shield, live control on the stamp |
| 5 | `…SunOnSunIsAConditionalBranchAndFogDuringTheWindowIsRefused` | the four interactions: `FMath::Max` at 0 (`J-F18`), `IsFogPrevented()` at 1 in `RaiseFog` (`J-F19`), `IsFogActive()` at **0** in `RaiseFog` (`J-F16` — no "already fogged" no-op), `AFogVolume::Find(` at 0 in `ResolveSpell` (`J-F17`) |
| 6 | `…ARefusedFogOrSunSpendsZeroGoldAndKeepsTheCardInHand` | the **writer census** (3 + 2), the refusal census (8), and **property 1 and property 2 as two separate assertions**, the second proven by call-order index |
| 7 | `…PlayAgainZeroesBothTimersWithoutTeachingTheGameModeAnyFogPolicy` | both zeroes in `ResetFog`, the byte-unchanged call site, and **five policy symbols at 0** across `SiegeGameMode.cpp` |
| 8 | `…BothAccessorsAreConstAndCallableWithoutCastingTheCard` | the two pinned signatures + the pure static's; the remainder recomputed from the clock; the duration accessor sampling the hero live; no writes inside the `const` accessor |

*(**8 new registrations**, counted at source — 8 `IMPLEMENT_SIMPLE_AUTOMATION_TEST` macros and 8 `"Siegebound.BrightSun.…"` names in the file. The moved/added rows inside `SiegeFogVolumeTest.cpp` test 1 are **not** counted here; they live inside an existing registration — see §6.)*
⚠️ **CORRECTION, recorded rather than quietly fixed:** I first wrote **+7** in this handoff and on the board, from memory rather than from a count. The number was wrong; the file has always had 8. It is corrected in all three places, and it is exactly the class of unmeasured claim this batch keeps paying for — a delta is a **measurement**, not a recollection.

**⛔ EVERY numeric pin in this file was MEASURED against the real source before it was typed** (a script re-implementing `CountOccurrencesInCode`'s comment-skipping rule, run over the shipped bytes). Two pins I had guessed were **wrong** and were corrected from the measurement: `return false;` in `ResolveSpell` is **8**, not 7 (I had forgotten the shared `!bResolved` tail), and `AddGold(TargetingCost)` in `TryConfirmSpellTarget` is **1**, not 2 (the affordability guard above it never spent, so it has nothing to give back).

**⛔ THE NaN IDIOM:** I first wrote `FMath::Sqrt(-1.f)` and then found `Tests/SiegeCastBarTest.cpp:125` had already ruled that spelling unsafe — it is constant-foldable, and a fast-math build may hand back something finite, making the totality rows pass while testing nothing. The file now uses the sanctioned bit-pattern `MakeQuietNaN()` **plus a `FMath::IsFinite` self-check** so a folded constant goes red instead of green.

**⚠️ DECLARED GAP, stated rather than discovered:** there is not one `SpawnActor` anywhere in `Siegebound/Tests/`, so the behaviours needing a live actor — that `ApplyBrightSun` really moves the expiry, that `RaiseFog` really refuses, that the remainder really counts down — are asserted **structurally** and are **not proven at runtime by this suite**. Each structural form was chosen to be the one that can still fail on the actual hazard.

**⚠️ ONE DELIBERATE OVERLAP:** test 3 re-asserts `HeightBonusStepUU == 152.4` and `HeightBonusPerStep == 0.10`, which `Tests/SiegeHighGroundTest.cpp` already covers. It is kept because the **tie** is meaningless without proving *which* numbers were tied, and because it is the read-back that makes the `1.0`/`1.10` rows non-vacuous.

---

## §6 — ⛔ THE 3 PIN MOVES + 1 RENAME, EACH WITH OLD VALUE, NEW VALUE AND WHY

All four are in **`Tests/SiegeFogVolumeTest.cpp`** (`TASK-998`'s file, verdict returned under `TASK-1011`, so this is not a mid-review mutation). **⛔ QA: this file is part of my diff and belongs in `TASK-986`'s subject list.** ⛔ I touched **nothing else** in it.

| # | pin | old ⇒ new | why it HAD to move |
|---|---|---|---|
| 1 | test 1's *"highest declared value"* row | `ESpellEffect::FogCover` ⇒ `ESpellEffect::FogClear`; `DeclaredCount >= 7` ⇒ `>= 8` | **the file's own comment instructed this in advance**: *"⭐ WHEN `FogClear` IS APPENDED BELOW, THIS ROW MOVES TO IT; it does not get deleted."* Appending correctly makes the old row red |
| 2 | test 2(c) `AFogVolume::FindOrSpawn(` inside `ResolveSpell` | `== 1` ⇒ `== 2` | the second fog arm reaches the **same one** state object through the **same** write door. ⭐ **This is not a loosening:** test 5 in the new file asserts each arm reaches it exactly once (`case FogCover` 1, `case FogClear` 1, `->RaiseFog()` 1, `->ApplyBrightSun(` 1), so the per-arm strength is preserved and the file-wide count is now a *census* |
| 3 | test 4's extraction signature | `void AFogVolume::RaiseFog(` ⇒ `bool AFogVolume::RaiseFog(` | the signature changed. `ExtractFunctionBody` **fails** on a stale signature by design (`SC-§38`), so an unmoved pin would have gone red and looked like a defect in shipped code |
| 4 | test 1's **registered name** | `Siegebound.Fog.TheSpellEffectEnumIsAppendOnlyAndFogCoverIsLast` ⇒ `…AndTheNewestValueIsLast` | the old name asserted the **opposite** of what the test now checks. A registered name is the one string a reader sees in the suite listing without opening the file. ⭐ The new name is value-agnostic so the **next** append does not have to move it again |

**⛔ UNCHANGED and verified still true:** test 2(a)/(b)/(d), test 3 (all), test 4's `+=`/`FMath::Max`/`=` counts, test 5 (all — I did not touch `SiegeCombatStatics.cpp`), test 6 (all), test 7 (all). **⛔ `Tests/SiegeFogClampTest.cpp` and `Tests/SiegeFogTest.cpp` are other rows' and are BYTE-UNTOUCHED**; so is `Tests/SiegeHighGroundTest.cpp`.

---

## §7 — ⛔ WHAT QA SHOULD SCRUTINISE HARDEST

1. **⛔⛔ THE HERO Z IS THE ACTOR ORIGIN (CAPSULE CENTRE), NOT THE FEET — A DECLARED RESIDUAL, AND THE ONE JUDGEMENT CALL IN THIS DIFF.**
   `GetBrightSunWindowSeconds` reads `Hero->GetActorLocation().Z`, so a hero standing on flat grass reads ≈ **+88 uu** (capsule half-height), not 0. **Effect: every step boundary is reached ≈88 uu of true altitude early — 5.8% of one step.** It changes **no** shipped case I can find (flat grass = 0 steps either way; a ×2 tower ≈2,488 vs 2,400 = 1 step either way).
   **⛔ I did NOT invent a foot correction, and that is deliberate:** the damage lane feeds `HeightAdvantageMultiplier` the *same* raw `GetActorLocation().Z`, so a hand-rolled offset here would make the two elevation lanes measure differently **by construction** — the exact divergence `J-F13` was ruled to close. There is no shipped "feet Z" accessor to reuse. ⚖️ **If the gate or the manager thinks the ~88 uu should be subtracted, it is a one-line change and it wants HIS word, not mine — it would move the whole boundary table.**
2. **⛔ THE ENUM NAME `FogClear`** — under-describes the effect (§3). I chose consistency with five predicting documents over descriptiveness. **Flagged for the gate to overrule if it disagrees; it is cheap now and a `CoreRedirect` later.**
3. **⛔ `RaiseFog()`'s `void` ⇒ `bool`** is the only signature change to a shipped function. Its single caller was updated. Verify there is no second caller anywhere (I measured: `->RaiseFog()` occurs once in `SpellLibrary.cpp` and nowhere else in `Source/`).
4. **⛔ THE EQUAL BOUNDARY** (`new == remaining` ⇒ **RESETS**) is a declared default from *"LESS than"*, not his word. Verify you agree with the literal reading.
5. **⛔ THE `<` IN `ApplyBrightSun` COMPARES A FRESH WINDOW AGAINST A REMAINDER, NOT AGAINST A DURATION.** That is correct per his sentence (*"less than the **current time**"* = time **left**), but it is the kind of thing that reads right and is wrong, so it is called out.
6. **⛔ `IsFogPrevented()` has two in-row callers** (`RaiseFog`, and `GetFogPreventionSecondsRemaining` is its value sibling). If the gate reads it as dead surface, note it is the exact public sibling of the shipped `IsFogActive()` and that `TASK-989`/`991` read the state through it or through the remainder.

---

## §8 — ⛔ FINDINGS I AM **REPORTING, NOT FIXING** (all outside my fence)

1. **🚨 `BrightSun` AND `Fog` BOTH GET A RETICLE TODAY, WHICH CONTRADICTS `FOG-§10.1`.**
   `SiegePlayerController.cpp:1037` routes to the instant no-reticle path on `Row->SpellEffect == ESpellEffect::GoldSteal` **and nothing else**. `FOG-§10.1` and `TASK-840` item (2) both say `Fog` and `BrightSun` are **NO RETICLE** (the `Pickpocket` precedent) — but the routing is a **blacklist of one**, so both fog cards enter **targeting mode** and the player aims a reticle at a map-wide effect. ⛔ Not my file (`TASK-989`/`991`/`983` territory), and `TASK-999` item (3b) already rules that this class of guard must be **derived from the delivery data, never from the effect enum**. ⇒ **route to the manager; it may want a row, and `TASK-999`'s fix does not cover the *routing*, only the *glossary text*.**
2. **`SiegeCombatStatics.h:405`** says `ReadFogState` reads *"the `AFogVolume` … (its **ONE** scalar + its tuning)"*. There are two scalars now — though the **seam** still reads only one, which is correct. It sits inside the block `TASK-981`'s fence already assigned to ⭐ **`TASK-1007`** (3 known-false sentences at `:399-405`). ⇒ **a fourth clause for `TASK-1007`.** ⛔ Not swept.
3. **`SpellLibrary.h`'s class-doc effect list names five effects and omits BOTH `FogCover` and `FogClear`.** The omission predates me (`TASK-998` did not add `FogCover` either) and the header is not in my `names:` line. ⇒ **report only.**
4. **`FogVolume.h`'s *"until `TASK-840` lands"* sentence** is stale and left standing by `qa/TASK-1014.md`'s own routing ⇒ ⭐ **`TASK-1016` item (3)**. Flagged in place, in the file. (§2)
5. **`FSiegeAssistantSnapshot::MarkPlaceGroundZ = 0.f`** — the third site holding the arena-floor datum. ⛔ Reported, ⛔ not swept, ⛔ not pinned. (§4)
6. **`FSiegeFogTuning` / `CoreRedirects` window** (`qa/TASK-981.md` NIT-3): still open in the sense `TASK-998` left it — `FSiegeFogTuning` is still not a member of any `UCLASS`. **⛔ I did not close or date it: `TASK-998` shipped the actor half and wrote that note, so it is not mine to date.** ⚠️ What **did** change is the size of the *other* serialisation hazard the same paragraph covers: `AFogVolume` now has **five** `EditDefaultsOnly` properties instead of one, so a rename/retire of any of them after a `BP_SiegeFog` child exists needs a `CoreRedirect`. The class doc states the growth explicitly rather than leaving it implied.

---

## §9 — SCOPE DISCIPLINE

**⛔ NOT DONE, by fence:** no compile · no editor · no MCP · no mutating Git (read-only `git diff`/`status` only, under `SC-§71a`) · no card row (`TASK-983`) · no card art (`TASK-984`) · no visual (`TASK-841`) · no curve (`TASK-981`) · no firing/notice ranges (`979`/`980`) · no hero spells (`J-F9` ruled NO) · no bot (`J-F4`) · no refusal **messages** (`TASK-989`/`991`) · no `TASK-1016` · no `DeckBuilderWidget.cpp` (`TASK-999`) · no `SiegeCombatStatics.{h,cpp}` · no `cards.csv` / `DT_Cards` · no `SummonedUnit.{h,cpp}` write of any kind.

**Files changed (6):**
- `Source/GitClaudeUnrealTest/Siegebound/FogVolume.h`
- `Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp`
- `Source/GitClaudeUnrealTest/Siegebound/CardRow.h`
- `Source/GitClaudeUnrealTest/Siegebound/SpellLibrary.cpp`
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogVolumeTest.cpp` *(4 forced edits only — §6)*
- **NEW** `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeBrightSunTest.cpp`

⚠️ **Three of the six are still UNTRACKED in git** (`FogVolume.h`, `FogVolume.cpp`, `Tests/SiegeFogVolumeTest.cpp`) — `TASK-998` created them and they have not been committed yet, so they enter the repository for the first time with ⭐ `TASK-987`. **⛔ `git status` will show them as `??`, not `M`; that is expected and is not a sign the edits are missing.**

**Caller census, measured after the edit:** `->RaiseFog()` occurs **exactly once** outside `FogVolume.{h,cpp}` (`SpellLibrary.cpp:690`), and `->ApplyBrightSun(` **exactly once** (`SpellLibrary.cpp:740`). ⇒ the `void` ⇒ `bool` change has no second call site to break.

Plus `.claude/pipeline/TASKBOARD.md` — **only** TASK-982's own `status:` line, re-read immediately before the edit and grepped back out after (1 occurrence, line 18323).
