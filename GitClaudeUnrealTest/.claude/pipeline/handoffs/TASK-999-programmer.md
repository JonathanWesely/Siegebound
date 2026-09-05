# TASK-999 — [GLOSSARY-GAP] handoff (gameplay-programmer, 2026-09-04)

**Status:** `ready-for-qa` · **Gate:** ⭐ `TASK-1013` — ✅ **CONFIRMED PRESENT** on the board
(`#### TASK-1013 — [GLOSSARY-GATE] … (qa-reviewer)`, `blocked-by: TASK-999`, status `boarded 2026-09-04`,
`parallel-safe: yes`). **Ship host:** ⭐ `TASK-987` (its clause names `TASK-999` as an added ship subject).

**BOTH halves landed.** The row's `blocked-by` is discharged: `ESpellEffect::FogCover` and
`ESpellEffect::FogClear` both exist in `CardRow.h` today (`FogClear` appended last, `== 7`), so the
glossary half and the delivery half were fixed in one open of the file.

---

## 1. What changed

### `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.cpp` — the only shipping file touched

The file was **git-CLEAN before this row** (it is absent from the session-start `git status`), so the
whole of its diff is mine and nothing here rides on another lane's uncommitted work.

**(a) Two missing effect arms — the BLANK line.** `switch (Row.SpellEffect)` had no arm for
`FogCover` or `FogClear`, so `Fog` rendered in the deck builder describing **nothing**. Five new
player-facing constants in `SiegeboundCardGlossary`:

| constant | gate | why |
|---|---|---|
| `SpellFogCover` | **unconditional** | the effect reads no row magnitude, so a magnitude gate would re-create the blank |
| `SpellFogCoverDurationFmt` | `EffectDuration > 0` | the only fog number that lives in the row |
| `SpellFogClear` | **unconditional** | as above |
| `SpellFogClearWindowRules` | **unconditional** | the prevention window's rules (height, one-way door, the J-F18 refusal) live in no cell |
| `SpellFogClearBaseFmt` | `EffectDuration > 0` | the base window |

⭐ The fog arms are the **only two unconditional arms in the composer**, and that is deliberate:
every other arm gates on a magnitude and can therefore compose nothing. A blank `EffectDuration`
cell now costs a *sentence*, never the description.

**(b) The delivery guard — the FALSE line, and the actual manager ruling.** Deleted
`if (Row.SpellEffect != ESpellEffect::GoldSteal)`. Replaced by a derivation, in the same precedence
order `USpellLibrary::GetEffectiveDelivery` itself uses:

```cpp
const bool bDeliveryAuthored     = (Row.SpellDelivery != ESpellDelivery::Auto);
const bool bRowCarriesAnAimPoint = (Row.AoERadius > 0.f);
const bool bAimed = bDeliversAsLine || bDeliveryAuthored || bRowCarriesAnAimPoint;
```

**(c) The `default:` arm is GONE.** Every declared value is now listed, `None` included, so appending
an `ESpellEffect` is a diagnostic wherever the toolchain warns on an unhandled enumerator — and the
new test is the belt for the toolchains that do not (MSVC's C4062 is off by default).

**(d) The spell block moved into `SiegeboundCardGlossary::AppendSpellLines` (free function, external
linkage), called from `AppendRuleLines`.** Reason, and it is the only reason: `AppendRuleLines` is
`private:` in `DeckBuilderWidget.h`, which this row does **not** own, so item (3)'s assertion had no
way to reach the composer at all. The function is pure row-in / lines-out — it touches no member of
`UDeckBuilderWidget` — so the extraction is mechanical.

### `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeCardGlossaryTest.cpp` — NEW

Boarded by the row's own `names:` line (*"a test in `Tests/`"*), items (3) and (3c). It is a **new
file that collides with no lane** — no other row in this batch owns a glossary test, and no existing
test referenced any glossary string (verified by census, below).

---

## 2. ⛔ THE PART QA MUST SCRUTINISE HARDEST — the dispatch's stated derivation does **NOT** work

The dispatch said *"DERIVE the guard from `SpellDelivery`"* and told me to read
`USpellLibrary::GetEffectiveDelivery` first. I did, and **the obvious reading of that instruction
reproduces the exact bug**:

> `GetEffectiveDelivery`'s `Auto` arm returns **`GroundCircle`** for every effect that is not
> `AoEDamage`/`Freeze` — **`GoldSteal`, `FogCover` and `FogClear` included.**

⇒ A guard written as `ResolvedDelivery == ESpellDelivery::GroundCircle` would have printed *"where
you place the reticle"* for `Fog`, for `BrightSun` **and** for `Pickpocket` — i.e. it would have been
**worse than the blacklist it replaced**, while looking exactly like the ruling asked for.

**The reason is structural and worth recording: `ESpellDelivery` has no value meaning "no aim at
all".** Delivery can say *which* aiming sentence; it cannot say *whether there is one*. So:

* the **cell** answers first (an authored cell is the per-card override lever the column exists to
  be — this is the half that is genuinely "derived from `SpellDelivery`");
* when the cell is `Auto` — every shipped row but two — **the row's own aim evidence** answers:
  a ground-placed spell resolves *inside* `AoERadius`, so a positive radius **is** the reticle's
  footprint and a zero radius means there is nothing on the ground to place.

**Measured against the shipped roster, by CardID** (`Docs/Data/cards.csv`, read at implementation
time; ⛔ no line numbers, `SC-§77`):

| CardID | SpellEffect | SpellDelivery cell | AoERadius | aiming line printed |
|---|---|---|---|---|
| `Fireball` | AoEDamage | **`HeroLine`** | 300 | hero-line ✅ (unchanged) |
| `FrostNova` | Freeze | **`HeroLine`** | 350 | hero-line ✅ (unchanged) |
| `Lightning` | TopTargetsDamage | *(blank ⇒ Auto)* | 700 | reticle ✅ (unchanged) |
| `BattleCry` | AllyBuff | *(blank ⇒ Auto)* | 400 | reticle ✅ (unchanged) |
| `Pickpocket` | GoldSteal | *(blank ⇒ Auto)* | **0** | **none** ✅ — now by CONSTRUCTION, not by being named |
| `Fog` | FogCover | *(blank ⇒ Auto)* | **0** | **none** ⭐ **the lie is gone** |
| `BrightSun` | FogClear | *(row not written yet — `TASK-983`)* | — | **none** ⭐ correct *before* its data exists |

⚖️ **It fails CLOSED.** An effect the derivation cannot place gets **no** aiming line — a gap, never
a lie. The blacklist failed **open**.

### The one path by which a wrong aiming claim can still be authored — declared, not hidden

`bDeliveryAuthored` means an author who writes `GroundCircle` into a global spell's cell **will** get
the reticle sentence. That is deliberate (the glossary agrees with the column; a wrong cell is a
DATA defect for a data gate) and it is **pinned by TEST 4 with a paired control**, so it reads as
design rather than as drift. ⛔ No shipped row does this. If QA rules the other way, the fix is to
drop `bDeliveryAuthored` from the disjunction and delete TEST 4 — one line and one test, no reshape.

### `bLineCapableEffect` still names two effects — and I kept it on purpose

It is **not** the blacklist this row came to kill, though it looks identical. It **mirrors the
resolver's own branch set** (`USpellLibrary::ResolveSpell` branches on delivery under `case
AoEDamage` and `case Freeze` and nowhere else), and it **fails closed** — a new effect is not
line-capable, so it can only ever be described as ground-placed or as nothing. Removing it would
make an authored `HeroLine` cell on, say, `Lightning` print *"aimed from your hero"* while the
resolver ground-places it. ⛔ Redesigning `ESpellDelivery` so this mirror is unnecessary is
`SpellLibrary`'s row, not mine.

---

## 3. ⛔ REPORTED, NOT FIXED — the routing half, and it is WIDER than the dispatch said

The dispatch fenced me out of `SiegePlayerController.cpp` and told me to report the routing defect.
Reporting it **larger than I was told**, because the census found a second site:

* **`ASiegePlayerController::PlayHandSlot`'s `case ECardType::Spell:`** — `if (Row->SpellEffect ==
  ESpellEffect::GoldSteal)` routes to `ResolveSpellInstant`; **everything else enters TARGETING
  mode.** ⇒ `Fog` and `BrightSun` get a real reticle today.
* **`EnterTargetingMode`'s own head** — the *same* `== GoldSteal` test again, guarding a direct
  hand-less entry, placed before the hero-dead gate.
* **`ResolveSpellInstant`'s comment** asserts *"only GoldSteal reaches this instant path"* — a claim
  that goes false the moment either guard is widened, in a **third** place.

⇒ It is the **same blacklist-of-one, three times, in the ROUTING instead of in the TEXT**, and it
contradicts `FOG-§10.1`'s *"NO RETICLE"*. Locate by SYMBOL — the file has moved repeatedly.

⚠️ **CONSEQUENCE FOR THIS DIFF, SAID PLAINLY SO NO REVIEWER DISCOVERS IT INSTEAD:** until that
routing row lands, **my corrected glossary text disagrees with live behaviour** for `Fog` and
`BrightSun` — the panel will correctly say there is no reticle while the game still shows one.
⚖️ That is the right direction to be wrong in: the text now matches the **ruled design**, and the
glossary is not the thing that should be bent to match a defect. But it is a real, temporary
mismatch and it should be closed by the routing row, not by reverting this one.

---

## 4. Premise corrections to the board (both in the row's own item (2))

1. ⛔ *"NO TEST REFERENCES `ESpellEffect` AT ALL"* — **stale.** `Tests/SiegeFogVolumeTest.cpp`
   (`TASK-998`) already iterates `StaticEnum<ESpellEffect>()` and pins `FogClear` as the append
   point. The board's conclusion still held (nothing tested the *glossary*), but the stated reason
   has been overtaken. I reused that file's `_MAX`-by-name idiom deliberately — one shape for one
   problem.
2. ⛔ *"the `switch` has a `default:` ⇒ COMPILE-SAFE"* — true, and now **repaired at the cause**
   rather than only papered over with a test.

---

## 5. Declared seams (⛔ read these before passing)

* **The fog duration the glossary prints is the ROW's `EffectDuration`, and the MECHANISM does not
  read that cell today** — `AFogVolume::FogDurationSeconds` (CDO) does, per `CardRow.h`'s
  `FogCover` block; ⭐ `TASK-1016` is the row that makes the cell authoritative. I print the row
  anyway because (i) `FogVolume.h` states *"`EffectDuration` on the `Fog` card row MUST match
  this"* — the agreement is a **contract**, not a coincidence — and (ii) it is the data-driven
  branch (GDD §3.0: this widget never hardcodes a magnitude it can read). Same seam, same reasoning,
  for `BrightSun`'s base window (`FOG-§10.1` pins the cell at the same figure).
* **NO magnitude is baked for the vision cut or the height reward** (`SC-§65`). Both sets of levers
  are `EditDefaultsOnly` *specifically* so Jonathan can retune them with no code change — and
  `BrightSunHeightStepUU` was **already amended by him on 2026-09-04 from 20 ft to 50 ft**, so a
  glossary line saying "20 feet" would be a lie today. The lines state the SHAPE (ranged reach
  collapses / melee untouched; higher is longer, read once at the cast, no cap), which survives
  every retune inside the editor's own clamps.
* **`SiegeFogStatics.h` is deliberately NOT included here** to interpolate `FogVisionCeilingUU`,
  even though the file's own doctrine prefers interpolation over a mirrors-comment:
  `SiegeCombatStatics.cpp`'s include comment asserts that header is *"consumed HERE and in no other
  translation unit"*, and a second consumer would falsify a claim in a file this row does not own.
  ⇒ stated as a shape instead, which needs no include at all.
* **String literals stay pure ASCII** (the file's own rule — a mis-decoded literal ships mojibake to
  the panel). Verified mechanically: all 39 glossary constant lines, 0 non-ASCII.

## 6. Suite DELTA (⛔ never executed — declared)

**+4 test cases, +1 file** (`Tests/SiegeCardGlossaryTest.cpp`). ⛔ Nothing in this row has been
compiled or run: no build, no editor, no MCP, no Git mutation (read-only `status`/`diff` only,
`SC-§71a`).

| test | claim |
|---|---|
| `…EverySpellEffectValueComposesANonEmptyEffectClause` | item (3), universally quantified over `StaticEnum<ESpellEffect>()` |
| `…NoSpellEffectClaimsAReticleOnARowWithNoAimPoint` | item (3c) half 2, quantified over the enum — the lie, killed for values that do not exist yet |
| `…TheShippedSpellShapesEachDescribeTheirOwnAiming` | the five roster shapes by CardID + the `Pickpocket` regression guard |
| `…AnAuthoredSpellDeliveryCellDecidesTheAimingSentence` | the one authored path, with its paired control |

**Red-proofing (`SC-§37`), stated because two of the four tests are ABSENCE claims:**

* The delivery sentences have **internal linkage** in `DeckBuilderWidget.cpp` and cannot be
  referenced, so the claims are made against a distinctive **substring**. A substring probe fails
  SAFE alone (reword the sentence and "the line is absent" passes for the wrong reason) ⇒ **every
  negative assertion is paired with a positive control in the SAME test** that requires the same
  probe to be PRESENT. Reword either sentence and a control goes red **first**.
* `CountEffectClauses` **subtracts the two aiming sentences before counting.** ⛔ This is the trap
  that let the defect ship: a `Fog` row with no effect arm still produced a non-empty description,
  because the delivery line was appended anyway. A test asserting "the composer produced something"
  would have **passed on the broken card**.
* Every sweep self-checks its own population (`>= 7` declared spell effects) so a null `UEnum` or an
  empty sweep cannot make a universally-quantified claim pass vacuously.

**Static checks run in place of a compile** (I may not build): brace/paren balance on both files via
a comment- and string-stripping parser, red-proofed against a known-good control file
(`Tests/SiegeFogVolumeTest.cpp`) — 0 imbalance on all three; symbol-collision census for every new
free symbol across the whole module (unity-build ODR risk) — **0 hits**; ASCII purity of all
glossary literals — **0 violations**; census for any other test asserting the old glossary strings —
**0 hits**.

⛔ **The one thing a reviewer should check that no static tool of mine can:** the test file
forward-declares `SiegeboundCardGlossary::AppendSpellLines` and relies on **external linkage** to
link against the definition in `DeckBuilderWidget.cpp` (same module). If the signature drifts, or
the definition is ever "tidied" into the anonymous namespace above it, this fails as a **link
error** — which is the intended failure mode, and must be repaired at the definition, **never** by
deleting the declaration.

## 7. Fences respected

⛔ No other source file edited · ⛔ `SiegePlayerController.cpp` **read only**, reported not fixed ·
⛔ `DeckBuilderWidget.h` untouched (the whole reason for the free-function extraction) · ⛔ no enum,
no card row, no `cards.csv`, no `DT_Cards`, no WidgetBlueprint, no layout · ⛔ no compile, no editor,
no MCP, no mutating Git · ⛔ board edit limited to this row's own `status:` line.
