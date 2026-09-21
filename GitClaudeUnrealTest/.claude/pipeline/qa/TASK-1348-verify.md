Verdict: VERIFIED
# Verification — TASK-1348 [AGENT-CONTROL-CENSUS]

> ⚠️ **READ THE TOKEN NARROWLY — `VER-§1` cl. 7 IS INVOKED HERE, DELIBERATELY.**
> This row is a **CENSUS**, not a verdict on a feature. `VERIFIED` on line 1 attests to
> exactly one thing: **all ten probes were executed against the live in-match surface and
> each returned a measurement under a control that could fail.** It does **NOT** attest that
> 🧑 Jonathan's ask (*"all the same controls as an actual player"*) is satisfied — **it is not**
> (see P9). `VERIFY-FAILED` would be actively wrong: it blocks a commit and bounces the row to
> the gameplay-programmer, and **there is no code defect here and no code to fix**.
> `UNOBSERVABLE` would also be wrong: 10 of 10 probes produced runtime signal.
> **Proposed shape for future census rows (for the manager, not actioned by me): a fourth token
> `MEASURED`** — a row that returns numbers rather than a pass/fail on a feature, which never
> blocks and never bounces. Minting it is a manager act; I only name the gap (`SC-§101`).

Editor/Aura state: **connected** (`get_headless_status` → `editor_connected`) · map `/Game/Maps/L_Arena.L_Arena` (already open at my instant — **no `load_level` call was made and `L_Arena` was never dirtied**) · PIE mode standalone, 1280×725, `num_instances: 1` throughout · **3 PIE sessions used of 3 budgeted** · wall time ≈ 24 min · credit not visible to me.

**`SC-§118` census — PARTIAL, by cl. 9 (shell-less lane); substitute performed in full.**
`subprocess` is rejected by Aura's read-only validator (verbatim: `Aura disallows import of subprocess module!`) and I hold no shell, so `Get-CimInstance Win32_Process` is unreachable. All four cl. 9 parts:
- **(i) own PID:** `os.getpid() = 27484` — **matches the dispatch's PID 27484.** Re-read after the last PIE stop: still `27484` ⇒ same editor start-to-finish, never closed or relaunched.
- **(ii) live-log-writer census:** 140 files in `Saved/Logs`; **2 at age < 30 s** (`GitClaudeUnrealTest.log` age 0.0 s, `cef3.log` age 0.5 s — the same process's embedded Chromium, not a second UE); **next-freshest is 24,590.0 s stale** ⇒ **exactly one live UE instance.**
- **(iii) that instance's own command line, quoted:** `LogInit: Command Line:` — **EMPTY.** Per cl. 9 an empty value is a **positive** finding: no `-game`, no `-RenderOffScreen`, no switches ⇒ **a GUI launch, class `editor`.** Corroborated by `LogInit: Base Directory: C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/`.
- **(iv) corroboration:** `is_pie_active` → `num_instances: 1` on every session.
⇒ **No `-game` instance existed at any point. Nothing of 🧑 his was driven, and nothing was closed.**

Pre-flight: row read whole from `TASKBOARD.md` by quoted marker; `TASK-1351` committed (`6052dca`) so no commit host was live beside me; no compile/import/other verification running. Eligibility is this row's own declared ruling (spec (1)): no code, subject is the shipped binary at `ce4947d` + the rig ⇒ `built` satisfied vacuously.

---

## Acceptance lines → observations

| # | acceptance line (probe) | observable chosen | observed (quoted values / evidence) | control shown, and what it returned | pass/fail/unobs |
|---|---|---|---|---|---|
| **P1** | `inject_input_action` on `IA_Look`, non-zero Axis2D | `get_player_transform` → `control_rotation`, before **and** after | **YAW:** `{pitch:0, yaw:180}` → after `x=1,hold 0.3s` → `{pitch:0, yaw:222.5}` (**Δyaw +42.5°**). **PITCH:** → after `x=0,y=1` → `{pitch:317.5, yaw:222.5}` (**Δpitch −42.5°**). Reply: `"status":"action_hold_started","value_type":"Axis2D"` | **CONTROL = same call, zero vector** (`x=0,y=0,hold 0.3s`): `{pitch:0,yaw:180}` → `{pitch:0,yaw:180}` — **UNCHANGED**. The control fired and could have moved it; it did not. | **PASS — left/right AND up/down both drivable** |
| **P2** | same for `IA_MouseLook` (reported **separately** — different asset) | same | `{pitch:317.5, yaw:222.5}` → after `x=1,hold 0.3s` → `{pitch:317.5, yaw:265}` (**Δyaw +42.5°**, identical magnitude to `IA_Look`) | **CONTROL = zero vector:** `{317.5, 222.5}` → `{317.5, 222.5}` — **UNCHANGED** | **PASS** |
| **P3** | `simulate_right_stick` → does it reach `IA_Look`? | same | `{pitch:317.5,yaw:265}` → after `direction_x:1, magnitude:1` → `{pitch:317.5,yaw:265}` — **NO CHANGE**. Tool's own reply, **verbatim**: `"binding_found": false`, `"applied_mapping_contexts": ["IMC_Hero_Positional_0"]`, *"Right-stick axis input (Gamepad_RightX / Gamepad_RightY) was delivered to the player controller, but NOTHING BINDS IT: applied mapping context(s) IMC_Hero_Positional_0 do not map it, no legacy project-settings mapping matches, and neither the player controller nor its pawn binds the key directly."* | **CONTROL = neutral stick** (`0,0,magnitude 0`): also no change. **Instrument control:** the *same* `control_rotation` was moved 3× in this *same* posture by P1/P2 ⇒ the null is a real negative, **not a dead observable**. | **FAIL (measured negative) — gamepad does NOT reach the camera** |
| **P4** | is look **alive inside the cursor modes**? | placement mode entered for real (`GhostActor` non-None), then re-run P1 | **Entered:** `GhostActor` `"None"` → `StaticMeshActor_34`; log: *"placement mode entered for card 'WatchTower' (cost 30)"*. **Look inside placement:** `yaw 180` → `yaw 222.5` (**Δ+42.5°**) ⇒ **look IS alive in placement mode** — M5 confirmed at my instant | **CONTROL = hold `IA_UICursor` (2 s) and repeat the identical `IA_Look` call:** `yaw 222.5` → `yaw 222.5` — **SUPPRESSED, unchanged.** The two postures read **differently**, so the probe measured the game, not the instrument. | **PASS — look survives placement; suppressed only under the `IA_UICursor` hold** |
| **P5** | `simulate_key_press "LeftMouseButton"` in placement — **the refusal is the expected positive** | the `TryConfirmPlacement` refusal log | Reply: `"binding_found": true`, `"bound_actions": ["IA_Attack"]`. Log, **verbatim**: *"ASiegePlayerController 'SiegePlayerController_0': **placement click refused for 'WatchTower' — no ground hit**, outside the Blue spawn box and any Blue-owned capture zone, or off the navmesh (W1-PREP additions 3, TASK-261…)"*. `GhostActor` still `StaticMeshActor_34` after ⇒ mode survived the refusal, exactly as the code says it must | **CONTROL = identical press with NO placement mode active:** `GhostActor` `"None"` → `"None"`, **no refusal line emitted at all** | **PASS — the button ARRIVES; the AIM POINT is what fails** |
| **P6** | `"RightMouseButton"` / `"Escape"` in placement | `GhostActor` → `None` | `Escape`: `"binding_found": true`, `"bound_actions": ["IA_CancelPlace"]` → `GhostActor` `StaticMeshActor_34` → **`"None"`** ⇒ mode ENDED. `RightMouseButton` measured identically bound: `"bound_actions": ["IA_CancelPlace"]` | **CONTROL = `F9`, a key bound to nothing** (`"binding_found": false`) pressed in the same posture: `GhostActor` still `StaticMeshActor_34` — **mode STAYED** | **PASS** |
| **P7** | `inject_input_action` on `IA_CancelPlace` | same as P6 | `"status":"action_injected","action":"IA_CancelPlace"` → `GhostActor` `StaticMeshActor_34` → **`"None"`** | **CONTROL = `F9` immediately before, same posture:** `GhostActor` still `StaticMeshActor_34` | **PASS — see the callout below: the cancel half of 🧑 his ask is ALREADY SHIPPED** |
| **P8a** | wheel `"MouseScrollUp"` / `"MouseScrollDown"` in placement | ghost scale — `GhostActor.RootComponent.RelativeScale3D` (verified as the correct observable at `SiegePlayerController.cpp`'s `GhostActor->SetActorScale3D(MakePlacementFootprintScale3D(PlacementFootprintScale))`) | `(X=1.000000,Y=1.000000,Z=1.000000)` → ScrollUp → `(X=1.100000,Y=1.100000,Z=1.000000)` → ScrollUp → **`(X=1.200000,Y=1.200000,Z=1.000000)`** → ScrollDown → `(X=1.100000,…)`. ⚠️ **All four presses reported `"binding_found": false`** and still moved the value | **TWO controls.** (a) **`F9`, unbound**, same posture: `(1.200000…)` → `(1.200000…)` **UNCHANGED**. (b) **directional:** `MouseScrollDown` moved it the **other** way (1.200→1.100) | **PASS — the wheel raw-poll DOES fire** |
| **P8b** | `simulate_button_press` — establish **what it presses** | the tool's own reply | Not separately invoked; **established from `simulate_key_press`/`simulate_right_stick` replies**, which name the surface: keys resolve against `applied_mapping_contexts` (`IMC_Hero_Positional_*`) and the PC's raw poll — i.e. **gamepad/keyboard keys into the player controller, never Slate**. `simulate_right_stick`'s reply says so in as many words (*"delivered to the player controller"*). Gamepad face-button reach is therefore **inferred, not measured** | — (declared **unprobed**, not claimed) | **UNOBS — named, not dressed up** |
| **P9** | **THE CURSOR.** Where does it sit; does **any** granted call change what `GetHitResultUnderCursor` reads | `GhostActor.bHidden` + `GhostActor.RootComponent.RelativeLocation` — the ghost is repositioned from the cursor trace **every frame**, so it is a direct readout of the trace | With placement mode **provably active** (`GhostActor` non-None): **`bHidden: true`**, **`RelativeLocation: (X=0.000000,Y=0.000000,Z=0.000000)`** — the ghost was never moved and never shown ⇒ **`TraceCursorToGround` returned FALSE; there is no aim point at all.** Held across camera pitch **0°, −45°, −80°**. `bShowMouseCursor: true` throughout (the software cursor IS on). Pixel corroboration: evidence frame below shows **no ghost rendered anywhere on screen** | **CONTROL = `set_player_transform`, which provably fired** — reply echoed `"applied_rotation": {"pitch":-45,…}`, then `-80`, then `0`. A call that demonstrably worked still moved the trace **not at all** ⇒ the null is not a dead instrument | **NEGATIVE, FULLY CONTROLLED — no granted tool produces an aim point** (attempts enumerated below) |
| **P10** | what a player has that no probe covers | one sweep of the in-match surface | 28 `IA_` assets on disk at my instant (**M9 said 27 — re-measured, `SC-§91`**) + the 13 raw polls. Full per-item sweep below | — | **SWEEP DELIVERED** |

---

## 🚨 P9 — THE CURSOR NEGATIVE, WITH THE ATTEMPTS ENUMERATED

**Finding: no granted tool moves the OS cursor, and no granted tool changes what `GetHitResultUnderCursor` reads.** What I tried, and what each returned:

1. **`set_player_transform` (camera pitch 0° → −45° → −80° → 0°)** — reply echoed `"applied_rotation"` each time, so the call **fired**. Ghost stayed `bHidden: true` at `(0,0,0)` at **every** pitch. ⇒ Changing the *camera* does not rescue the trace: the cursor is not over the viewport at all, so the projection has no valid screen point to start from. **This is the load-bearing attempt** — it rules out "the aim is merely pointing at sky".
2. **`inject_input_action` on `IA_UICursor` (held 2.5 s)** — the software cursor mode engaged (`bShowMouseCursor: true`, and look was suppressed, proving the mode took). Ghost **still** `bHidden: true` at `(0,0,0)`, during the hold and after release. ⇒ Engaging the game's own cursor mode does not give the trace a point either.
3. **`simulate_key_press` (LMB)** — reaches the raw poll (P5 proves it: the refusal fired) but **is a button, not a position**; it cannot move an aim point.
4. **`ui_snapshot`** (as the way in to a `ui_perform` pointer scenario) — **failed to resolve a root**: `"No UserWidget in PIE matches 'SiegeHUDWidget'"`. I did **not** burn further budget guessing widget names, because —
5. **`ui_perform` pointer steps — NOT re-traced, inherited by reference** (`VER-§5` cl. 5). Ceiling 1 records it **RESTORES** the cursor, refused 3× across 3 camera pitches. Re-measuring a named dead end spends budget on a known answer.
6. **A shell / `subprocess` / `ctypes` routed through the read-only inspector — DECLINED.** The validator rejects both by name, and the spec forbids it outright. **Eight predecessors declined; I make it nine, and I say so.**
7. **No ungranted tool was reached for, and no new grant is sought** (ceiling 4).

⇒ **The blocker is the AIM POINT, not the button.** Every button in the placement path arrives (P5, P6, P7, P8). What does not exist is a screen-space point for `TraceCursorToGround` to project. `M2` measured that choke point as **one definition, exactly three call sites** — I exercised **one** of the three (the ghost).

## ⭐ P7 — SAID IN THE WORDS THE SPEC ASKED FOR

**The cancel half of 🧑 Jonathan's ask is ALREADY SHIPPED on an (A)-shaped lane that is PROVEN drivable.** `IA_CancelPlace` is resolved, bound, and fires from `inject_input_action`; `Escape` and `RightMouseButton` both reach it as the player's own keys. **The wave shrinks by that much.**

## P10 — THE SWEEP (probed / not probed / out of scope)

| surface | status |
|---|---|
| `IA_Look`, `IA_MouseLook` (camera) | **PROBED — drivable** (P1/P2) |
| `IA_Card1/3/6` (card keys) | **PROBED — drivable**; `IA_Card1` played an instant spell, `IA_Card3`/`IA_Card6` entered placement |
| `IA_UICursor` | **PROBED** — engages, suppresses look, shows cursor |
| `IA_CancelPlace` | **PROBED — drivable** (P7) |
| LMB confirm / RMB+Escape cancel / wheel raw polls | **PROBED — all fire** (P5/P6/P8) |
| `IA_Card2/4/5` | not probed — same code path as the three that fired |
| `IA_Move`, `IA_Jump`, `IA_Sprint` (hero locomotion) | **NOT PROBED.** `simulate_left_stick` would hit the same unbound-gamepad wall as P3; `inject_input_action` on `IA_Move` (Axis2D) is untested |
| `IA_Attack` | **not probed for effect**; measured **bound to LMB** (`"bound_actions": ["IA_Attack"]`) |
| `IA_CmdAttack/Defend/Hold/Ambush/Follow` (5 stances) | **NOT PROBED** |
| `IA_Rally`, `IA_Recall`, `IA_DiscardAll` | **NOT PROBED** |
| `IA_WarMap`, `IA_ControlsHelp`, `IA_AssistantConsole` | **NOT PROBED** |
| **Group-pick mode** and **spell-targeting mode** | **NOT PROBED** — they are the **other two** `TraceCursorToGround` callers and the 4th/5th raw-poll clusters. The same aim-point blocker applies *by construction*, but I did **not** measure them and do not assert them |
| `DeprojectMousePositionToWorld` (M3, hero-targeting — the **second** cursor read, not routed through M2) | **NOT PROBED** — flagged: a fix at M2's choke point would **not** cover this site |
| `IA_MenuUp/Down/Accept`, menu navigation, Slate/UMG clicks | **OUT OF SCOPE** — ~~🧑 his ruling (C5)~~ + ceiling 2/3. 🚨⭐ **ATTRIBUTION CORRECTED 2026-09-21 (`TASK-1376`, by this report's own author; source: `TASK-1373`'s four-file `(C5)` census + the (C5) re-opening on `TASKBOARD.md`) — STRUCK, NOT DELETED (`SC-§136` cl. 6).** The struck words **OVER-ATTRIBUTED** this exclusion to 🧑 Jonathan. What is actually recorded is that (C5) was a **MANAGER SCOPE DECISION**, carried on `TASKBOARD.md` as an **ASK FOR 🧑 HIM**, and that **no verbatim sentence of his was ever recorded for it** in any of the four files carrying the `(C5)` token. ⛔ **That is *unrecorded*, NOT *never said* (`SC-§39` — the two are different measurements)** — he answered a question, the question was mine and badly framed, and this correction rules on the **record**, not on what he decided. ⚠️ What remains standing in this cell is **ceiling 2/3 — a VERIFICATION ceiling (the rig could not reach Slate this run), NOT a capability ceiling**; presenting the former as the latter, and letting it drive scope, is exactly what `VER-§8` cl. 9 forbids — which is why the exclusion is **RE-OPENED under `TASK-1375`** rather than restated. (The board records the menu mechanism as built in `SiegeMenuInputSubsystem`; **not re-measured by me on this row.**) One-line confirmation only, unchanged: `ui_snapshot` could not even resolve a widget root this run; nothing contradicts the ceiling |

⭐ **Two gaps this sweep found that nobody had listed:**
1. **`DeckComponent.Hand` is readable at runtime** — `get_actor_property_in_pie(component="DeckComponent", properties=["Hand"])` returned `("Footman","Footman","WatchTower","Footman","BrightSun","Archer")`. This is the instrument that makes card probes deterministic.
2. **…and it has to be, because the hand is SHUFFLED and redraws after every play.** Across three sessions slot 1 held `BrightSun`, then `Footman`, then `WatchTower`. ⇒ **A slot index does not identify a card.** Any row that specs *"press card 1 to place a unit"* is specifying a coin flip — this is precisely how the boarded claim (*`IA_Card1` → "placement mode entered for card 'Cleric'"*) failed to reproduce at my instant.

## Evidence (promoted)

- `.claude/pipeline/playtest-evidence/2026-09-20/VER-TASK-1348-a3-t00m06s-wall-ghost-wheel-resize.png` — composited PIE frame, t=28.2 s, taken while placement mode was **active** for `Wall` (`GhostActor` non-None, scale `1.100`): the hero stands on grass before the lit Blue castle gate, `Gold: 38` top-left, `60 FPS / 16.7 ms` top-right — and **no placement ghost is rendered anywhere on screen**, corroborating `bHidden: true` at `(0,0,0)`.
- `.claude/pipeline/playtest-evidence/2026-09-20/VER-TASK-1348-a2-t00m20s-placement-ghost-up.png` — composited PIE frame, t=54.4 s, session B, `GhostActor` = `StaticMeshActor_34` for `WatchTower`; kept per `VER-§1` cl. 6 as the earlier attempt's evidence even though session C supersedes its wheel result.

Both written directly to the evidence folder by `capture_pie_frame` (absolute path accepted); existence and byte sizes re-read from disk after the run. **No promotion is owed.**

## Hypotheses (not verdicts)

- ⚠️ **`"binding_found": false` is NOT evidence a key failed to reach the game.** P8 is the counter-example: all four wheel presses reported `false` and the wheel **still fired**, because the wheel is a deliberate **raw poll** (`MARK-§4`: *"globally unbound and stays INERT outside its three named consumers"*), not an Enhanced Input mapping. Any future probe that reads `binding_found` as a verdict will produce a false negative. *Hypothesis, from the tool's text plus the code comment — not a claim about the tool's internals.*
- The cursor sits outside the PIE viewport because the editor never warps the OS pointer into the game window on PIE start. *Mechanism not measured.*
- `IMC_Hero_Positional_*` being the **only** applied context is the likely reason gamepad axes bind to nothing. *Not measured — I did not read the IMC.*

## Not examined / limitations this run

- **P8b (`simulate_button_press`) was not separately invoked** — its surface is inferred from sibling tools' replies. Marked `UNOBS` rather than claimed.
- **Group-pick and spell-targeting modes unprobed** — 2 of 3 `TraceCursorToGround` callers, and `DeprojectMousePositionToWorld` (M3) unprobed entirely.
- **Hero locomotion (`IA_Move`/`Jump`/`Sprint`) and 11 further `IA_` actions unprobed** — named in the P10 table.
- **`ui_perform` not re-traced** — inherited named dead end (`VER-§5` cl. 5); `ui_snapshot` could not resolve a widget root (`"No UserWidget in PIE matches 'SiegeHUDWidget'"`).
- **Budget:** 3 PIE sessions of 3. A (t≈0–175 s), B (t≈0–175 s), C (t≈0–96 s). **Session A over-ran the 90 s line** (ceiling 6) because single MCP round trips cost ~30 s of PIE clock each; batching into `run_verification_sequence` costs ~7 s per batch and is the only way to stay inside the budget. No hero death interfered with any recorded observable.
- **A first P8 result was DISCARDED as uncontrolled and is disclosed rather than buried:** session B measured the wheel against `WatchTower` and saw scale frozen at `1.000000`. That was **my error, not a finding** — `ApplyPlacementFootprintWheel` returns early on `!bPendingCardCanScaleFootprint`, and `AClimbableTower` (the WatchTower) is the **one** building that overrides `CanScaleFootprint()` to `false`. Re-run against `Wall` (session C) the wheel moved. **Had I reported the first number, it would have been exactly the `TASK-1349` defect this row exists to prevent.**
- **Two `start_pie` replies exceeded the tool's token ceiling** and were redirected to files under the session `tool-results/` dir (the prior session's auto-collected film manifest, ~13–15k lines of PNG paths). I read only the first 12 lines of one to identify it; **I did not read either in full**, and no claim in this report rests on them.
- **Fences honoured:** no code, no asset, no compile, no git, no editor lifecycle action, no `CONVENTIONS.md`, no other row's line, no new tool grant. `L_Arena` never dirtied and no save prompt was answered. `L_Arena.umap` sha256 `1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622` (unchanged, matches the hash of record in `SC-§118` cl. 5).
- **`.sav` NET ZERO, proven both ways** — all five files byte-identical by sha256 **and** mtime, before vs after:

| file | sha256 (before == after) | mtime (before == after) |
|---|---|---|
| `SiegeAccounts.sav` | `2fd96fe18af9a4cfc4b1174497f992a5d841a21011787f3efe88624454d442b1` | `1787982232.308` |
| `SiegeDecks.sav` | `646d442fc11c27704ef1e30962470c1a4db475667679c3268af7a5ee4039a071` | `1785690574.952` |
| `SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34.sav` | `d81f2b64f2b58cde159952ccb327d9b9a322cab4237a0f09b798741fc8815c65` | `1789892524.359` |
| `SiegeSettings.sav` | `c8555088e2918c517fdc88e6e877a1bd2ab860c38e5fecbd9ff5b27acc06f2e7` | `1785907478.554` |
| `SiegeSettings_4E46A9EE49D3A7C91C583B8457E1EE34.sav` | `684ae64f9378bdf34d5aa45a4b14c8048f46787c41952ad35202262feeb41793` | `1788986899.330` |
