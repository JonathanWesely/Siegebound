# TASK-838 — THE VISIBILITY CEILING: THE FOG CLAMP INSIDE THE FUNNEL — programmer handoff

**Status:** `ready-for-qa` · **Gate:** `TASK-850` · **Law:** `FOG-§1`/`§2`/`§4`/`§5`/`§6`/`§7`/`§7a`/`§7b`, `SC-§38`, `WITCH-§1`/`§2`
**Blockers consumed:** `TASK-828` (the funnel, gate `TASK-848` PASS) · `TASK-837` (the fog statics, gate `TASK-847` PASS) · `TASK-829` (the veil, released `SiegeCombatStatics.{h,cpp}`)
**Compile / editor / MCP / Git:** ⛔ none touched — build-master's lane. The editor was live under two art tasks for this whole task.
**Fence honoured:** ⛔ no `SiegeBotController.{h,cpp}` (`TASK-851`, running in parallel — it shows as modified in `git status`, and that is **851's diff, not mine**) · ⛔ no `SummonedUnit` break-call edits (`829`'s, untouched) · ⛔ no fog actor / spell / card row / visual (`839`/`840`/`841`) · ⛔ no `SpellLibrary.cpp`, `SpellLineSweep.cpp`, `SiegeCheatManager.cpp`.

---

## 1. WHAT SHIPPED, IN ONE PARAGRAPH

`FSiegeCombatStatics::GatherHostileAgents` gained a **second optional parameter** — `const FSiegeVisionQuery* Vision` — beside `VeilPolicy`, exactly as `TASK-829` left written in the header. The **five** `FOG-§7` row-1 vision sites hand over the two facts only they know (where they look **from**, and the reach they look **with**); every other lane hands over **nothing** and therefore *cannot* be clamped. Inside the funnel — and **nowhere else in `Source/`** — `FSiegeFogStatics::EffectiveVisionRadius` is called **unconditionally** on that request, and candidates beyond the returned radius are dropped with `RemoveAll`. `FOG-§7b` shipped **both halves**. Eight new automation tests assert the **wiring**, not `TASK-837`'s arithmetic; three of them are built to fail the obvious-wrong implementation, and one of them **refutes a claim in the law it was written from**.

---

## 2. FILES TOUCHED

| file | what |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.h` | new `struct FSiegeVisionQuery` (+2 named constructors); `GatherHostileAgents` gains a **defaulted 5th param**; new **private** `ReadFogState`; fwd-decl `FSiegeFogTuning`; `Math/NumericLimits.h` include |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeCombatStatics.cpp` | `ReadFogState` body (the fog-state seam); the fog cut inside `GatherHostileAgents`; 1 include |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeFogStatics.h` | **`FOG-§7b`(a)** `ClampMin "0"` → `"304.8"`; totality doc updated to `<= 0`; ⚠️ **one prose correction — see §5** |
| `Source/GitClaudeUnrealTest/Siegebound/SiegeFogStatics.cpp` | **`FOG-§7b`(b)** guard `Ceiling < 0.f` → `Ceiling <= 0.f` |
| `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.cpp` | **sites 1–2** — vision query at `AcquireTarget` + `AcquireEnemyNearPoint` (⛔ no break-call touched) |
| `Source/GitClaudeUnrealTest/Siegebound/Tower.cpp` | **sites 3–4** — vision query at `AcquireTarget` + `FireChainZapAt` |
| `Source/GitClaudeUnrealTest/Siegebound/HeroCharacter.cpp` | **site 5** — vision query at `DoMeleeAttack` |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogClampTest.cpp` | ⭐ **NEW — +8 tests** |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeFogTest.cpp` | ⛔ **comment-only**, spec (6) — the two arithmetic corrections. **Zero assertions changed.** |

⚠️ **Three files are outside my `names` list and it is not scope creep — the spec put them there.** `TASK-838(1)` names the five gatherer calls in `ASummonedUnit`/`ATower`/`AHeroCharacter` explicitly, and `qa/TASK-848.md` ruled the clamp *"needs a radius the funnel signature does not take, so it must arrive **at the call site**"*. Those five sites are the only place that radius exists. Each edit is one `const FSiegeVisionQuery Vision = …;` line plus the extended gather call (`MyLocation` was hoisted a few lines at three of them — same expression, same value).

---

## 3. ⭐⭐ THE SHAPE, AND WHY IT IS THIS ONE (⚠️ QA: this is the ruling to scrutinise)

```cpp
static void GatherHostileAgents(const UWorld* World, ETeamId ViewerTeam, TArray<AActor*>& Out,
    ESiegeVeilPolicy VeilPolicy = ESiegeVeilPolicy::SuppressVeiled,
    const FSiegeVisionQuery* Vision = nullptr);
```

**The exemption is `if (Vision)` and nothing else.** No branch inspects the caller, no enum names a call-site kind, no list of exempt sites exists anywhere. A lane is clamped exactly when it hands over an act of seeing. `FOG-§7` rows 2/3/4/5 hand over nothing, so they **cannot** be clamped — not *"are not clamped today"*, **cannot be**, because there is no radius at the funnel to clamp them with.

**Three shapes I rejected, with the measurement that killed each:**

1. **A second named function** (`GatherVisibleHostileAgents`). ⛔ `SiegeAcquisitionFunnelTest` test 8 pins `GatherHostileAgents` at **exactly one declaration** in the header, and test 4 pins the qualified call count **per file** (2/2/1/2/1/1). A rename would have driven three of those to zero and forced me to rewrite the site census that exists to catch a tenth site.
2. **The clamp at the call site.** ⛔ `SiegeAcquisitionFunnelTest` test 9 asserts `FSiegeFogStatics` and `EffectiveVisionRadius` appear **ZERO** times in all five call-site files, and says so in its own message: *"including for TASK-829 and TASK-838, which must NOT relax this row."* Measured after my diff: still **0** in all five.
3. **A default-constructed "unbounded" query instead of a null pointer.** ⛔ Unbounded is a *legitimate vision query* (two sites use it) — under fog it clamps to the ceiling. Making it the default would have silently clamped the blast, the two spell gathers, the friendly buff and the cheat lane. Absence had to mean **"not an act of seeing"**, which is a different thing from "seeing without limit".

**The per-candidate metric is closest-point-on-collision with the origin fallback**, and that is a correctness requirement, not a preference: it is byte-identical to what `ASummonedUnit::GetDistanceToTarget` and the hero's melee sweep already use, and it is **never stricter** than any site's own metric (closest-point ≤ origin-to-origin), so the cut can only ever remove what the fog removed — a tower measuring origin-to-origin keeps its own gate untouched.

**The no-op guard is on the seam's OUTPUT, not on the fog's state**, and that is deliberate:

```cpp
if (EffectiveRadiusUU < Vision->RequestedRadiusUU)   // ⛔ never `if (bFogActive)`
```

`EffectiveVisionRadius` is called **unconditionally** (gate row honoured). The guard is *strictly stronger* than a state check: it also skips the loop for every degenerate tuning in `FOG-§7a`'s totality list (NaN, negative, **zero**, `Ceiling <= Onset`), all of which return the request unchanged. With fog off it costs **zero** collision queries per acquisition poll, so the acquisition surface is byte-for-byte the game that shipped.

---

## 4. ⚖️🧑 `J-F9` — THE DEFAULT I BUILT, IN ONE PLAIN SENTENCE

> **Under fog the hero's line spell still reaches its full 900 uu, so he can hit something at 900 uu that he literally cannot see (his eyes reach 609.6).**

That is the recorded proceeding default, built as recorded and **not improvised**. `ASpellLineSweep::LineRange = 900.f` already exceeds the ceiling, so a blind clamp would have been a **−32.3% live nerf** to a card he has already played. His sentence — *"ranged **units** will not be able to fire beyond this range"* — does not answer it, because a hero spell is not a unit.

⭐ **Flipping it is ONE line:** construct a `FSiegeVisionQuery` at `SpellLineSweep.cpp`'s gather. Nothing in `SiegeCombatStatics.{h,cpp}` changes either way, and `SiegeFogClampTest` test 2 has the row to invert.

---

## 5. ⛔⛔ A MEASURED FINDING THAT **REFUTES THE LAW THIS TASK WAS BOARDED FROM** — `Lightning` ships `AoERadius = 700`

`FOG-§7` and `TASK-838(5b)` both state that **every** AoE radius in the game is under the 609.6 ceiling, enumerating six cards (Sapper 250 · BombTower 250 · Wizard 250 · Fireball 300 · FrostNova 350 · BattleCry 400). **That claim is false.** Measured from `Docs/Data/cards.csv` (31 rows parsed, all aligned with the header):

| card | `AoERadius` | resolver | `FOG-§7` row |
|---|---|---|---|
| **`Lightning`** | ⛔ **700** | `ResolveTopTargetsDamage` (reticle radius) | **row 3** — directed spell geometry |
| FrostNova | 350 | `ResolveFreeze` | row 3 |
| BattleCry | 400 | `ResolveAllyBuff` | row 5 |
| Fireball | 300 | `ResolveAoEDamage` → `ApplyRadialDamage` | row 2 |
| Sapper / BombTower / Wizard | 250 | `ApplyRadialDamage` | row 2 |

**Three consequences, in order of how much they matter:**

1. ⭐⭐ **`J-F9` covers a CLASS of surfaces, not one spell.** A blind clamp would have been **two** live nerfs, not one: the line sweep **−32.3%** *and* Lightning's 3-target selection **−12.9%** — small enough that nobody would have caught it in a playtest, large enough to change which three targets a strike picks. Both are row 3, both are exempt for the same structural reason.
2. ⚠️ **The spec's own "synthetic" value was a shipped number.** `TASK-838(5b)` proposes `700` as the synthetic over-the-ceiling radius. It is `Lightning`'s live value — so a test written to the letter of the spec would have been drawn from today's data after all, which is precisely the failure (5b) exists to prevent, one level up from itself. My synthetic radius is therefore **derived** (`2 × ceiling`), and test 3 additionally asserts it appears nowhere in the shipped AoE column.
3. ✅ **Nothing is broken, and that is the vindication.** `FOG-§7`'s reasoning was *"a blind clamp on blasts is inert today by coincidence of current data"* — the coincidence was **never even true**, and the only reason it costs nothing is that rows 2 and 3 are **structural**: they do not call `EffectiveVisionRadius` at all, so the day someone raises an AoE past the ceiling that card does **not** silently become fog-dependent. That day was already yesterday.

⇒ **Manager amendment to `FOG-§7`, not a code fix.** I corrected the same wrong sentence where it also lives in `SiegeFogStatics.h`'s prose (a wrong number in a comment is a real defect in this project) and pinned all of it in test 3.

---

## 6. ⚖️ `FOG-§7b` — BOTH HALVES, AS RULED

| half | shipped | why neither alone is sufficient |
|---|---|---|
| **(a)** | `FogVisionCeilingUU`'s `meta = (ClampMin = "0")` → **`"304.8"`** — the **onset's own pinned value** (`FOG-§1`, 10 ft), not a new number | protects the **designer**: below the onset the model has no band at all, so the slider's floor is the tightest ceiling that is still a fog |
| **(b)** | the guard `Ceiling < 0.f` → **`Ceiling <= 0.f`** | protects the **game**: `ClampMin` constrains the editor spinner and nothing else — an `.ini`, a Blueprint default or a line of C++ can still write `0` |

Documenting `0` as a deliberate "blind" capability was **refused on the record**, and I did not re-open it. Asserted in test 6: a ceiling of `0` returns the request **bit-identically, exactly as a negative one does** (`Exact` tolerance), **and combat actually continues** — the predicate is exercised too, because *"returns 3600"* and *"can still shoot"* are different claims and it is the second one Jonathan would notice.

📌 **Declared:** `ClampMin = "304.8"` is now the **second textual occurrence** of `304.8` in `Source/`. UHT meta values cannot reference a C++ constant, so it is unavoidable. It is a **meta string, not a float literal**; the line says so, and test 6 asserts the string tracks `FogVisionOnsetUU`, so an onset retune that forgets it goes red rather than leaving a slider floor that means nothing.

---

## 7. ⚠️⚠️ DECLARED RESIDUALS — say these out loud rather than let QA find them

1. ⛔⛔ **FOG DOES NOT EXIST AT RUNTIME YET, AND THIS TASK DOES NOT CHANGE ONE UNIT'S BEHAVIOUR.** `AFogVolume` is `TASK-839`'s and `TASK-839` is **blocked by this task**, so the wiring lands first and the state lands second. `ReadFogState` returns `false` today ⇒ `EffectiveVisionRadius` returns every request bit-identically ⇒ the cut can never run. **A landed clamp is not a landed feature.** `TASK-839` replaces exactly one `return false` and changes nothing else — not the signature, not the call site, not one line at any of the five vision sites. Test 8 pins this as a **decision** and names the row `839` must **invert, not delete**.
2. 📌 **The unit acquisition site is essentially untouched by fog, and `FOG-§2`'s table does not describe it.** `AggroRadius` is the GDD §3.8 **profile constant 600** — *below* the 609.6 ceiling — not the card's `Range`. `FOG-§2`'s −83.1% Longbowman figure is about the row's `Range`, which gates **firing** (`UpdateState` / `PerformAttack`, `<= AttackRange` at five sites), **not** the gather. ⇒ under fog a unit's acquisition changes by nothing; **the card bites at the TOWERS** (ArrowTower 900 −32.3%, BombTower/CrystalTower 800 −23.8%, BallistaTower 1400 −56.5% with its `MinRange 300` intact ⇒ the 300–609.6 annulus) and at the commanded-zone retarget. ⚠️ If Jonathan expects a Longbowman to stop *firing* past 609.6, that is the `<= AttackRange` gate — **not a gather site**, so it is out of `FOG-§7` row 1 and would be a new task.
3. ⚠️ **The chain zap's candidate pool narrows under fog.** `FireChainZapAt` hands over an **unbounded** query from the tower, because it has no tower-range gate of its own (bounces are measured from the previous target and may legally leave the tower's ring, M5 ruling 9). Fog-off: bit-identical. Fog-on: the pool is what the tower can see, so a bounce can no longer walk out into the fog. The bounce rule itself is untouched.
4. ⛔ **The bot's strategic planner stays omniscient** (`J-F4`, deferred). Its **units** go blind — every unit, tower and hero acquires through the funnel I edited — but its **card-play planning** still reads a perfect world snapshot through three `TActorIterator` scans in `SiegeBotController.cpp`. That file is `TASK-851`'s this wave, for **invisibility only**; the asymmetry is deliberate and is not an inconsistency.
5. ⚠️ **An arrow already in the air still lands** (`TASK-837` ruling 4, inherited unchanged) — acquisition only.
6. ⛔ **No test drives the clamp over a populated world.** The house rule forbids `SpawnActor`/`CreateWorld` in `Siegebound/Tests/`. **Green here is not "fog works"** — it is "the ceiling is wired to exactly five lanes and to no others". `TASK-850(3a)`'s PIE pass is where the live behaviour is judged.

---

## 8. ⚠️⚠️ A PRE-EXISTING RED ROW I FOUND AND **DID NOT TOUCH** — for `TASK-849`/`TASK-850`

`SiegeAcquisitionFunnelTest.cpp` **test 9** (`Siegebound.Acquisition.VeilAndFogSuppressionLiveOnlyInTheFunnel`) asserts five tokens are **zero** in five call-site files. Measured today, with the house comment-skipping scanner:

| file | `bIsInvisible` | `FSiegeInvisibilityStatics` | `IsVisibleTo(` | `FSiegeFogStatics` | `EffectiveVisionRadius` |
|---|---|---|---|---|---|
| **`SummonedUnit.cpp`** | ⛔ **3** | ⛔ **4** | 0 | ✅ 0 | ✅ 0 |
| Tower / Hero / SpellLineSweep / SiegeCheatManager | 0 | 0 | 0 | ✅ 0 | ✅ 0 |

⛔ **This is `TASK-829`'s, not mine** — those hits are `IsInvisible()` / `GrantInvisibility()` / `BreakInvisibility()`, the veil's **write doors**, which `WITCH-§6` requires to live on `ASummonedUnit`. **Both fog tokens are zero everywhere**, so my half of that row is clean. The row's *intent* is still honoured: `SummonedUnit.cpp` holds the veil **state**, not a suppression **check** (`IsVisibleTo(` is 0 there).

⇒ **Suggested fix, for whoever owns it — a scoped token list, not a code change:** split the five tokens into **GUARD** tokens (`IsVisibleTo(`, `FSiegeFogStatics`, `EffectiveVisionRadius`) that must be zero in all five files, and **STATE** tokens (`bIsInvisible`, `FSiegeInvisibilityStatics`) that must be zero in four, with `SummonedUnit.cpp` exempt **by name** because `WITCH-§6` makes it the veil's home. ⛔ I did not edit that file: it is `TASK-829`'s diff, still under gate `TASK-849`, and patching another task's gate mid-loop is how a real defect gets hidden.

---

## 9. TESTS — `Tests/SiegeFogClampTest.cpp`

⭐ **SUITE DELTA: +8 TESTS, in one new file.** Census re-taken by grep of `IMPLEMENT_*_AUTOMATION_TEST` across `Siegebound/Tests/`: **408 declared across 30 files** after my diff (**392 / 29** when I started). ⛔ Per `TL-§5c` the **delta is the trustworthy figure** — the tree moved under `TASK-851`/`853` while I worked, and `declared` is a count of declarations, **not a pass count: I ran nothing.**

⛔ **This file deliberately does not re-assert `TASK-837`'s arithmetic** (spec 5a). Every row asserts which lane is wired to the ceiling and which is not.

| # | test | what makes it able to FAIL |
|---|---|---|
| 1 | `ExactlyTheFiveVisionSitesHandOverAVisionQuery` | per-file census (2/2/1) **plus zeros** for the four unclamped lanes, each with a positive control; then the same five counted **tree-wide**, so a sixth in a file nobody listed is caught. ⛔ An INCREASE is a silent nerf; a DROP is a lane that keeps full range under fog |
| 2 | ⭐⭐ `TheHeroLineSpellKeepsItsFullRangeUnderFog` (`J-F9`) | re-measures `LineRange = 900.f` **at source** (a retune red-flags the ruling rather than silently defending a dead number); asserts the **counterfactual** (a clamp would cut it to 609.6, **−32.3%**) so the exemption is load-bearing, not decorative; asserts the sweep hands over **zero** vision queries |
| 3 | ⭐⭐⭐ `TheBlastLaneIsExemptAndTheExemptionIsStructural` | the **derived** synthetic radius (2× ceiling) + a row proving it appears **nowhere** in shipped data; the blast body's zero vision queries and zero fog symbols; the `cards.csv` tripwire pinning **Lightning 700** and **1** shipped radius above the ceiling; and Lightning's own **−12.9%** counterfactual |
| 4 | ⭐⭐ `TheCeilingIsAppliedInsideTheFunnelAndNowhereElse` | the leaf-grep gate at **2** tree-wide (definition + one call) ⇒ a third hit is a per-site clamp; the call is inside the **funnel body**; `if (bFogActive` == **0**; the veil consult still == 1; no `Sort(`; the friendly lane takes no vision parameter **at its declaration** |
| 5 | ⭐⭐ `AnAsymmetricFogIsUnrepresentableBecauseNoFogFunctionTakesATeam` | seven asymmetry tokens == 0 across **both** fog files with a per-file positive control, **plus** the same for the new `FSiegeVisionQuery` slice. ⛔ Deliberately **not** a *"both teams get the same answer"* test — that would be trivially true and would report SAFE forever |
| 6 | ⭐⭐⭐ `AZeroCeilingReturnsTheRequestAndNeverStopsCombat` (`FOG-§7b`) | zero **and** negative ceilings return the request at `Exact`; the **predicate** confirms a Longbowman at 3000 and a melee at 100 still engage; the source form `Ceiling <= 0.f` == 1 and `Ceiling < 0.f` == **0**; `ClampMin = "304.8"` == 1 and the string **tracks** the shipped onset |
| 7 | ⭐ `EveryVisionSiteIsBitIdenticalWithFogOff` | the six radii the five sites actually hand over (including the unbounded sentinel) at `Exact`; then the **no-op property** — with fog off the effective radius is never shorter, so the cut provably cannot run; plus the never-raised rows for melee 150 and aggro 600 |
| 8 | ⭐⭐ `TheFogStateIsReadInExactlyOnePlaceAndIsNotLiveUntilTask839` | `ReadFogState` named exactly **3×** tree-wide; called **once per gather**, not per candidate; the seam's honest `return false` count, labelled as the row `839` must **invert**; and `OutTuning` assigned unconditionally |

**Existing suites re-verified against my diff** (measured with the house scanner, not assumed): `TASK-828` test 4 per-file gather counts **2/2/1/2/1/1** ✓ · test 6's `GatherTeamAgentsFiltered` signature and ban list **untouched** ✓ · test 7's `…HostileAgents, ESiegeVeilPolicy::IncludeVeiled)` needle **still 1**, `SuppressVeiled` in the blast body **still 0**, `Distance > Radius` **still 1** ✓ · test 8's header counts `GatherHostileAgents`/`GatherFriendlyAgents`/`GatherTeamAgentsFiltered` **1/1/1** ✓ · `TASK-829`'s `IsAgentVisibleTo(` in the funnel body **still 1** and `Sort(` **still 0** ✓.

---

## 10. WHAT QA SHOULD SCRUTINISE

1. ⭐⭐ **The `if (EffectiveRadiusUU < Vision->RequestedRadiusUU)` guard.** Read it as a test of the seam's **output**, not of the fog's state — that is the whole argument for why it is not the `if (bFogActive)` the gate forbids. If you disagree, it deletes cleanly at the cost of a collision query per candidate per poll with fog off.
2. ⭐ **The two UNBOUNDED sites** (`AcquireEnemyNearPoint`, `FireChainZapAt`). Handing over any *other* radius would narrow them **with fog off** — a shipped behaviour change under a fog card. Unbounded is the only behaviour-neutral answer, and residual §7.3 is its declared cost.
3. ⭐ **The closest-point metric inside the funnel.** The claim is that it is never stricter than any site's own metric. Two sites use it identically; the tower's is origin-to-origin (looser cut ⇒ superset ⇒ its gate unchanged).
4. ⛔ **§5 — the `Lightning 700` finding.** It contradicts `FOG-§7` and `TASK-838(5b)` in writing. Please confirm the measurement before the manager amends the law.
5. ⛔ **§8 — the pre-existing test-9 red row.** Confirm it is `TASK-829`'s and route it, rather than attributing it to this diff.
6. 🔒 No compile, no editor, no MCP, no Git. No inference, no `Capture()`, no latch spend, no token figure.
