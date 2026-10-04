<!-- MOVED from .claude/pipeline/CONVENTIONS.md on 2026-10-04 by Tools/split_conventions.py. Sections are byte-identical to the original; this comment is the only addition. Cite clauses by tag (e.g. `VER-§3 cl. 6`); CONVENTIONS.md '## Law index' maps tags to files. -->
## ⚖️ HIGH GROUND — tripled ranged reach + the elevation damage bonus (2026-08-30) — namespace **HIGH-§**

📌 **BORN WITH ITS NAMESPACE PREFIX (`HIGH-§N`)**, per the `KBD-§`/`NAV-§`/`WM-§`/`PKG-§`/`SHIP-§`/`HELP-§` precedent. Cite as `HIGH-§2`, never a bare `§2`.

**Trigger — Jonathan's directive, verbatim (2026-08-30):** *"lets give all archers and wizards and range units increased range. Triple the range of their attacks and make their attacks deal more damage the higher elevation they are. I would say that for every 5 feet that their elevation increases, their damage multiplier increases by 10%."*

### HIGH-§1 ⭐⛔⛔ THE UNIT CONVERSION — PINNED ONCE, WITH THE ARITHMETIC SHOWN, SO ⛔ NOBODY RE-DERIVES IT

> ### ⭐ **"5 FEET" IS NOT AN ENGINE UNIT. UNREAL IS CENTIMETRES, AND 1 uu = 1 cm.**
> ### **1 ft = 30.48 cm (exact, by international definition).**
> ### **5 ft = 5 × 30.48 = **152.4 cm** = ⭐ **152.4 uu**.**

- ⛔⛔ **`152.4` IS THE ONLY NUMBER. ⛔ NOT 150 ("close enough"), ⛔ NOT 152, ⛔ NOT 5 (feet-as-units), ⛔ NOT 500.** ⚠️ **A silent guess here is a 3× design error and it would be invisible in review** — `150` looks like a designer's round number and is wrong by 1.6%; `5` is wrong by **30×** and would make every unit on the field a god.
- **It ships as ONE named, commented, `EditDefaultsOnly` constant** carrying the arithmetic in its comment — **`HeightBonusStepUU = 152.4f`** — and ⛔ **no second literal of it exists anywhere in the codebase.** The bonus per step ships beside it as **`HeightBonusPerStep = 0.10f`** (= his 10%).
- ⭐ **THE TUNABLE IS THE POINT: he chose 5 ft and 10% in feet, in prose.** Both are `EditDefaultsOnly` so his next sentence retunes them **without a code change** — and the comment tells the next reader *why* the number is 152.4 and not 150, so a future "tidy-up" cannot round it away.

### HIGH-§2 ⚖️ "THE HIGHER ELEVATION **THEY** ARE" — RELATIVE TO **WHAT**? THE PROCEEDING DEFAULT, AND WHY

⚠️ **His sentence reads two ways and the difference is enormous. It is RULED, ⛔ not blocked on.**

- ⛔ **(b) ABSOLUTE WORLD Z — REFUSED as the default.** It makes a unit on a hill hit harder than a unit at sea level **even when both are shooting a target standing beside them on that same hill**, and it makes the bonus depend on where the level designer put Z=0. ⚖️ *That is not what "high ground" means to anybody who has played a game.*
- ✅⭐ **(a) HEIGHT ABOVE THE TARGET — THIS IS THE DEFAULT.** `ΔZ = AttackerZ − TargetZ`. **The bonus applies ONLY when `ΔZ > 0`**; level ground and shooting UPWARD both give **exactly ×1.0**. ⛔ **NO penalty for low ground — he asked for a bonus, and inventing a malus is inventing a mechanic.**
- **The formula, pinned:** `Multiplier = 1 + HeightBonusPerStep × max(0, ΔZ) / HeightBonusStepUU` — ⭐ **CONTINUOUS (linear), ⛔ not stepped.** ⚖️ **Why continuous:** a stepped rule puts invisible breakpoints on a hillside that the player cannot see, cannot aim for, and cannot learn; a 1-uu step flipping damage by 10% is worse feel and harder to test. **Both readings are defensible English; continuous is the better game.** 🧑 **Rulable by Jonathan in one word.**
- ⛔ **BOTH Z VALUES COME FROM `GetActorLocation().Z`, on both sides, no exceptions.** ⚠️ **The known consequence, stated rather than discovered later: a large-footprint target (a castle) reports its ORIGIN Z, which sits at its base.** Same convention on both sides is what makes the difference meaningful; a mixed convention (origin vs. capsule vs. bounds) is how a sign error hides.
- ⛔⛔ **IT IS SELF-LIMITING BY CONSTRUCTION AND THAT IS THE WHOLE ARGUMENT FOR (a):** a unit on a tower shooting a unit on the same tower gets **nothing**. The bonus is *height advantage*, which is what he pictured.

### HIGH-§3 ⛔ THE ONE SEAM — ⛔ NEVER A SPECIAL CASE FOR TOWERS, AND ⛔ NEVER A SECOND COMPOSE POINT

- ⭐ **THE INSERTION POINT IS ALREADY BUILT AND IT WAS FOUND AT SOURCE: `ASummonedUnit::ComputeOutputDamage(const AActor* Target)` (`SummonedUnit.cpp:3206`).** It already takes the Target, it already composes Charge · Slayer · War-Banner aura · Ancient-Grounds permanent boost in **ONE place**, and its own comment states the contract: *"this ONE insertion covers BOTH delivery modes — melee … and ranged … and every keyword unit, because they all funnel through this function."* ⇒ **The elevation bonus is COMPOSE POINT 3 and it goes here.** ⛔ **A second compose point anywhere is a QA BLOCKER.**
- ⭐ **THE ARITHMETIC IS A PINNED PURE SEAM** (the `WM-§2`/`HeightToBrightness` precedent — a testability obligation gets a testability seam): `static float ASummonedUnit::HeightAdvantageMultiplier(float AttackerZ, float TargetZ, float StepUU, float BonusPerStep);` — **`public`, plain C++ static, ⛔ NOT a `UFUNCTION`, exactly four parameters, none defaulted (`SC-§33`)**, no world access, no actor access, trivially testable headlessly.
- ⛔⛔ **THE RULE READS THE WORLD'S REAL HEIGHT AND ⛔ NOTHING ELSE. THERE IS NO `bIsOnATower` FLAG, NO TOWER QUERY, NO OCCUPANCY LOOKUP, NO `#include` OF ANY TOWER HEADER.** ⚖️ **A unit on a hill and a unit on a tower at the same Z must deal identical damage.** ⭐ **This is what makes `TOWER-§` work for free: the tower grants elevation by BEING TALL, and the damage rule never learns it exists.** A tower special case would guarantee that hills and towers eventually disagree.
- ⛔ **⛔ AND IT NEVER READS THE WAR MAP** — `WM-§8d`, which is `SHIP-§9`'s class. `GetActorLocation().Z` from live actors, full stop.

### HIGH-§4 ⚖️ WHO GETS IT — THE RANGED SET IS **ENUMERATED FROM THE DATA**, ⛔ NEVER GUESSED

- ⭐ **THE SELECTOR, PINNED: `bRanged == true` AND `CardType == Unit`.** Read from `Docs/Data/cards.csv` (2026-08-30), the shipped set is **EXACTLY THREE**:

  | CardID | Cost | Damage | **Range (shipped)** | **Range (×3)** | Note |
  |---|---|---|---|---|---|
  | **Archer** | 12 | 10 | **700** | **2,100** | |
  | **Wizard** | 24 | 15 | **700** | **2,100** | `AoERadius 250` — ⛔ **UNCHANGED**, splash is not reach |
  | **Longbowman** | 18 | 18 | **1,200** | **3,600** | |

- ⛔ **NOT IN THE SET, each with its reason, so nobody re-litigates it:**
  - **ArrowTower (900) · BombTower (800) · BallistaTower (1,400)** — `bRanged=true` but **`CardType == Building`. They are ⛔ NOT UNITS**, and his sentence says *"archers and wizards and range units."* **Proceeding default: towers are UNCHANGED.** 🧑 **One-word overrule available (`HIGH-§7` row R-2).**
  - **CrystalTower** — `bRanged=false`, and its attack is an **instant chain zap, not a projectile**. Not a unit either.
  - **Cleric (Range 400)** — `bRanged=false`; that Range is a **HEAL radius on a unit that cannot attack** (`Profile=Support`). ⚠️ **Tripling it would triple a HEAL, which he did not ask for.** ⛔ UNCHANGED.
  - **Sorcerer** — cannot attack (Range 0). **Hero (`AHeroCharacter`)** — melee, separate damage path, ⛔ not an `ASummonedUnit`. **All melee units** — Range 120 is contact reach.
- ⭐ **THE SAME THREE-CARD SET GETS THE ELEVATION BONUS, and it is gated by the ALREADY-SHIPPED `bRangedAttack` member** (`SummonedUnit.cpp:1197`, bound from `Row->bRanged`) — ⛔ **not by a new CardID list, ⛔ not by a hardcoded name check.** ⚖️ **A future ranged card inherits both behaviours from its own data row and needs zero code.** ⛔ Melee units get **×1.0**, unconditionally.

### HIGH-§5 ⛔⛔ NO CAP IS INVENTED — BUT THE WORST CASE IS **COMPUTED AND HANDED TO HIM**

- ⛔ **HE DID NOT ASK FOR A CAP AND ⛔ ONE IS NOT ADDED.** ⚖️ *Inventing a ceiling he did not request is silently softening his number, which is the same sin as not tripling the range.*
- ⭐ **THE COMPUTED WORST CASES, so a cap is HIS decision made with figures rather than a surprise at playtest** (all at +10% per 152.4 uu, **additive and uncompounded** — the plain reading):

  | Height advantage ΔZ | Arithmetic | Multiplier |
  |---|---|---|
  | 152.4 uu (his 5 ft) | 1 + 0.10×1 | **×1.10** |
  | ~1,000 uu — the tallest shipped hill crown (`ElevationReliefCeiling`'s measured derivation) | 1 + 0.10×(1000/152.4) = 1 + 0.6562 | **×1.656 (+65.6%)** |
  | ~1,200 uu — `TOWER-§`'s proposed platform, on flat ground | 1 + 0.10×7.874 | **×1.787 (+78.7%)** |
  | ~2,200 uu — **the tower ON a tall hill, target in a valley** | 1 + 0.10×14.435 | ⚠️ **×2.44 (+144%)** |
  | ~8,000 uu — a castle shell, if a unit ever reached one | 1 + 0.10×52.49 | ⛔ **×6.25** — theoretical; ⛔ no unit can stand there today |

- ⚠️ **THE ONE TO ACTUALLY WATCH IS ×2.44** (tower on a hill). ⭐ **A Longbowman there deals `18 × 2.44 = 43.9` per shot at 3,600 range against a target that, if it is another Longbowman on flat ground, cannot reach it at all.** **That is the stacking case, it is stated before he plays it, and it is his to cap or keep.**
- **If he later wants one, the sanctioned shape is ONE `EditDefaultsOnly` `HeightBonusMaxMultiplier` (0 = uncapped, the shipped default), ⛔ not a magic number in the formula.**

### HIGH-§6 ⚠️ THE CONSEQUENCES OF ×3 RANGE — MEASURED AGAINST THE REAL ARENA, ⛔ NOT ASSERTED

- **The arena is BIGGER than the number sounds** (`USiegeScatterConfig::ArenaHalfExtent = (26000, 12000)` ⇒ **52,000 × 24,000 uu**; castles at **X = ±25,000**, i.e. **50,000 uu apart**). ⇒ **Longbowman at 3,600 covers 7.2% of the castle-to-castle line and 15% of the field's width.** ⭐ ⛔ **"They will hit across most of the arena" is FALSE, and it is worth telling him so** — the M7.6 10× scale-up absorbs most of the ×3.
- ⚠️⚠️ **THE REAL CONSEQUENCE, AND IT IS SEVERE: EVERY DEFENSIVE TOWER IN THE GAME IS NOW OUTRANGED BY A UNIT IT CANNOT ANSWER.**

  | Defender | Range | vs Archer 2,100 | vs Longbowman 3,600 |
  |---|---|---|---|
  | ArrowTower | 900 | outranged **2.3×** | outranged **4.0×** |
  | BombTower | 800 | outranged **2.6×** | outranged **4.5×** |
  | CrystalTower | 800 | outranged **2.6×** | outranged **4.5×** |
  | BallistaTower | 1,400 | outranged **1.5×** | outranged **2.6×** |

  ⇒ **A stationary archer can demolish any tower card for free.** Towers stop being defence. ⛔ **This is NOT softened — he said triple.** ✅ **It is STATED, with the fix he might want (triple the towers too) offered as `HIGH-§7` row R-2.**
- ⚠️ **AI ENGAGEMENT DISTANCES CHANGE.** Ranged units halt and open fire from 3× further out; the bot's approach logic was tuned against the shipped ranges. ⛔ **This is a WATCH ITEM for his playtest, ⛔ not a speculative fix task** — no observation exists yet, and boarding a repair for an unobserved symptom is guessing.
- ✅ **The castle itself has NO attack loop** (`ATower` is the only auto-firing actor — `Tower.h:13-24`), so ⛔ there is no "outranging castle defences" interaction to fear. **The range task re-confirms this in one grep and declares it.**

### HIGH-§7 📌 THE FOR-JONATHAN ROWS THIS LAW OWES (all proceeding defaults — work does ⛔ NOT stop for them)

| Row | Question | **Proceeding default** |
|---|---|---|
| **R-1** | Bonus relative to the TARGET (a) or absolute world Z (b)? | ✅ **(a) above the target, bonus only when positive** (`HIGH-§2`) |
| **R-2** | Do the three ranged **TOWERS** get ×3 range too? | ⛔ **No — units only.** They are buildings, not units |
| **R-3** | Cap on the elevation bonus? | ⛔ **None** — uncapped; worst case ×2.44 stated (`HIGH-§5`) |
| **R-4** | Continuous or stepped per 5 ft? | ✅ **Continuous (linear)** |
| **R-5** | Does the **hero** get the bonus? | ⛔ **No** — melee, separate damage path |
| **R-6** | Do **melee** units get a high-ground bonus? | ⛔ **No** — his sentence is about ranged attacks |

### HIGH-§8 📌 BATCH SHAPE + M8

- **ONE QA gate over the batch's code diffs · ONE compile · ONE commit (code + `cards.csv` + Content together) · then Jonathan plays it.**
- ⛔⛔ **`Docs/Data/cards.csv` HAS EXACTLY ONE OWNER IN THIS BATCH** (the ×3 cells **and** `TOWER-§`'s new row land in the **same** task). ⚠️ **Two tasks editing one CSV is a board write-race and this batch does not run one.**
- ⚠️ **A `cards.csv` edit is INERT until `/Game/Data/DT_Cards` is REIMPORTED** — an **editor** step, and therefore **build-master's at integration**, ⛔ never claimed done by the task that edits the text. **The reimport is an explicit integration acceptance line.**
- 📌 **M8: ⛔ NO replicated property, ⛔ NO new class tier, ⛔ NO RPC.** The damage composition is server-authoritative exactly as every existing multiplier in `ComputeOutputDamage` already is; the range is a data row. **Each code handoff declares it.**

---

