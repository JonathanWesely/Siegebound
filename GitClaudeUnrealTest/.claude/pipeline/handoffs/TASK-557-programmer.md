# TASK-557 — [WR-3] THE RE-DERIVATION LEDGER — every constant the old castle size authored

**Agent:** gameplay-programmer · **Date:** 2026-08-15 · **Status → `ready-for-qa`**
**Law:** CONVENTIONS `WR-§0` · `WR-§1` · `WR-§2` · `SC-§34` · `SC-§22` · `SC-§18c` · `SC-§15` · `SC-§33`
**Gate:** `qa/TASK-565.md` (re-runs this enumeration independently) · **Compile:** TASK-566 · **Commit:** TASK-570

⛔ **No compile, no editor, no MCP, no PIE, no Git, no `Content/`, no `.csv`, no `Tests/`.** Files only.
⛔ **NO TOKEN FIGURE IS QUOTED ANYWHERE IN THIS HANDOFF** (`AS-§12g`; TASK-552 still owes the `zoneA_tok` print).

---

## 0. THE ENTIRE CODE DIFF IS EIGHT INITIALISER LINES — `SC-§33` DOES NOT FIRE, AND THAT IS PROVEN, NOT ASSERTED

Command run over the five owned files, filtering out every comment line:

```
git diff -U0 -- Castle.h Castle.cpp SiegePlayerController.h SiegeBotController.h ScatterConfig.h \
  | grep -E "^[+-]" | grep -vE "^(\+\+\+|---)" | grep -vE "^[+-]\s*(\*|//|/\*\*|\*/)" | grep -vE "^[+-]\s*$"
```

```
- constexpr float CastleDamageNumberHeightZ = 3150.f;      + ... = 9450.f;
- HPBarWidget->SetRelativeLocation(FVector(0,0, 3150.0f));  + ... 9450.0f);
- FVector2D SpawnBoxHalfExtent = FVector2D(2460.f,2460.f);  + FVector2D(7380.f,7380.f);   [ACastle]
- FVector GateBlockerRelativeLocation = (6,-525,284);       + (18,-1575,852);
- FVector GateBlockerExtent = (260,135,226);                + (900,405,678);
- float CastleKeepClearRadius = 1500.f;                     + 4500.f;
- FVector2D SpawnBoxHalfExtent = FVector2D(2460.f,2460.f);  + FVector2D(7380.f,7380.f);   [ASiegePlayerController]
- FVector2D SpawnBoxHalfExtent = FVector2D(2460.f,2460.f);  + FVector2D(7380.f,7380.f);   [ASiegeBotController]
```

**Zero signatures changed · zero parameters added · zero functions added or removed.** `SC-§33` binds a *defaulted parameter added to a function that already has call sites*; nothing here is a function at all. **No call-site grep is owed** — and this is the mechanical proof rather than the "every call site stays byte-identical" sentence the law explicitly refuses.

**Files touched (exactly the five in my `names:` block):**
`Source/GitClaudeUnrealTest/Siegebound/Castle.h` · `Castle.cpp` · `SiegePlayerController.h` · `SiegeBotController.h` · `ScatterConfig.h`
**NOT touched, as required:** `CaptureZone.{h,cpp}` · `SiegeNetLimits.h` · any `Content/` · any `.csv` · any `Build.cs` · `Tests/` — **and additionally** `SiegePlayerController.cpp` · `SiegeBotController.cpp` · `SiegeGameMode.{h,cpp}` · `SummonedUnit.h` · `HeroCharacter.{h,cpp}` · `BattlefieldScatter.{h,cpp}`, each of which turned up ledger rows and is named to an owner in §3.

---

## 1. ⛔ THE ONE DEPARTURE FROM THE SPEC, DECLARED (`SC-§15`) — `GateBlockerExtent.X` IS **900**, NOT THE SPECCED **780**

`WR-§2` row 2 and spec item (2) both say `(260,135,226) → (780,405,678)` — a flat ×3. **I refused the X term on a checkable mechanism. Y and Z are ×3 exactly as specced.**

**The arithmetic, which is the whole argument:**

| | shipped (3× castle) | naive ×3 (9×) | delivered |
|---|---|---|---|
| gate clear opening | 600 wide | 1800 wide (`WR-§1`) | 1800 wide |
| blocker X span | 520 | 1560 | **1800** (half-extent 900) |
| **jamb gap PER SIDE** | **≈40 uu** | **≈120 uu** | **≈0 uu** (±18 from the mesh-local X offset) |

- The shipped ≈40 uu gap was safe **only because 40 < the narrowest agent DIAMETER on the field**: hero capsule r≈42 ⇒ **84 uu** (`SiegeGameMode.h`'s own figure), Cavalry r45 ⇒ **90 uu** (CONVENTIONS "Castle 3× HOLLOW", the sizing agents).
- At ≈120 uu **both of those capsules fit through the gap and walk into the enemy keep AROUND the blocker.** The team-gated interior — the physical half of a belt-AND-braces ruling — is silently deleted, and **every bounds readback still passes.**
- ⛔ **Bodies did not grow (`WR-§1` / `SC-§34`'s human-scale exemption), so the GAP TOLERANCE may not grow either.** The opening is a shell feature and scaled; the gap is a body quantity and may not. **900 = half of the 1800-uu opening**, i.e. the blocker spans the full opening — no invented margin, one division. Embedding into the jambs costs nothing: `ConfigureTeamGating` sets the volume to Ignore **every** channel except the enemy team's.
- Residual ±18 uu from the (unchanged, ×3'd) mesh-local X offset is far under 84 uu and cannot pass a body.

**Y and Z ×3 verified sound, not assumed:**
- **Z → [174, 1530].** The bottom, 174, is **exactly** the 9× interior floor height (3 × 58, `WR-§1`) — the blocker still sits *on* the threshold. The top, 1530, still clears the ≈1356 gate clear height as 510 cleared ≈452. A hero jump (≈250 apex + ≈190 capsule ≈ 440 over the floor) reaches ≈614 — nowhere near 1530.
- **Y → 810 deep** through a wall that got thicker. Strictly safer against capsule tunnelling than the shipped 270.

**Sign convention verified before typing, as spec item (2) demanded — not assumed.** Y is negative because the gate corridor mouth is on the local −Y side and +Y is "deeper into the keep" (`InteriorAnchorRelativeLocation`'s doc), and the shipped span Y [−660, −390] confirms it. ×3 preserves sign and direction: Y [−1980, −1170].

⚠️ **For TASK-567/569:** 1800 is `WR-§1`'s own figure (600 × 3) and TASK-555 authors the mesh in parallel. `GateBlockerExtent` stays `EditAnywhere`; PIE-verify the cover against delivered geometry. **It may not be reduced below "opening span minus one agent diameter" without re-opening the reasoning above.**

---

## 2. THE LEDGER — `WR-§2`'S 13 ROWS, WORKED ONE BY ONE

Marks: **(i)** re-derived, with the arithmetic · **(ii)** deliberately unchanged, with the reason · **(iii)** outside this task's ownership, named to its owner.

| # | constant | mark | arithmetic / reason |
|---|---|---|---|
| 1 | **`SpawnBoxHalfExtent`** — 3-way paired tunable | **(i)** | `(2460,2460) → (7380,7380)` at **ALL THREE** sites: `ACastle` `Castle.h`, `ASiegePlayerController` `SiegePlayerController.h`, `ASiegeBotController` `SiegeBotController.h`. ×3, preserving the law's own intent "half-extent ≈ the castle's full width" against 9× bounds `7313.7 × 7384.5`. **All three cross-note doc blocks rewritten in the same edit.** |
| 2 | **`GateBlockerExtent`** | **(i)** | `(260,135,226) → (900,405,678)`. **X departs from the specced 780 — see §1.** |
| 3 | **`GateBlockerRelativeLocation`** | **(i)** | `(6,−525,284) → (18,−1575,852)`, ×3. Mesh-LOCAL; the mesh scaled uniformly under it, so ×3 is exact, not an estimate. Sign verified (§1). |
| 4 | **`USiegeScatterConfig::CastleKeepClearRadius`** | **(i)** + **(iii)** | Header default `1500 → 4500` (×3), `ScatterConfig.h`. ⛔ **The saved `Content/Data/DA_BattlefieldScatter.uasset` OVERRIDES this default — `ASiegeBattlefieldScatter` reads the DataAsset, never the C++ default. A header-only change is INERT. → TASK-569.** 📌 Honest note in the doc block: 4500 does **not** circumscribe the 9× castle (XY half-diagonal ≈5197), exactly as 1500 did not circumscribe the 3× one (≈1732) — the ×3 preserves the shipped relationship, so the corner shortfall is pre-existing and unchanged in proportion, **not** a regression. |
| 5 | **`ACastle::HPBarWidget` relative Z** | **(i)** — **DIAGNOSED, NOT GUESSED** | ⭐ **It is C++-AUTHORED, not BP-authored:** `Castle.cpp` ctor, `HPBarWidget->SetRelativeLocation(FVector(0,0,3150))`. ⇒ re-derived here: `3150 → 9450`. Lineage 1050-over-900 → 3150-over-2694 → 9450-over-8083, the same ≈1.17× headroom at all three scales. **This answers TASK-569's item (4): the lane is C++, and it is CLOSED here — TASK-569 owes nothing on it.** ⛔ `DrawSize (256,32)` deliberately NOT scaled: a screen-space widget's size is in *screen pixels*. |
| 6 | **`InteriorNavModifier` extent** | **(ii)** — **VERIFIED, NOT ASSUMED** | Grepped the whole module: the component is constructed, given `AreaClass` + `ForceNavigationRelevancy(true)`, and `SetAreaClass`/`RefreshNavigationModifiers` are the only other calls. **No literal extent is set anywhere** — the `NavModifierVolume` shape derives from owner bounds, so it follows the mesh for free. Nothing to change. |
| 7 | **`ACaptureZone::ZoneHalfExtent` (840,840)** | **(ii)** | Unchanged per the ruling: the "same size as the spawn box" origin was descriptive, never a pairing law, and scaling mid changes capture gameplay this directive does not touch. Identical ruling to CASTLE-3X. ⚠️ Its **doc CLAIMS** are now false — see §3 row S4. |
| 8 | **Threshold step chain / max step 38** | **(ii)** | ⛔ **Human-scale — the LIMIT stays.** Verified in code: `AHeroCharacter::HeroMaxStepHeight = 50` and `HeroWalkableFloorAngle = 50` (`HeroCharacter.h`) are **UNCHANGED and must be** — multiplying them is the defect (`WR-§1`). Units carry no C++ step override; they run the CMC/Recast agent defaults. **The CHAIN is geometry ⇒ TASK-555 (art).** No C++ constant encodes it. |
| 9 | **Gate clear-opening floors ≥500 × ≥450** | **(ii)** | Bodies, not the shell. **Verified they exist nowhere in C++** — they are CONVENTIONS law + mesh acceptance; the delivered 1800 × 1356 simply exceeds them by more. |
| 10 | **`SM_Castle_Crumble01/02/03`** | **(iii)** | Art — **TASK-555**. No C++ reference to crumble *geometry* exists; `ACastle::ApplyCrumbleStage` resolves by path and is scale-agnostic. |
| 11 | **22-hull `UCX_SM_Castle` set** | **(iii)** | Art / `pipeline_manifest.json` — **TASK-555**. |
| 12 | **`CastlePlinthClearance` / `IsPointInsideCastlePlinth`** | **(ii)** | ⛔ **STAYS RETIRED.** Verified: three tombstone comments only (`SiegePlayerController.h`, `SiegeBotController.h`, `SiegeBotController.cpp`), **zero live symbols**, `grep` for either name returns comment lines exclusively. Not resurrected. ⚠️ Its one numeric legacy — the `MinStandoff = 570.f` literal — is a live finding, §3 row S2. |
| 13 | **`SiegeNet::ArenaRelevancyDistance` 60000** | **(ii)** | ✅ **VERIFIED unchanged** at `SiegeNetLimits.h:57`. The **ARENA** did not grow — only the castle. Zero edits to that file (it is on my NOT-touched list). Recorded because it is the obvious false positive. |

---

## 3. ⛔ `SC-§22` — THE TABLE IS A LOWER BOUND, AND THE SWEEP FOUND **ELEVEN MORE ROWS**

Sweeps run (spec item 7), raw counts on the post-edit tree, game module, tests excluded:

| sweep | command | hits | verdict |
|---|---|---|---|
| A | `grep -rn "2460" Source/GitClaudeUnrealTest --include=*.h --include=*.cpp` | **10** | 8 = my own deliberate lineage comments · **2 = stale cross-notes, NOT MY FILE** (row S8) |
| B | `grep -rn "2442" …` | **0** | ✅ nothing |
| C | `grep -rn "2694" …` | **2** | both my own lineage comments |
| D | `grep -rn "\b840\b" …` | **27** | all are `ZoneHalfExtent` / `AncientGroundHalfExtent` (row 7, correctly unchanged) or lineage comments — **except the stale claims in row S4** |
| E | `grep -rniE "castle" … \| grep -iE "extent\|radius\|clearance\|half\|offset\|standoff\|width\|bounds\|tall\|height\|footprint\|plinth\|keep-?out\|keep-?clear\|span"` | 80+ | **this is the sweep that produced S1–S11** |
| F | `grep -rn "1219\|814\|810\|820\|898\|3150\|1050\|420\|570\|1200" …` | 40 | corroborated S1, S2, S5 |

⚠️ **A sweep that finds nothing else is a RESULT** — B is that result and is reported as one. The other five are not.

### ⛔⛔ S1 — `L_Arena`'s Blue `PlayerStart` PUTS THE HERO **INSIDE THE 9× CASTLE**. THIS IS THE SECOND SILENT KILLER AND IT IS NOT IN `WR-§2`.

- `ASiegeGameMode::ResolveHeroStart` branch **2** accepts the level `PlayerStart` at **≈(−23800, 0, 98)** — **1,200 uu** from `Castle_Blue` (−25000) — **before** branch 3's hardened castle-relative fallback ever runs. Its only acceptance test is *"same side of the centerline"*.
- 9× castle half-depth on X = **3,656.85 uu** ⇒ that PlayerStart is **2,457 uu INSIDE the castle footprint.** (Today it is ≈19 uu inside the *box* bound and works only because the 22-hull UCX is tighter than the box.)
- ⇒ **the Blue hero spawns inside the keep at match start and at every respawn** — landing on the interior floor at best, and at worst `SpawnActor failed because of collision` ⇒ **a pawnless player.**
- ⚖️ **This is TASK-357's BLOCKER-5 reproduced on the OTHER branch.** Branch 3 was hardened then (`GetActorBounds` + `HeroSpawnCastleClearance`); branch 2 never was — it was never castle-aware. The `SiegeGameMode.h` doc even records the lesson verbatim: *"hardcoded extents rot."*
- ⛔ **The PlayerStart transform is LEVEL DATA and `WR-§3` forbids saving `L_Arena`.** **But a C++-only fix exists and is cheap:** add a castle-footprint rejection to branch 2's acceptance test, reusing branch 3's already-shipped `GetActorBounds` derivation — a PlayerStart inside the own castle's colliding bounds falls through to the hardened branch. **No level save.**
- **(iii) → `SiegeGameMode.{h,cpp}`. Owned by NO task in this batch** (TASK-554 finished with it). ⛔ **Manager ruling required.**

### ⛔⛔ S2 — `ASummonedUnit::DefendRadius = 2500` MAKES THE **DEFEND STANCE A NO-OP**.

- `SummonedUnit.cpp`: `AcquireEnemyNearPoint(OwnCastle->GetActorLocation(), DefendRadius)` — a disc of 2500 around the castle **CENTRE**.
- 9× castle half-width = 3,656.85 ⇒ **the entire disc is inside the castle's own footprint.** Enemies are physically blocked at the gate, so a besieger stands at **≥3,657** from centre — **outside 2500, never acquired.** Defenders under DEFEND fall back toward home forever while the castle is battered.
- Today the disc reaches **1,281 uu past the wall face** — a real, working defence band. It was never re-derived at CASTLE-3X either.
- Candidates: **≈4,940** (hold the shipped 1,281-uu band past the new face — the `WR-§1` decomposition: shell scales, engagement band does not) or **7,500** (flat ×3).
- **(iii) → `SummonedUnit.h`. No owner in this batch.** ⚠️ Jonathan-flagged tunable ("Q6 default; FLAGGED tunable for the 10x arena") ⇒ **his call, not mine.**

### S3 — `ASiegeBotController::TowerDefenseStandoff = 750` + the `MinStandoff = 570.f` literal
Measured from the castle **centre**, so castle-derived by construction, and **already stale before this batch**: 343 uu *outside* the M1 wall → **469 uu inside** after CASTLE-3X (never re-derived) → **2,907 uu inside** at 9×. The clamp floor 570 (= retired `CastlePlinthClearance` 420 + 150, sized against the ~814-uu castle) is 3,087 uu inside. ⛔ **A ×3 does NOT fix it** (2,250 is still 1,407 inside) — which is exactly why it is reported, not multiplied. **The correct repair is structural** — derive from the castle's live colliding bounds the way `ASiegeGameMode` already does (`SC-§34`'s preferred escape). That edit and the 570 literal both live in **`SiegeBotController.cpp` — no owner in this batch.** **(iii)**; the header default is left at 750 with the full arithmetic in its doc block, because a header-only bump is a guess that cannot fix the clamp floor.

### S4 — `ASiegeBattlefieldScatter::CastleQueryInset = 1200`
Its own doc says *"≈1200 clears a ~810-unit castle footprint"* — **so it is castle-derived, and it has been stale since CASTLE-3X**: at half-depth 1,218.95 the inset point already lands ≈19 uu *inside* the footprint, the exact false-negative the constant exists to avoid. At 9× it is **2,457 uu inside**. ⛔ **×3 (3600) does NOT fix it either** — still 57 uu inside 3,656.85. The honest value is **> 3,657 and < 4,500** (the new `CastleKeepClearRadius`, inside which the pad is guaranteed obstacle-free) — **≈4,000**. **(iii) → `BattlefieldScatter.h`. No owner in this batch.**

### S5 — stale CLAIMS with no constant behind them (comment-only, all **(iii)**, none in my files)
| site | false claim | now |
|---|---|---|
| `CaptureZone.h:44` | zone "is the SAME size as each side's spawn box" | spawn box is 7380 half vs zone 840 half — **false since CASTLE-3X**, worse now |
| `CaptureZone.h:168-169` | `(840,840)` "= 2x the castle footprint" | castle is 7313.7 wide; 1680 is **0.23×**, not 2× |
| `HeroCharacter.cpp:428` | "the castle: ~800x800 footprint" | ~7314 × 7385. Mechanism is fine — `ActorGetDistanceToCollision` is bounds-aware and auto-follows |
| `SiegeGameMode.cpp:~700` | "the HP-bar widget sits 3,150 uu up" | **9,450** as of this task. `bOnlyCollidingComponents=true` means it never mattered, but the number is now wrong |
| `SiegeBotController.cpp:566/628/1228` | "an 840 box" | 7380 |
| `SummonedUnit.h:723` | "generous for every wall/gate face of the 2437×2461 castle" | 7313.7 × 7384.5 — see S7, the **value** is correct |

### S6 — `USiegeScatterConfig::AncientGroundMaxAbsX = 21000` — **(ii) DELIBERATELY UNCHANGED, FLAGGED, and the flag is load-bearing**
CONVENTIONS "Ancient Grounds …" states in terms: *"THE REAL BINDING CONSTRAINT ON RAISING `AncientGroundMaxAbsX` IS `SpawnBoxHalfExtent` … Anyone tuning the band upward measures against **22540**."* Row 1 just moved that edge **22,540 → 17,620**. A ground at the 21,000 ceiling now sits **3,380 uu INSIDE a team's own spawn box** (4,220 at its footprint edge) instead of 1,540 in front of it ⇒ ***"you fight over it, you do not spawn on it" is INVERTED.*** Symmetric for both teams (the 180° law), so it is not a fairness break — it **is** a design change.
⛔ **Not retuned here, for `WR-§2` row 7's reason exactly:** 21,000 is a value CONVENTIONS records as **law under a different feature section** ("defaults are the law"), and silently retuning objective placement inside a castle-scaling ledger is the very edit `SC-§34` exists to prevent. **The ruling is one number if it goes the other way: 16,080** (= 17,620 − the same 1,540 margin the original derivation used). ⚠️ **Either way the CONVENTIONS clause quoting 22,540 is now FALSE and needs a manager amendment** — it is the clause a future tuner would trust. Full note left in the header.

### S7 — `ASiegeBotController::BotCastleSpawnOffset = 1750` — **(ii) DELIBERATELY UNCHANGED, FLAGGED FOR A RULING**
1,750 sat **1,343 uu in front of the wall** when Jonathan chose it (M7.6 ruling #1, castle half-depth ~407), **531 uu in front** after CASTLE-3X (never re-derived), and at 9× it is **1,907 uu INSIDE the footprint** ⇒ *"castle-FRONT materialize, then march the field"* becomes *"materialize in the hall and funnel out through one gate."*
⛔ **Not changed, and the reason is that TWO RECORDED RULINGS POINT OPPOSITE WAYS** — M7.6 ruling #1 makes castle-front march the intent; CASTLE-3X ruling 3 already accepted bot anchors resolving *inside* the hollow castle as "fine". **Adjudicating between two Jonathan-era rulings is the manager's job, not a ledger task's.** Candidates: **5250** (×3 — preserves the geometry exactly: 5250 − 3656.85 = 3 × 531.05), 4188 (hold today's face clearance), 5000 (restore M7.6's). ⚠️ ruling #1's band *"~1,500–2,000"* is **castle-relative** and cannot be read as an absolute now that its castle is 9× the volume.

### S8 — the pairing law's cross-notes in `SiegePlayerController.cpp` — **(iii) → TASK-563**
`SiegePlayerController.cpp:1858` (*"half-extent SpawnBoxHalfExtent (2460: covers the …"*) and `:4068` (*"2460 since TASK-349 — the box covers the 3× castle's walkable interior"*) still quote **2460**. My `names:` block owns `SiegePlayerController.**h**` only. **TASK-563 owns `SiegePlayerController.{h,cpp}` and is blocked on me — it should fix both while it is in the file.** All three *header* cross-notes (the ones the pairing law actually turns on) **are** updated.

### S9 — `ASiegeGameMode::HeroSpawnCastleOffset (1500)` + `HeroSpawnCastleClearance (300)` — **(ii) NO CHANGE, and this row is the good news**
⭐ **This is `SC-§34`'s structural escape working exactly as designed, and it is the model the other findings should be repaired against.** `DerivedSpawnDistance = CastleBoxExtent.X + HeroSpawnCastleClearance` measures the castle at runtime, so at 9× it resolves ≈3,957 automatically; the authored 1,500 is only a floor and correctly loses. **300 is body-scale** (hero capsule r≈42) ⇒ unchanged. **Only the doc blocks' worked examples rot** (*"with the live 3× castle (half-extent 1,219) this derives 1,519"*). **(iii)** for the comment refresh — `SiegeGameMode.{h,cpp}`, no owner.

### S10 — `ASummonedUnit::StructureGoalProjectionExtent (800,800,600)` — **(ii)**
Its own doc says *"Deliberately NOT castle-half-diagonal-sized"*; it covers the gap between a wall face and the nearest team-allowed navmesh poly — a function of **agent-radius erosion (~34 uu) and hull margin**, not of castle size. Bodies did not grow ⇒ unchanged. Z 600 still finds the new interior floor (z≈174) from a wall-height collision point. Only the "2437×2461" citation rots (S5).

### S11 — verified (ii), no edit, recorded so silence is not mistaken for an omission
`ACastle::InteriorAnchorRelativeLocation` = `ZeroVector` — **a zero vector is scale-invariant**; the castle's centre is its centre at any scale, and it is only ever a navmesh *destination* (the mover projects onto the floor whatever its height). ⚠️ Its old comment claimed the interior floor is *"FLAT AT GROUND LEVEL with a ≤40 uu threshold step"* — the floor is z≈58 today and z≈174 at 9×, reached by a re-derived stair/ramp; **the ≤40 STEP LIMIT survives, the single step does not.** Comment corrected. · `SpawnBoxAnchorInset = 40` (ring-search margin, algorithm-scale) · `PlayerStartKeepClearRadius = 800` (subsumed by the 4500 castle disc at that position) · `CastleRedFallbackLocation (25000,0,0)` and the ±25000 castle anchors (**arena** coordinates — the arena did not grow) · `AGoldNode` / mine spacing constants (arena-relative, none castle-derived).

---

## 4. TWO CONSEQUENCES OF THE 7380 BOX, BOTH RECORDED IN-HEADER SO THEY DO NOT READ AS DEFECTS

1. The box now reaches **|X| = 32,380** against a ±26,000 `ArenaHalfExtent.X`. **Harmless** — `IsPointInSpawnBox` is only the *first* gate; nav projection, collision and the existing clearances all still run, and there is no navmesh past the arena, so the overhang can never yield a placement.
2. It moves the spawn-box edge to **|X| = 17,620** — which is S6.

---

## 5. WHAT QA (TASK-565) SHOULD SCRUTINISE HARDEST

1. ⭐ **§1 — is `GateBlockerExtent.X = 900` right, or is 780 right?** This is the one place I overrode the spec. Re-run the capsule-diameter arithmetic yourself: hero r≈42 / Cavalry r45 vs a 120-uu jamb gap. **If I am wrong, the castle has an invisible enemy-sized side door.**
2. **The three-way `SpawnBoxHalfExtent` check** — `ACastle` ≡ `ASiegePlayerController` ≡ `ASiegeBotController`, all at `(7380,7380)`. Two of three is the silent bug the pairing law exists for. (Then note the two `.cpp` cross-notes in S8 that are **deliberately** still at 2460 and named to TASK-563.)
3. ⛔ **S1 and S2 are feature-killing and neither is in `WR-§2`.** `SC-§34` leg 2 asks you to re-run the enumeration independently — **please pressure-test these two hardest**, and check whether I missed a third of the same shape.
4. **Row 5 was a diagnosis, not a guess** — verify the HP-bar Z really is C++-authored (`Castle.cpp` ctor) and that **TASK-569 therefore owes nothing** on it.
5. **Row 4 is half-done by design** — confirm the handoff says loudly enough that the DataAsset overrides the header and TASK-569 must land the other half, or the change is inert.
6. **My three (ii)-FLAGGED rows (S6, S7, and the header half of S3)** — is "report, don't change" the right posture, or should any of them have been re-derived here? I judged each to be a *design* question (or, for S3, structurally unfixable from a header), not a mechanism question. **Rule on it.**
7. **Counts** — my sweep counts are in §3 with the exact commands; re-run and compare.

---

## 6. NOT DONE, BY DESIGN

⛔ No compile · no editor/MCP/PIE · no Git · no `Content/` · no `.csv` · no `Build.cs` · no `Tests/` · no `L_Arena` · **no new trailing defaulted parameter** (§0) · **no token figure quoted** · **no navmesh claim** (`WR-§3` — the interior's navigability is the integration task's *measurement*, and I made no prediction about it).
📌 **M8:** this task adds **no replicated property, no new replicated class, no new relevancy tier, and no RPC.** All eight changed lines are `EditDefaultsOnly`/`EditAnywhere` defaults or file-local constants — design-time data, identical on both machines by construction. `SiegeNet::ArenaRelevancyDistance` is verified untouched (row 13).
