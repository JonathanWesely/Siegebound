# TASK-1122 — GFX-LEVERS — handoff (gameplay-programmer)

**Marker:** `TASK-1122-GFX-LEVERS` · **Date:** 2026-09-07 · **Gate:** `TASK-1123` (qa-reviewer) · **Host:** `TASK-1124`
**Spec source:** the board row **as RESHAPED 2026-09-07** + `handoffs/TASK-1112-programmer.md` §3.1–§3.5 + `handoffs/TASK-1113-programmer.md` §4.
Law: `GFX-§9` (as corrected — the **fog** and **View-Distance** levers are STRUCK and are not re-added) · `GFX-§5` · `GFX-§8` · `GFX-§10` · `GFX-§11` · `FIELD-§2` · `FIELD-§6` · `FIELD-§7` · `SC-§79` · `SC-§83` · `SC-§87` · `SC-§90` · `SC-§91` · `SC-§94` · `SC-§101`.

## 0. FILES WRITTEN — three, nothing else

| File | What |
|---|---|
| `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.h` | the lever's public pure API (2 statics + 1 constant), the cached read, the cull-band record struct, `ApplyFoliageCullBands()` |
| `Source/GitClaudeUnrealTest/Siegebound/BattlefieldScatter.cpp` | the read at the funnel, the mapping, the re-apply pass + its read-back log, and the two named `SetCullDistances` sites |
| `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeScatterCullBandTest.cpp` | **NEW** — 9 automation tests + the `SC-§83` mutation table (M1–M8) |

⛔ **ZERO `Content/**`** (`DA_BattlefieldScatter.uasset` **not opened, not edited** — this is code reading values the asset already holds) · **ZERO `Config/**`** · **ZERO `.uasset`** · **ZERO `L_Arena.umap`** · **ZERO fog files** (`FogVolume.*` / `SiegeFogStatics.*` untouched; `r.VolumetricFog` appears nowhere in my diff) · **ZERO widget files** (`TASK-1118`/`1115`/`1120` own those and at least one is live) · **ZERO `CONVENTIONS.md`** · **NO compile, NO editor, NO MCP, NO git.**

---

## 1. 🚨 THE ONE THING TO READ BEFORE ANYTHING ELSE — THE CALL I MADE ON THE GETTER'S NAME

**I call `GetFoliageQualityScale()`. As of my write, the symbol in the tree is still `GetFoliageDensityScale()` (`SiegeGraphicsSettingsSubsystem.cpp:1085`, `.h:669`).** ⇒ **the tree does not compile until `TASK-1118` cl. (10a)'s rename lands.**

That is deliberate, not an oversight, and the grounds are the board's own words on my row: *"CALL `GetFoliageQualityScale()`, NEVER `GetFoliageDensityScale()`"*, ratified in `GFX-§9`'s NAME RULING (*"`TASK-1122` is written against the NEW name from the start"*). The alternative — writing the old name and relying on `1118` to census a file outside its own stated fence (`SiegeGraphicsSettingsSubsystem.{h,cpp}` + `Tests/SiegeGraphicsSettingsTest.cpp`) — is the higher-risk half of the same coupling, and it is the half that leaves a struck lever's name at a call site.

**I made the coupling cost exactly one word.** `GetFoliageQualityScale` occurs **once** in my whole diff, at `BattlefieldScatter.cpp:406` (a second occurrence at `:401` is inside the comment explaining the name). The tests never touch the subsystem — they exercise pure statics — so the identifier is not in the test file at all.

⇒ **For `TASK-1124` (host):** if `TASK-1118` did **not** land, the tree is fixed by changing `BattlefieldScatter.cpp:406` from `GetFoliageQualityScale()` to `GetFoliageDensityScale()`. If it **did** land, nothing is owed. Either way the tree only has to be consistent at your compile, which is the coupling `GFX-§9` explicitly accepted when it made you both rows' host.

---

## 2. THE READ SITE, AND WHY THERE

**`ASiegeBattlefieldScatter::RunScatterPasses(int32 Seed, bool bAuthoritativeGenerate)` — `BattlefieldScatter.cpp:365`, immediately after `FRandomStream Stream(Seed);`.** Exactly the site `TASK-1112` §3.2 measured and `GFX-§9` pinned; I did not re-choose it and I did not guess.

Why it is the right site, restated from the code rather than relayed:
- It is the **single funnel.** `GenerateScatter:322` (authority) and `OnRep_GenerationIndex:360` (client mirror) are the only two callers, and `ScatterLayer` is only ever reached from here. A read at `ScatterLayer`'s early-out (`:516`) or at the `OuterTarget` computation (`:625`) would re-answer the same question **once per layer**, seven times.
- It runs **before any placement**, so one read caches into a member (`FoliageCullScaleCached`, the shipped `CorridorHalfWidthCached` idiom) instead of a subsystem lookup inside the per-instance rejection loop at `:637`.
- **No tick, no poll, no CVar sink.** Two callers, both once per match (`BeginPlay:202`) ⇒ the panel's *"applies at the next match start"* sentence is literally true and the `GFX-§9` obligation to keep it is kept.

⚠️ **`bAuthoritativeGenerate` is in scope and I deliberately do NOT consult it.** The cull band is a per-client render choice; branching on authority would push the *server's* quality level into the *client's* picture — the inversion this lane exists to avoid. Both machines read their own setting and still place identical instances. Written into the code comment so a later reader does not "fix" it.

**The fail-safe is `1.0f`** — set before the lookup, so a dedicated server, a test, a null `UGameInstance` or a missing subsystem all render the **authored** `DA_BattlefieldScatter` bands.

---

## 3. ⭐ THE METRE MAPPING — RULED HERE, WITH ITS REASONING

### 3.1 Why a bare `Band × Scale` had to be refused, quantitatively

`GetFoliageQualityScale()` returns a **project-invented ladder** — `0.25 / 0.50 / 0.75 / 1.00 / 1.00` (`SiegeGraphicsSettingsSubsystem.cpp:1085-1102`) — authored for the **density** lever `GFX-§9` struck, where `0.25` means *"a quarter of the instances"*.

**Drawn instances under a cull band scale with the AREA the band covers, ≈ end².** So reusing the count scalar as a distance multiplier is wrong **by a square**: `0.25 × band` draws `0.25² = 6.25 %` of the instances — a **16× overshoot** of the trade the ladder was designed to make — and pulls the grass band from **90 m to 22.5 m**.

⇒ **THE RULING: `distance factor = sqrt(quality)`.** It is not "a gentler number I liked"; it is the *unique* factor that reproduces exactly the drawn-instance reduction the ladder was authored to deliver, through the one mechanism that is determinism-safe. `factor² == quality` is asserted as a test.

### 3.2 The band this is applied to — RE-MEASURED, and the `0.8` is NOT inherited (`SC-§91`)

`GFX-§9` strikes `TASK-1084`'s *"~72 m at `r.ViewDistanceScale 0.8`"* as unreproducible from config. I did not use it. The authored bands below are **engine read-backs off the live DataAsset**, recorded by the rows that read them:

| layer | authored `CullStart / CullEnd` | source |
|---|---|---|
| `Trees` | **24,000 / 32,000 uu** = 240 / 320 m | `handoffs/TASK-1083-buildmaster.md` §2 read-back table |
| `Grass` | **6,000 / 9,000 uu** = 60 / 90 m | `handoffs/TASK-1084-buildmaster.md` §0b (*"live DA values, read from the engine"*) |
| `Plants` | **8,000 / 12,000 uu** = 80 / 120 m | same |
| `Hills` | `0 / 0` = **never culled** — ⚠️ *authored intent* per `ScatterConfig.h:261-269` (*"hills take 0 … their silhouette must read across the 10× field"*); I did **not** read the DA to confirm it, and I am not claiming it as measured. Whatever it holds, `End == 0` is passed through unchanged by construction. |
| `Rocks` | **not measured by me** — same treatment; no code path depends on knowing it. |

### 3.3 THE RULED TABLE, IN METRES — all five levels

| level | `q` | factor `√q` | **Trees** | **Grass** | **Plants** | drawn area |
|---|---|---|---|---|---|---|
| **Low (0)** | 0.25 | **0.500** | 120 / **160 m** | 30 / **45 m** | 40 / **60 m** | **25 %** |
| **Medium (1)** | 0.50 | **0.707** | 170 / **226 m** | 42 / **64 m** | 57 / **85 m** | **50 %** |
| **High (2)** | 0.75 | **0.866** | 208 / **277 m** | 52 / **78 m** | 69 / **104 m** | **75 %** |
| **Epic (3)** | 1.00 | **1.000** | 240 / **320 m** | 60 / **90 m** | 80 / **120 m** | **100 %** |
| **Cinematic (4)** | 1.00 | **1.000** | ⛔ **byte-identical to Epic** — `GFX-§9`: never above the authored baseline | | | 100 % |

Plus **a near-field floor of 3,500 uu (35 m)**, and that number is **measured, not picked**: the castle keep-clear disc (`CastleKeepClearRadius` 4,500 uu, DA-serialised) puts the nearest possible scattered tuft **≥ 35 m from the hero's spawn** (`handoffs/TASK-1084-buildmaster.md` §0b item 3). A cull end shorter than that renders the layer **invisible from spawn** — the lever would be switching a layer **off**, which is the *"control that lies"* `GFX-§9` forbids.

⚠️ **HONEST SCOPE ON THE FLOOR: it never binds today.** The shortest applied end across all shipped layers is grass at Low, **45 m**. The floor is a bound on **future** `DA_BattlefieldScatter` edits (the DA is content and moves without a code review), and it is exercised only by a **synthetic** layer in the tests. ⛔ Do not read its presence as evidence it fired (`SC-§36.1` — a guard with no live caller is a surface, and I am declaring it as one rather than letting a green imply otherwise).

The floor is itself **capped by the authored end** (`Min(3500, BaseEnd)`), so the lever is **monotone**: it can shorten a band and can *never* lengthen one. A layer authored tighter than 35 m is exempt, not stretched.

### 3.4 ⚠️ THE COMPOUNDING I AM NOT SILENTLY CORRECTING — flagged, not solved

These are the **authored metres handed to `SetCullDistances`**. The engine's own **ViewDistance** group multiplies `r.ViewDistanceScale` (`0.4` @Low → `1.0` @Epic) on top at draw time, and `GFX-§9` **struck** any compensating factor for it. ⇒ a player at **Foliage=Low + ViewDistance=Low** gets an effective grass end of ≈ `45 m × 0.4 = 18 m`.

My recommendation is **accept**: that is two honestly-labelled Low sliders doing what they say, and compensating would re-open exactly the double-scaling `GFX-§9` struck. But it is a lane-shape decision, not mine to take mid-row — see F-2.

### 3.5 WHAT A PLAYER ACTUALLY SEES

- **At Epic (the default, and what ships today):** ⛔ **nothing changes at all.** Byte-identical bands, structurally — see §4.2.
- **At Low:** the **ground detail** thins; the **shape of the battlefield does not.** Grass begins fading at 30 m and is gone by 45 m (against 60→90 m at Epic), plants at 40→60 m, and the **treeline still reads to 160 m**, so the horizon silhouette survives. The **Hills** layer is `End == 0` and is therefore untouched at every level — the field's large-scale silhouette is *by construction* immune to this slider.
  From the castle spawn specifically: the keep-clear disc already puts the nearest tuft at ~35 m, so a Low player sees a **narrow 35–45 m grass strip** where an Epic player sees a 35–90 m field. That is a visibly scaled setting. The refused mapping would have ended the grass at **22.5 m — inside the disc — i.e. no grass at all from spawn.**

---

## 4. THE DETERMINISM PROOF — the delivery criterion's inverted half

**Claim: placed-instance counts are IDENTICAL at every Foliage level for the same seed. Here is why, structurally.**

### 4.1 Nothing in the diff can reach the layout

1. **The value is read at `:365` and stored in exactly one member**, `FoliageCullScaleCached`. `grep` that member: it is **read at exactly one place**, `ApplyFoliageCullBands()`, and passed as an argument at the two `SetCullDistances` sites. It is never read inside `ScatterLayer`, never near an `FRandomStream` draw, never by anything deciding *where* or *how many*.
2. **Census of the added lines:** `InstanceCount`, `OuterTarget`, `AddInstance`, `RemoveInstance` and `Stream.` appear in my added lines **only inside comments** (5 comment hits, 0 code hits — verified by `git diff -U0 | grep '^+'`). No arithmetic anywhere in the diff touches a count.
3. **The lever is applied AFTER both placement passes complete.** `ApplyFoliageCullBands()` is called after the pass-2 loop; `Stream` is not passed to it and it takes no stream.
4. **`SetCullDistances` is render-side state on an already-populated component.** It changes `InstanceStartCullDistance` / `InstanceEndCullDistance` and nothing else — it does not add, remove, or move an instance, and it does not touch collision, navmesh, or the authority-only `ValidateTraversability():2064` / `CullCorridorBlockers():2464` instance sets. ⇒ the accepted residual at `:339-340` (*"client obstacles are a superset"*) is **unchanged**: a low-spec client still holds every obstacle the authority does; it merely draws fewer of them.
5. **The proxy band is functionally moot and stays so.** The proxy is `SetVisibility(false)`, so it never renders or culls as geometry, and nav/collision are unaffected by a cull distance. Scaling it preserves the existing "uniform pair state" comment's intent and cannot change blocking.

### 4.2 And the no-regression half is STRUCTURAL, not arithmetic

`ComputeFoliageScaledCullBand` returns **before any arithmetic** when the factor is `>= 1.0f`, handing back `Max(Start,0) / Max(End,0)` — the *identical* expression the pre-lever call used. So Epic, Cinematic, the null-subsystem fallback, a dedicated server and a unit test all produce today's battlefield as a property of **control flow**, not of IEEE rounding. `sqrt(1.0f) == 1.0f` exactly, so the guard is reached exactly.

### 4.3 ⛔ WHAT THIS PROOF IS NOT

It is a **reading**, not an observation. Nothing has been compiled, run, or rendered by this row: **no placed-instance count at Low has ever been compared to one at Epic on this machine.** The test `TheLeverConsumesNoRandomStreamDraws` proves the *pure functions* move no stream — the miniature, not the system. The system-level claim rests on (1)–(5) above, which the gate can check by reading and which `TASK-1124` can check by running a match at each level and diffing the existing `Layer '%s': placed %d instances` log lines.

---

## 5. 🚨 A DEFECT I FOUND WHILE READING, AND FIXED — THE PLAY-AGAIN PATH

**`ClearScatter()` (`:471-479`) clears instances but KEEPS the components**, and `ResolveComponentForMesh` **returns early on the reuse path** (`:902-908`) — *above* every render-profile call including `SetCullDistances`.

⇒ applying the band only at the creation site would have shipped a lever that works when a match starts from a level load and **silently does nothing after Play Again** — a lever live on one path and dead on another, which is the `TASK-1109` / `SC-§94` shape exactly, and it would have made the panel's *"applies at the next match start"* sentence **false for half the ways a match starts**.

**Fix:** `ApplyFoliageCullBands()`, a pass over ~59 recorded components run after both placement passes. It re-applies from a **recorded authored band** rather than from whatever `Layer` is in scope, because re-reading `Layer` would hand a mesh **shared by two layers** the *last* layer's band where the one-HISM-per-mesh law gives it the *first* layer's — a drift that would be a regression **at Epic**. The record (`FScatterCullBandRecord`) is a weak-pointer secondary index in the shipped `VisualToProxy` / `HillSurfaceComponents` idiom, appended only on the creation path, so it is bounded by the unique-mesh count for the actor's life, not by re-scatter count.

⚠️ **This is the part of the diff I most want the gate to read**, because it is a call-graph change and both blockers in this lane's history lived in call graphs.

---

## 6. THE LOG — the `GFX-§9` / `SC-§94` engine-log half

`ApplyFoliageCullBands()` emits, on `LogSiegeTerrain` at **`Log`** (default verbosity — no `Verbose` flip needed, and an empty log therefore cannot be misread as a pass):

- **One summary line on EVERY generate, including Epic**, carrying `q`, the factor, the floor, the component count, `changed=`, `neverCulled=`, `readBackOk=`, `readBackMismatch=` and the applied end range in **uu and metres**. At Epic `changed=0` **is** the no-regression measurement; at Low `changed>0` is the lever working. It prints unconditionally on purpose: an instrument that only speaks when something changed cannot distinguish *"the lever is at Epic"* from *"the lever never ran"*.
- **One line per component when the factor is < 1.0**, naming layer + mesh and printing `authored uu (m) -> applied uu (m)`. Silent at Epic, so the default path adds no per-match noise.
- **A `Warning` on any read-back mismatch.**

⛔ **Every number in those lines is READ BACK off the component** via `UInstancedStaticMeshComponent::GetCullDistances()` **after** the `SetCullDistances` call — it reports what the component *holds*, not what we *asked for* (`SC-§94` cl. B). ⚠️ **I have not seen this line print.** No PIE, no capture, no frame — the `SC-§94` pixels rung is **entirely unspent by this row**.

---

## 7. TESTS — `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeScatterCullBandTest.cpp`

**9 tests.** Subject: the two **pure statics**. No `UWorld`, no `AActor`, no `UObject`, no allocation, no clock, no RNG — the `HIGH-§3` testability idiom, so the whole mapping runs in-process with no PIE.

`EpicAndCinematicReturnTheAuthoredBandsByteForByte` · `NeverCulledLayersStayNeverCulledAtEveryLevel` · `TheQualityScalarIsTranslatedBySquareRootNotIdentity` · `TheRuledMetreMappingForTheMeasuredLayers` · `LowAndEpicCullDistancesDifferAndLowIsAlwaysShorter` · `TheLadderIsMonotoneNeverExceedsTheAuthoredBandAndNeverInverts` · `TheNearFieldFloorHoldsAndNeverLengthensAnAuthoredBand` · `AnAuthoredHardPopStaysAHardPop` · `TheLeverConsumesNoRandomStreamDraws`.

⛔ **Every expected number is transcribed from §3.3's ruled table, in metres — never re-derived by calling the subject.** A test computed from its subject agrees with it by construction and reports SAFE for ever (`SHIP-§9c`). The metre assertions carry a **0.2 m tolerance** deliberately: the ruling is a distance *policy*, not a bit pattern, and pinning the exact int would make a harmless float-rounding change look like a defect.

### 🚨 `SC-§83` — the named mutations, in Addendum B form

| # | mutation | predicted red |
|---|---|---|
| **M1** | factor returns `ClampedQuality` (identity — the refused naive mapping) | **≥ 3 rows**, including `TheQualityScalarIsTranslatedBySquareRootNotIdentity`, `TheRuledMetreMappingForTheMeasuredLayers`, `LowAndEpicCullDistancesDifferAndLowIsAlwaysShorter` |
| **M2** | delete **only** the `BaseEndUU <= 0` early return | ⛔ **DECLARED ZERO ROWS — NO RED.** Today's arithmetic independently yields `0/0` (`Min(floor,0)=0` ⇒ `Clamp(0,0,0)`), so the guard is invisible to every assertion here. It is defence-in-depth against a future floor edit; **M3** is the mutation that shows that edit is the real hazard. Recorded rather than quietly claimed as covered. |
| **M3** | delete that early return **and** drop the `Min` cap on the floor | **≥ 1 row**, `NeverCulledLayersStayNeverCulledAtEveryLevel` (a Hills layer authored `0` would start culling at 35 m) |
| **M4** | drop the `Min` cap, keep the early return | **≥ 1 row**, `TheNearFieldFloorHoldsAndNeverLengthensAnAuthoredBand` (a layer authored 1,000 uu would be *stretched* to 3,500) |
| **M5** | delete the `DistanceFactor >= 1.0f` early return | **≥ 1 row**, `EpicAndCinematicReturnTheAuthoredBandsByteForByte` — ⚠️ **and only its authored-HARD-POP case.** For an ordinary band the arithmetic reproduces the authored pair exactly, so the three real layers stay **green** under M5. The inverted pair is the whole discriminator, which is the only reason that assertion is in the file. |
| **M6** | floor `3500` → `1800` | **≥ 1 row**, `TheNearFieldFloorHoldsAndNeverLengthensAnAuthoredBand` |
| **M7** | drop the `FMath::Clamp(…, 0, 1)` in the factor | **≥ 1 row**, `TheQualityScalarIsTranslatedBySquareRootNotIdentity` (`factor(1.5) == 1.0` / `factor(-0.5) == 0.0`) |
| **M8** | delete the `FMath::IsFinite` fail-safe | **≥ 1 row**, `TheQualityScalarIsTranslatedBySquareRootNotIdentity` (NaN + infinity, and the band's NaN case — without it a NaN survives `Clamp` (every comparison against NaN is false), survives `sqrt`, fails `>= 1.0f`, and reaches `RoundToInt`, where the conversion is **undefined behaviour** on its way into `SetCullDistances`) |

⛔⛔ **NO WITNESSED RED.** Not one of M1–M8 was executed. This row does not compile and does not run the suite. **Every row above is a derived prediction from reading the code, not an observed transition** (`SC-§90`, `SC-§101`), and `TASK-1113`'s **M4** is the standing proof that a prediction in this lane can be confidently wrong. **M2 is already an admitted no-red, written down before the gate had to find it.**

⚠️ **ONE GUARD IS UNREACHABLE BY PROOF AND NO MUTATION IS CLAIMED FOR IT:** the `FMath::Min(…, OutEndUU)` on the scaled start in the non-hard-pop branch. Given `BaseStart < BaseEnd` and a factor in `[0,1)`, the scaled start is always ≤ the scaled end and the floor only *raises* the end, so no input can drive it. Kept as a guard against a future reordering, declared here rather than covered by a test that could not fail.

### `SC-§79` — what these tests CANNOT detect (written in the file too)

- ⛔ **A rendered pixel.** Nothing here draws. Whether grass really disappears at 45 m is the `GFX-§9` / `SC-§94` pixels rung and it is **unspent**.
- ⛔ **The actor's call graph.** That the read is at the funnel, that the re-apply pass is called, that Play-Again re-applies, that `SetCullDistances` reached the render proxy — all need a world and the DataAsset. **The gate should read `RunScatterPasses`, `ApplyFoliageCullBands` and `ResolveComponentForMesh` rather than trust the test file.**
- ⛔ **The instance counts.** §4's proof is structural. Structural is not observed.

### `SC-§87` / the suite count

**This row adds 9 tests.** Last **measured** baseline: **`517 / 0` at `42734b7`.** ⇒ this row alone contributes a floor of `517 + 9 = 526`. ⛔ **I do not quote a total**, because `TASK-1113` and `TASK-1115` also added unmeasured tests to the same tree — any total I named would be a guess dressed as arithmetic. ⛔ Never `529`, never `514`, never `511`, never `306`. **Only `TASK-1124`'s executed `N / M` under an `SC-§87` bound counts.**

---

## 8. ⚖️ FLAGGED DECISIONS FOR `TASK-1123`

| # | decision | why it needs a ruling |
|---|---|---|
| **F-1** | 🚨 **I call `GetFoliageQualityScale()`, which does not exist in the tree yet.** | §1. Board-ordered and `GFX-§9`-ratified, but it means the tree is **uncompilable until `TASK-1118` cl. (10a) lands**. I reduced the coupling to **one word at `BattlefieldScatter.cpp:406`**. Wanted: confirmation this is the intended coupling and not a blocker, plus the one-line fallback carried to `TASK-1124`. |
| **F-2** | ⚠️ **Foliage=Low × ViewDistance=Low compounds to ≈18 m of grass.** | §3.4. `r.ViewDistanceScale` is `0.4` at ViewDistance=Low and `GFX-§9` struck any compensation. My recommendation is **accept** (two honestly-labelled Low sliders), and compensating would re-open the double-scaling the law struck. But 18 m is the *same number* the reshape called ruinous, arriving through a different door. **Wanted: a yes/no, and if "no", it is a floor change, not a factor change.** |
| **F-3** | **`sqrt(q)` is my ruling and nothing above me ratified it.** | §3.1. It is defensible (`factor² == q` reproduces the ladder's intended trade exactly) and it is asserted as a test, but the manager may prefer a hand-picked table. If the mapping changes, **§3.3's table, the header block and the test file's literals all move together** — they are three copies of one ruling. |
| **F-4** | **`ApplyFoliageCullBands()` is a new pass over ~59 components on every generate**, including at Epic where it writes identical values. | §5. It is what makes Play-Again correct, and I judged a redundant identical write cheaper than a lever that is dead on one path. If the gate prefers it gated on `factor < 1.0`, note that would also skip the **restore** when a player moves Low → Epic and hits Play Again. |
| **F-5** | **The floor never binds today** (§3.3) and is exercised only by a synthetic test layer. | `SC-§36.1` says a built, tested, zero-caller mechanism is a surface rather than a fix. I kept it because `DA_BattlefieldScatter` is content that moves without code review, and declared it rather than letting a green imply it fired. **Wanted: keep or drop.** |
| **F-6** | **The proxy HISM's band is scaled too**, though the proxy is invisible and its cull band is already documented as functionally moot. | Preserves the shipped "uniform pair state" intent and cannot change blocking or nav (a cull distance does not affect either). Named because it doubles the recorded components and the per-component log lines at Low. |
| **F-7** | **`Hills` and `Rocks` authored bands are NOT measured by me.** | §3.2. I did not open the `.uasset` (fence) and no code path needs the value — `End == 0` is passed through by construction. But my *"the silhouette is untouched at every level"* sentence in §3.5 **depends on Hills being `0`**, which is `ScatterConfig.h`'s stated intent, not a read-back. ⚠️ **If Hills is authored non-zero, that sentence is wrong** and the gate should say so. |
| **F-8** | **The summary log line prints on every generate, at `Log`.** | Deliberate (§6): a line that only speaks on change cannot distinguish Epic from "never ran". If that is judged noise, demoting it to `Verbose` re-opens the empty-log-reads-as-a-pass trap the memory of this project was bought on. |

---

## 9. WHAT QA SHOULD SCRUTINISE HARDEST

1. ⭐ **§5's re-apply pass — it is a call-graph change, and both blockers in this lane lived in call graphs.** Check: is `ApplyFoliageCullBands()` actually reached on both the authority path and the client mirror? (It is inside `RunScatterPasses`, after pass 2, so yes for both — verify, do not take it.) Are all components created before it runs?
2. **The three `SetCullDistances` sites** (`:1075` the new pass, `:1274` = the row's `:1003`, `:1374` = the row's `:1092`) — and confirm `grep -c "SetCullDistances" ` finds no fourth.
3. **The determinism census by grep, not by comment:** `InstanceCount`, `OuterTarget`, `AddInstance`, `RemoveInstance`, `Stream` in added lines ⇒ comments only. If any is code, the row is a blocker.
4. **The getter identifier count:** `GetFoliageQualityScale` must be **one code site**. More than one multiplies F-1's cost.
5. **M5's narrowness** — I predict it reddens on *one* assertion only. If the gate reasons it should redden more broadly, one of us is wrong about the arithmetic and it should be settled before the host compiles.
6. **F-7's Hills assumption**, which is the one place my prose out-runs my measurement.
7. ⚠️ **Nothing here has been compiled or executed.** The three compile risks I could not check: UHT's tolerance of `static constexpr int32` + non-`UFUNCTION` statics in a `UCLASS` body (widespread in engine code, but unverified here); `TWeakObjectPtr` on a forward-declared `UHierarchicalInstancedStaticMeshComponent` in a nested struct; and `<limits>` in an automation test TU.
