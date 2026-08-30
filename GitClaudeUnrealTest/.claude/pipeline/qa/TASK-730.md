# QA Report — TASK-730
Verdict: **PASS** — 0 BLOCKERS · 4 WARN · 6 NIT

`SC-§27` diff-scoped review over the batch's code + data as ONE artifact: **TASK-722 · 723 · 724 · 726 · 729 (+ TASK-727's tooling code)**.
Read first, and honoured: the board's four-part READ-THIS-FIRST clause — (i) `TOWER-§4`'s watchdog claim is STRUCK and `AClimbableTower` **declining to cite it is the PASS condition** (verified: the class cites the correction at `ClimbableTower.h:65-70` and claims no coverage); (ii) geometry judged against **`TOWER-§2a`** (30° / 2,078 / 300 / ≥200 / solid wedge / flush), ⛔ never the struck 40°; (iii) `bCanWalkOffLedges` = engine-default `true` is `TOWER-§4b`'s ruling, not an oversight — ⛔ not raised; (iv) the two colour spaces in `WarMapWidget.cpp` are `WM-§8e` and deliberate — ⛔ not flagged.

⛔ No edits · ⛔ no compile · ⛔ no engine/MCP · ⛔ no Git · ⛔ no board write. ⛔ `Tools/Packaging/` and TASK-716/700 untouched.

---

## 1. ⭐⭐ THE PINNED NUMBER — the batch's highest-value check

| Check | Result |
|---|---|
| The literal, read character by character at `SummonedUnit.h:923` | ✅ **`float HeightBonusStepUU = 152.4f;`** — ⛔ not 150, ⛔ not 152, ⛔ not 5, ⛔ not 500 |
| Comment carries the `5 × 30.48` arithmetic | ✅ `SummonedUnit.h:897-921` writes it out in full, plus the whole trap list and *why* it is not round |
| `HeightBonusPerStep` | ✅ `0.10f` at `:936`, `EditDefaultsOnly`, beside it |
| ⭐ **Do the TESTS DERIVE it rather than restate it?** | ✅ **`SiegeHighGroundTest.cpp:86-88`: `constexpr float StepUU = FeetPerStep * CentimetresPerFoot;`** — re-derived from the international foot and the `5` in Jonathan's sentence, with `:83-84` stating explicitly that `152.4f` was NOT written because *"an expectation transcribed from the subject cannot disagree with a wrong subject."* Test 8(a) then checks the **CDO value by reflection** against that derivation ⇒ it disagrees with a wrong header instead of agreeing with it. **This is exactly right.** |
| ⭐ **Exactly one copy codebase-wide?** | ⚠️ **NO — there are two code literals.** `SummonedUnit.h:923` (the shipped one) **and `Tests/SiegeClimbableTowerTest.cpp:421`** (TASK-726's precondition). See **W-1**. Everything else that matches `152.4` in `Source/` is prose: `SummonedUnit.h:904`/`:906`, `ClimbableTower.h:137`, and 8 comment/message occurrences in `SiegeHighGroundTest.cpp`. |

⇒ **The shipped number is correct and its derivation is tested. The "one literal" law is broken once, on the test side, loudly-failing, and it is a WARN, not a BLOCKER** (reasoning in W-1).

## 2. `HIGH-§` semantics, read at source

`SummonedUnit.cpp:3213-3238` / `.h:644`:

- **`R-1` height above the TARGET, positive-only** — `FMath::Max(0.f, AttackerZ - TargetZ)`; level ground and shooting UP both return **exactly 1.0**; ⛔ no malus branch exists. ✅
- **`R-4` continuous** — ⛔ no `FloorToFloat`, no rungs. ✅
- **`R-3` uncapped** — ⛔ no `FMath::Min`, no ceiling. ✅
- **`R-5`/`R-6` excluded** — `AHeroCharacter` is not an `ASummonedUnit` (asserted structurally, test 9(a)); melee skips the branch. ✅
- ⛔⛔ **No tower special case** — `ClimbableTower.cpp:10-16` states the include contract and the file honours it; `grep` finds ⛔ no `Tower.h`/`ClimbableTower.h` include, ⛔ no `bIsOnATower`, ⛔ no occupancy lookup anywhere in `SummonedUnit.{h,cpp}`. A hill and a tower at equal ΔZ are the same call. ✅
- ⛔⛔ **It does not read the war map's clamped bake** — ⛔ no `#include "WarMapWidget.h"`, ⛔ no call to `HeightToBrightness`, ⛔ no read of `ElevationReliefCeiling` from any gameplay file. The three `HeightToBrightness` occurrences the programmer pre-flagged are **comments citing the `WM-§2` precedent** (`SummonedUnit.h:604`, `.cpp:3215`, test `:49`) — zero includes, zero symbol references. ✅
- **Firewall, reverse direction** — `WarMapWidget.{h,cpp}` reference ⛔ no `HIGH-§` symbol. The only non-widget include of `WarMapWidget.h` is `SiegePlayerController.cpp:47`, which is the widget's **owner** (`CreateAndAddToViewport`/`OpenMap`) and reads nothing from the bake. **Checked and clear, both ways.** ✅
- **ONE compose point** — `SummonedUnit.cpp:3313-3320`, immediately after `GetPermanentDamageMultiplier()`. `grep HeightAdvantageMultiplier Source/` returns **three** non-test lines: decl `.h:644`, defn `.cpp:3213`, **one** call `.cpp:3315`. ⛔ No second application site. ✅
- **Gated on the shipped `bRangedAttack`** (`SummonedUnit.h:1644`, bound from `Row->bRanged` at `.cpp:1197`) — ⛔ not a CardID list, ⛔ not a name check. ✅
- **Melee output bit-for-bit unchanged** — the branch is skipped entirely for `bRangedAttack == false`, so `ComputeOutputDamage` returns `AttackDamage` untouched for a non-keyword, un-auraed melee unit. The contract sentence at `.cpp:3242-3251` and the header doc `.h:1539-1550` were **corrected in the same edit** to insert the word MELEE — the M7.7 prose-drift lesson applied correctly. ✅
- Zero-divide guard written `!(StepUU > 0.f)` so a NaN step lands in the guard — ✅ deliberate and correct.
- **M8**: no replicated property, no RPC, no class-tier change. ✅ Airlock clean (⛔ no `Capture()`/`EnsureSnapshot()`, ⛔ no token figure).

## 3. `WarMapWidget` (TASK-722)

- **Values typed as DISPLAY/sRGB** — `ElevationGrassDark (0.0626, 0.1000, 0.0624)` / `ElevationGrassLight (0.6257, 1.0000, 0.6243)` at `:269-270`, byte-identical to `WM-§8e`'s display column. ⛔ The DO-NOT-TYPE linear column appears in the repo **only** inside the comment that forbids it (`:241`). ✅
- **The lerp stayed in DISPLAY space** — `BrightnessToRampColor` (`:928-967`) lerps per channel on the encoded floats then `RoundToInt(x*255)`. ⛔ No `ToFColor(true)`, ⛔ no `sRGBToLinear`, ⛔ no `Pow(x,2.2)` on the path. Midpoint holds at display 0.55. ✅
- **`AncientGroundIconTint = (0.04, 0.22, 0.09)` is TRUE LINEAR** (`:203`) because Slate encodes at draw. ✅
- **Both sites name their space** (`WM-§8e`: a colour constant without its space is not shippable) — the POI block at `:166-178` and the ramp block at `:234-247`, plus inline `// TRUE LINEAR (Slate tint)` on both tints and the byte figures on the ramp. ✅
- **`HeightToBrightness` byte-untouched** (`:915-926`): same `public` plain static, **exactly three parameters, none defaulted** (`SC-§33`), same `Max(Ceiling,1)` floor, same `Clamp(...,0,1)`. The colour mapping did ⛔ NOT leak into it. ✅
- **Channel order** — `return FColor(ToByte(R), ToByte(G), ToByte(B), 255);` is constructor order R,G,B,A ✅; the `Memcpy` into `PF_B8G8R8A8` at `:1208-1217` is unchanged and its BGRA comment is intact. **R and B did not swap.** ✅
- **`ElevationFloorLuma` fully retired** — one hit in the whole of `Source/`, inside the comment documenting the retirement (`:229`). ✅
- ⛔ **Nothing exported to gameplay** — `BrightnessToRampColor` has one caller (the bake, `:1199`) and its declaration doc names the 1,000 uu clamp and `SHIP-§9` at the signature. ✅

## 4. `cards.csv` (TASK-723) — counted, not trusted

Read the whole file. Header = **31 fields / 30 commas**.

| Claim | Measured |
|---|---|
| Exactly 3 `Range` cells changed | ✅ `Archer` **2100** (line 3, field 8) · `Wizard` **2100** (line 30) · `Longbowman` **3600** (line 12) |
| Nothing else on those rows | ✅ Archer 12/10/1.2/350 · Wizard 24/15/1.6/350 · Longbowman 18/18/1.5/300 all intact; the edit landed on field 8, with Damage in 7 and Cadence in 9 |
| ⚠️ `Wizard.AoERadius` | ✅ **250**, field 19, unchanged |
| Ranged **towers** untouched (`R-2`) | ✅ ArrowTower **900** · BombTower **800** · BallistaTower **1400** · CrystalTower **800** |
| `Cleric` heal radius | ✅ **400**, unchanged |
| `WatchTower` row arity | ✅ **30 commas / 31 fields**, byte-identical in arity to the header and to all 30 prior rows; ⛔ **no comma inside `Notes`** (semicolons + parentheses only) |
| `bRanged` | ✅ **false** (field 14) |
| `DeckCount` | ✅ **0** (field 13) |
| ⭐ `sum(DeckCount)` | ✅ **exactly 50** — hand-summed every row: 9+8+3+3+3+4+3+3+0+3+2+2+2+0+0+0+0+0+0+0+0+0+2+1+0+0+0+0+0+2+0 = **50** |
| `Cost` / `HP` | ✅ **30** / **250** (Barracks parity, `TOWER-§3`) |
| Shape vs `Barracks` | ✅ identical except identity, `Notes`, `CardArt`, and the zeroed spawner triple |

**Accepted, declared:** `DisplayName = "Watch Tower"` (from `TOWER-§3` verbatim, matching `Arrow Tower`/`Bomb Tower`); `Notes` wording is the author's, ASCII-only, comma-free.
⛔ **This edit is INERT until `/Game/Data/DT_Cards` is reimported** — TASK-731's, and it is a named acceptance line, not an assumption.

## 5. `AClimbableTower` (TASK-726)

- ⛔ **No fire path at all** — ⛔ no `OnStatsLoaded` override (verified in both files), ⛔ no timer, ⛔ no target acquisition, ⛔ no projectile, ⛔ no tick. Belt-and-braces over the row's `Cadence 0`. ✅
- **Sibling, not subclass** — `AClimbableTower : public ABuilding`; `ABuilding` is its immediate super; `Tower.{h,cpp}` are not included by either shipped file and are not touched by anything in this batch. ✅
- **Team gating derives block + predicate from ONE source** — `AscentBlockedChannel()` → `SiegeEnemyTeamObjectChannel()` (`SiegeNavAreas.h:63`); `ConfigureAscentGate`'s single `ECR_Block` (`:120`) and `CanTeamAscend` (`:72`) both route through it. Mechanism = `ACastle::GateBlockerVolume` verbatim (object type = own channel, Ignore-all base, one Block, `QueryAndPhysics` last) — ⛔ no new channel, ⛔ no new area class, ⛔ no bespoke filter. `ETeamId` is `{Blue, Red}` only, so the predicate is total. ✅
- ⛔ **No capacity term, ⛔ no ranged filter, ⛔ no occupant bookkeeping** — asserted structurally by test 4's token walk over the class's own 6 declared members. ✅
- ⛔ **ZERO `HIGH-§` coupling in both directions** — no `SummonedUnit.h` include in the shipped files (the *test* file's include is test-only and declared); `ASummonedUnit` declares no member containing `Tower`/`Climb`/`Platform`/`Occupan`/`Ascen` (checked at source — every hit in `SummonedUnit.h` is comment prose). ✅
- **Gate never affects navigation** — `SetCanEverAffectNavigation(false)` at `.cpp:40`, so the shell can ⛔ never strand a unit; a blocked enemy is blocked **on** the navmesh. ✅ Degenerate tuning leaves the gate **inert with a warning** rather than arming a nonsense volume (`:95-101`) — correct fail-open.
- **The 124 uu clearance arithmetic checks out**: `SiegeSpawnConstants.h:9` = 88 half-height ⇒ capsule top 176 ⇒ 300 − 176 = **124**. ✅
- **M8** clean: no replicated property, no RPC, no class-tier change; `TObjectPtr` UPROPERTY on the box (GC-safe); `SetupAttachment(VisualMesh)` is legal (the base ctor has already created it).

### ⚖️ Ruling on TASK-726's two `SC-§15` deviations — **BOTH ACCEPTED**

**D-1 — the ELEVATION SHELL instead of a doorway box: ACCEPTED.** The *mechanism* is the shipped, proven-in-play one, unmodified; only the *shape* differs, and the reason is checkable rather than aesthetic: `SM_WatchTower` did not exist when the class was authored, so a doorway box would have been a guess about mesh-local space. It fails **open** (a missed rule), ⛔ never into a stuck unit, and it cannot strand anyone because it is navigation-irrelevant. ⚠️ It ships with a measured correction — see **W-3**, which also **resolves** 726's declared unverified input in the class's favour.

**D-2 — REFUSING the castle's `UNavModifier`/`UNavFilter_Team*` lane: ACCEPTED.** Argument (b) is structural and decisive: the castle needs the nav lane because the enemy castle **is a path goal**, so paths genuinely route inside it; this platform is a **dead end** whose only nav connection is back down the ramp, so no route through it is ever shorter and Recast never chooses it. Argument (c) is equally sound: a team-excluded hole punched around a **runtime-placed** building would manufacture the exact `NAV-§` stranding this design avoids. Refusing to add a mechanism is the conservative choice and the navmesh stays exactly as `ABuilding` already ships it. ⚠️ Argument (a) is **overstated** — see **N-6** — but the refusal does not rest on it.

## 6. TASK-729 — comment-only

- ⛔ **Zero behaviour, zero player-facing string changed.** The sentence at `SiegeControlsHelpWidget.cpp:966-968` matches the board's quoted text verbatim; `Shift+Z is accepted` clause intact; ⛔ `Escape` not touched; ⛔ no key handler added, removed or altered. ✅ Suite delta **0**.
- ⭐ **The line cites RESOLVE — all of them, re-run against the working tree after this batch's edits:**

| Cite | Resolves to | Verdict |
|---|---|---|
| `:927-930` | the F-1 reversal paragraph, 4 lines, ends "⛔ not a help edit." | ✅ |
| `:934-936` | the carve-out boundary sentence, exactly | ✅ |
| `:948-965` | the DO-NOT-TOKENISE block, first line → last line | ✅ |
| `:966` (implied "immediately above the sentence it guards") | `TEXT("Modified presses are not the accept key — …")` | ✅ block ends 965, sentence starts 966 |
| `SiegeKeyboardLayoutSubsystem.h:256-260` | the `KBD-§8`/`KBD-§0` ruling-1 pin | ✅ |
| `GenericCommands.cpp:19` | `UI_COMMAND(Undo, …, FInputChord(EModifierKey::Control, EKeys::Z))` | ✅ **read at the installed 5.8 engine** |
| `SlateEditableTextLayout.cpp:154-157` | `MapAction(FGenericCommands::Get().Undo, …)` | ✅ **read at the engine** |
| `SlateEditableTextLayout.cpp:1170-1172` | the `Ctrl+Shift+Z` redo branch | ✅ **read at the engine** |

⭐ The engine gives the claim a **second** independent support the handoff did not claim: `GenericCommands.cpp:20` binds `Redo` to `FInputChord(Control|Shift, EKeys::Z)` as well. The help text tells Jonathan the truth, and the carve-out (the two `Ctrl` mentions only; the trailing `Shift+Z` IS the accept key) is correct.

## 7. TASK-727's TOOLING CODE — `Tools/**/*.py` verdict

**`Tools/ArtPipeline/build_watchtower.py` — PASS.**

| Checklist item | Result |
|---|---|
| **Secret handling** | ✅ **Zero.** No `HF_TOKEN`, no `os.environ`/`getenv`, no token of any kind — grep-clean, case-insensitive. |
| **Network timeouts** | ✅ **N/A, structurally** — no `requests`, no `urllib`, no `http`, no `subprocess`. Imports are `hashlib/json/math/sys/pathlib/bmesh/bpy/numpy/mathutils`. Zero TRELLIS/Meshy spend, as declared. |
| **Write confinement** | ✅ Every write is under a declared output dir, derived from `Path(__file__).resolve().parents[2]`: `Content/RawAssets/WatchTower.fbx` · `Content/RawAssets/Textures/WatchTower/` · `Tools/ArtPipeline/Cache/WatchTower/{report,previews,debug}`. ⛔ No argv, ⛔ no user input, ⛔ no path that can escape, ⛔ nothing written into another chain's territory. |
| **Headless-bpy** | ✅ `bpy.ops` use is the **shipped, proven idiom** — `select_only()` sets `view_layer.objects.active` before every operator, and `cube_project`/`pack_islands`/`object.bake`/`render.render(write_still=True)` are the same calls `build_entry_dressing_props.py` already ships headless. ⛔ No window/screen/area assumptions. `export_fbx` applies the UE handedness pre-comp and **reverts it in a `finally`**. |
| **Idempotent re-runs** | ✅ `wipe_scene()` first, `RandomState(727)` seeded, all outputs overwritten, `mkdir(parents=True, exist_ok=True)` before every write. |
| **Are the numbers measured?** | ✅ `measure_nav()` raycasts a BVH of the **exported hull soup** and of the render mesh — slope, junction lip, per-sample deviation and up-clearance are all computed, not asserted (one exception at N-2). |

### ⭐⭐ THE TRAP — verified at source, present AND absent cases

**Present case (WatchTower): CLOSED, and I traced it through the shipped functions rather than trusting the claim.**
`pipeline_manifest.json:728-746` gives `"category": "building"` with `"team_region"` a **non-null object** ⇒ `_category_of()` (`reimport_meshes.py:180-186`) returns `"building"`, ⛔ not `"goldnode"`, ⛔ not `"unit"` ⇒ the dispatch at `:477-481` takes the else branch ⇒ `_apply_box_collision(sm, [], result)` hits `if not boxes:` at **`:305-307`**, appends a WARN and **returns before the `set_convex_decomposition_collisions` bootstrap at `:313`** ⇒ the imported UCX collision is **left intact**. ⛔ `remove_collisions()` is never reached. The JSON is well-formed and the entry is purely additive. ✅

**Absent case: the generic defect is STILL OPEN — see W-4.** `_category_of()` continues to default *any* unmanifested CardID to `"unit"`, whose branch (`:287-288`) does `remove_collisions()` + `set_convex_decomposition_collisions()`. The fix protects `WatchTower`; it does not protect the next hand-collided asset.

**Judged against `TOWER-§2a` (⛔ never the struck 40°):** the script's own constants are the law's numbers — `SLOPE_DEG = 30.0`, `RUN = 1200/tan30 = 2078.4610`, `DECK_W = 300.0` (commented "NOT 200"), platform clear `780 × 780` ≥ 600, solid wedge, flush junction. ⛔ Zero deviations from the live budget.

---

## Findings

- **[WARN] `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeClimbableTowerTest.cpp:421` — a SECOND code literal of `152.4f`.**
  `TestEqual(TEXT("PRECONDITION: the shipped elevation step is 152.4 uu (5 ft × 30.48 cm)"), StepUU, 152.4f, Tolerance);`
  This contradicts `HIGH-§1` ("⛔ no second literal of it exists anywhere in the codebase") and makes TASK-724's pasted-grep claim ("EXACTLY ONE `152.4` LITERAL EXISTS IN THE CODEBASE") false **as of the merged batch** — 726 landed the second one in parallel, after 724 measured.
  **Why it is a WARN and not a BLOCKER:** it is not a tuning value, it is an *expectation* asserted against the CDO, so it **disagrees with a wrong header** (it fails loudly if the shipped value moves) — it is not the fake-instrument class. It cannot reach gameplay. **The real cost:** `HeightBonusStepUU` is `EditDefaultsOnly` precisely so Jonathan can retune it, and a retune now breaks a magic number buried in a *tower* test, which is exactly the maintenance hazard the one-literal law exists to prevent.
  **Suggested fix (one line):** `constexpr float ExpectedStepUU = 5.f * 30.48f;` and assert against that — matching 724's derivation discipline — or drop the precondition and lean on 724's test 8, which already owns the regression claim.

- **[WARN] `Tests/SiegeClimbableTowerTest.cpp:453-454` — the parity assertion does not prove parity.**
  `TowerMultiplier = H(TargetZ + PlatformHeightUU, TargetZ)` and `HillMultiplier = H(TargetZ + 1200.f, TargetZ)` collapse to the **identical call** `H(1200, 0)` once `PlatformHeightUU == 1200`, so `TestEqual(TowerMultiplier, HillMultiplier)` can only fail in the same case the height equality two lines above (`:451-452`) already catches. It is a redundant restatement of "PlatformHeightUU is still 1,200", not a proof that a hill and a tower agree. (It is **not** unfailable-forever, which is why this is a WARN: it does fail on a retune.)
  **The property IS genuinely covered in this batch** — `SiegeHighGroundTest.cpp` test 7 does it correctly, with pairs at **different absolute Z** (a: 1200/0 vs 2100/900; b: 2200/0 vs 1700/−500), translation invariance over six offsets including ±25,000, and a same-platform ⇒ exactly 1.0 clause. That test refutes the absolute-Z reading outright.
  **Suggested fix (one line):** site the hill at a different absolute Z, e.g. `H(BasinZ + 1200, BasinZ)` with `BasinZ = -500.f`. The handoff's claim that this pair "avoids the X == X trap" holds for the *height* comparison but is over-stated for the *multiplier* one.

- **[WARN] `Source/GitClaudeUnrealTest/Siegebound/ClimbableTower.cpp:106-107` + `ClimbableTower.h:232` — the ascent shell is centred on a pivot the footprint is not centred on.**
  `ConfigureAscentGate` pins the box to `SetRelativeLocation(0, 0, GateCentreZ)` with `AscentGateHalfExtentXY = (1500, 500)` ⇒ it covers **X ∈ [−1500, +1500]**. The authored mesh's footprint is **asymmetric about that pivot: X ∈ [−600, +2458.461]** (`build_watchtower.py:96-107`, `RAMP_X1 = 2458.461`; `pipeline_manifest.json:732` `target_dims_ue = [3058.461, 940, 1420]`).
  ⇒ the shell **misses the outer ≈958 uu of ramp run** and over-hangs ≈900 uu of open ground to the west. **Measured consequence:** an enemy entering at the toe is stopped at X = 1500, i.e. after **≈958 uu of run and ≈553 uu of climb**, not the "≈520 uu of run" the header (`:201-203`) and handoff state. **T-3 still holds** — the platform (X ∈ [−480, 300], Z 1200) is entirely inside the shell, so no enemy reaches it — and it still fails **open**, never into a stuck unit.
  ⭐ **This also RESOLVES TASK-726's one declared unverified input, in the class's favour: the authored ramp DOES run along local +X ⇒ ⛔ do NOT swap the two numbers.**
  **Suggested fix, for TASK-728/731 (⛔ not for this diff):** either (a) accept the later block point and state it, or (b) raise `AscentGateHalfExtentXY.X` — but note that full coverage without the western overhang needs a **centre offset the class does not expose** (the relative location is hardcoded to `(0,0,Z)`). A one-property `AscentGateCentreOffsetX` is the sanctioned shape and is a follow-on, out of this batch's fence.

- **[WARN] `Tools/reimport_meshes.py:180-186` — the manifest fix closes the present case; the DEFAULT is still the trap.**
  Verified closed for `WatchTower` (trace in §7). But `_category_of()` still returns `"unit"` for **any CardID with no manifest entry**, and `_apply_unit_hulls` (`:287-288`) calls `remove_collisions()` then `set_convex_decomposition_collisions()` — so the *next* hand-collided asset that ships without a manifest entry gets all its authored hulls deleted and a solver's opinion substituted, behind a green run and a `DONE` log. The fix is per-asset; the hazard is systemic.
  **Suggested fix (⛔ out of this batch — `reimport_meshes.py` is not in this diff; board it):** make the unknown-CardID default **non-destructive** (fall through to "leave imported collision") or hard-STOP with a named error, so an unmanifested asset can never silently lose authored collision.

- **[NIT] `Tools/ArtPipeline/pipeline_manifest.json:733` — stale prose bound.** `_dims_source` says `Bounds X[-560, 2458.461]`, but the westmost geometry is the buttress at **X = −600** (`build_watchtower.py:107`), which is what makes `target_dims_ue` `3058.461`. The numeric field is right; the prose is 40 uu stale. No consumer reads the prose; one-word fix whenever the file is next touched.

- **[NIT] `Tools/ArtPipeline/build_watchtower.py:1016` — `"solid_wedge": True` is hand-typed inside a report block whose every other field is measured.** Harmless (the wedge is structural in `wedge_faces()`), but a literal `True` in a measurement report is the `SHIP-§9` shape in miniature. Prefer deriving it (a downward ray under the ramp) or renaming it `solid_wedge_by_construction`.

- **[NIT] `Tools/ArtPipeline/build_watchtower.py:944-959` — missing `None` guard.** `top_hit()` returns `None` on a miss and the deck loops handle that (`:917`, `:931`), but the junction-seam and slope blocks use the result in arithmetic directly. A miss raises a bare `TypeError` instead of a named failure. Non-blocking (the script exits 1 on any exception, and both surfaces exist by construction); a two-line guard would name the failure instead of stack-tracing it.

- **[NIT] `SummonedUnit.h:922`/`:935` — both tunables ship `BlueprintReadOnly` as well as `EditDefaultsOnly`.** That is Blueprint surface, which sits slightly at odds with the handoff's §7.1 rationale for reading them reflectively ("adds ZERO shipped surface"). It is read-only, it matches the neighbouring keyword tunables (`:894`), and it is harmless. Recorded for the record only — ⛔ no change asked.

- **[NIT] TASK-722 deviation 4 — RULED: leave the three stale `"full white"` doc phrases** (`WarMapWidget.cpp:851`, `.h:572`, `.h:712`). Editing them would put diff lines **inside the byte-untouched function's own documentation**, which is precisely the guarantee this gate exists to check, and `WM-§8a` item (1) outranks a wording repair. The drift is already self-documented at `SiegeWarMapTest.cpp:2265-2269`. An optional 4-word repair should ride a **future** war-map task, ⛔ not this batch. Silence would have left them ambiguous; this is the explicit ruling the handoff asked for.

- **[NIT] TASK-726's nav-lane refusal argument (a) is overstated.** "An enemy melee unit's path would end up to ~1,300 uu from the tower body, against a melee reach of ~150 uu" assumes the reach test measures to the tower *body*. It does not: the attack reach is a **closest-point test against the whole actor's collision** (`Building.h:43-48`), which includes the ramp wedge running out to the footprint edge — so a melee unit stopped at an exclusion boundary would plausibly still be in reach of the ramp toe. **The refusal is still correct and still ACCEPTED** — it stands on (b) the dead-end/no-pile-up asymmetry and (c) the runtime-placed stranding hole, both of which are structural.

## ⚖️ Rulings the batch asked for

| # | Deviation | Ruling |
|---|---|---|
| **R-1** | **722 also touched `WarMapWidget.h` (one seam decl) + `Tests/SiegeWarMapTest.cpp`** | ✅ **ACCEPTED — the orchestrator's provisional acceptance is CONFIRMED.** Board item (5) *mandates* extending that test file in place and declaring a suite total; the two grass constants are `.cpp`-file-local statics with internal linkage, so **a test physically cannot reach the mapping without a declared seam**, and the only alternative is a test that re-types its subject's literals — which item (5) itself forbids and which is the exact fixture defect that cost a loop the day before. The board's own ownership + `names:` lines grant this task `WarMapWidget.{h,cpp}` **and** that test file, so the "`.cpp` only" phrasing reads as the cross-task fence, ⛔ not a repeal of item (5). The addition is one `public` plain static, ⛔ not a `UFUNCTION`, **exactly one parameter, none defaulted** (`SC-§33`), ⛔ no new include, behaviour identical. ⭐ Decisively: `WM-§8e`'s own shipped detector — *"the ramp's midpoint byte must equal the average of the two end bytes"* — is **only implementable through that seam**, and it is implemented exactly as the law specifies (`SiegeWarMapTest.cpp:2356-2374`, expectation computed from the ends, ±1 byte). **A test that cannot see its subject is the defect; the seam is the cure.** |
| **R-2** | 722's log-string repair `"clamped full white"` → `"clamped to the ramp's light end"` | ✅ **ACCEPTED.** Runtime output a human reads while looking at the map during the very playtest this feature is judged in; naming a colour the map no longer contains would misdirect that eye. Display-only, in-file, one string. |
| **R-3** | 722's three stale `"full white"` doc phrases | ✅ **LEAVE THEM** — see the NIT above. |
| **R-4** | 726's **elevation-shell** gate instead of a doorway box | ✅ **ACCEPTED**, with W-3's measured correction carried to TASK-728/731. |
| **R-5** | 726's **refusal of the castle's nav lane** | ✅ **ACCEPTED** on grounds (b) + (c); ground (a) is overstated (N-6) but the ruling does not rest on it. |
| **R-6** | 724's practice of **writing, then deleting, two unfailable assertions and leaving a comment where each stood** (`SiegeHighGroundTest.cpp:201-204` and `:693-698`) | ⭐ **VERIFIED PRESENT AND RULED CORRECT — adopt it as house practice.** Both comments exist, both name the specific vacuity ("`StepUU` is this file's OWN constant, so a band check on it asserts the compiler's multiplication"; "would have both sides equal by construction and would report SAFE against every possible implementation"), and both say where the real claim went. ⚖️ *A claim that cannot fail is worse than no claim, because it occupies the space where a real one would have gone* — and deleting it silently loses the reasoning. **An assertion removed for being unfailable leaves a comment saying why, at the site.** |

## Can every new test actually fail? — `SHIP-§9c`

**724 — 9 tests: YES, all nine.** Fixture derived, ⛔ never transcribed (`:86-88`). Test 1 fails against a symmetric formula (dropped `max(0,·)`); test 2 fails against a skipped divide-by-step (the feet-as-units arithmetic shape); test 3 fails against a floored implementation at four fractional points **plus** a 120-sample strict-increase sweep at 1/40-step spacing, and its equal-increment pair carries a non-zero clause so a staircase cannot pass it by returning 0 twice; test 4 fails against `pow(1.1,n)` at 2 and 10 steps with a `bonus > 0` guard; test 5 cross-checks `HIGH-§5`'s decimals and fails against **any** finite cap (still climbing at 100,000 uu); test 6 covers zero/negative step and zero bonus **and** carries a clause that stops the whole test being satisfied by `return 1.f;`; test 7 is the real parity/absolute-Z refutation (different absolute Z, translation invariance, self-limiting); test 8 reads both tunables **by reflection** and fails on 150/152/5/500 by name, on a rename (explicit `AddError`, ⛔ never a silent skip), and feeds the shipped defaults through the shipped seam; test 9 fails the day the hero is re-parented or `bRangedAttack`'s default is flipped, with a positive control so the "hero has none" claim cannot pass vacuously.

**726 — 5 tests: YES, and the self-checks fire.** I verified each instrument at source rather than taking the claim: test 1's self-check (`ATower IsChildOf ABuilding`) holds and is load-bearing; test 2's seven probe names are **all real `UPROPERTY`s on `ATower`** (`Tower.h:172, 176, 180, 190, 199, 208, 217`) and **none exists on `ABuilding`**, so the probes are live and the absence claims mean something; test 3's channel-distinctness self-check holds (`ECC_GameTraceChannel1 != 2`) and it pins direction, catching "consistent but backwards"; test 4's reflection walk finds **6** declared members (5 properties + `GetPlatformHeightUU`) ⇒ `>= 4` and both named members resolve, so the banned-token scan is not scanning an empty list — and I checked the 6 names against all 7 banned tokens for false positives (none: `AscentGateFloorUU` ≠ "Fall", `AscentGateHalfExtentXY` ≠ "Fall"); test 5's three self-checks are real, and its (d) token walk over `ASummonedUnit` finds `HeightBonusStepUU` and produces **no** false positive (no `ASummonedUnit` member name contains `Tower`/`Climb`/`Platform`/`Occupan`/`Ascen` — every match in that header is comment prose).
**The one exception is W-2**, and it is a redundancy, not a vacuity.

**722 — 1 test: YES.** (a) fails on a reversed or non-total ramp; (c) fails on a reverted gray ramp and on an R/G or G/B constructor transposition, at 21 sweep points; (d) fails on a plateau; **(e) is `WM-§8e`'s named detector and fails by ~46 bytes on green against a linear-space lerp**, computed from the ends, ⛔ nothing transcribed. (f) is honestly declared weak (one-byte margin) **inside the test itself**, with the real control named as the seam's comment — that is the right way to ship a weak guard.

## Suite total — ⭐ RECONCILED TO ONE NUMBER

**Measured now, in the working tree:** `^IMPLEMENT_*_AUTOMATION_TEST(` at line start across `Siegebound/Tests/` = **171** in 15 files (WarMap 27 · AssistantSelection 28 · StuckStatics 20 · ControlsHelp 13 · AssistantGrammar 12 · DeckSlots 12 · **HighGround 9** · Cloud 9 · AssistantGuard 9 · Account 7 · Settings 7 · KeyboardLayout 7 · **ClimbableTower 5** · AssistantZoneA 5 · CastleTransform 1).

⇒ **`171` is the number TASK-731 asserts against.**

**There is ⛔ NO disagreement between the handoffs — the three figures are consistent snapshots taken at different points in the wave, and they reconcile exactly:**

| Declared | When it was measured | Reconciles as |
|---|---|---|
| 724: **156 → 165** | 724's file alone | 156 + 9 |
| 726: **165 → 170** | after 724, before 722's edit | 156 + 9 + 5 |
| 722: "+1 attributable = 157; **full tree 166**" | after 724's file existed, **before 726's landed** | 156 + 9 + 1 = 166 ✓ |
| 729: **0** | — | — |
| **TOTAL** | **now** | **156 + 9 + 5 + 1 + 0 = 171** ✅ |

⚠️ **The risk this reconciliation removes:** no handoff states `171` anywhere, and TASK-731's own board spec names only "722, 724 and possibly 729" — **726's +5 is missing from that list.** If build-master asserts 166 or 170 it will read a correct suite as wrong. 722's own handoff anticipated this and said "731 must re-count rather than trust this line."

## Notes for build-master (TASK-731) — what the compile must watch

1. ⛔⛔ **`Build.bat` returns exit 0 on a FAILED build — parse the log for `Result: Succeeded`/`Result: Failed`, ⛔ never `$LASTEXITCODE`.** And if the compile dies in ~2 s with `0x800711C7`, that is **Smart App Control**, ⛔ not a code error — do ⛔ not loop QA on it.
2. **Three new translation units** — `ClimbableTower.cpp` (first UHT pass over a new `UCLASS` ⇒ `ClimbableTower.generated.h`), `Tests/SiegeHighGroundTest.cpp`, `Tests/SiegeClimbableTowerTest.cpp`. No `.Build.cs` change is needed (module wildcard), and none was made.
3. ⚠️ **`SiegeClimbableTowerTest.cpp` is compile-time coupled to TASK-724** — it statically calls `ASummonedUnit::HeightAdvantageMultiplier` and reads `HeightBonusStepUU`/`HeightBonusPerStep` by reflection. The two land together in this commit; if 724 is ever re-worked, that file must be re-checked.
4. **Unity-build hazard:** two new test files add file-scope namespaces (`SiegeHighGroundTestFixture`, `SiegeClimbableTowerTestFixture`) — both are named, so no anonymous-namespace collision. Watch for a duplicate-symbol only if a merge renames them.
5. **Suite: assert `171`, and report both declared and actual.** ⛔ A red test is a STOP. If it reads 166 or 170, re-count before calling it a failure — see the reconciliation above.
6. ⭐⛔ **The `DT_Cards` reimport is a named acceptance line, ⛔ not a step to assume.** Read back `Archer.Range == 2100`, `Longbowman.Range == 3600`, `Wizard.Range == 2100` **and `Wizard.AoERadius == 250`**, plus a `WatchTower` row with `Cost == 30`, `HP == 250`, `bRanged == false`, `DeckCount == 0`. Watch for a missing-column notice. *A CSV edit without a verified reimport is a no-op that looks done.*
7. **On the `SM_WatchTower` import (TASK-728's chain):** ⛔ **read back `convex_hull_count == 14` and `box_count == 0`. ZERO hulls is a STOP** — that mesh is not collisionless on purpose, and W-4's trap is exactly what a wrong number there would mean. Import with `auto_generate_collision = False`, `import_materials = False`, slots `[TeamRegion, WatchTowerPBR]`, Nanite OFF.
8. ⭐ **Verify the card path STRING character-for-character:** `/Game/Blueprints/Buildings/BP_Building_WatchTower.BP_Building_WatchTower_C` against what `SiegePlayerController.cpp:3842` builds. A mismatch is a card that silently does nothing.
9. **Carry W-3 to the placement step:** the shell covers X ∈ [−1500, +1500] against a footprint of X ∈ [−600, +2458.461]. The axis assumption is **confirmed correct** (ramp along +X) — ⛔ do not swap the numbers. Decide (and record) whether to raise `AscentGateHalfExtentXY.X` via the `EditAnywhere` property or accept the later block point.
10. **Also expect,** and treat as non-blocking: a `LOD_STEP_FAILED` token on a 464-tri mesh (`TL-§3`), and the `WARN: no manifest boxes for WatchTower; leaving imported collision.` line — **that WARN is the wanted outcome**, not a fault.
11. ⛔ `Tools/Packaging/`, TASK-716/700 and TASK-735 are **out of this commit**. One commit on `main`, explicit pathspecs, ⛔ no push unless Jonathan asks; verify `git` HEAD vs origin first — he sometimes commits and pushes milestones himself.

---

**Reviewed at source, not on a relay:** `SummonedUnit.{h,cpp}` · `Tests/SiegeHighGroundTest.cpp` · `WarMapWidget.{h,cpp}` · `Tests/SiegeWarMapTest.cpp` · `ClimbableTower.{h,cpp}` · `Tests/SiegeClimbableTowerTest.cpp` · `SiegeControlsHelpWidget.cpp` · `Docs/Data/cards.csv` (every row) · `Tools/ArtPipeline/build_watchtower.py` · `Tools/ArtPipeline/pipeline_manifest.json` · `Tools/reimport_meshes.py` · `SiegeNavAreas.h` · `Building.h` · `Tower.h` · `TeamId.h` · `SiegeSpawnConstants.h` · `SiegeKeyboardLayoutSubsystem.h` · and the UE 5.8 engine sources `GenericCommands.cpp` + `SlateEditableTextLayout.cpp`.
