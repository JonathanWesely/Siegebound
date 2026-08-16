# TASK-569 — [WR-15] SCENE + DATA INTEGRATION, the PIE MATRIX, and the NAV MEASUREMENT (build-master handoff)

**Date:** 2026-08-15/16 · **HEAD:** `f205eb5` (⛔ **UNCHANGED — no commit, no push**) · **Branch:** `main`, `0 0` vs `origin/main`
**Route:** entry ledger → data verify + `BP_Torch`/`BP_CommanderNpc` authored → `SC-§9` dirtiness sweep → **graceful close** → ⭐ **FRESH EDITOR BOOT** → Instrument-B readback → **PIE run 1 (3m14s)** → **PIE run 2** → STOP.
⛔ **NO COMMIT. NO PUSH. NO `L_Arena` SAVE.**

---

## 0. THE THREE-LINE RESULT

1. ⭐⭐ **THE NAV MEASUREMENT PASSES. D1 NEVER REACHES JONATHAN.** From a fresh boot with **no save**, the runtime regenerated the bake over the 9× castle and the **definitive, event-driven** check printed `CONFIRMED (nav settled: 0 pending)` with **0 pending tiles and 0 culls**. 🔒 **The spent TASK-350 exception stays spent; no fresh exception is requested.**
2. ⭐ **BOTH WAVE-2 ROWS I COULD REACH PASS, WITH ARITHMETIC:** hero spawn distance **3957** (row (n)) and bot waves **1,343 uu in front of the Red wall face** (row (p)).
3. ⛔⛔ **BUT THE MATRIX DID NOT PASS, AND IT MAY NEVER BE REPORTED AS IF IT DID.** **11 of 18 rows are UNOBSERVED** — the MCP surface has **no input-injection and no console-exec lane**, so every row needing a keypress, a click or an order is unreachable by me. **AND ⛔ ROW (r) IS STRUCTURALLY UNRUNNABLE UNDER THIS TASK'S OWN CONSTRAINTS** (§6) — that one needs a manager ruling, not a retry.
4. ⛔ **ONE NEW DEFECT, FOUND BY BEING THE FIRST PIE OF THIS BATCH: `ABP_Footman` on `ACommanderNpc` throws 2 Blueprint runtime errors PER FRAME PER NPC — 1,806 in 49 s.** ⛔ **Not caused by my Blueprints.** Row (m) fails on it. §7 finding 1.

---

## 1. ⛔⛔ ITEM (1a) — THE SPEC'S PREMISE IS **FALSE**. IT IS A VERIFY-AND-CLEAR, ⛔ NOT A REAL EDIT.

**The spec says:** *"`CastleKeepClearRadius` `1500` → `4500` … ⛔ The saved asset OVERRIDES TASK-557's header default — without this edit the C++ change is INERT."*
⛔ **MEASURED FALSE. There is no override. TASK-557's change is ALREADY LIVE and always was.**

| instrument | reading |
|---|---|
| loaded asset `/Game/Data/DA_BattlefieldScatter` | `castleKeepClearRadius` = **4500** · `ancientGroundMaxAbsX` = **16080** |
| the CDO `Default__SiegeScatterConfig` | **4500** / **16080** — **identical** |
| `is_dirty` (cold process, after reboot) | **false** ⇒ read **from disk**, not from a stale in-memory edit |
| the `.uasset` on disk | **byte-untouched since `ea2a70f` (TASK-350, Jul 28 21:01)** — ⛔ not in `git status` |
| header default **at `ea2a70f`** | ⭐ **`CastleKeepClearRadius = 1500.f`** |
| header default **today** | `4500.f` (TASK-557) · `AncientGroundMaxAbsX = 16080.f` (TASK-576) |

⭐⛔ **THE DECISIVE ARGUMENT, AND IT IS AIRTIGHT: a serialised delta CANNOT change when the header changes.** The package was written when the CDO said `1500`. If a `1500` delta had been serialised, the asset would still read `1500` today. **It reads `4500`.** ⇒ **no live override on either field.** ⇒ ⛔ **I CHANGED NOTHING, and changing it would have written a redundant override that re-creates the exact rot `SC-§34` forbids.**

### ⚠️⭐ WHY TASK-576's FILE-SIDE READING LOOKED LIKE AN OVERRIDE — AND WHY IT IS THE SAME INSTRUMENT ERROR TASK-589 ALREADY RECORDED

TASK-576 reported the saved asset *"contains `CastleKeepClearRadius` but NO `AncientGround*` property at all."* ✅ **Both halves reproduce exactly** — my byte scan finds `CastleKeepClearRadius` **1×** and `AncientGroundMaxAbsX` **0×**.
⛔ **But a NAME in the package's name table is ⛔ NOT a live VALUE override** — asset-registry tag data and prior-version names live there too. ⚖️ **This is precisely TASK-589's finding 2 in a new costume** (*"a name-table byte scan cannot prove a widget instance absent"*) — here it is *"a name-table byte scan cannot prove a property override PRESENT."*
⇒ 📌 **Worth a `TL-§` line: the only instrument that settles an override question is the LOADED VALUE, compared against the CDO, ideally across a known header change.**

**Item (1b) `AncientGroundMaxAbsX`:** ✅ **VERIFY-AND-CLEAR, verified CLEAN — no override, header default `16080` live.** Exactly as TASK-576 predicted and item (1c) instructed. ⛔ Nothing set.
**Item (1c):** ✅ **Honoured — ⛔ no editor step invented for TASK-575's constants**, and they are now **positively confirmed live at runtime** by row (p) below, which is better evidence than any readback.

---

## 2. ⭐⭐ ITEM (2) — THE NAV MEASUREMENT. **PASS.** THE HEADLINE.

**Conditions:** editor **closed gracefully and re-launched fresh**; `L_Arena` opened by the startup-map setting; ⛔ **never saved**; PIE entered with ⛔ **no `startTransform` override** (an override would have destroyed row (n)).

```
[2026.08.16-06.36.37:829][744]LogSiegeTerrain: [BattlefieldScatter 'BP_BattlefieldScatter_C_0']
  Traversability CONFIRMED (nav settled: 0 pending) — 0 tile task(s) pending;
  Blue→Red castle path + 6 mine path(s) exist (after 0 cull(s))
  [definitive: OnNavigationGenerationFinished].
```

| fact | measured |
|---|---|
| verdict string | ⭐ **`CONFIRMED (nav settled: 0 pending)`** — the grep the spec named |
| **pending tile count** | ⭐ **0** |
| **settle time** | ⭐ **≈8.4 s** after PIE start (PIE ≈06:36:29.4 → 06:36:37.829) |
| culls required | ⭐ **0** — ⛔ nothing had to be deleted to make the lane exist |
| which pass | ⭐⭐ **`[definitive: OnNavigationGenerationFinished]`** — the **event-driven** check, ⛔ not the settle-poll |
| mine paths | **6 of 6** reachable |
| `PROVISIONAL` occurrences | **0** |

⭐⛔ **WHY THE `[definitive: ...]` TAG IS THE WHOLE ANSWER AND NOT A DETAIL:** `NAV-§4` is explicit that a settle-poll `CONFIRMED` **deliberately does NOT discharge the guarantee** (*"a queue that reads empty at +5 s can be a lull between waves of dirty areas"*), and that only the pass driven by the engine's own generation-finished signal does. **The pass that fired here is that one.** ⇒ **The claim is the strong one the law reserves.**

⇒ ✅ **`RuntimeGeneration = Dynamic` regenerates the bake over the 9× castle unaided.** 🔒 **The one-time exception stays SPENT and EXPIRED. ⛔ No ask goes to Jonathan. ⛔ No `L_Arena` save. D1 is closed by measurement, exactly as ruling 8 intended.**

### ⭐⭐ AND IT IS **REPRODUCED**, ⛔ NOT A SINGLE SAMPLE — I RAN PIE TWICE

⚖️ **A one-shot green on an async subsystem is exactly the kind of result `NAV-§4` was written to distrust** (its whole complaint is against verdicts decided by wall-clock timing), so I ran a second session and re-measured.

| | run 1 | run 2 |
|---|---|---|
| verdict | **CONFIRMED (nav settled: 0 pending)** | **CONFIRMED (nav settled: 0 pending)** |
| pending tiles / culls | **0 / 0** | **0 / 0** |
| mine paths | **6** | **6** |
| ⭐ which pass | **`[definitive: OnNavigationGenerationFinished]`** | ⭐ **`[definitive: OnNavigationGenerationFinished]`** |
| settle latency | **≈4.8 s** after the castles furnished | **≈2.7 s** after PIE start (nav data warm in-process) |
| `PROVISIONAL` lines | **0** | **0** |

✅ **4 CONFIRMED lines across the session, ZERO PROVISIONAL.** ⭐ **Rows (n) and the furnishing lines reproduced BYTE-IDENTICALLY across both runs too** (`spawn distance 3957 … -> (-21043, 0, 100)`; `6 of 6 torch anchors spawned (cap 6), commander spawned` for both castles, both runs).

⚠️ **HONEST SCOPE — ⛔ READ THIS BEFORE QUOTING THE PASS.** What is proven is the **engine's own reachability query**: a Blue-anchor → Red-castle path plus 6 mine paths, against a **settled** navmesh. ⛔ **What I did NOT observe is a human ordering a unit through the door and watching it stand on the interior floor** — that is row (b), and it needs an input lane I do not have (§5). **The spec named the grep as the instrument and the grep is green; I am flagging the gap rather than letting the grep imply more than it measures.**

---

## 3. ITEM (1d) — `BP_Torch` + `BP_CommanderNpc`. ✅ **DELIVERED, AND CONFIRMED AT RUNTIME.**

| check | result |
|---|---|
| `/Game/Blueprints/BP_Torch` | ✅ created, parent **`/Script/GitClaudeUnrealTest.Torch`** |
| `/Game/Blueprints/BP_CommanderNpc` | ✅ created, parent **`/Script/GitClaudeUnrealTest.CommanderNpc`** |
| ⭐ `TorchLightRelativeOffset` | ✅ **`(50, 0, 75)`** — read back verbatim, and again from a **cold process** |
| class defaults only | ✅ ⛔ **no graph, no components, no variables, no event nodes** — `BlueprintTools.create` one call each |
| `BP_CommanderNpc` defaults | ✅ ⛔ **untouched** — `interactRadius` **400**, `enemyRevealCost` **30**, inherited |
| compile (`warnings_as_errors=true`) | ✅ **clean, both** |
| `is_dirty` after reboot | ✅ **false, both** ⇒ persisted, load clean **from disk** |
| the live C++ base is the compiled one | ✅ `torchAttenuationRadius` **1200**, `torchLightColor` **(1.00, 0.72, 0.42)** — `WR-§4`'s pinned values, inherited |

⭐⭐ **THE CONFIRMATION IS BETTER THAN THE ONE THE SPEC ASKED FOR.** Item (1d)(i) said to confirm by finding the fallback `Log` line **absent**. ✅ **It is absent (0 occurrences of `torch blueprint … unavailable`, 0 for the NPC).** ⛔ **But absence is weak evidence**, so I also took the **positive** one: **all 12 spawned torches are class `BP_Torch_C` and the commander is `BP_CommanderNpc_C`** — the Blueprints at the hardcoded `Castle.cpp:127-128` paths are what the castle actually instantiated. **A path typo could not have produced that.**

---

## 4. ITEM (4) — CLOSED, AS THE BOARD SAID. ⛔ NOTHING AUTHORED. ⭐ AND CONFIRMED ON THE COMPONENT, NOT JUST THE BOUNDS.

Read live off the PIE castle's own `HPBarWidget` component:

```
HPBarWidget   relativeLocation = (0, 0, 9450)     drawSize = (256, 32)
```

✅ **Both halves of item (4) confirmed exactly:** the re-derived **`9450`** Z landed (TASK-557), and ⭐ **`DrawSize` is still `(256, 32)` — deliberately NOT scaled**, because a screen-space widget's size is in SCREEN PIXELS. ⛔ **I authored nothing.** ⚠️ **Whether the bar reads correctly on screen is an APPEARANCE claim and is Jonathan's** (§8).

### ⭐ WHILE I WAS THERE — THE TWO ACTOR-SIDE COMPONENTS `WR-§1`'s CRUMBLE RULING TURNS ON

`Castle.cpp:179-191` keeps `GateBlockerVolume` and `InteriorNavModifier` **ACTOR-side** precisely so a crumble mesh swap can never strip them. **Both are present on the live castle** (`CastleMesh · HPBarWidget · GateBlockerVolume · InteriorNavModifier · HitFlashComponent`), and the blocker reads:

```
GateBlockerVolume   boxExtent = (900, 405, 678)   relativeLocation = (18, -1575, 852)
```

- ✅ **`relativeLocation` is `(18, −1575, 852)` — `WR-§2` row 3's re-derivation, exact.**
- ⚠️⭐ **`boxExtent.X` is `900`, ⛔ NOT the `780` `WR-§2` row 2 still prints.** ✅ **This is NOT a defect — I checked the header before reporting it: `Castle.h:461` authors `FVector(900.f, 405.f, 678.f)`, and the runtime matches it exactly.** ⇒ **`900` is the `W4-R4`-RATIFIED value** (that ruling ratified the value and retired its argument, after TASK-555 measured the real aperture at 1560 collision / 1470 visual). ⇒ 📌 **`WR-§2` row 2's "⛔ RE-DERIVE ×3 → `(780, 405, 678)`" is now STALE TEXT and should be annotated to the ratified `900`** — the same drift class the batch keeps catching.
- ⛔ **This is STRUCTURE, ⛔ not row (c) or row (l).** It proves the components exist and carry 9×-derived numbers; it does **not** prove the blocker holds a besieger at runtime, and I am not claiming it does.

---

## 5. ⛔⛔ THE PIE MATRIX — 7 OBSERVED, 11 NOT. THE INSTRUMENT WALL, STATED PLAINLY.

### 5.0 ⛔ THE CAPABILITY FACT THAT DECIDES 11 ROWS — MEASURED FROM THE TOOL SCHEMAS, ⛔ NOT INFERRED FROM A FAILURE

**Unreal MCP `EditorAppToolset` gives `StartPIE` / `StopPIE` / `IsPIERunning` / `CaptureViewport` / `CaptureEditorImage`.** ⛔ **It exposes NO keyboard injection, NO mouse injection, and NO console-exec tool** — `SearchCVars` is read-only, and TASK-567's full 19-toolset enumeration already recorded that there is no `exec` door anywhere in the surface. `ProgrammaticToolset` is sandboxed to registered tools.
⇒ ⛔ **Every row needing a keypress, a click, a card play or a unit order is unreachable from this lane.** ⚖️ **I did NOT route around it with OS-level `SendInput` keystroke injection into the PIE window** — §6 gives the reason, and it is the latch.

| row | verdict | evidence / why not |
|---|---|---|
| **(a)** castle 9×, on the terrain | ✅ **PASS** | `Castle_0` bounds **`7313 × 7384`**, **min Z = `0`** ⇒ ⛔ no float, no sink. Matches TASK-567's `7313.576 × 7384.367` and `WR-§0`'s ±10 % band. `Castle_1` symmetric at `+25000`. |
| **(b)** unit AND hero walk IN | ⛔ **NOT OBSERVED** | needs a unit order + hero movement. ⚠️ **The nav query (§2) is adjacent evidence, ⛔ not a substitute.** |
| **(c)** enemy blocked at the gate | ⛔ **NOT OBSERVED** | see §5.1 — attempted and reported honestly. |
| **(d)** own units spawn INSIDE | ⛔ **NOT OBSERVED** | needs a card play. |
| **(e)** torches spawn / no orphans | ✅ **PASS (mechanism)** · ⚠️ partial | Both castles: **`furnished — 6 of 6 torch anchors spawned (cap 6), commander spawned`**. **12 torches, all `BP_Torch_C`.** ⭐ **Teardown observed LIVE**: when the Blue castle fell, Blue's 6 torches **and** its NPC were destroyed and Red's 6 + 1 survived (12→6, 2→1) — `WR-§4`'s teardown law, watched tripping. ⭐ **Orphan check across a PIE restart: exactly 12, indices restarted at `_0`** ⇒ no accumulation. ⛔ **The in-game *Play Again* (`ResetCastle`) RESPAWN path is NOT tested** — it needs the match-end button. ⛔ *"lights the hall"* is appearance ⇒ Jonathan's. |
| **(f)** NPC + table in the hall | ✅ **PASS (numeric)** | NPC at **`(24535, 810, 174)`** — **z = 174 is the interior floor height** (`WR-§1`), inside the Red footprint `21343…28656`. All 6 Red torches at **z = 954**, x `23565…26680` ⇒ in the hall volume, ⛔ not the corridor. |
| **(g)(h)(i)(j)** map open / dots / pay / clear | ⛔ **NOT OBSERVED** | need the **`M`** key. ⭐ **`IMC_Hero` carries the mapping (TASK-589 proved it 5 ways) — the gap is the KEYPRESS, ⛔ not the binding.** Per (3z), `M` is the only instrument; ⛔ I did not invent a console route. |
| **(k)** console opens anywhere | ⛔ **NOT OBSERVED** | needs **Enter**. |
| **(l)** crumble swap + post-swap traversal | ⛔ **NOT OBSERVED** | needs the castle driven to 25 % damage on demand. ⚠️ **In run 1 the Blue castle went from full to destroyed, so stages were crossed — but ⛔ I could not pause at a stage and re-run the traversal, and an unmeasured crossing is ⛔ NOT evidence.** ⭐ **TASK-567 already proved the collision INVARIANCE statically (identical count/extent/aperture/lintel per stage); this row is its runtime half and it remains OWED.** |
| **(m)** Message Log clean | ⛔ **FAIL** | **1,806 Blueprint runtime errors in 49 s** (§7 finding 1). ⛔ Not clean. Remaining 13 `Error:` are benign Live Coding `ggml-cpu-*.dll` variant notices. **0** `Fatal`, **0** `ensureAlways`, **0** `LogOutputDevice: Error`. |
| **(m)** Play Again ×3 | ⛔ **NOT OBSERVED** | needs the button. **2 full PIE sessions** run instead; stated as the weaker test it is. |
| ⭐ **(n)** hero spawns OUTSIDE the keep | ✅⭐ **PASS** | see §5.2 — the strongest row in this report. |
| ⭐ **(o)** DEFEND acquires | ⛔ **NOT OBSERVED** | needs a DEFEND order. **`DEFEND band resolved` count = 0**, which is ⛔ **correct and expected** (no unit ever entered the stance), ⛔ **not a defect signal.** ⚠️ **The half-width-1218 tell the dispatch warned about did NOT appear — but that is because the line never printed at all, ⛔ so it proves nothing either way.** |
| ⭐ **(p)** bot waves in front of its castle | ✅⭐ **PASS** | see §5.3. |
| **(q)** the short-balance refusal | ⛔ **NOT OBSERVED** | needs the reveal button. ⛔ **I did NOT report *"nothing appeared to happen"* — per `WR-§7` that is not evidence, and I had no gold reading to compare.** |
| **(r)** marker click → console box | ⛔⛔ **STRUCTURALLY BLOCKED** | **§6 — this is not merely an input gap.** |

### 5.1 ROW (c) — WHAT I ACTUALLY DID, AND WHY I AM NOT CLAIMING IT

I ran a second PIE specifically to sample enemy unit positions against the Blue castle's measured colliding AABB (`x −28657…−21344`, `y −3693…3691`) while the bot's waves marched in.
⛔ **REPORTED AS NOT OBSERVED.** ⚖️ **A position sample cannot distinguish *"blocked by `GateBlockerVolume`"* from *"still walking"* from *"stopped to attack the wall"*, and the row demands BOTH lanes — the blocker **and** the nav filter — be seen holding. ⛔ **Inferring a gameplay guarantee from a scatter of coordinates would be exactly the *"nothing appeared to happen"* error `WR-§7` condemns**, so I am naming it unobserved rather than dressing a weak sample as a pass.

⚠️⭐ **AND THE SECOND RUN COULD NOT SUPPLY THE SAMPLE ANYWAY — AN INSTRUMENT LIMIT WORTH RECORDING FOR THE NEXT AGENT:** run 2 produced **ZERO bot waves in ~5 minutes**, against run 1's first wave at **+38 s**. The log's frame counter advanced ~458 frames in 225 s (**≈2 fps**) with long dead stretches. ⇒ **An unfocused/background PIE window is throttled hard**, and run 1 only ticked usefully because my MCP polling kept pumping it. 📌 **A no-input PIE observation that depends on the sim ADVANCING is unreliable from this lane; anything time-dependent needs either a focused window or a driver.**

📌 **What IS true and is worth carrying:** in run 1 the Blue castle was destroyed by the bot **from outside** — ⛔ castle damage does not require entry, so ⛔ **its destruction is NOT evidence that the gate was breached.**

### 5.2 ⭐ ROW (n) — PASS, WITH THE ARITHMETIC

```
[SiegeGameMode_0] Castle-relative hero start for Blue: castle X -25000,
  measured colliding half-extent 3657 + clearance 300 => spawn distance 3957
  (authored floor 1500) -> (-21043, 0, 100).
```

| required | measured |
|---|---|
| the branch-3 line present | ✅ present |
| resolved spawn distance ≈ **3,957** | ⭐ **3957** — `3657 + 300`, **the derivation beating the 1,500 floor**, exactly as `WR-§2b` row A predicted |
| hero **OUTSIDE** the footprint | ⭐ hero X **−21043** vs the Blue footprint edge **−21343** ⇒ **300 uu clear, outside** |
| on the ground, ⛔ not in the slab | ✅ **Z = 100** (spawn), hero actor alive in-world afterwards |
| a pawn at all | ✅ **`BP_HeroCharacter_C_0` present** ⇒ ⛔ not a pawnless player |
| the **designed** Warning (`WR-§9` row 11) | ✅ **fired EXACTLY ONCE** — designed output, ⛔ not spam |
| ⛔ **no Red-tagged warning** | ✅ **0** — the only occurrence is the Blue one |
| ⛔ **no `SpawnActor` collision Error** | ✅ **0** |

⚠️ **The castle import landed BEFORE this grading** (TASK-566 R10), so this reads against the shipped 9× bounds — the ordering hazard did not bite.
⛔ **Respawn was NOT re-tested** (it needs a hero death I cannot cause); the row asks for match-start **and** after a respawn, so ⛔ **the respawn half is OWED.**

### 5.3 ⭐ ROW (p) — PASS, AND IT REPRODUCES THE ORIGINAL AUTHORED RELATIONSHIP

**12 bot waves, every one of them:**

```
[Bot SiegeBotController_0] Rule 4 (Attack): played unit 'Cavalry' (cost 21)
  castle-front (20000, 172, 20) — marching (M7.6 ruling #1) — gold 37->16.
```

| fact | measured |
|---|---|
| spawn X, **all 12 waves** | ⭐ **20000** (Y varies `−853 … +846`) |
| Red castle measured footprint edge | **21343** (from the live bounds readback) |
| ⇒ distance in FRONT of the wall face | ⭐⭐ **1,343 uu — OUTSIDE the castle** |
| do they march? | ✅ **`— marching (M7.6 ruling #1)`** on every line |
| ⛔ any wave in the Red hall? | ✅ **NONE** |
| ⛔ the invalidating one-shot `no usable Castle_Red colliding bounds` | ✅ **ABSENT (0)** ⇒ the derivation used **real measured bounds**, ⛔ not the authored fallback — **so the rows are valid** |

⭐⭐ **AND THE NUMBER IS NOT MERELY "OUTSIDE" — IT IS THE ORIGINAL RELATIONSHIP, RECOVERED.** `WR-§2b` row F records that Jonathan's authored `1750` sat **1,343 uu in front of the wall face** when he wrote it. **The derived anchor now lands at 1,343 uu in front of the wall face at 9×.** ⇒ ⭐ **`SC-§34`'s structural escape did exactly what it promises: it restored the DESIGNED relationship rather than picking a new number.** ⛔ **`WR-§9` outcome 9's "certain, every time" hall spawn is gone.**

---

## 6. ⛔⛔ ROW (r) IS **STRUCTURALLY UNRUNNABLE** — A CONTRADICTION I WILL NOT RESOLVE ON MY OWN AUTHORITY

⚖️ **This is not my input limitation. Even with a full input lane, the task as written cannot be executed, and the manager needs to know that.**

- **The board (row (r)) REQUIRES a submission:** *"on a FIRST open, before any console sentence, there are ⛔ NO place markers to click. ⇒ **Send ONE console sentence first**, THEN open the map"* — because `WR-§9` row 12 makes `ResolvePlace` unreachable until the first turn allocates the snapshot.
- **The dispatch FORBIDS every submission:** 🔒 *"`ReportFirstCapture` latches on the FIRST LIVE assistant capture… ⛔ **DO NOT SEND.** Submitting spends the latch and destroys a measurement nobody can retake."*

⇒ ⛔ **The prerequisite for row (r) is the prohibited act.** There is no ordering that satisfies both: no sentence ⇒ no markers ⇒ nothing to click; one sentence ⇒ the latch is spent.
⛔ **I did NOT send. ⛔ No `DumpAssistantPrompt`, no `SpikeEval`, no `SpikePrompt`, the assistant console was never opened, and 🔒 the latch is UNSPENT.** ⛔ **No token figure appears anywhere in this document** (`AS-§12g`).

🚩 **THE RULING OWED:** either (i) TASK-552 spends the latch deliberately and row (r) runs after it, or (ii) row (r) is re-scoped to TASK-571 as Jonathan's check (he can spend the latch knowingly), or (iii) the markers' first-open gap (TASK-580) lands first. ⛔ **A build-master may not pick.**
📌 **And note (3z) already anticipated half of this:** it rules the marker-click leg *"runs with an EMPTY tree"* because markers are painted in C++ — ✅ **true, and the tree is no longer empty anyway (TASK-589 landed it)** — ⛔ **but neither fact reaches the snapshot prerequisite.** **TASK-579's status line is likewise unobserved, and (3z) already names it as having no other instrument.**

---

## 7. 🚩 FINDINGS — ⛔ NONE FIXED HERE

### 1. ⛔⛔ NEW DEFECT — `ABP_Footman` ON `ACommanderNpc` FLOODS 2 BLUEPRINT ERRORS **PER FRAME, PER NPC**

```
PIE: Error: Blueprint Runtime Error: "Accessed None trying to read (real) property
  CallFunc_TryGetPawnOwner_ReturnValue in not an UClass".
  Node: Set GroundSpeed / Set bIsMoving   Graph: EventGraph
  Function: Execute Ubergraph ABP Footman   Blueprint: ABP_Footman
```

- **Cause, read at the source:** `CommanderNpc.cpp:32` assigns `/Game/Characters/ABP_Footman.ABP_Footman_C` to the avatar, and **`ACommanderNpc` is an `AActor`, ⛔ not an `APawn`** (`CommanderNpc.h:129`). `ABP_Footman`'s EventGraph calls **`TryGetPawnOwner()`**, which returns **None** on a non-Pawn owner, so **both** its `Set GroundSpeed` and `Set bIsMoving` nodes fail **every frame**.
- **Measured: 1,806 errors in 49 s** with 2 NPCs alive (903 per node), continuous.
- ⛔⛔ **NOT CAUSED BY MY BLUEPRINT, AND THIS MATTERS FOR TRIAGE:** the AnimBP is assigned in the **C++ constructor**, and my `BP_CommanderNpc` carries **zero** overrides ⇒ **the C++ fallback class behaves identically.** ⚖️ **It has simply never been seen because ⛔ no task in this batch ran PIE before me** (566/567/568/589 all record "no PIE") — all 10 backup logs show **0** occurrences for exactly that reason.
- ⚠️ **The author guarded the wrong failure mode:** `CommanderNpc.h:313-322` reasons carefully about null-safety (*"unresolved ⇒ no anim instance ⇒ ref pose, never a crash"*) — ⛔ **but a RESOLVED AnimBP that assumes a Pawn owner is the case that was not considered.**
- ⚠️ **OPERATIONAL IMPACT ON THIS PIPELINE, worth a `TL-§` line: it breaks the MCP `ProgrammaticToolset` batcher during PIE** — every `execute_tool_script` call returns the accumulated script errors instead of its result. ✅ **Direct single tool calls still work**, which is the workaround I used.
- ✅ **A no-recompile mitigation EXISTS and is Jonathan's/the manager's call, ⛔ not mine:** `AvatarAnimClassAsset` is `UPROPERTY(EditAnywhere)` ⇒ **clearing it on `BP_CommanderNpc` silences the flood** at the cost of the idle (ref pose). ⛔ **I did NOT do it** — item (1d)(iv) says leave the NPC's defaults untouched, and choosing "silence vs. breathing" is a design call. **The real repair is a Pawn-safe locomotion source for a non-Pawn avatar.**

### 2. ⚠️ `WR-§9` ROW 13 AND ITEM (1d)(iv) NAME THE WRONG ARTIFACT — THE OUTCOME SURVIVES, THE CLAIM DOES NOT

Both say `BP_CommanderNpc` makes **`CommanderWarTableForwardOffset`** *"a no-recompile dial."*
⛔ **`CommanderWarTableForwardOffset` is a file-scope `constexpr float` (`CommanderNpc.cpp:70`), ⛔ NOT a `UPROPERTY`** ⇒ it is **not exposed on the Blueprint** and changing it **does** require a recompile.
✅ **The intended OUTCOME is still reachable**, by the mechanism the code's own comment describes (`:64-68`): the ctor sets it as **`WarTableMesh`'s component-template relative location**, so the dial Jonathan actually gets is **`WarTableMesh`'s Location X on `BP_CommanderNpc`**. ⇒ 📌 **The law should name the component transform, not the constant.** (Same family as the `ResolveHeroStart` phantom: a `names:`-level wrong symbol that propagates.)

### 3. ⚠️ BALANCE DATAPOINT (⛔ not a defect) — a **fully passive** player (zero input, no cards, no defenders) loses the Blue castle in **3 min 14 s**. Recorded because it bounds how long any future no-input PIE observation can run before the world changes underneath it.

### 4. ⚠️ PRE-EXISTING, ⛔ NOT THIS BATCH — missing-asset warnings logged once each: `S_CastleDestroyed`, `S_DefeatMusic`, `DA_AssistantVocabulary`, `ABP_Miner` / `ABP_Knight` / `ABP_Cavalry`, `S_MinerClink`; two `Tree_Pack_2` materials missing the `InstancedStaticMeshes` usage flag; 6 `BattlefieldScatter` `SymmetryAssert` twin-ground-Z warnings; the `LLM_LOAD` / `LLM_BUDGET` spike-harness warnings (the sealed spike is still in the build).

### 5. 📌 `keep_front_east` — ⛔ **I did NOT independently re-measure the ~1.1 % hall-floor clip**, and per the dispatch it is inherited, exactly ×3, and ⛔ not this batch's defect. ⭐ **The adjacent evidence is good:** the nav check confirmed with **0 pending tiles and 0 culls**, so whatever it clips did not cost the interior its reachability.

---

## 8. 🔒 STATE LEDGER — MEASURED, ⛔ NOT ASSERTED

| item | state |
|---|---|
| 🔒 `Content/Maps/L_Arena.umap` | ✅ SHA256 **`b3dbc5d9ae484a7bd02cafad52b4681da68b011477479b65ee7781ae459f8268`** — **IDENTICAL at 5 checkpoints**: entry, after the graceful close, pre-PIE, after PIE run 1, at finish. **535,522 B, mtime still `2026-07-29T03:53`.** **Hash, ⛔ never mtime.** ⭐ **`Saving Package: /Game/Maps` = `0`** for the whole session. ⛔ Opened and played only, ⛔ **never saved.** 🔒 **The spent exception was not touched and ⛔ no new one is requested.** |
| ⭐ `Content/Blueprints/BP_Torch.uasset` | **NEW**, 24,333 B, git `A ` |
| ⭐ `Content/Blueprints/BP_CommanderNpc.uasset` | **NEW**, 24,605 B, git `A ` |
| `Content/Data/DA_BattlefieldScatter.uasset` | ✅ ⛔ **BYTE-UNTOUCHED** — absent from `git status`, mtime `Jul 28 21:01`. **No edit was owed and none was made** (§1) |
| `Saving Package:` — the COMPLETE list | ⭐ **`/Game/Blueprints/BP_Torch` ×1, `/Game/Blueprints/BP_CommanderNpc` ×1. ⛔ NOTHING ELSE.** Both by **explicit path** — ⛔ **never `save_assets([])`, which is a save-all** |
| `.gen.cpp` / `Intermediate/` / `.umap` / `Saved/` in `git status` | ✅ **ZERO of each** — the `SC-§29b` STOP did not trip |
| commits / pushes | ✅ **NONE.** `HEAD` = **`f205eb5`**, **`0 0`** vs `origin/main` |
| the staged set | ⚠️ my two BPs auto-staged `A ` by the editor's SCC (⛔ **I ran no `git add`**). Pre-existing auto-staged entries untouched. ⛔ **TASK-570 still owes the `git diff --cached` reconciliation** |
| ⛔ `Content/UI/WBP_WarMap.uasset` | ✅ **NOT TOUCHED, NOT STAGED BY ME** — still `AM` (index holds the empty 23,105 B blob; worktree has the rooted 33,193 B). ⛔ **TASK-570's `git add` — left exactly as TASK-589 left it** |
| 🔒 TASK-552's one-shot latch | ✅ **UNSPENT** — ⛔ no sentence sent, assistant console never opened, ⛔ no `DumpAssistantPrompt` / `SpikeEval` / `SpikePrompt`. ⛔ **No token figure anywhere in this document** |
| C++ / compile | ⛔ **NONE** — this task changes no `.cpp`. ⛔ No `Tools/**/*.py` added or edited ⇒ **no `SC-§27` gate owed** |
| 🔒 sealed holdout | ✅ untouched |
| TASK-567's meshes · `IMC_Hero` · `IA_WarMap` · other WBPs | ⛔ **untouched** |

### THE EDITOR — `W7-R2`'s THREE LIMITS DISCHARGED A FOURTH TIME
1. ✅ **GRACEFUL ONLY** — `Process.CloseMainWindow()` → accepted `True`, exited within timeout, `Get-Process` confirmed **no `UnrealEditor` process remained**. ⛔ **`Kill()` was never called.**
2. ✅ **DIRTINESS RE-MEASURED BY ME IMMEDIATELY BEFORE CLOSING** (`SC-§9` — ⛔ TASK-589's ledger was not trusted): **3,229 found · 58 `__External*` excluded · 3,171 scanned ⇒ ⭐ `dirty_count: 0`.** ⭐ **The count reconciles exactly: 3,227 → 3,229 is +2, my two new Blueprints.** **Nothing was Jonathan's to pick.**
3. ✅ **RE-OPENED FRESH** (the boot D1 required, **PID 8688**) and **left running with MCP live**.
4. ✅ **CLOSING SWEEP — the editor is handed back CLEAN:** PIE stopped (`IsPIERunning = false`), and a second full sweep re-measured **3,229 found · 58 excluded · 3,171 scanned ⇒ `dirty_count: 0`.** ⛔ **Nothing is Jonathan's to pick, and ⛔ nothing is left dirty for TASK-570 to trip over.**

### THE COMPLETE SET OF FILES THIS TASK TOUCHED — ⛔ THREE, AND ⛔ NO MORE
```
A  GitClaudeUnrealTest/Content/Blueprints/BP_Torch.uasset          (new, LFS)
A  GitClaudeUnrealTest/Content/Blueprints/BP_CommanderNpc.uasset   (new, LFS)
?? GitClaudeUnrealTest/.claude/pipeline/handoffs/TASK-569-buildmaster.md
```
**plus one surgical `- status:` line on `TASKBOARD.md` (line 7900).** ✅ **Verified surgical: the file is still 18,903 lines with 522 `#### TASK-` headers, `ready-for-qa` 96 and `ready-for-integration` 106 — ⛔ no historical token moved.** ⛔ **I ran NO `git add`, NO `git restore --staged`, NO commit, NO push** — the two `A ` entries are the editor's SCC auto-staging, the same behaviour TASK-355/566/568 all recorded.

---

## 9. ➡ WHAT TASK-586 AND TASK-570 INHERIT

- ⭐ **TASK-586 — EXPECT CONVERGENCE, ⛔ NOT REPAIR.** TASK-567 measured all four castle-class meshes already on **`[1.0, 0.4, 0.15]` / `[28698, 14348, 7174]`**. ⛔ **Do not "fix" a chain that is already correct.**
- ⛔⭐ **TASK-570 — THE COMMIT TRAP IS LIVE, AND I RE-MEASURED IT RATHER THAN RELAYING IT.** `Content/UI/WBP_WarMap.uasset` reads **`AM`**. **Worktree = 33,193 B (the rooted asset). The staged blob = `git cat-file -s` → 130 bytes**, i.e. **the Git-LFS POINTER for the OLD empty shell.** ⛔ **`git add` that path first, or the commit ships the EMPTY widget while `git status` looks satisfied.** ⚠️ **The 130-byte reading is worth knowing: under LFS the staged blob is a pointer, so a size check on the INDEX will never look like a `.uasset` — ⛔ do not "sanity check" this by expecting 23,105.**
- ⛔ **TASK-570's `names:`-derived commit list must include `Content/Blueprints/BP_Torch.uasset` and `Content/Blueprints/BP_CommanderNpc.uasset`**, and ⛔ **must NOT include `DA_BattlefieldScatter.uasset`** — §1 measured that no edit was owed, so it is correctly absent from the diff.
- 🚩 **TASK-571 inherits the 11 unobserved rows** as Jonathan's first checks — **(b) (c) (d) (g) (h) (i) (j) (k) (l) (o) (q) (r)**, plus row (n)'s **respawn** half and row (e)'s **Play Again respawn** half. ⛔ **This may NEVER be reported as "the matrix passed."**

⛔ **AND THE STANDING ONE: NOTHING IN THIS DOCUMENT CLAIMS ANYTHING LOOKS RIGHT.** I deliberately did **not** substitute a rendered frame for the pixel check: every claim above is a log line, a property readback, an actor-bounds measurement, a file hash, an engine-source citation or a process fact. **Whether the torches light the hall, whether the HP bar reads correctly above a 9× castle, and whether the war map's chrome looks right on screen is Jonathan's pixel check.**
