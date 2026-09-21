Verdict: VERIFIED
# Verification — TASK-1352 [AGENT-AIM-LANE-PROBE]

> **⛔ NARROWING, ON THE FACE OF THE REPORT (`VER-§1` cl. 7 shape). What this `VERIFIED` attests to is
> EXACTLY this: the probe's own acceptance lines (A1–A5) all executed, the positive instrument control
> fired BEFORE the treatment, and the row's question got a **YES**. It does **NOT** attest that a
> shippable aim mechanism exists — the aim point I produced is **TRANSIENT and does not latch** (§4),
> and that limit is measured, quoted, and is `TASK-1353`'s to price.**
>
> **⛔ `MEASURED` WOULD BE WRONG HERE AND I CHECKED RATHER THAN REACHING FOR THE NEW TOKEN.** `VER-§1`
> cl. 3a defines `measured` as a **CONTROLLED NEGATIVE** — *"the observable DID NOT MOVE"*. **My
> observable MOVED.** Under cl. 5a's precedence order, branch (2) (no `fail`, ≥1 `pass`) wins and line 1
> reads `VERIFIED`. `fail` was unavailable by construction (no code under test), and nothing failed.

## 🚨 THE ONE-LINE ANSWER, FIRST

**YES. A granted write produces an aim point.** Not `set_actor_property` — **`pie_scene_edit`'s
`call_actor_function` verb**, calling `APlayerController::SetMouseLocation(X, Y)`, which is a shipped
`UFUNCTION(BlueprintCallable)` whose body is `Viewport->SetMouse(X, Y)`.

⇒ **`TASK-1348`'s `P9` conclusion was an INCOMPLETE NEGATIVE, exactly as `TASK-1349` WARN-B predicted.
Mechanism (i) — move the REAL viewport cursor so `GetMousePosition` is UNTOUCHED — is NOT shut.
It has a route, and the route was driven twice in two sessions with the camera frozen.**

Editor/Aura state: connected **y** · map `/Game/Maps/L_Arena.L_Arena` (confirmed before **and** after the run) ·
PIE mode **standalone, 1280×720 requested, `client_index 0`** · editor instance **PID 27484, command line `''` (EMPTY), `-game` ABSENT ⇒ GUI editor**, project `GitClaudeUnrealTest.uproject`; identification is
**`PARTIAL, by SC-§118 cl. 9`** — I hold no shell, no `subprocess`, no `ctypes`, so I identified the instance I am
attached to from inside it (in-process PID + empty command line + project path + `-game` absent) and did **not**
enumerate other processes. **Nobody may fail this run for that (cl. 9).** ·
attempts used **2 of 3** (two PIE sessions; both kept and named, `VER-§1` cl. 6) ·
PIE wall/clock: session A `t=30.07 s → 138.29 s`, session B `t=10.43 s → 14.34 s` ·
**No editor lifecycle action was taken. PID 27484 was up before, during and after, and is up now.** ·
Credit visible in-frame: `Gold: 60` (session A, t=50.31 s), `Gold: 24` (session B, t=14.34 s) ·
⚠️ environment condition visible on both frames: red banner **`Video memory has been exhausted (172.082 MB over budget). Expect extremely poor…`** — recorded, not diagnosed. ·
No editor log line is quoted in this report, so no log file is named (`VER-§1` cl. 2 is satisfied vacuously).

---

## Acceptance lines → observations

| # | acceptance line | observable chosen | observed (quoted values, evidence path) | pass/fail/unobs |
|---|---|---|---|---|
| A1 | **"⛔ READ THE SURFACE ⛔ BEFORE YOU WRITE TO IT. `get_actor_property_in_pie` on the ⛔ PLAYER CONTROLLER ⇒ ⛔ print the property names it actually returns."** | `get_actor_property_in_pie(name="SiegePlayerController0", discover=true)` — **`discover` is NOT a valid param inside `run_verification_sequence`** (validator: *"unknown param 'discover'. Valid params: ['client_index', 'component', 'name', 'properties']"*), so it was taken as a standalone read at `t=117.36 s` | **81 readable properties returned** on `actor_class: SiegePlayerController`. The **complete cursor-adjacent set, quoted**: `bShowMouseCursor` (uint8) · `bEnableClickEvents` (uint8) · `bEnableTouchEvents` · `bEnableMouseOverEvents` · `bEnableTouchOverEvents` · `ClickEventKeys` (TArray) · `DefaultMouseCursor` (`TEnumAsByte<EMouseCursor::Type>`) · `CurrentMouseCursor` (same) · `DefaultClickTraceChannel` (`TEnumAsByte<ECollisionChannel>`) · `CurrentClickTraceChannel` (same) · `HitResultTraceDistance` (float). ⇒ **Every one of them governs WHETHER or HOW a cursor trace happens. NOT ONE is a screen position.** The nearest-to-position names on the whole surface are `RootComponent`, `PivotOffset` and `Instigator` — all world-space actor state. **This is an ENUMERATED statement, not a guessed "not found"** | **pass** |
| A2 | **"⛔ THE POSITIVE INSTRUMENT CONTROL — 🚨 ⛔ TAKE IT ⛔ BEFORE THE TREATMENT, ⛔ NOT AFTER: ⛔ perform ⛔ ONE property write via `set_actor_property` whose ⛔ READ-BACK ⛔ DEMONSTRABLY CHANGES"** | `set_actor_property` on `SiegePlayerController0` → `HitResultTraceDistance` (deliberately chosen: it is the float `GetHitResultUnderCursor` itself reads, so the control proves the lane reaches **the very object under probe**; raised 100000→123456 so it can never cause a miss) | **THE LANE IS ALIVE.** Write at `t=49.76 s` returned `{"op":"set_actor_property","actor":"SiegePlayerController0","property":"HitResultTraceDistance","previous_value":"100000.0","applied_value":"123456.0","ok":true}`. **Independent read-back at `t=49.78 s`: `{"name":"HitResultTraceDistance","value_number":123456,"value":"123456.0"}`**, and still `123456.0` at `t=50.30 s`. Baseline read at `t=30.07 s` had been `"100000.0"`. ⛔ **Taken BEFORE the treatment (`t=49.76` < `t=49.79`), as the line demands.** ⇒ every null in this row would have been a real null, not a dead instrument | **pass** |
| A3 | **"⛔ THE TREATMENT — ⛔ DOES ⛔ ANY granted write change what `GetHitResultUnderCursor` READS? … `GhostActor.bHidden` ⛔ + `GhostActor.RootComponent.RelativeLocation`, ⛔ with placement mode ⛔ PROVABLY ACTIVE … THE NEGATIVE BASELINE IS ⛔ ALREADY MEASURED: `bHidden: true` at `(0.000000,0.000000,0.000000)`"** | the same live per-frame readout `P9` used, re-baselined **in-session** | 🚨 **IT MOVED.** Baseline at `t=49.78 s`, **after the control write and before the treatment**, with placement mode provably active (`GhostActor` = `StaticMeshActor_34`, non-`None`): **`bHidden: true`, `(X=0.000000,Y=0.000000,Z=0.000000)`** — `P9`'s exact negative, reproduced. Treatment at `t=49.79 s`: `call_actor_function` `SetMouseLocation` `{X:640,Y:360}` → `{"actor":"SiegePlayerController0","function":"SetMouseLocation","ok":true}`. Observable at `t=50.30 s`, **0.5 s later**: **`bHidden: false`, `(X=-23270.310680,Y=2639.999850,Z=1591.671238)`**. ⇒ **`TraceCursorToGround` returned TRUE.** Corroborated by a 3-point sweep at a **byte-identical camera** and by a **rendered green ghost** (§2, §3; `…-a2-aim-point-latch-check_t14.34s_f1890177.png`) | **pass** |
| A4 | **"⛔ `pie_scene_edit` — ⛔ ENUMERATE ITS ⛔ OWN VERBS AND SAY WHICH ONE COULD PLAUSIBLY BEAR ON A ⛔ CURSOR OR A ⛔ SCREEN POINT, ⛔ THEN TRY ⛔ ONLY THOSE."** | the tool's own verb list, **quoted not summarised**, + a reasoned exclusion, + execution of only the two survivors | **The seven verbs, verbatim from the tool's schema:** `"teleport_player"` · `"spawn_actor"` · `"delete_actor"` · `"set_actor_transform"` · `"set_actor_enabled"` · `"set_actor_property"` · `"call_actor_function"`. **Which could bear on a cursor or a screen point:** `teleport_player` / `set_actor_transform` move **world** transforms — that is `P9` attempt 1's lane (`set_player_transform` at three pitches), a **NAMED DEAD END not re-traced** (`VER-§5` cl. 5) · `spawn_actor` / `delete_actor` create or destroy **world** objects — no screen point exists to write · `set_actor_enabled` toggles tick/visibility — no screen point · **`set_actor_property` COULD, if a screen-position UPROPERTY existed — A1 enumerated the surface and none does** · ⭐ **`call_actor_function` CAN, and DOES** — it reaches `UFUNCTION`s, and `APlayerController::SetMouseLocation` is one. **Only those two were tried.** Arg schema learned from the tool's own errors: the actor key is **`name`** (not `actor`) — *"set_actor_property requires 'name'"*, *"call_actor_function requires 'name'"*; args go in **`args`** — *"function 'SetMouseLocation' takes parameters and no 'args' were given. Expected: X: int32, Y: int32"*; `function_name` is rejected — *"call_actor_function requires 'function'"* | **pass** |
| A5 | **"⛔ `bShowMouseCursor` IS ⛔ ALREADY `true` AND THAT IS ⛔ NOT THE QUESTION (⛔ `P9` measured it) … ⛔ A row that reports *'the cursor is enabled'* has ⛔ answered a question nobody asked."** | honoured as a prohibition — and the boarded premise was re-measured rather than inherited (`SC-§91`) | ⚠️ **THE BOARDED PREMISE DOES NOT HOLD AT MY INSTANT, AND I REPORT IT AS A CORRECTION, NOT AS AN ANSWER.** At `t=30.07 s`, in-match, before any injection: **`{"name":"bShowMouseCursor","value_bool":false,"value":"false"}`** and `{"name":"bEnableClickEvents","value_bool":false,"value":"false"}`. `P9`'s `true` was read **while `IA_UICursor` was being HELD** (its attempt 2); the resting state is `false`. ⛔ **This report asserts nothing from that flag** — and it never became the observable. It matters only because §5 names it as the leading suspect for why the aim point does not latch | **measured** |

**Derivation (`VER-§1` cl. 5a, precedence order, first match wins):** no `fail` (unavailable by construction — there is no code under test) · **≥1 `pass` ⇒ branch (2) ⇒ `VERIFIED`** · the single `measured` cell (A5) is listed under *Not examined / limitations* with its control, exactly as cl. 5a directs.

---

## §1 — THE MECHANISM, TRACED TO ENGINE SOURCE BEFORE IT WAS DRIVEN (`SC-§101`: a claim, not a guess)

`C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Engine\Classes\GameFramework\PlayerController.h:756-758`:

```cpp
	/** Positions the mouse cursor in screen space, in pixels. */
	UFUNCTION( BlueprintCallable, Category="Game|Player", meta = (DisplayName = "Set Mouse Position", Keywords = "mouse" ))
	ENGINE_API void SetMouseLocation( const int X, const int Y );
```

`…\Engine\Source\Runtime\Engine\Private\PlayerController.cpp:2291-2305` — the body resolves the local player's
`UGameViewportClient`, takes its `FViewport*`, and calls **`Viewport->SetMouse( X, Y )`**.

⇒ This is **not** a parallel aim source and **not** a second code path. It writes the **real viewport cursor**,
which is precisely the state `UGameViewportClient::GetMousePosition` reads and therefore the state
`GetHitResultUnderCursor` → `ASiegePlayerController::TraceCursorToGround` consumes. **`GetMousePosition` is
UNTOUCHED — that is the definition of mechanism (i).**

---

## §2 — THE DISCRIMINATION: THREE SCREEN POINTS, ONE FROZEN CAMERA (session A, `t=81.22 s → 82.57 s`)

The camera-drift control is **byte-identical**, read immediately before and immediately after the sweep:
`location {x:-21007.81640625, y:0, z:98.15000009536743}` · `control_rotation {pitch:0, yaw:180, roll:0}` ·
`camera_location {x:-20607.81640625, y:-4.898587196589408e-14, z:98.15000009536743}` ·
`camera_rotation {pitch:0, yaw:180, roll:0}` · `velocity.speed: 0` · `view_target: "BP_HeroCharacter0"`.
**Nothing about the camera changed between the first and last reading — so nothing about the camera can explain the ghost moving.**

| `SetMouseLocation(X,Y)` | `GhostActor.bHidden` | `GhostActor.RootComponent.RelativeLocation` | ground distance ahead of camera |
|---|---|---|---|
| `(200, 500)` — left, LOW on screen | `false` | `(X=-21014.583111, Y=157.166193, Z=0.000000)` | **≈ 407 uu — NEAREST** |
| `(640, 360)` — centre | `false` | `(X=-21256.557317, Y=305.019975, Z=0.000000)` | ≈ 649 uu |
| `(1000, 300)` — right, HIGH on screen | `false` | `(X=-22470.000122, Y=-1043.143854, Z=275.860003)` | **≈ 1862 uu — FARTHEST** |

⭐ **The depth ordering is the signal, and it is the ordering a real cursor projection produces:** lower on
screen ⇒ the ground ray strikes nearer; higher on screen ⇒ it strikes farther. A constant, a cached value, or
"any call nudges it" cannot produce a monotone depth ramp keyed to screen Y. **Three distinct inputs, three
distinct, geometrically consistent outputs, at a camera that did not move.**

---

## §3 — THE PIXEL CORROBORATION (session B, `t=14.34 s`)

`GhostActor.bHidden: false` at `(X=-21382.831129, Y=407.684206, Z=29.000000)` — close enough to the camera to
be in frame, and **the frame shows it**: a translucent **green humanoid placement ghost holding a sword**,
standing on the pale stone ledge at mid-left, clearly left of the cloaked hero.

🚨 **This is the first frame in this entire wave on which a placement ghost is rendered at all.** Both of
`TASK-1348`'s promoted frames were taken with placement mode provably active and show **no ghost anywhere**
(`TASK-1349` §5 opened both and confirmed it independently). **Same map, same hero, same castle arch, same HUD —
one difference: `SetMouseLocation` had been called.**

---

## §4 — 🚨 THE LIMIT, MEASURED: THE AIM POINT DOES **NOT** LATCH

**This is the finding `TASK-1353` most needs and it is a cost, not a defect.** A single `SetMouseLocation`
does not hold the aim point; the trace reverts to a miss within about a second.

Session B, camera byte-identical throughout (`{x:-21007.81640625, y:0, z:98.15…}`, `camera_rotation {0,180,0}` at `t=14.33 s`):

| PIE time | what happened | `bHidden` | `RelativeLocation` |
|---|---|---|---|
| `t=10.99 s` | placement entered; **no set yet this session** | `false` | `(X=-23071.000023, Y=1265.141468, Z=1414.891231)` |
| `t=11.23 s` | **0.2 s after** `SetMouseLocation(300,420)` | `false` | `(X=-21237.191857, Y=-240.247828, Z=0.000000)` |
| `t=12.06 s` | **+0.8 s, no further set** | 🚨 **`true`** | `(X=-20974.495991, Y=-350.495128, Z=0.000000)` *(stale — the ghost is only re-located on a hit)* |
| `t=14.08 s` | **+2.0 s, no further set** | `false` | `(X=-23390.000070, Y=-440.352548, Z=160.749066)` |
| `t=14.31 s` | **0.2 s after re-issuing the SAME `(300,420)`** | `false` | `(X=-21382.831129, Y=407.684206, Z=29.000000)` |

**Two things follow, and I state both:**
1. **A repeat of an identical coordinate does not reproduce an identical world point.** `(300,420)` twice gave
   `Y=-240.25, Z=0.00` then `Y=+407.68, Z=29.00` — ≈648 uu apart at an unmoved camera. Session A's repeat of
   `(640,360)` was worse: it read back **`bHidden: true`**.
2. **Between sets the observable wanders on its own** (`t=12.06` miss, `t=14.08` a hit 2400 uu from the set point).

⇒ **The aim point is DRIVEN by the write but is CONTESTED by something else every frame.**

---

## §5 — THE CURSOR MOVED FOR REAL: IT SURVIVED A PIE TEARDOWN

⭐ Session B's **very first** ghost read — `t=10.99 s`, **before any `SetMouseLocation` in that session** — was
already **`bHidden: false`** at `(X=-23071.000023, Y=1265.141468, Z=1414.891231)`, i.e. **already a valid ground
hit**, where session A had opened from the dead `(0,0,0)` baseline.

Between those two readings PIE was **stopped and restarted** (`stop_pie` at session-A `t=152.59 s`; new session
began at `0`), which destroys the viewport, the `UGameViewportClient` and every cached value inside them.
**A merely-cached number cannot survive that. The OS cursor position can, and did.** ⇒ strong corroboration that
`SetMouseLocation` moved **the real pointer into the PIE window**, not an internal proxy —
which is the whole of what mechanism (i) requires.

⚠️ **Cost of that, stated rather than hidden: session B therefore had NO clean origin baseline of its own.** Its
role in this report is the latch measurement (§4) and the pixel corroboration (§3); **the controlled
before/after that carries A3 is session A's**, where the `(0,0,0)` baseline was taken in-session, after the
control and before the treatment.

---

## Evidence (promoted)

All three frames are in `.claude/pipeline/playtest-evidence/2026-09-20/`.
⚠️ **NAMING DEVIATION, DECLARED (`VER-§4` cl. 2):** I passed the `VER-TASK-1352…` base name, but
`capture_pie_frame` appends its own frame stamp (`_t50.31s_f1883165`) and I hold no shell to rename. The paths
below are **the real bytes on disk**; the `-t<MM>m<SS>s` re-spelling is **owed to the host row**, and I claim no
path that does not exist. No slug contains "pass" or "fail".

- `.claude/pipeline/playtest-evidence/2026-09-20/VER-TASK-1352-a1-ghost-after-setmouselocation_t50.31s_f1883165.png`
  — composited, 1086×615, PIE `t=50.31 s`, frame `1883165`, `mean_luma 135`. Cloaked white-and-red-cross hero centred
  on grass before the lit castle arch, two wall sconces burning, `Gold: 60` top-left, `59 FPS / 16.8 ms` and `0/6`
  top-right, red banner `Video memory has been exhausted (172.082 MB over budget)…` across mid-frame.
  **No ghost in frame — and that is consistent, not contradictory:** the trace point at this instant was
  `(-23270.31, 2640.00, 1591.67)`, ≈44.8° off-axis and ≈1493 uu above a pitch-0 camera, i.e. **outside the frustum**.
  The `bHidden:false` fact is carried by the property read; this frame is the posture record.
- `.claude/pipeline/playtest-evidence/2026-09-20/VER-TASK-1352-a1-ghost-visible-at-set-cursor-point_t82.59s_f1885085.png`
  — composited, PIE `t=82.59 s`, frame `1885085`, `mean_luma 136`. The frozen-camera posture for the §2 three-point
  sweep. Taken 0.03 s after the read that had gone `bHidden: true`, so **no ghost is rendered**; kept as the
  sweep's posture evidence (`VER-§1` cl. 6 — every attempt's evidence is kept, not only the flattering one).
- `.claude/pipeline/playtest-evidence/2026-09-20/VER-TASK-1352-a2-aim-point-latch-check_t14.34s_f1890177.png`
  — ⭐ **THE LOAD-BEARING FRAME.** Composited, PIE `t=14.34 s`, frame `1890177`, `mean_luma 136`. A translucent
  **green humanoid placement ghost with a sword**, standing on the pale stone ledge at mid-left of frame, to the
  hero's left — matching `bHidden:false` at `(-21382.83, 407.68, 29.00)`. `Gold: 24` top-left, `60 FPS / 16.7 ms`
  and `0/6` top-right, card-name HUD strip bottom-centre. **The first rendered placement ghost of this wave.**

---

## Hypotheses (not verdicts)

- **H1 — why the aim point does not latch.** `bShowMouseCursor` reads **`false`** at rest (A5). With the software
  cursor off, the viewport keeps the pointer captured for mouse-look and re-asserts its position every input
  frame, overwriting the warp. **Prediction that would test it:** hold `IA_UICursor` (which `M5` shows sets
  `SetIgnoreLookInput(true)`) or set `bShowMouseCursor: true` via `set_actor_property` — **a lane A2 proved is
  alive** — then re-issue `SetMouseLocation` and see whether the point holds across ≥2 s. **NOT MEASURED.**
- **H2 — why repeats scatter so widely.** The camera sat at `pitch: 0` (dead horizontal) for every reading, so the
  ground ray is **grazing**; sub-pixel cursor jitter is amplified into hundreds of uu of world distance. A
  downward pitch would compress the same jitter. **NOT MEASURED** — and note re-pitching the camera is `P9`
  attempt 1's lane, which `VER-§5` cl. 5 forbids me to re-trace **for its original purpose**; used for *this*
  purpose it would be a new probe, and it is `TASK-1353`'s to board if it wants it.
- **H3 — for the implementation row, offered as a constraint and NOT as a mechanism ruling (`SC-§82`, and spec (0):
  I measure, `TASK-1353` rules).** Everything above bears on **(i)** only. I propose nothing, and I have not
  written, hinted at, or costed **(ii)**, and certainly not **(B)**.

---

## Not examined / limitations this run

- **A5 is scored `measured`, with its control:** the flag was read on a live controller in the same posture that
  produced every other number in this report, so the read fired; the reading (`false`) simply **contradicts the
  boarded premise**. It never became an observable and nothing here rests on it.
- **THE LATCH IS NOT SOLVED, ONLY MEASURED (§4).** I established that a single write produces an aim point and
  that it does not persist. **I did NOT establish any way to make it persist.** A row that reads this report as
  *"the cursor is settable, ship it"* has made the `TASK-1286` error the whole wave exists to avoid.
- **The persistence probe in session A returned NO signal and is `unobs`:** the bot killed the undriven hero
  between `t=82.6 s` and `t=135.6 s` (`pawn_class` `BP_HeroCharacter_C` → **`BP_SiegeGhostPawn_C`**,
  `view_target` → `BP_SiegeGhostPawn0`), placement mode tore down and `GhostActor` went to `None`
  (*"Path segment 'GhostActor' is a null object reference"* ×3). **The kill landed EARLIER than the boarded
  ≈2m50s ceiling** — this is why session B exists, front-loaded to `t≈11 s`.
- **Only ONE of `TraceCursorToGround`'s three callers was exercised** — the placement ghost. The spell reticle and
  the group-pick reticle were **not probed** and I assert nothing about them (same restraint as `P10`).
- **`DeprojectMousePositionToWorld` (`M3`, hero targeting — the SECOND cursor read, not routed through the choke
  point) was NOT probed.** It is *likely* to benefit from a real cursor for the same reason, but **likely is not
  measured** and `TASK-1353`'s standing instruction on that site is unaffected by this report.
- **No accuracy claim is made.** I did not establish that screen point *(X,Y)* maps to any *specific* world point,
  only that distinct points map to distinct, depth-ordered world points (§2). The viewport's true pixel
  dimensions were never read this run (`start_pie` was asked for 1280×720; capture returned a downscaled
  1086×615), so **`(640,360)` is not proven to be the viewport centre** — and at "centre" the hit was `Y=305.02`,
  not `Y≈0`, which is consistent with that unknown.
- **Named dead ends inherited by reference, NOT re-traced** (`VER-§5` cl. 5): `set_player_transform` across three
  pitches · `IA_UICursor` held · `simulate_key_press` LMB · `ui_snapshot` / `ui_perform` · `L_MainMenu` · Slate.
- ⛔ **Shell / `subprocess` / `ctypes` routed through the read-only inspector — DECLINED, by name. Nine
  predecessors declined it, `TASK-1349` made it ten; I MAKE IT ELEVEN, and I say so.**
- ⛔ **No new tool or MCP grant was reached for or is sought** (ceiling 4).
- **No acceptance line in this row depended on `simulate_button_press`**, whose surface remains unmeasured
  (`TASK-1349` §9).

---

## Fences — measured, not asserted

- **`.sav` NET ZERO, proven by sha256 AND mtime, all 5 files, before and after:**
  `SiegeAccounts.sav` `2fd96fe1…d442b1` / `2026-08-28T22:43:52.307531` · `SiegeDecks.sav` `646d442f…4039a071` /
  `2026-08-02T10:09:34.952297` · `SiegeDecks_4E46A9EE….sav` `d81f2b64…8815c65` / `2026-09-20T01:22:04.358541` ·
  `SiegeSettings.sav` `c8555088…cc06f2e7` / `2026-08-04T22:24:38.554086` · `SiegeSettings_4E46A9EE….sav`
  `684ae64f…eeb41793` / `2026-09-09T13:48:19.329635`. **Every hash and every mtime identical. `SAV_COUNT` 5 → 5.**
- **Never-save law:** `L_Arena.umap` sha256 `1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622`,
  mtime `2026-09-05T00:42:58.649188`, 605098 B. **`DIRTY_PACKAGES = NONE`** after the run. No save prompt was
  answered because none was raised.
- **Editor:** PID **27484** before and after, command line **`''`**, `-game` **absent**, level `/Game/Maps/L_Arena.L_Arena`
  before and after. **No launch, no close, no restart, no Live Coding.**
- **Scope:** no code, no asset, no compile, no git, no `CONVENTIONS.md`, no other row's line. My only writes are
  this file, my own `status:` line on `TASKBOARD.md`, and the three evidence PNGs.
