# TASK-1075 — [FOGSCALE-PIXEL] RUNG 4: does the thing the player sees change?

**Agent:** art-director · **Date:** 2026-09-05 · **Measured against:** `HEAD 05c3fc0` (parent `c6bb376`)
**Editor:** opened by me under the standing grant, MCP `127.0.0.1:8000` live. **`L_Arena` NEVER saved.**

---

## 0. THE ANSWER IN THREE LINES

1. **A fog box at the CORRECTED scale renders, and it is unmissable.** +76.7 % mean luma over a
   zero-control in the editor at his vantage; **+84.7 %** in a **live ticking PIE world**. Detail
   std collapses 0.120 → 0.070. It is not subtle; it is a **total white-out**.
2. **A fog box at the BROKEN scale renders NOTHING.** −0.043 % vs control — *inside* today's
   ±0.045 % noise floor, and the frame is visually identical to a level with no fog actor at all.
3. ⛔ **I DID NOT DRIVE THE REAL RUNTIME SPAWN, AND I DID NOT SEE THE READ-BACK LOG LINE.**
   I used **preference (iii), the FLOOR**. Read §4 before telling 🧑 Jonathan anything.

---

## 1. ZERO-CONTROL FIRST — and the noise floor RE-MEASURED today

Editor viewport, **his vantage** `(-21580, -44, 201)`, pitch `−15`, yaw `0`, roll `0`, 2764 × 828.
Every capture's returned `cameraLocation` was checked to equal the request; **all 19 matched exactly.**
Level `L_Arena` contained **no `BP_SiegeFog` actor of any kind** (verified by `find_actors`).

| frame | mean luma (linear) | detail std | PNG bytes |
|---|---|---|---|
| CTRL_ed_r1 | 0.261151 | 0.119750 | 4,399,922 |
| CTRL_ed_r2 | 0.261177 | 0.120027 | 4,409,583 |
| CTRL_ed_r3 | 0.261386 | 0.120111 | 4,410,045 |
| CTRL_ed_r4 | 0.261189 | 0.120055 | 4,407,405 |

⇒ **ZERO-CONTROL = `0.261226` mean.** Peak-to-peak spread `0.000235` = **0.090 %**, i.e. a noise
floor of **±0.045 %**. ⛔ This is **today's** floor, measured this session — not the ±0.05 % quoted on
the row (`SC-§91`). It reproduces `TASK-1071`'s `0.261408` to within +0.07 %, across an editor
restart, which is why I trust the instrument.

---

## 2. THE THREE ROWS, ALL MEASURED TODAY, ALL AT THE SAME POSE

### 2a. The BROKEN state — and the value is DERIVED, not transcribed

I re-read the vendor component template **live, today**:
`/Game/FogArea/Blueprints/BP_FogArea.BP_FogArea_C:Mesh_GEN_VARIABLE` →
**`RelativeScale3D = (20, 20, 5)`**, `Mobility = Static`, `bVisible = true`.
That is the value the engine substitutes, taken from its source rather than copied off my own §1 table.

| frame | mean luma | detail std | PNG bytes |
|---|---|---|---|
| BUG20_r1 | 0.261125 | 0.119734 | 4,399,781 |
| BUG20_r2 | 0.261122 | 0.119749 | 4,399,901 |
| BUG20_r3 | 0.261095 | 0.119733 | 4,398,644 |

⇒ mean **`0.261114`** = **−0.043 % vs control**, **INSIDE the ±0.045 % floor.** Detail std intact
(0.1197 vs 0.1200). ⛔ **This is not "faint fog" — it is arithmetically indistinguishable from no fog
actor existing**, and the promoted PNG shows crisp grass and a legible enemy gate. That is exactly
what 🧑 he described three times.

### 2b. The CORRECTED state — bursts, converging, best-converged frame quoted

⚠️ **A SINGLE CAPTURE HERE WOULD HAVE BEEN WORTHLESS AND WOULD HAVE LOOKED EXACTLY LIKE A REAL ONE.**
The volumetric integration converges over frames; the first frame reads **5.6 % low**:

| frame | mean luma | detail std | PNG bytes | vs control |
|---|---|---|---|---|
| FIX640_r1 | 0.435727 | 0.084889 | 1,788,748 | +66.8 % |
| FIX640_r2 | 0.445936 | 0.083502 | 1,454,249 | +70.7 % |
| FIX640_r3 | 0.460265 | 0.073229 | 1,051,877 | +76.2 % |
| **FIX640_r4** | **0.461514** | **0.072401** | **959,085** | **+76.7 %** |
| FIX640_r5 | 0.460557 | 0.072001 | 999,068 | +76.3 % |

⇒ **BEST-CONVERGED = `0.461514`** (r5 within 0.21 % of it ⇒ converged). **`+76.7 %` over control.**
Detail std collapses **0.1201 → 0.0724 (−40 %)**; PNG bytes fall **4.41 MB → 0.96 MB** (reported as
corroboration, ⛔ never relied on — `SC-§85`).

### 2c. The LIVE PIE WORLD — the vendor `ReceiveBeginPlay` and `ReceiveTick` actually running

PIE control (no fog actor in the duplicated world) 3 frames: `0.261465 · 0.262560 · 0.262917`
⇒ mean **`0.262314`**, spread **0.55 %** — a *live* world drifts far more than the static editor
viewport, so **this** is the floor the PIE row is judged against, not §1's.

PIE with the corrected-scale actor, 7 frames:
`0.430827 · 0.439058 · 0.454128 · 0.478998 · 0.507708 · 0.482275 · 0.469131`
⇒ settled band (last 4) mean **`0.484528`** = **+84.7 %** over the PIE control
(band spans **+78.9 % … +93.6 %**). Detail std **0.1205 → 0.069–0.074**.

---

## 3. THE THREE-WAY SUMMARY

| state | mean luma (linear) | vs its own control | verdict |
|---|---|---|---|
| zero-control, editor, no fog actor | **0.261226** | — | — |
| **BROKEN** achieved scale `(20,20,5)` | **0.261114** | **−0.043 %** | ⛔ inside the ±0.045 % floor — **invisible** |
| **CORRECTED** scale `(640,360,260)` | **0.461514** | **+76.7 %** | ✅ unmistakable |
| PIE live-world control | 0.262314 | — | — |
| PIE live world, corrected scale | **0.484528** | **+84.7 %** | ✅ unmistakable |

**The gap between the broken row and the corrected row is a factor of ~1,780 in effect size**
(0.043 % vs 76.7 %). No converged measurement can be ambiguous about which one is on screen.

---

## 4. 🚨⛔ WHAT I DID **NOT** OBSERVE — READ THIS BEFORE REPORTING TO 🧑 JONATHAN

**I used preference (iii), the FLOOR (`SC-§79` scope statement).** Not (i), not (ii).

### 4a. Why (i) — driving the real runtime spawn — was unreachable

The whole fog chain below the click is **unreflected C++**, so there is no MCP / console / Python door:

- `USpellLibrary` has **ZERO `UFUNCTION`s** (`ResolveSpell` included) — re-verified today.
- `AFogVolume` has **ZERO `UFUNCTION`s** — `RaiseFog`, `RefreshFogVisual`, `SpawnFogVisual` are all
  plain C++. (`TASK-1071` §6 said this; **I re-tested it rather than relaying it**, per the row's
  instruction, and it is still true at `05c3fc0`.)
- The only reflected doors are `ASiegePlayerController::PlayHandSlot` and `EnterTargetingMode`
  (both `BlueprintCallable`) — **and both stop at TARGETING MODE.** Fog is a `FogCover` spell with an
  empty `SpellDelivery` ⇒ not a line spell ⇒ the confirm requires `bTargetingSurfaceValid` **and** a
  literal `WasInputKeyJustPressed(EKeys::LeftMouseButton)` polled in `PlayerTick`
  (`SiegePlayerController.cpp:721-724`). `TryConfirmSpellTarget()` is **private and non-reflected**,
  with exactly one caller: that LMB poll.
- ⇒ the cast needs a **real mouse click into a running game**. **OS-level input injection is blocked
  by this machine's permission policy** (`GetForegroundWindow()` returns `0` from my window station;
  the `SetThreadDesktop` probe was **denied by the classifier**). ⛔ I did not attempt to work around
  it.
- The bot (`ASiegeBotController` rule 3) *is* an input-free `ResolveSpell` caller, but it only casts
  `FireballCardID` / `LightningCardID` from **its own hand**, and Fog has `DeckCount = 0`. Bending
  that would have meant rewriting gameplay tuning to manufacture a result — the opposite of this row.

### 4b. Why (ii) — the read-back log line — was also unreachable

Preference (ii) still requires a real cast to have happened, because the line only prints from inside
`SpawnFogVisual()`. **No cast occurred, so the line never printed.** I searched the whole session log
for `Fog VISUAL|ACHIEVED|WRONG SCALE`: **zero hits.**

⭐ **AND THAT ABSENCE IS THE PROOF THAT I USED A PROXY, RATHER THAN MY WORD FOR IT.** Every fog actor
in this session was created by the **editor Actor Factory** — the log says so verbatim:
`LogActorFactory: Actor Factory spawned Blueprint /Game/Blueprints/BP_SiegeFog … as actor: BP_SiegeFog_C_0`.

### 4c. ⛔ THE ONE LINK STILL UNOBSERVED, STATED PLAINLY

**Nobody has yet watched `SpawnFogVisual()`'s own `SpawnActor` call and read what scale came out of
it.** My corrected-state rows reproduce **the REQUESTED value `(640,360,260)`** — which is precisely
the `SC-§94` cl. A hazard the row warned me about, and I am declaring it rather than dressing it up.

**And I MEASURED that my path does not reproduce the trap:** I asked `add_to_scene_from_asset` for
`(640,360,260)` and got `(640,360,260)` back. The runtime path would have returned `(20,20,5)`.
⇒ **the editor placement path does NOT exercise `SCS_Node.cpp`'s substitution at all.** It is a proxy.

⇒ **Rung 4 NARROWS the gap. It does not close it.** The gap is the same one rungs 1–3 declared, and
it is now the *only* thing between 🧑 him and fog.

### 4d. ⭐ WHAT I *DID* CLOSE — the two hazards that could have made the fix read clean and render nothing

**(1) WARN-1, the vendor `ReceiveTick` re-clobber — DEAD, by two independent methods.**
- *Static:* neither `BP_FogArea.uasset` nor `BP_SiegeFog.uasset` contains `SetRelativeScale3D`,
  `SetActorScale3D`, `SetWorldScale3D` or `K2_SetActorTransform` anywhere in its name table. There
  is **no scale-write node in the vendor graph to run.** (Read off disk — ⛔ no editor load, so
  nothing could dirty.)
- *Empirical:* in a **live PIE world**, with `ReceiveBeginPlay` and `ReceiveTick` genuinely running,
  I read the achieved scale back off the **PIE actor itself**
  (`/Game/Maps/UEDPIE_0_L_Arena.L_Arena:PersistentLevel.BP_SiegeFog_C_1`) **~60 s and thousands of
  frames after BeginPlay**: still **`(640, 360, 260)`**, bounds `min(-32000,-18000,-6000)` /
  `max(32000,18000,20000)`. And the fog got **stronger** over that window, never weaker.
  ⇒ ⛔ **a tick-driven re-clobber does not happen.**

**(2) A hazard NOBODY in this lane had named — the vendor root is `Mobility: Static`.**
A post-spawn `SetActorScale3D` on a *static* primitive is exactly the shape of "the log reads clean
and the pixels never move". **I tested it rather than reasoning about it:** I drove the live,
already-registered, inherited-SCS `Mesh` component **640 → 20 → 640** and the render followed **in
both directions** (0.4615 → 0.2611 → 0.4615). ⇒ the correction's *operation* takes effect and
repaints on this exact component despite Static mobility.

### 4e. WHAT WOULD CLOSE IT — one cast, and it is self-announcing

🧑 **Jonathan plays the Fog card once.** Two outcomes, both decisive, no instrument needed:
- the screen **whites out** ⇒ shipped; and the log prints `ACHIEVED (640, 360, 260)`.
- nothing happens ⇒ the shipped `Error` line fires by itself naming the achieved scale, because
  `SpawnFogVisual` now compares and shouts on mismatch.

---

## 5. THE LIMIT OF MY OWN ANSWER

⛔ **I am answering "DOES IT RENDER?" — and the answer is YES, decisively, at the corrected scale,
including in a live ticking game world.**

⛔ **I am NOT answering "DOES IT READ RIGHT?" That is 🧑 Jonathan's eye, and mine is not a
substitute** (`AS-§6 A(e)`).

### 5a. 🧑 RECOMMENDATION ON LEGIBILITY — offered as a recommendation, ⛔ NOT a verdict

**At `(640,360,260)` with today's level settings, his vantage is a TOTAL WHITE-OUT.** I looked at the
frames, not just the numbers:

- The corrected frame shows **no ground, no army, no enemy gate — a flat beige field.** Detail std
  falls 0.120 → 0.072 (−40 %); the PNG compresses 4.6× smaller because there is almost nothing left
  to encode. Compare `TASK-1075-A` (crisp grass, gate visible at the horizon) with `TASK-1075-C`.
- **The simulation and the picture disagree in an interesting direction.** Units are clamped to
  609.6 uu (87.8 % blind), so "you cannot see" is thematically *right* — but a full white-out also
  removes his ability to **see his own army, read world-space bars, and aim**. The row asked whether
  it is legible "without whiting out his view of his own army". **Measured answer: it does white it
  out.**
- **Where the knobs are, if he wants "dense but navigable" instead of opaque:** the box spans
  Z `−6,000 → +20,000` — his camera at Z ≈ 201 sits **deep inside** it, with 6,000 uu of fog below
  the ground plane contributing nothing but extinction. Lowering the vertical extent, or the vendor
  material's density, would keep silhouettes readable at short range. ⛔ **Both are HIS call and a
  separate row — I changed nothing and I am not recommending a specific value.**

---

## 6. FENCES — verified

✅ **`L_Arena` NEVER saved.** `sha256` **before and after**:
`1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622` — **identical**. No save prompt
was accepted. Editor autosave read back `false` before any actor was placed.
✅ **Both probe actors REMOVED** (`BP_SiegeFog_C_0`, `BP_SiegeFog_C_1`), verified by `find_actors`:
the only fog-named actor left in `L_Arena` is the level's own `ExponentialHeightFog_0`.
✅ **`Content/` is ENTIRELY CLEAN** — `git status --porcelain --untracked-files=all -- Content/`
returns **nothing**, re-checked *after* the vendor template read. ⛔ **No `Content/FogArea/` path went
`M`.** ⛔ No vendor edit, no `recompile`, no connect/disconnect.
✅ **`BP_SiegeFog` not modified** — only instantiated as a level actor and removed.
✅ ⛔ **ZERO `Source/` edits. ZERO Git operations** beyond read-only `status` / `log` / `sha256sum`.
Nothing staged. ⛔ No `checkout --` / `restore` / `stash` / `reset` / `clean`. ⛔ No board row created.
⚠️ **`L_Arena` is dirty in memory** (two transient probes placed and removed). **On-disk package
untouched. ⛔ Decline any save prompt.** Editor left **UP**; PIE stopped.
⚠️ **Naming discrepancy reported, not resolved by me:** the board's `names:` line for TASK-1075 says
`handoffs/TASK-1075-artist.md`; my dispatch instructed `handoffs/TASK-1075-artist-pixel.md`. I wrote
**the dispatched name only** — I did not write both, and I did not edit the `names:` line.

---

## 7. EVIDENCE (promoted; ⛔ NOT committed by me)

`.claude/pipeline/playtest-evidence/2026-09-05/`

| file | what it is |
|---|---|
| `TASK-1075-A-editor-zero-control-no-fog-actor-luma-0.261386.png` | control: no fog actor at all |
| `TASK-1075-B-editor-BROKEN-achieved-scale-20x20x5-luma-0.261125.png` | **what he has been seeing** — indistinguishable from A |
| `TASK-1075-C-editor-CORRECTED-scale-640x360x260-luma-0.461514.png` | best-converged corrected frame |
| `TASK-1075-D-PIE-live-world-control-no-fog-luma-0.261465.png` | live-world control |
| `TASK-1075-E-PIE-live-world-corrected-scale-luma-0.507708.png` | live world, vendor tick running |

**Render context, declared explicitly:** every frame is an **editor level-viewport scene capture**
(the axis gizmo is visible in each). The `PIE*` rows were captured **while a PIE session was live and
the level viewport was showing the PIE world** — confirmed independently, because the MCP scene tools
resolved to `/Game/Maps/UEDPIE_0_L_Arena…` during those captures. ⛔ **I have never observed 🧑 his
standalone process's own framebuffer and I am not implying otherwise.**

Working PNGs (session scratch, not committed): `…\scratchpad\fog3\`.
