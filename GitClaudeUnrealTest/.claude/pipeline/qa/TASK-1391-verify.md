Verdict: MEASURED
# Verification — TASK-1391 — PROBE B, [PLACEMENT-COMPOSE-PROBE]

## 🚨 THE ANSWER, IN ONE LINE, FIRST

**A CARD WAS PLAYED FROM HAND, TWICE, WITH A CONTROL THAT REFUSED BOTH TIMES.** The composed recipe —
`pie_scene_edit`'s `call_actor_function SetMouseLocation(300,420)` **and** `simulate_key_press
"LeftMouseButton"` **inside ONE `run_verification_sequence`** — spent the gold, consumed the card from the
hand, and spawned the unit. The **same batch**, with the **same button**, on the **same card**, with the
**set step OMITTED**, placed **nothing** and logged the `TryConfirmPlacement` refusal instead.

> ⛔ **NARROWING, ON THE FACE OF THE REPORT (`VER-§1` cl. 7 shape).** This attests to **exactly** this: under
> the binding batched recipe, on `ECardType::Unit` cards, on `L_Arena`, at this camera, a placement
> confirmed. It does **NOT** attest that a Building, an upgrade, a swarm or a spell can be confirmed
> (§ *Not examined*), and it does **NOT** relax `VER-§8` cl. 7 **wall (a)** — see §2, which is about a
> **different tool** and is **untouched by this report**.
>
> ⛔ **THE AMENDMENT IS NOT MINE TO WRITE.** Spec (8)(a) and `SC-§101` reserve any `VER-§8` cl. 7 amendment
> to the **manager**, on this report. I measured; I wrote no law and proposed no code.
>
> ⛔ **WHY LINE 1 READS `MEASURED` AND NOT `VERIFIED`, CHECKED RATHER THAN ASSUMED.** `VER-§1` cl. 3a's
> general reading of `measured` is a CONTROLLED NEGATIVE, and **my observable MOVED** — which is why
> `TASK-1352` and `TASK-1357` both reasoned their way to `VERIFIED`. **This row's own ROUTING overrides that
> for a probe:** *"LEGAL VERDICT WORDS: `MEASURED` · `UNOBSERVABLE` · `🚧 blocked`. NEVER `VERIFIED` /
> `VERIFY-FAILED` over another row"*, and spec (8)(a) designates `MEASURED` for *"a card is placed"*. There
> is no subject row here to pass. Line 1 is the row's word, not cl. 3a's.

Editor/Aura state: Aura connected **y** (`get_headless_status` → `editor_connected`) · editor instance
**PID 26992, command line `''` (EMPTY), `-game` ABSENT ⇒ GUI editor**, project
`GitClaudeUnrealTest.uproject`, read **before and after** and identical both times. Identification is
**`PARTIAL, by SC-§118 cl. 9`** — I hold no shell, no `subprocess`, no `ctypes` (`VER-§8` cl. 8), so I
identified the instance I am attached to **from inside it** and did **not** enumerate other processes.
**Nobody may fail this run for that (cl. 9).** ·
**MAP: I had to load it, and I report it as the dispatch required.** `TASK-1390` left the editor on
`/Game/Maps/L_MainMenu`; `load_level` → `{level: /Game/Maps/L_Arena, previous_level: /Game/Maps/L_MainMenu,
already_open: false, loaded: true, discarded_unsaved: false}`. **`discard_unsaved` was NOT passed** — a
dirty editor would have refused rather than lose 🧑 his work; `DIRTY_PACKAGES_BEFORE=[]` confirms there was
nothing to lose. ·
Pre-flight, quoted: `is_pie_active` → **`is_active: false`** ⇒ **no session of 🧑 his was running, and
nothing was ever stopped that I did not start**; board grep ``^- status: `integrating`` → **0 rows**
(`VER-§2` cl. 2); no compile and no import were live (`VER-§2` cl. 1 — `TASK-1390` complete, no other
verification running). ·
PIE mode **standalone, 1 client, 1280×720 requested, viewport 1280×725, `client_index 0`** ·
**`SC-§134` cl. 7 LIMB (a):** my `names:` carves out my own `status:` line, so I flipped it to
`in-progress — 🎮 PIE PROBE RUNNING` **before my first PIE call**, retained the prior wording
struck-not-deleted (cl. 8), and read the flip back (`SC-§104`). ·
attempts used **2 of 3** (two PIE sessions, both kept and named, `VER-§1` cl. 6): session **A**
`t=2.97 → 144.73 s`, session **B** `t=15.17 → 51.09 s` · wall ≈ 25 min ·
**No editor lifecycle action was taken. PID 26992 was up before, during and after, and is up now.** ·
Credit visible: **none surfaced in any tool reply** · Gold visible in-frame: `Gold: 84` (A, t=83.10 s),
`Gold: 30` (B, t=32.07 s) ·
⚠️ environment condition on frame A: red banner **`Video memory has been exhausted (79.586 MB over budget).
Expect extremely poor…`** — recorded, **not diagnosed**; **absent** on frame B ·
Log file named per `VER-§1` cl. 2, and lines **are** quoted from it:
`C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Saved\Logs\GitClaudeUnrealTest.log`

🧑 **`VER-§3`:** the dispatch states Jonathan is present **and has said go**, in his own words, and
instructs that no further go be waited for. None was. No PIE session of his existed to disturb.

---

## §1 — 🚨 THE GRANT / ROUTE RE-DERIVATION (spec (3)) — **GRANTED, AND NOT BY A LOOPHOLE**

**Posting duty first: the route WAS granted.** The dispatch's stop condition never triggered, because
there was **no refusal**.

**What `TASK-1357`/`TASK-1352` actually used**, re-derived from `qa/TASK-1352-verify.md:17-19` verbatim:
*"Not `set_actor_property` — **`pie_scene_edit`'s `call_actor_function` verb**, calling
`APlayerController::SetMouseLocation(X, Y)`, which is a shipped `UFUNCTION(BlueprintCallable)` whose body is
`Viewport->SetMouse(X, Y)`."*

**The manager's boarding measurement is CORRECT, and I re-measured it:** ⛔ **`pie_scene_edit` is NOT a
standalone tool on my surface**, and `call_actor_function` is not a tool at all. I never called either as a
tool.

**The surface, checked as the dispatch's lead directed — and here prose and enum AGREE** (unlike
`TASK-1390`'s finding, where the prose whitelist omitted three names the JSON enum carried):

- `run_verification_sequence`'s **prose whitelist**, quoted: *"Supported action types (CLOSED whitelist):
  … `survey_pie_scene`, **`pie_scene_edit`**, `get_actor_property_in_pie`, …"*
- its **JSON action enum** (`VerificationSequenceAction.type`) likewise carries **`"pie_scene_edit"`**.
- and `VerificationActionParams.operations`, quoted: *"`pie_scene_edit`: ORDERED list of scene mutations
  applied in array order in ONE round trip, 1-64 entries (required for `pie_scene_edit`). Each entry is
  `{"op": "teleport_player"|"spawn_actor"|"delete_actor"|"set_actor_transform"|"set_actor_enabled"|
  "set_actor_property"|**"call_actor_function"**, ...}` … Acts on the LIVE PIE world and never writes to
  disk."*

**It executed, twice, first try:**
`{"op":"call_actor_function","actor":"SiegePlayerController0","function":"SetMouseLocation","ok":true}`
with `applied: 1, failed: 0` at **A t=82.757 s** and **B t=31.716 s**.

⭐ **AND THIS IS NOT A TOOL THAT "SLIPPED THROUGH" — `VER-§7` cl. 2 NAMES THIS EXACT SITUATION AND
PRESCRIBES EXACTLY WHAT I DID:** *"The sequence runner's own whitelist is a GRANT SURFACE … on N1 and N4
the verifier staged a tower through `pie_scene_edit spawn_actor` — a name on neither its `tools:` line nor
`permissions.allow` — reached as a STEP inside `run_verification_sequence` … **A verifier that reaches a
census §5 name through the runner reports it in `## Not examined / limitations`, as N1 and N4 did.**"
That declaration is below, line by line. ⛔ **I hunted for nothing, and whether this needs a real grant is
🧑 Jonathan's call and the orchestrator's — never mine.**

---

## §2 — 🚨 WALL (a) IS ABOUT A DIFFERENT TOOL, AND THIS REPORT DOES NOT TOUCH IT (spec (2))

**`VER-§8` cl. 7 wall (a), verbatim from `CONVENTIONS.md:12286`:** *"WALL **(a)** STANDS UNRELAXED: the
confirm is a RAW KEY POLL ⇒ **`inject_input_action IA_Attack` can NEVER confirm a placement**. THAT HALF IS
UNTOUCHED BY THE 2026-09-20 AMENDMENT and nobody may cite the amendment against it."*

⇒ **That sentence is scoped to `inject_input_action`. I never injected `IA_Attack`, I assert nothing about
it, and nothing in this report relaxes it.** An Enhanced Input action sets the *action*, not the *key
state*, and the confirm polls the key state — that is as true today as it was when it was written.

**The tool I drove the confirm with is `simulate_key_press "LeftMouseButton"`, which is wall (b), and
cl. 7 already records wall (b) as DOWN** — verbatim from `CONVENTIONS.md:12267`: *"**(b)**
`simulate_key_press "LeftMouseButton"` **DOES reach the poll** (proven by the refusal lines it produced) —
and the placement then fails one layer deeper: … the **OS mouse cursor is not over the PIE game window**,
so every confirm logged *'placement click refused … no ground hit, outside the Blue spawn box …'*."*
`qa/TASK-1348-verify.md:54` says the same: *"**`simulate_key_press` (LMB)** — reaches the raw poll (P5
proves it: the refusal fired) but **is a button, not a position**; it cannot move an aim point."*

⇒ **Wall (b) was the BUTTON half and was already down; the one layer deeper was the AIM, and `TASK-1357`
supplied the aim. This row composed the two. Wall (a) is a different tool and stands exactly where it was.**
A report that cited wall (a) as covering `simulate_key_press` would have reproduced this wave's own defect;
this one does not.

**Poll address RE-MEASURED as cl. 7 instructs** (*"this address moves with every edit to that file"*) —
`Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp:921-924`, **unchanged**:

```cpp
	if (WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		TryConfirmPlacement();
	}
```

---

## Acceptance lines → observations

| # | acceptance line | observable chosen | observed (quoted values, evidence path) | pass/fail/unobs |
|---|---|---|---|---|
| **C1** | **"the grant/route re-derivation from (3) printed with its outcome"** | the two prior reports re-read, my own surface re-measured, and the call actually issued | **GRANTED — no refusal, so the stop condition never fired.** `pie_scene_edit` is **not** a standalone tool on my surface (manager's boarding measurement confirmed) but **is** in `run_verification_sequence`'s step vocabulary in **both** the prose whitelist **and** the JSON enum, with `call_actor_function` a documented `op`. Executed `ok:true`, `applied:1, failed:0`, at **A t=82.757 s** and **B t=31.716 s**. Declared line-by-line under *Not examined* per `VER-§7` cl. 2 (§1) | **pass** |
| **C2** | **"the hand read BEFORE any press + the card named with its reason"** | `get_actor_property_in_pie(component="DeckComponent", properties=["Hand"])`, taken as the **first** action of the batch, before any input of any kind | **A, `t=12.0865 s`, before any press:** `("Longbowman","Footman","Footman","Fireball","Ogre","FrostNova")`. **B, `t=15.1659 s` AND re-read `t=31.2163 s`, both before any press, byte-identical.** `("Pikeman","Archer","Footman","Cleric","Sorcerer","MilitiaMob")`. ⇒ **the hand was read, never a bare slot index assumed.** **CARD + REASON (A): `Footman`, slot 1, cost 9** — the **cheapest** card in that hand (gold was 22), `cardType: Unit` so it takes the plain-unit spawn path with **no** §3.5 building-clearance gate and **no** `AClimbableTower::CanScaleFootprint()` override (the thing that froze a previous probe's only observable), and `swarmCount: 0` so **exactly one** actor spawns ⇒ a clean +1 census delta. **Slots 1 AND 2 both held `Footman`**, so the choice was robust to an off-by-one in the slot mapping. **(B): `Archer`, slot 1, cost 12** — same `cardType: Unit` / `swarmCount: 0` reasoning, affordable at gold 41, and **deliberately a DIFFERENT card** so the replication is not a re-run of one lucky row | **pass** |
| **C3** | **"the SET and every dependent read shown to be in ONE batch"** | the action indices and `pie_time_seconds` of a single `run_verification_sequence` call | **YES, in both attempts.** A: one call, 21 actions, `t=82.2466 → 83.1009 s` (**0.854 s** end to end) — the set is index 11, its dependent ghost read index 13, the confirm index 14, the three post-reads indices 16/17/18. B: one call, 25 actions, `t=31.1996 → 32.0731 s` (**0.874 s**) — set index 15, dependent read 17, confirm 18, post-reads 20/21/22. **No MCP round trip separates any set from its dependent read** (`VER-§8` cl. 7's 2026-09-20 (ii) boarding rule) | **pass** |
| **C4** | 🚨 **"the OMITTED-SET CONTROL in the SAME batch, printed"** | the identical `simulate_key_press "LeftMouseButton"` into the identical live placement mode, **with the `SetMouseLocation` step omitted**, immediately before the treatment | 🚨 **IT DISCRIMINATED, BOTH TIMES.** **A control** (press `t=82.4905 s`, read `t=82.7072 s`): **Gold `92 → 92` UNCHANGED** · `GhostActor` **still** `StaticMeshActor_34` ⇒ **still in placement mode, so no confirm and no exit** · `bHidden: true` at `(X=0.000000,Y=0.000000,Z=0.000000)` — `TASK-1352`/`TASK-1357`'s un-aimed negative reproduced · **Hand UNCHANGED** (`t=82.7238 s`) · census `167 → 168`, the **+1 being the ghost `StaticMeshActor_34` itself**, with **no unit**. **B control** (press `t=31.4496 s`, read `t=31.6663 s`): **Gold `41 → 41` UNCHANGED** · `GhostActor` still `StaticMeshActor_34` · `bHidden: true` at `(0,0,0)` · **Hand UNCHANGED** · census `155 → 156` (the ghost), `Unit_` classes still **only** `BP_Unit_Pikeman_C ×1` at `x=+18166` (the **red** side's, 39 km away — not mine). ⇒ **a probe that could fail, and on this arm it did (`SC-§137`)** | **pass** |
| **C5** | **"the named observable's read, before and after"** | **three observables, named in advance, none of them the absence of a refusal:** (i) a **new actor** in the level, (ii) the **gold total dropping** by the card's cost, (iii) the **hand changing** | 🚨 **ALL THREE MOVED ON THE TREATMENT ARM, BOTH TIMES.** **(i) NEW ACTOR** — A `t=83.0676 s`: census `168 → 169`, **`BP_Unit_Footman_C` count 1, `BP_Unit_Footman0` at `(-21394.41, 405.35, 121.15)`** (absent from the identical census 0.33 s earlier). B `t=32.0397 s`: `156 → 157`, **`BP_Unit_Archer_C` count 1, `BP_Unit_Archer0` at `(-21341.37, 377.43, 109.08)`** — **an ARCHER, matching the card that was in slot 1**. **(ii) GOLD** — A `t=83.0343 s`: **`92 → 84`** (Footman cost 9; see H2 on the residual +1). B `t=32.0064 s`: **`41 → 29`, a drop of EXACTLY 12 = `DT_Cards.Archer.cost`, no residual.** **(iii) HAND** — A `t=83.0509 s`: `("Longbowman",**"MilitiaMob"**,"Footman","Fireball","Ogre","FrostNova")` ⇒ **slot 1's `Footman` consumed, replaced by a drawn `MilitiaMob`**. B `t=32.0231 s`: `("Pikeman",**"MilitiaMob"**,"Footman","Cleric","Sorcerer","MilitiaMob")` ⇒ **slot 1's `Archer` consumed.** **Plus a fourth, independent of all three: `GhostActor` went `StaticMeshActor_34 → None` on both treatment arms ⇒ `ExitPlacementMode` ran — which the control arm proves a refusal does NOT do.** Pixels: both frames show an **opaque** unit, not a ghost | **pass** |
| **C6** | **"any refusal line quoted verbatim"** | `LogGitClaudeUnrealTest`, substring `placement click refused`, in the named log | ⭐ **`total_matches = 2` IN THE WHOLE LOG — EXACTLY ONE PER CONTROL ARM, AND NONE FOR EITHER TREATMENT ARM.** Verbatim: `[2026.09.22-21.35.14:744][395]LogGitClaudeUnrealTest: ASiegePlayerController 'SiegePlayerController_0': placement click refused for 'Footman' — no ground hit, outside the Blue spawn box and any Blue-owned capture zone, or off the navmesh (W1-PREP additions 3, TASK-261; GDD §3.5, TASK-030; plinth keep-out retired, TASK-349).` and `[2026.09.22-21.37.26:851][129]LogGitClaudeUnrealTest: … placement click refused for 'Archer' — no ground hit, outside the Blue spawn box …` (same string). ⭐ **THE PRESENCE of these lines is this probe's POSITIVE CONTROL THAT THE BUTTON ARRIVED** — `simulate_key_press` reached `TryConfirmPlacement` and was refused at the aim gate, reproducing `TASK-1348` P5 exactly. ⛔ **Their ABSENCE on the treatment arms is NOT what the confirm rests on** (`VER-§8` cl. 7's B4 lesson: an absence proves only that we were not in that branch) — the confirm rests on C5's three positive observables | **pass** |
| **C7** | **"an EXPLICIT sentence distinguishing wall (a) (`inject_input_action`) from `simulate_key_press`"** | §2 above, with both walls quoted from `CONVENTIONS.md` | **Written, with both quoted and the poll address re-measured.** Wall (a) is scoped to `inject_input_action IA_Attack`, was never exercised here, and **stands unrelaxed**. Wall (b) is `simulate_key_press`, was already recorded **DOWN** for reaching the poll, and is the half this row composed with `TASK-1357`'s aim | **pass** |
| **C8** | **"PIE timestamps"** | `pie_time_seconds` on every action | Every value in this report carries its own PIE stamp. A: `t=12.09 → 83.10 s`. B: `t=15.17 → 32.07 s` | **pass** |

**Derivation:** no `fail` (unavailable by construction — spec (4)/(8) put no code under test) · **≥1 `pass`**,
and the row's ROUTING fixes the probe's verdict word ⇒ **`MEASURED`** per spec (8)(a) (*"a card is placed"*).

---

## §3 — THE TWO ARMS, SIDE BY SIDE

| | **CONTROL — set OMITTED** | **TREATMENT — set INCLUDED** |
|---|---|---|
| tool driving the confirm | `simulate_key_press "LeftMouseButton"` | `simulate_key_press "LeftMouseButton"` — **identical** |
| card | A `Footman` / B `Archer` | **the same card, the same placement session** |
| the ONLY difference | — | one `pie_scene_edit call_actor_function SetMouseLocation(300,420)` step |
| ghost before the press | `bHidden: true` at `(0,0,0)` | A `bHidden: false` at `(-21440.000107, 437.757051, 32.938384)` · B `bHidden: false` at **`(-21382.831129, 407.684206, 29.000000)`** |
| gold | A `92 → 92` · B `41 → 41` | A `92 → 84` · B **`41 → 29` (−12 = Archer's exact cost)** |
| hand | **unchanged** both | A slot 1 `Footman → MilitiaMob` · B slot 1 `Archer → MilitiaMob` |
| new unit | **none** both | A **`BP_Unit_Footman0`** · B **`BP_Unit_Archer0`** |
| `GhostActor` after | **still `StaticMeshActor_34`** (mode alive) | **`None`** ⇒ `ExitPlacementMode` ran |
| refusal line | **1 logged, quoted** | **0 logged** |

⭐ **B's aim point `(X=-21382.831129, Y=407.684206, Z=29.000000)` is BIT-IDENTICAL to `TASK-1357`'s B1/B5a
value and `TASK-1352`'s session-B value** — the same screen point, the same camera, now a **third
independent run on a different day**, agreeing to six decimals. A's differs; see **H1**, and I do not
explain it away.

---

## ⏱️ §4 — THE BUDGET MISS ON ATTEMPT 1, DECLARED RATHER THAN BURIED

**Attempt 1's observables landed at `t=82.2 → 83.1 s`** — past the row's `t≈60 s` fence and **at/just past
the `t=82.6 s` earliest-observed bot kill.** The kill did **not** land (`pawn_class: BP_HeroCharacter_C`,
`view_target: BP_HeroCharacter0` at `t=83.084 s`), so attempt 1's numbers are clean — **but that was margin
I did not have, not margin I planned.**

**Cause, measured:** the single MCP round trip between batch 1 (ended `t=12.1365 s`) and batch 2 (began
`t=82.2466 s`) cost **≈70.11 s of PIE clock** — more than double the ≈30 s the row budgets per round trip.
The two-batch shape was forced by the hand read: the card cannot be chosen until the hand is known, and a
batch cannot branch on its own result.

**Attempt 2 exists to fix exactly that**, and did: one batch, everything inside it, observables at
`t=31.2 → 32.07 s`, **half the `t≈60 s` fence**, with an in-batch `wait_pie_seconds(16)` (costing no round
trip) used to raise gold from 25 to 41 so any card in the hand would be affordable.

---

## Evidence (promoted)

Both frames were written **directly** into `.claude/pipeline/playtest-evidence/2026-09-22/` — **no copy-out
is owed to the host row.** `capture_pie_frame` appends its own stamp, which `VER-§4` cl. 2 as amended
accepts; no slug contains "pass" or "fail". Both composited, 1086×615.

- `.claude/pipeline/playtest-evidence/2026-09-22/VER-TASK-1391-a1-composed-confirm-footman_t83.10s_f3124431.png`
  — PIE `t=83.10 s`, frame `3124431`, `mean_luma 135`. ⭐ An **OPAQUE blue-team footman in mail and a
  blue-plumed helm**, sword and shield in hand, standing on the pale stone ledge at **mid-left** with a blue
  health bar above him — the spawned `BP_Unit_Footman0`, **not** a translucent ghost. The cloaked
  white-and-red-cross hero stands centred on grass before the lit castle archway (two wall sconces burning,
  mossy-green rock scatter both sides). **`Gold: 84` top-left, matching the property read exactly.**
  `55 FPS / 18.2 ms` and `0/6` top-right. Red banner `Video memory has been exhausted (79.586 MB over
  budget). Expect extremely poor…` across mid-frame — recorded, not diagnosed.
- `.claude/pipeline/playtest-evidence/2026-09-22/VER-TASK-1391-a2-composed-confirm-replication_t32.07s_f3132167.png`
  — PIE `t=32.07 s`, frame `3132167`, `mean_luma 136`. ⭐ An **OPAQUE brown-tunic ARCHER carrying a bow**,
  blue team sash, blue health bar, on the same ledge at mid-left — the spawned `BP_Unit_Archer0`, and
  **visibly a different unit type from attempt 1's frame**, matching the different card. Same hero and
  archway composition. `Gold: 30` top-left (the property read 0.07 s earlier said `29`; one passive income
  tick landed between the read and the capture). `59 FPS / 16.9 ms`, `0/6` top-right. **No VRAM banner.**

⭐ **Both frames show a SOLID PLACED UNIT. Every prior frame in this wave — `TASK-1352` §3, `TASK-1357`
frame A — showed at most a TRANSLUCENT GREEN GHOST.** That is the visible difference between an aim point
and a played card.

---

## Hypotheses (not verdicts)

- **H1 — attempt 1's aim point is unexplained, and I do not explain it away.** A read
  `(X=-21440.000107, Y=437.757051, Z=32.938384)` where attempt 2, `TASK-1357` (≥14 reads) and `TASK-1352`
  all read `(X=-21382.831129, Y=407.684206, Z=29.000000)` at the same screen point and a byte-identical
  camera. Leading suspects, neither measured: the ghost's `RelativeLocation` is the **post-snap placement
  position**, which `UpdatePlacementGhost` adjusts per **card footprint** (Footman's differs from Archer's,
  though `TASK-1357` used Footman and got the canonical value), and attempt 1 reported
  `applied_mapping_contexts: ["IMC_Hero_Positional_0"]` against attempt 2's `["IMC_Hero_Positional_1"]`.
  **MECHANISM NOT MEASURED.** The recipe itself is not in doubt — attempt 2 reproduced the canonical value
  to six decimals — only attempt 1's offset is unaccounted for.
- **H2 — the residual in attempt 1's gold.** Observed `−8` against a Footman cost of `9`. Passive income is
  **independently measured at exactly +1/s** (22 @ `t=12.10` → 92 @ `t=82.26` = +70 over 70.16 s; and 25 @
  `t=15.18` → 41 @ `t=31.23` = +16 over 16.05 s), and attempt 1's before/after reads span 0.771 s, so one
  income tick landing there gives `−9 + 1 = −8`. Attempt 2's `−12` matched Archer's cost with **no
  residual**, consistent with no tick in its 0.773 s window. ⛔ **This is an INFERENCE, not a separate
  measurement.** What is measured without inference: **gold went DOWN while passive income is strictly
  non-negative ⇒ a spend occurred.**
- **H3 — offered as a constraint, NOT a mechanism ruling (`SC-§101`; the row reserves the amendment to the
  manager).** Everything here bears on **wall (b)** composed with `TASK-1357`'s aim recipe. I have not
  touched, hinted at or costed **wall (a)**, I propose no code, and I write no law.

---

## Not examined / limitations this run

- ⭐ **`VER-§7` cl. 2 DECLARATION — the census §5 name reached THROUGH the runner, line by line, as the law
  requires:** `pie_scene_edit` was invoked **twice total**, once per attempt, **only** as a step inside
  `run_verification_sequence`, each time carrying **exactly one** operation:
  `{"op": "call_actor_function", "name": "SiegePlayerController0", "function": "SetMouseLocation",
  "args": {"X": 300, "Y": 420}}`. ⛔ **No other `op` was used at any point** — no `spawn_actor`, no
  `delete_actor`, no `set_actor_property`, no `set_actor_transform`, no `set_actor_enabled`, no
  `teleport_player`. ⛔ **`pie_scene_edit` was never called as a standalone tool** (it is not on my
  surface). The effect is PIE-transient by the tool's own contract (*"never writes to disk"*) and
  `DIRTY_PACKAGES_AFTER = []` confirms it. **Whether this reachability should become a real grant is 🧑
  Jonathan's and the orchestrator's call, not mine.**
- **Only ONE of `TraceCursorToGround`'s three callers was exercised** — the placement ghost. The **spell
  reticle** and the **group-pick reticle** were not probed and I assert nothing about them.
- 🚨 **THE CONFIRM WAS EXERCISED ONLY ON `ECardType::Unit` CARDS** (`Footman`, `Archer`). The **Building**
  path (`bPendingIsBuilding`), the **upgrade** path (`PlacementUpgradeState::Ready → ConfirmStackUpgrade`),
  the **swarm** path (`MilitiaMob`, `swarmCount: 4`), the **miner-cap** path and the **spell** path
  (`TryConfirmSpellTarget`, which `TASK-1357` §2 records is **not** a `UFUNCTION`) are **ALL UNTOUCHED**.
  ⛔ A row that reads this as *"any card can now be played"* has over-read it.
- **NO ACCURACY CLAIM.** `(300,420)` is not proven to be any particular viewport location: `start_pie` was
  asked for 1280×720, `is_pie_active` reported viewport **1280×725**, and capture returns **1086×615**.
  **Reproducible ≠ calibrated** — the same gap `TASK-1352` and `TASK-1357` declared.
- **NO PERSISTENCE CLAIM ACROSS A ROUND TRIP.** Both aims were set and consumed inside one batch, per the
  binding recipe. I did not test whether an aim survives an MCP round trip, and nothing here licenses a
  two-call *"set the cursor, then read the ghost"*.
- **`bPlacementValid`, `PendingCardID`, `PendingCost`, `PlacementLocation` are NOT reflected** — plain C++
  members at `SiegePlayerController.h:3196-3211` with no `UPROPERTY` — so the validity flag could not be
  read directly and the arms rest on `GhostActor` / `bHidden` / gold / hand / census instead. **Named, not
  worked around.**
- ⚠️ **`start_pie`'s reply EXCEEDED the tool output limit on BOTH calls** (264,336 chars / 6,610 lines, and
  504,242 chars / 12,589 lines — it carries a recovered-recording manifest). **I read only the first 14
  lines of the first spill file and NONE of the second; I have NOT read either in full and I summarise
  nothing from them.** Session start was confirmed independently — attempt 1 by `is_pie_active`
  (`is_active: true, time_seconds: 2.97`), attempt 2 by the sequence's own `pie_time_seconds` stamps.
- ⚠️ **BUDGET MISS on attempt 1** — §4, declared in full.
- ⚠️ **NO PRE-RUN `.sav` BASELINE WAS TAKEN**, so my `.sav` net-zero claim is an **inference from mtime, not
  a measured before/after pair** — see *Fences*, where its three legs are stated.
- ⚠️ **A DECK CHANGE I OBSERVED AND MUST REPORT, THOUGH IT IS NOT MINE:** attempt 1's hand contained
  **`Fireball` and `FrostNova`** — the two `HeroLine` spells `TASK-1357` B4 measured **ABSENT** from the
  active profile deck save on 2026-09-20 — and attempt 2's held `Pikeman`/`Cleric`/`Sorcerer`. The save
  `SiegeDecks_4E46A9EE….sav` now reads sha256 `d5c6fed47d54c9e9…` / 6518 B / mtime **`2026-09-22T11:41:03`
  local**, against `TASK-1357`'s `d81f2b64…8815c65` / `2026-09-20T01:22:04`. **That mtime is 2 h 54 min
  BEFORE my first PIE call (14:35 local; the log's `21:35` stamps are UTC).** ⛔ **My run did not write it** —
  I hold no tool that writes a `.sav`, and `DIRTY_PACKAGES_AFTER = []`. Who changed it is **not mine to
  say**; 🧑 Jonathan editing his deck is the obvious candidate and that is a **HYPOTHESIS**. ⇒ **Anyone
  citing `TASK-1357` B4/B5b should know its deck census is dated 2026-09-20 and is now STALE.**
- **`discover=true` was NOT used inside a sequence** (cl. 10(c) respected) — and no standalone discovery was
  needed either: every property path (`PlayerState.Gold`, `DeckComponent.Hand`, `GhostActor…`) was derived
  from source **before** PIE started, which is also how the budget was kept in attempt 2.
- 🚨 **`binding_found` IS REPORTED AND IS EXPLICITLY NOT AN OBSERVABLE (`VER-§8` cl. 10).** All four
  `simulate_key_press` calls returned `binding_found: true`, `enhanced_input_active: true`,
  `bound_actions: ["IA_Attack"]`. ⛔ **It is actively MISLEADING here**: it reports the Enhanced Input
  binding for `IA_Attack`, which has nothing to do with the raw `WasInputKeyJustPressed` poll the confirm
  actually uses — **and it read `true` identically on the arm that placed and the arm that refused, so it
  could not discriminate and did not.** Every claim in this report rests on the gold, the hand, the census
  and the log line.
- ⛔ **`TASK-1390`'s result was NOT imported into any reasoning here**, as the dispatch required: different
  tool (`ui_perform`'s pointer route vs `simulate_key_press`), different mechanism (`UButton.OnClicked`
  Slate delegate vs a raw key poll), different map (`L_MainMenu` vs `L_Arena`). Nothing here rests on it and
  nothing here bears on it. Its *lead* about prose-vs-enum was checked against my own surface (§1) and, as
  it happens, **did not reproduce** — for `pie_scene_edit` the two agree.
- **Named dead ends inherited by reference, NOT re-traced** (`VER-§5` cl. 5): `inject_input_action
  IA_Attack` as a confirm (wall (a)) · `set_player_transform` across pitches **as an aim source** ·
  `IA_UICursor` held · `ui_snapshot` / `ui_perform` · `L_MainMenu` · Slate.
- ⛔ **Shell / `subprocess` / `ctypes` routed through the read-only inspector — DECLINED, by name.** Twelve
  predecessors declined it; **I MAKE IT THIRTEEN, and I say so.**
- ⛔ **No new tool or MCP grant was reached for or is sought.** No call was refused, so the dispatch's stop
  condition never triggered — had it, I would have reported `🚧` and stopped rather than route around it
  (`SC-§54` cl. 1).

---

## Fences — measured, not asserted

- **Never-save law: `DIRTY_PACKAGES_BEFORE = []` and `DIRTY_PACKAGES_AFTER = []`.** No save prompt was
  raised and none was answered.
- **`L_Arena.umap` sha256 `1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622`, 605098 B,
  mtime `1788594178.6491876`** — **byte-identical to the value `TASK-1352` and `TASK-1357` both recorded**
  (`1f78419d…f15622`, mtime `2026-09-05T00:42:58.649188`). **Unchanged.**
- **`SAV_COUNT = 5 → 5.** Four of the five match `TASK-1357`'s ledger by sha256 prefix **and** by fractional
  mtime to the microsecond: `SiegeAccounts.sav` `2fd96fe1…` / `1787982232.307531` · `SiegeDecks.sav`
  `646d442f…` / `1785690574.9522967` · `SiegeSettings.sav` `c8555088…` / `1785907478.554086` ·
  `SiegeSettings_4E46A9EE….sav` `684ae64f…` / `1788986899.3296347`. **The fifth changed BEFORE my run** —
  declared in full above. **The three legs of the net-zero claim, stated because the pre-run baseline is
  missing:** (a) four of five match the prior ledger exactly, (b) the fifth's mtime predates my first PIE
  call by 2 h 54 min, (c) `DIRTY_PACKAGES_AFTER = []` and I hold no tool that writes a `.sav`.
- **Editor: PID 26992, command line `''`, `-game` absent — before and after.** Level
  `/Game/Maps/L_Arena.L_Arena` after. **No launch, no close, no restart, no Live Coding, no compile, no
  git, no asset write, no Blueprint edit.**
- **`is_pie_active` read `is_active: false` before I started** ⇒ **no PIE session belonging to 🧑 Jonathan
  was found or touched.** Both sessions were started **and** stopped by me (`t=144.73 s`, `t=51.09 s`).
  The two placed units died with their PIE worlds; nothing persists.
- ⚠️ **EDITOR STATE LEFT BEHIND:** the editor is **UP (PID 26992)**, **not** in PIE, on
  **`/Game/Maps/L_Arena`** — **NOT** the `L_MainMenu` it was on when I received it. Dirty packages `[]`.
- **Scope:** no code, no asset, no compile, no git, no `CONVENTIONS.md`, no `CLAUDE.md`, no other row's
  line, no other agent's report, nothing under `.claude/agents/`. **My only writes are this file, my own
  `status:` line on `TASKBOARD.md`, and the two evidence PNGs.**
