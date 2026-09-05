# TASK-991 — [BS-REFUSE-2] `J-F18`: the sun-on-sun conditional refusal

**Author:** gameplay-programmer · **Date:** 2026-09-04 · **Status:** `ready-for-qa` · **Gate:** `TASK-992` · **Ship host:** `TASK-987`
**Law:** `FOG-§10.7` (A) · `FOG-§10.6` · `FOG-§10.3` · `SC-§37` · `SC-§40` cl. 2 · `SC-§38` · `SC-§65` · `SC-§77` · `TL-§5b`/`§5c`
**Rulings built to (⛔ not re-derived):** ✅ `J-F18` · `J-F24` · `J-F25` · `J-F26` · `J-F13`/`J-F14`/`J-F15` (via `TASK-982`'s accessor)
**Inputs consumed:** `handoffs/TASK-989-programmer.md` (the shared formatter) · `handoffs/TASK-982-programmer.md` (§5's pinned signature block)

---

## §0 — ⛔ READ FIRST: THE FIVE THINGS THE GATE MUST NOT TAKE ON TRUST

1. **⛔⛔ THE REFUSAL IS CONDITIONAL, AND THE CONDITION IS PINNED.** The shipped predicate is
   `if (WouldBeWindowSeconds < RemainingWindowSeconds)`, censused at **1** inside the guard's own
   region, and the routing switch is asserted to **still follow** the message. ⛔ **`FMath::Max` is
   censused at ZERO across the whole of `PlayHandSlot`** — the ruling's only visible instrument,
   asserted on the entry side to match `TASK-982`'s assertion on the state-object side.
2. **⛔⛔ THE FORMATTER WAS REUSED, ⛔ NOT COPIED.** `ASiegePlayerController::WholeSecondsText` is
   still declared **once** (header, censused at 1) and defined **once** (cpp, censused at 1); my
   call site calls it **twice**, so `PlayHandSlot` now holds **3** calls. ⛔ I added **no** second
   formatter and I did **not** generalise `TASK-989`'s one-value message — both wordings still live
   inline at their own call sites, each with its own `NSLOCTEXT` key, which is the structural proof
   that no optional-second-value builder exists.
3. **⛔⛔ `Y` IS COMPUTED, ⛔ NOT DUPLICATED.** `AFogVolume::GetBrightSunWindowSeconds(CasterTeam)` —
   `TASK-982` item (5a), called from **outside** the cast path, censused at **1** in the whole
   controller. ⛔ The height formula is **absent** from this file: `FMath::FloorToFloat` **0**,
   `FMath::FloorToInt` **0**, `AFogVolume::BrightSunWindowSeconds(` (the pure static) **0**, and all
   four tunables — `BrightSunBaseDurationSeconds`, `BrightSunBonusSecondsPerStep`,
   `BrightSunHeightStepUU`, `ArenaGroundReferenceZUU` — at **0**. ⛔ **The card is never cast to find
   out whether to cast it:** nothing in the entry writes an expiry (`ApplyBrightSun` **0**,
   `FogPreventedUntilTimeSeconds` **0**).
4. **⚠️⚠️ I RE-POINTED ⛔ THREE PINS IN `Tests/SiegeFogRefusalTest.cpp` AND ⛔ DELETED NONE.** §5
   lists each one with its old value, its new value and the reason the **claim** is unchanged.
   That file's gate `TASK-990` had **returned (PASS)** before I ran, which is the direction that
   permits the edit — the same direction that kept `Tests/SiegeBrightSunTest.cpp` untouched below.
5. **⛔ SUITE DELTA ONLY: +4 registered automation tests**, all in the **existing**
   `Tests/SiegeFogRefusalTest.cpp` (counted at source: the file now holds **10**
   `IMPLEMENT_SIMPLE_AUTOMATION_TEST` macros and 10 distinct `"Siegebound.FogRefusal.…"` names,
   6 of which are `TASK-989`'s). ⛔ **NO absolute is claimed. I ran no compile and no suite;
   nothing in this batch has been executed by anyone.**

---

## §1 — WHAT SHIPPED (3 files, ⛔ none of them new)

| file | change |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` | the whole sun-on-sun guard in `PlayHandSlot`, immediately **after** its `FogCover` sibling; plus one amended `#include` comment |
| `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h` | `PlayHandSlot`'s doc gains the new refusal (`SC-§65` — an enumeration of refusals that silently omits a new member is the drift defect that section exists to stop) |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogRefusalTest.cpp` | **+4 tests** (7-10), one fixture helper with three callers, **3 re-pointed pins**, and two amended notes |

⛔ **`AFogVolume.{h,cpp}` is BYTE-UNTOUCHED** — both accessors were **called**, never changed.
⛔ **`SpellLibrary.{h,cpp}` is BYTE-UNTOUCHED** — the `FogClear` resolver arm is `TASK-982`'s.
⛔ **`Tests/SiegeBrightSunTest.cpp`, `Tests/SiegeFogVolumeTest.cpp`, `Tests/SiegeFogTest.cpp`,
`Tests/SiegeFogClampTest.cpp`, `Tests/SiegePlacementTest.cpp` are BYTE-UNTOUCHED.**

---

## §2 — THE SHIPPED STRING, AND WHY IT IS SPELLED THAT WAY

```cpp
NSLOCTEXT("Siegebound", "CardRefused_BrightSunWouldShorten",
          "Using Bright Sun right now would reduce fog prevention time from {0} to {1}")
```
⇒ the player reads **"Using Bright Sun right now would reduce fog prevention time from 143 seconds to 83 seconds"**.

- 📌 It is **his sentence**, near-verbatim: *"using bright sun right now would reduce fog prevention
  time from 'x' time to 'y' time"*. The only edit is that his `'x' time` / `'y' time` become the
  formatter's own output (which already spells the unit), because *"from 143 seconds time"* is not
  English.
- ✅ **`J-F24` is inherited, not re-decided.** Both numbers come out of `WholeSecondsText`, so whole
  seconds, rounded, even past 60 — and his one-word retune stays one word for **both** refusals.
- ⭐ **`{0}` = `X` = the CURRENT time left; `{1}` = `Y` = the NEW window.** §5 test 9 pins that order
  **exactly**, because swapping the two arguments produces *"…from 83 seconds to 143 seconds"* —
  a sentence that reads as an **increase** while refusing the card, and which no reviewer would
  catch by reading the format string alone.

---

## §3 — ⛔⛔ THE ROW'S ACTUAL DIFFICULTY: COMPUTING A FULL CARD EFFECT PURELY TO REFUSE IT

```cpp
if (Row->SpellEffect == ESpellEffect::FogClear)
{
    if (const AFogVolume* const FogState = AFogVolume::Find(GetWorld()))
    {
        const AHeroCharacter* const CasterHero = Cast<AHeroCharacter>(GetPawn());
        const ETeamId CasterTeam = IsValid(CasterHero) ? CasterHero->GetTeamId() : ETeamId::Blue;

        const float RemainingWindowSeconds = FogState->GetFogPreventionSecondsRemaining();  // X
        const float WouldBeWindowSeconds   = FogState->GetBrightSunWindowSeconds(CasterTeam); // Y

        if (WouldBeWindowSeconds < RemainingWindowSeconds)
        { … RefuseCardPlay(CardID, FText::Format(…, WholeSecondsText(X), WholeSecondsText(Y))); return; }
    }
}
```

- ⭐⭐ **`Y` IS THE EFFECT, WORKED OUT AND THEN THROWN AWAY.** `TASK-982` shipped the duration
  accessor public/`const`/side-effect-free precisely so this could happen without either
  duplicating the formula or casting the card. I called it; I re-derived nothing.
- ⛔ **BOTH VALUES ARE LIVE, READ AT THE CLICK.** `X` recomputes from the world clock on every call;
  `Y` re-samples **hero height** on every call (`TActorIterator` + team filter + `!IsDead`, inside
  the accessor). ⇒ **`Y` changes as he climbs**, which is exactly what makes the message worth
  showing. Nothing is cached: the controller header is censused at **0** stored remainders
  (`PreventionSeconds`, `FogPrevent`) and **0** stored windows (`WouldBeWindow`, `BrightSunWindow`).
- ⛔ **The READ door `Find`, never `FindOrSpawn`** — `AFogVolume::FindOrSpawn(` is at **0** in this
  file. A refusal pre-check may not spawn a state actor as a side effect of saying no, and
  `J-F17`'s pre-emptive-first-cast case is still covered by the **resolver's** own `FindOrSpawn`.
- ⛔ **The gate is the DATA** — `ESpellEffect::FogClear`, censused at **1**; `TEXT("BrightSun")` at
  **0**. A renamed card cannot silently stop being refused.

### ⭐⭐ THE ONE DESIGN DECISION IN THE DIFF: ⛔ NO EXTRA `RemainingWindowSeconds > 0.f` PRE-GATE

The sibling `FogCover` refusal gates on `PreventionSecondsRemaining > 0.f`. **Mine deliberately does
not**, and this is the choice QA should press first.

- ⭐ **The predicate is bit-identical to `AFogVolume::ApplyBrightSun`'s** — same two accessors, same
  operand order, same strict `<`. That identity is the point: **this entry gate may never refuse a
  cast the state object would have accepted, nor wave through one the state object will then
  refuse.** A second, differently-spelled condition here is precisely how the two halves would come
  to disagree, and the disagreement would present as *"the toast said no but the card fizzled"* (or
  worse, the reverse).
- ⛔ **It is not a missing check.** `GetFogPreventionSecondsRemaining()` returns **0** whenever the
  machine is not `SHIELDED`, and a window is always at least `BrightSunBaseDurationSeconds`, so
  `Y < 0` is unreachable with any sane tuning ⇒ **with no window up the guard falls straight
  through and the card plays normally.** Under a degenerate (negative-base) tuning both halves
  would refuse **together**, which is the property that matters.
- **Cost:** one `TActorIterator` over heroes per `BrightSun` **click** (not per tick). Negligible,
  and stated rather than hidden.

### ⛔ THE BOUNDARY IS A DECLARED DEFAULT, ⛔ NOT HIS WORD

He wrote *"**LESS** than"*, so I shipped the **strict `<`** ⇒ **EQUAL RESETS** (a legal, if
pointless, cast). Test 7 pins the absence of the non-strict spelling so the two readings cannot both
be true. A float-equal window is unreachable in practice, which is exactly why the reading has to be
the literal one rather than the convenient one. ⚖️ **A refusal on EQUAL would be a defensible
alternative; it is his call, not mine, and this is a decision rather than an accident.**

### ⭐ THE CASTER TEAM, DERIVED THE SHIPPED WAY

`Cast<AHeroCharacter>(GetPawn())`, falling back to `ETeamId::Blue` — **character-for-character the
idiom both shipped spell resolvers use** (`TryConfirmSpellTarget`'s `TargetingHero` is itself
`Cast<AHeroCharacter>(GetPawn())` recorded at mode entry; `ResolveSpellInstant` casts `GetPawn()`
directly). ⛔ Deliberately not a second convention: if my team differed from the cast's team, the
accessor would measure a **different hero** and the `Y` I show would not be the `Y` the cast uses.

---

## §4 — ⚠️ THE RESIDUAL, DECLARED RATHER THAN DISCOVERED — AND IT IS `TASK-989`'s, IN THE OTHER DIRECTION

**Under today's (defective) routing there is a window my guard does not cover.** `BrightSun` carries
`ESpellEffect::FogClear`, which is not `GoldSteal`, so the switch below sends it into **targeting
mode**: the player clicks (my guard passes — `Y ≥ X` from where he stands), aims, **descends or
waits**, and confirms seconds later. `ApplyBrightSun` then re-reads both values, finds `Y < X`, and
returns false ⇒ the player gets the generic **"Spell fizzled"** with a **full refund and the card
kept**.

- ⛔ **His two default properties still hold in that window** — net-zero gold, card retained
  (`TryConfirmSpellTarget` refunds on the false branch and its `ConfirmPlayFromHand` sits after the
  refusal's `return`, both measured by `TASK-982`). ⛔ Only the **wording** is generic.
- ⭐ **The window closes by construction when `TASK-1018` lands**, because `Fog`/`BrightSun` will
  then resolve on the click with no targeting mode to sit inside.
- ⛔ **I did not build a second call site for it, and that is deliberate:** it would be dead the
  moment `TASK-1018` ships — surface built to cover a defect already boarded to be deleted. ⭐ This
  is the **identical** residual `TASK-989` declared for its own refusal, in the opposite direction,
  and `qa/TASK-990.md` graded it **a declared residual, not a defect**. ⚖️ If the gate now
  disagrees, the fix is one `if` in `TryConfirmSpellTarget` and it wants a ruling, not my preference.
- ⛔ **And `ApplyBrightSun`'s own guard is NOT made redundant by mine.** It is the state object's
  rule; it still covers the **bot**, which reaches `USpellLibrary::ResolveSpell` without ever
  passing through `PlayHandSlot`. Removing either would be wrong.

---

## §5 — ⚠️⚠️ THE THREE RE-POINTED PINS — ⛔ OLD VALUE, ⛔ NEW VALUE, ⛔ REASON. ⛔ NONE DELETED.

All three live in `Tests/SiegeFogRefusalTest.cpp`, all three say so **in place**, and in every case
the **claim** is unchanged — only the arithmetic followed the second refusal onto the board.

| # | test | pin | old | new | why the claim is unchanged |
|---|---|---|---|---|---|
| 1 | 2 (`TwoRefusalsASecondApart…`) | `GetFogPreventionSecondsRemaining()` in `PlayHandSlot` | 1 | **2** | The claim is ⛔ **"one live read PER REFUSAL, and no second, older read"** — never "one read in the file", which was only that claim's arithmetic with one refusal on the board. The per-refusal half is pinned **exactly** by the local-binding rows (989's, still at 1) plus my region rows (mine, at 1), so a build that put both reads in one refusal and none in the other dies **there**, not here. |
| 2 | 2 | `GetFogPreventionSecondsRemaining()` file-wide | 1 | **2** | same claim, file scope |
| 3 | 5 (`TheMessageRidesTheOneShippedRefusalSurface`) | `RefuseCardPlay(CardID, FText::Format(` in `PlayHandSlot` | 1 | **2** | The claim is *"every prevention refusal rides the shipped formatted-refusal idiom"*; there are now two refusals and both ride it. |

⭐ **THREE PINS `TASK-989` DELIBERATELY DID NOT WRITE, PRECISELY SO THIS ROW WOULD NOT GO RED, ALL
CONFIRMED STILL GREEN:** its comment predicted `TASK-991` would add `ESpellEffect::FogClear` (it
did — pinned at **1** in my test 7, where it is an assertion about *my* refusal rather than a stale
zero left in test 4), left the total `ESpellEffect::` count unpinned (`TASK-1018`'s), and wrote its
formatter-call census as a **`>= 2` minimum** rather than an exact count. **All three judgements
paid off, exactly as written.**

⚠️ **THE PIN SCHEDULED TO GO RED ON `TASK-1018` — I MET IT AND LEFT IT ALONE.** Test 4's
`if (Row->SpellEffect == ESpellEffect::GoldSteal)` census is **still 1** and **byte-identical**;
I added nothing matching it. ⛔ Confirming the fence the row named: **the line moved (`:1037` ⇒
`:1090` ⇒ further again after my 79-line insertion) but the CODE did not.** ⛔ I wrote **no line
numbers into any source file** (`SC-§77`); every reference in the diff is by key or by symbol.

---

## §6 — THE NEW TESTS (**+4**) — `Tests/SiegeFogRefusalTest.cpp` tests 7-10

| # | registered name | what it can catch |
|---|---|---|
| 7 | `…SunOnSunRefusesONLYWhenTheNewWindowWouldBeShorter` | ⭐⭐⭐ **the row.** The strict comparison must exist between the effect gate and the message (an **unconditional** refusal deletes that substring and dies **here and only here**); `FMath::Max` at **0** across the entry; the non-strict `<=` spelling at **0**; ⛔ **the routing switch must still FOLLOW the message** — without it, a build that refused every `BrightSun` would pass every other assertion in this file while making the card permanently uncastable; plus `ApplyBrightSun`/`FogPreventedUntilTimeSeconds` at **0** (the LONGER branch is not reimplemented here), the `FogClear` data gate at **1**, no CardID literal, the READ door at 1 and `FindOrSpawn` at 0 |
| 8 | `…SunOnSunComputesTheWouldBeWindowLiveAndNeverDuplicatesTheFormula` | ⭐ **executed:** `AFogVolume::BrightSunWindowSeconds` called directly at two heights ⇒ a higher perch yields a **strictly longer** window ⇒ a **different sentence** (`120` vs `240` s) — the half of *"`Y` is live"* that a fixed-height test can never see; plus `X`'s injectivity across one second. **structural:** the duration accessor bound to a local inside my region (1) and at **1** file-wide; my own `X` read at 1 in-region; and the **anti-duplication census** — `FloorToFloat` 0, `FloorToInt` 0, the pure static 0, all four tunables 0, header stores 0 |
| 9 | `…ARefusedBrightSunSpendsZeroGoldKeepsTheCardAndShowsBothValues` | the **three properties by name**: `SpendGold`/`AddGold` 0 and `ConfirmPlayFromHand`/`ConfirmInstantDraw`/`DiscardFromHand` 0 **before this refusal is raised** (measured on the prefix, because my guard sits **lower** in the function than its sibling and could in principle have been placed past a spend the sibling precedes), the accept stinger not yet played, `CanAfford` already run, the refusal before the switch — and ⭐⭐ **the argument-order pin**: `{0}` is the argument followed by a **comma**, `{1}` the one that **closes the call**, so a swap goes red in **two** rows; plus his sentence and both slots pinned verbatim |
| 10 | `…TheTwoValueSentenceReusesTheOneSharedFormatterTwice` | ⛔ **the arity fence**: one declaration, one definition, **3** calls in `PlayHandSlot` (1 sibling + 2 mine); **both** wordings still inline at their own call sites with their own keys — the structural proof that no one-arity builder with an optional second value was created; the formatter still knows nothing about `BrightSun` or `CardRefused_`; the shipped surface unchanged (no new delegate, no new widget) |

**⛔ EVERY numeric pin above was MEASURED against the shipped bytes before it was typed**, with a
script re-implementing `CountOccurrencesInCode`'s comment-skipping rule and `ExtractFunctionBody`'s
signature/column-0-brace rule, run over the edited source. **51 source-text measurements, all
green** — including every pre-existing pin in tests 1-6, which is how the three that needed moving
were found rather than discovered by the gate. ⛔ That is a measurement of the **source text**,
⛔ **not** an executed suite.

**⭐ THE FIXTURE HELPER I ADDED — `ExtractSunOnSunRegion` — HAS THREE CALLERS** (tests 7, 8, 9) and
fails loudly on a missing boundary (`SC-§38`): a region probe that silently degraded to an empty
string would report every *"must contain"* row red and every *"must not contain"* row **green**,
i.e. it would pass the ban rows while testing nothing.

**⚠️ THE DECLARED GAP, stated rather than discovered:** there is still not one `SpawnActor` anywhere
in `Siegebound/Tests/`, so the true end-to-end claim — stand on a tower, click `BrightSun`, walk
down, click again, and see two different `Y` values — is **not executed**. The executed half proves
the window varies with height and the formatter is injective; the structural half proves the call
site asks the **live** accessor and stores nothing. `TASK-982` and `TASK-989` declared the identical
gap for the same reason.

---

## §7 — ⛔ WHY I ADDED TO `Tests/SiegeFogRefusalTest.cpp` RATHER THAN CREATING A THIRD FILE

The row said *"CHECK FOR AN EXISTING FRAME FIRST"*. I did, and `TASK-989` had left one on purpose —
its own header block says so: *"⭐ AND IT IS THE FRAME `TASK-991` SHOULD GROW INTO."*

1. ⭐ **ITS GATE HAS RETURNED.** `qa/TASK-990.md` is **PASS, 0 blockers**. That is the direction —
   not the convenience — that permits the edit: `TASK-989` declined to touch
   `Tests/SiegeBrightSunTest.cpp` because its gate `TASK-986` had **not** returned, and
   `TASK-982` was permitted to edit `SiegeFogVolumeTest.cpp` because that file's had. ⛔ My row is
   the third application of the same rule, by direction.
2. ⛔ **`Tests/SiegeBrightSunTest.cpp` IS STILL BYTE-UNTOUCHED BY ME** — `TASK-986` is still
   outstanding, so the file that was a review subject for `TASK-989` is still one for me.
3. ⛔ **SAME SUBJECT.** Tests 7-10 are about `ASiegePlayerController`'s card-play refusal — its
   condition, its wording, its ordering and its surface — exactly what tests 1-6 are about, in the
   same function, consuming the same formatter. ⛔ **Not one assertion is duplicated**: `TASK-982`
   asserts `FMath::Max == 0` inside `AFogVolume::ApplyBrightSun`; I assert it inside
   `ASiegePlayerController::PlayHandSlot`. Different function, different possible introduction site
   — a `max` could have been written at either end, and only one of the two would have been seen.

---

## §8 — ⛔ FINDINGS I AM **REPORTING, NOT FIXING** (all outside my fence)

1. **⚠️⚠️ THE TOAST LENGTH — and it is SHARPER for this message than for its sibling.** My sentence
   is ~85 characters and carries **two** numbers the player must read, on a single-line HUD channel
   whose show-then-hide lifetime `VID-005` measured at **≈1.8 s** (`CardHandWidget.h`'s own doc says
   *"~2 s"* — two independent sources agreeing). ⛔ **I did not shorten his sentence and I did not
   touch the lifetime.** The lifetime lives in `WBP_CardHand`, is not assertable from C++, and
   changing it is a **game-wide UI change** ⇒ 🧑 **Jonathan's call** (`J-F25`, closed). ⇒ **Re-raise
   at a checkpoint, never as a task.** ⚖️ If he wants it shorter, *"Bright Sun would shorten fog
   prevention: 143s → 83s"* is a one-line change to one `NSLOCTEXT`.
2. **🚨 `SiegePlayerController.cpp`'s spell routing is still a BLACKLIST OF ONE** (`GoldSteal`), so
   `BrightSun` enters **targeting mode** and the player aims a reticle at a map-wide effect. ⛔ NOT
   TOUCHED, by explicit fence: it is ⭐ `TASK-1018`, which must consume ⭐ `TASK-999`'s
   delivery-derived predicate. ⭐ `TASK-982` §8(1) and `TASK-989` §8(1) reported it independently;
   this is a **third corroboration**, not a new sighting to board. §4 is its consequence in my lane.
3. **`SpellLibrary.h`'s class-doc effect list still omits `FogCover` and `FogClear`** — inherited,
   reported by both predecessors; ⛔ not my file, ⛔ not swept.

---

## §9 — ⛔ WHAT QA SHOULD SCRUTINISE HARDEST

1. **⛔⛔ THE ABSENT `> 0.f` PRE-GATE (§3).** This is the one design decision in the diff. I hold
   that a predicate **bit-identical to `ApplyBrightSun`'s** is stronger than a defensive extra
   condition, because the failure mode being prevented is the entry and the state object
   **disagreeing**. ⚖️ If you disagree it is a two-line addition, and it is a ruling, not a
   preference.
2. **⛔ THE BOUNDARY (§3).** EQUAL **resets**, per his literal *"less than"*. A refusal on EQUAL is
   the alternative reading; declared as a default, not claimed as his word.
3. **⛔ THE THREE RE-POINTED PINS (§5).** Verify each re-point preserves the claim rather than
   weakening it, and that I moved **only** what my diff genuinely changed — the exact-string local
   binding pins in test 2 are still at **1** precisely because I gave my locals **different names**
   rather than reusing the sibling's.
4. **⛔ THE ARGUMENT ORDER (§2, test 9).** `{0}` = current, `{1}` = new. A swap still compiles, still
   refuses, still shows two live values — and reads as an increase. Confirm the two comma/paren pins
   really do die on a swap.
5. **⛔ THE CASTER-TEAM DERIVATION (§3).** It must match what the cast would use, or `Y` is measured
   off the wrong hero. I copied both resolvers' idiom; check I copied the right one.
6. **⛔ THE DECLARED RESIDUAL (§4)** and whether the generic *"Spell fizzled"* is acceptable there
   until `TASK-1018`. `qa/TASK-990.md` already ruled yes for the mirror case.

---

## §10 — SCOPE DISCIPLINE

**⛔ NOT DONE, by fence:** no compile · no editor · no MCP · no mutating Git (read-only `git diff`/
`status` only, `SC-§71a`; ⛔ no `checkout`/`restore`/`stash`/`reset`/`clean` of any kind) ·
⛔ **no edit to `AFogVolume` of any kind — both accessors CALLED, never CHANGED** · ⛔ **no touch to
the `GoldSteal` routing predicate (`TASK-1018`)** · ⛔ **no second formatter and no generalised
message builder (`TASK-989`'s surface consumed, not modified)** · ⛔ no re-implementation of the
LONGER branch (`TASK-982` item 7a) · no other refusal's wording · no toast lifetime or styling
(**report only**) · no card row (`TASK-983`) · no card art (`TASK-984`) · no curve (`TASK-981`) ·
no firing/notice ranges (`979`/`980`) · no `SpellLibrary.{h,cpp}` · no `cards.csv` / `DT_Cards` ·
⛔ no line numbers or row counts written into source (`SC-§77`).

**Files changed (3):**
- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp`
- `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h`
- `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogRefusalTest.cpp`

Plus `.claude/pipeline/TASKBOARD.md` — **only** TASK-991's own `status:` line, re-located by heading
immediately before the edit and grepped back out after (⚠️ anchored on the `status:` **and**
`blocked-by:` lines together, because `TASK-992`'s status shape is otherwise identical and the board
has shifted twice today by amounts small enough for a stale anchor to match the **neighbouring
row**). ⛔ `TASK-982`, `TASK-986`, `TASK-989`, `TASK-990`, `TASK-992`, `TASK-999` and `TASK-1018`
rows untouched.

⚠️⚠️ **STAGING NOTE FOR THE COMMIT HOST (`TASK-987`):** `Tests/SiegeFogRefusalTest.cpp` is
**UNTRACKED** in git (`??`, not `M`) — it entered the tree with `TASK-989` and has never been
committed. **It must be staged by pathspec** or this row's four tests, *and* `TASK-989`'s six,
commit as **silently absent**. The same is true of `TASK-998`/`TASK-982`'s `FogVolume.{h,cpp}`,
`Tests/SiegeBrightSunTest.cpp` and `Tests/SiegeFogVolumeTest.cpp`, all of which my file's probes
depend on being present. `SiegePlayerController.{h,cpp}` are tracked (`M`).
⚠️ Per `SC-§78` the untracked/tracked claims above are **declared from a read-only `git status`**,
not from any mutating command.
