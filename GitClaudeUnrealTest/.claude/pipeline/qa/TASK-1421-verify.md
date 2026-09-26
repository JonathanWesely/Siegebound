Verdict: VERIFIED
# Verification — TASK-1421 [WAVE-C-VERIFY]

**Editor/Aura state:** Aura `editor_connected`. Editor identified **IN-PROCESS**, not by census —
`os.getpid()` returned **18236** from inside the editor that answered the MCP call (matches the
dispatch); `sys.executable` = `C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe`
(the **GUI editor**, not a `-game` session); `get_game_world()` was **None** before I started PIE, so no
`-game` sitting of Jonathan's existed. ⚠️ **`FCommandLine::Get()` returned the EMPTY STRING**, so the
`SC-§118` identification rests on in-process PID + exe path + editor-world presence, **not** on a
command-line string — declared, not glossed (same limit `TASK-1413` hit).
**Binary confirmed in-process:** `Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll` =
**9,890,816 B @ 2026-09-25 02:42:30** — the post-Row-A link, exactly as the dispatch declared.
Map: editor world was `/Game/Maps/L_Arena`; I called `load_level /Game/Maps/L_MainMenu` myself
(`previous_level: /Game/Maps/L_Arena`, `discarded_unsaved: false`).
PIE mode: standalone, 1 client, 1280×720 requested → viewport **1280×725**. `is_pie_active` read
**`is_active: false` before I started**, so the session was mine to start and mine to stop; stopped at
`pie_time_seconds: 624.27`, `is_active: false` confirmed after. **Editor PID 18236 left UP.**
**Attempts used: 1 of 3.** PIE wall time ≈ 10.4 min (t=5.80 s → t=624.27 s).
🧑 Jonathan's explicit go was carried in the dispatch (*"Run both now."*). I sat **one** of the two
sittings: the deck builder and session menu were **not** touched (that is `TASK-1427`).

---

## 🚨 INSTRUMENT LIVENESS — CHECKED FIRST, BECAUSE AN EMPTY LOG READS AS A FALSE PASS

R6 asked for four `Log … Verbose` console writes. **They were not issued and could not be: I hold no
console-command verb, and `execute_unreal_python_readonly` is read-only.** What I did instead was
measure the compiled defaults and then *see the lines*:

| category | declaration | default verbosity |
|---|---|---|
| `LogSiegeMenuInput` | `SiegeMenuInputSubsystem.h:33` | `Log` |
| `LogSiegeAccount` | `SiegeAccountSubsystem.h:15` | `Log` |
| `LogSiegeGraphics` | `SiegeGraphicsSettingsSubsystem.h:16` | `Log` |

All three instruments printed. First lines of the session:

```
[16.36.58:774] LogSiegeAccount:   [SiegeAccount] Subsystem initialized — slot 'SiegeAccounts' (user 0),
                                  1 profile(s), active: JonBonWes.
[16.36.58:774] LogSiegeGraphics:  [SiegeGraphics] Subsystem initialized — Overall=-1 (Custom),
                                  ResolutionScale=87.0%, 3200x1800, WindowMode=1, VSync=off, …
[16.36.58:909] LogSiegeMenuInput: […] L_MainMenu: IMC_MainMenu applied at priority 0 …
[16.36.58:931] LogSiegeMenuInput: […] ApplyInitialFocus: focus placed on the TOP option 'Button_0'
                                  ("Play (vs Bot)"), index 0 of 7, in menu instance 'WBP_MainMenu_C_0'.
```

⇒ **The instruments are demonstrably alive (111+ `LogSiegeMenuInput` lines this session), so every
silence reported below is a REAL silence.**
⛔ **ONE VERBOSITY GAP, DECLARED:** `Verbose`-level lines did **not** print — specifically the
plain-`UButton` *"Left/Right do nothing, deliberately"* line, and any `LogSiegeGraphics` **Verbose**
per-focus scroll instrument. **No conclusion below rests on seeing a `Verbose` line**; §4's negative
control is argued by elimination over the `Log`-level branches, exactly as `TASK-1413` did.

Pre-registered falsifiers were written to disk **before PIE started** (while `is_pie_active: false`):
`…\scratchpad\TASK-1421-prereg.md`. Nothing below was decided after a number was seen.

---

## §0 — R7 HONOURED, AND ITS PRE-DECIDED VERDICT **DID NOT FIRE**

Order run: **Settings first**, then Graphics **reached only through Settings' `GraphicsButton`**, then
Login. No direct attempt was made to open Graphics.

🚨 **Settings' Accept is NOT inert on this binary.** R7 pre-decided that a still-inert Accept meant
*Settings takes the `VERIFY-FAILED`, Graphics gets `UNOBSERVABLE — unreachable`.* **That branch is dead:**
Accept fired `OnClicked.Broadcast()` twice on the product's own route —

```
t≈41.46 s  IA_MenuAccept -> OnClicked.Broadcast() on 'Button_4' ("Settings").
t≈137.96 s IA_MenuAccept -> OnClicked.Broadcast() on 'GraphicsButton' ("Graphics").
```

⇒ Graphics was **genuinely reached through the door R7 named**, and neither subject is blocked.

---

## §1 — PER-SUBJECT VERDICT TABLE (the commit host reads THIS, not line 1)

| TASK | subject | verdict | the one sentence |
|---|---|---|---|
| **TASK-1415** | Settings | **VERIFIED** | Registered **3 focus stop(s)** exactly as predicted, opened with the ring already on the `UCheckBox`, and Down walked all three with a clean wrap. |
| **TASK-1417** | Graphics | **VERIFIED** | Registered **19 focus stop(s)**, and 18 consecutive Downs from stop 0 reached `BackButton` while the 19th wrapped to `ShowFrameRateCounterCheckBox`. |
| **TASK-1419** | Login | **VERIFIED** | Opened in **`LoggedIn`** mode with the ring on `LogoutButton`, walked 3 stops with a clean wrap both ways. ⚠️ Its own spec-(3) **text-box egress criterion is `UNOBSERVABLE`** this sitting — see §5. |
| **TASK-1469** | Row A | **VERIFIED** | Limbs **2 and 3 measured live**: Graphics 24 → **19**, `BackButton` 23 → **18**, and **no index repeated twice in a row**. ⚠️ Limb 1's text-box payout is **`UNOBSERVABLE`** — see §5. |

**Row-level line 1 = `VERIFIED`**: every acceptance line that had a reachable runtime signal was observed
passing. **Nothing failed.** The two `UNOBSERVABLE` sub-limbs are carved out explicitly rather than
folded into a pass.

---

## §2 — THE (a)–(f) TABLE, PER SCREEN

### SETTINGS — `SettingsMenuWidget_0`

| limb | observable | observed — node name + PIE `t=` | result |
|---|---|---|---|
| (a) reach it by injection | main-menu Down ×4 then Accept | `0→1→2→3→4 of 7`; `IA_MenuAccept -> OnClicked.Broadcast() on 'Button_4' ("Settings")` **t≈41.46 s** | **pass** |
| (b)/(g1) any node `focused: true` on open | `ui_snapshot` | `BackdropBorder/RootPanel/ConfirmToggleCheckBox` (CheckBox) **`focused: true`**, **t=42.68 s** | **pass** |
| (c) does `IA_MenuDown` MOVE it | the `MoveFocus` line | `0 -> 1 of 3 ('GraphicsButton')` **t≈95.0 s** · `1 -> 2 of 3 ('BackButton')` **t≈95.4 s** · `2 -> 0 of 3 ('ConfirmToggleCheckBox')` **t≈95.8 s** (wrap) · Up `0 -> 2 of 3 ('BackButton')` **t≈96.3 s** (reverse wrap) | **pass** |
| (d) a NON-`UButton` stop | class of the reached stop | **`ConfirmToggleCheckBox` — `UCheckBox`** — held the ring at open *and* was reached by the Down-wrap | **pass** |
| (e) `IA_MenuRight` on a slider | — | no slider on this screen | n/a |
| (f) on close, where is focus | `ui_snapshot` after Accept on `BackButton` **t≈463.32 s** | main menu **`Overlay_19/VerticalBox_0/Button_0`** ("Play (vs Bot)") **`focused: true`**, **t=464.84 s** — `TASK-1400`'s re-arm, measured for free | **pass** |
| (g2) Accept inside the sub-screen prints the subsystem's line | the log | `IA_MenuAccept -> OnClicked.Broadcast() on 'GraphicsButton' ("Graphics")` **t≈137.96 s** — **PREDICTION (YES) CONFIRMED** | **pass** |
| (g3) does `IA_MenuBack` CLOSE the screen | screen still present? | **NO — INERT, as predicted.** Injected **t=123.79 s**; at **t=125.21 s** `SettingsMenuWidget_0` still `realized: true` with `BackButton` `focused: true`. Positive log line, not a silence: *"IA_MenuBack: active target 'SettingsMenuWidget_0' (SettingsMenuWidget, from the registration stack) implements no ISiegeMenuNavCloseTarget — Back is INERT for it, and no teardown is guessed."* | **inert (predicted)** |

🚨 **(g3) DID NOT CLOSE THE SCREEN ON ANY SUB-SCREEN.** The ceiling is not retired. (It was tested on
Settings **and** on Login, both inert with the same positive line.) **I amended no ceiling block.**

### GRAPHICS — `SiegeGraphicsMenuWidget_0`

| limb | observable | observed — node name + PIE `t=` | result |
|---|---|---|---|
| (a) reach it | **only** via Settings' `GraphicsButton` (R7) | `[GraphicsMenu] Code-authored tree built: 10 quality-group rows + preset + resolution scale + 3 steppers + VSync + Auto-Detect + Back.` then `[GraphicsMenu] Opened (class 'SiegeGraphicsMenuWidget', ZOrder 20)` **t≈137.96 s** | **pass** |
| (b) ring on at open | derived from the walk (see §3 note) + `ui_snapshot` at the wrap | `ShowFrameRateCounterCheckBox` **`focused: true`**, **t=247.74 s** | **pass** |
| (c) `IA_MenuDown` MOVES it | the whole-ring walk | 19 consecutive presses, **t≈168.87 → 247.12 s**, strictly advancing, **every line `of 19`** — full list in §3 | **pass** |
| (d) a NON-`UButton` stop | classes reached | **14 of the 19 stops are non-`UButton`**: `ShowFrameRateCounterCheckBox` + `VSyncCheckBox` (`UCheckBox`), `OverallQualitySlider` + ten group sliders + `ResolutionScaleSlider` (`USlider`) | **pass** |
| (e) `IA_MenuRight` on the slider | the **facade**, per `qa/TASK-1410.md` BLOCKER-1 | focus `ViewDistanceQualitySlider`; **one** `IA_MenuRight` **t=387.16 s** → `slider 'ViewDistanceQualitySlider' 0.5000 -> 0.7500 (step 0.2500 … range [0.0000, 1.0000]); read back 0.7500` + `OnControllerCaptureEnd.Broadcast()` + `[SiegeGraphics] 'ViewDistanceQuality' changed — broadcasting OnGraphicsSettingsChanged (broadcast #1).` **FACADE: `ViewDistanceQuality` 2 → 3**, all nine other groups unchanged, `ResolutionScalePercent` unchanged at 87.0 | **pass** |
| (f) on close, where is focus | Accept on Graphics' own `BackButton` **t≈431.53 s** | `menu nav target unregistered -> 'SettingsMenuWidget_0' (registered screen), 3 focus stop(s), 1 screen(s) registered.` + `[GraphicsMenu] Back pressed — dismissing the graphics panel only.` ⇒ the ring went **BACK TO SETTINGS**, not to the main menu: `ConfirmToggleCheckBox` `focused: true` **t=433.05 s**. The `[Settings, Graphics]` → `[Settings]` nesting claim is **verified across files**. | **pass** |

⭐ The second, benign unregister fired and **logged rather than warned**, exactly as `TASK-1417`
predicted: `UnregisterMenuNavTarget('SiegeGraphicsMenuWidget_0'): not registered (already unregistered,
or never was) — no change.`

### LOGIN — `AccountMenuWidget_0`

| limb | observable | observed — node name + PIE `t=` | result |
|---|---|---|---|
| (a) reach it | main-menu Down ×5 then Accept | `IA_MenuAccept -> OnClicked.Broadcast() on 'Button_5' ("Login")` **t≈483.19 s** | **pass** |
| — | **which mode** | 🚨 **`LoggedIn`, NOT `Chooser`** — `BackdropBorder/RootPanel/StatusText` reads **"Logged in as JonBonWes"**, and the boot line already said `1 profile(s), active: JonBonWes`. The row asked me to *"say which was seen"*: **`LoggedIn`.** | recorded |
| (b)/(g1) any node `focused: true` on open | `ui_snapshot` **t=484.71 s** | `BackdropBorder/RootPanel/LogoutButton` **`focused: true`** | **pass** |
| (c) `IA_MenuDown` MOVES it | the `MoveFocus` line | `0 -> 1 of 3 ('SyncNowButton')` **t≈534.05 s** · `1 -> 2 of 3 ('BackButton')` **t≈534.48 s** · `2 -> 0 of 3 ('LogoutButton')` **t≈534.90 s** (wrap) · Up `0 -> 2 of 3 ('BackButton')` **t≈535.31 s** (reverse wrap) | **pass** |
| (d) a NON-`UButton` stop | classes reached | ⛔ **UNOBSERVABLE in this account state.** All three live stops are `UButton`. All four `UEditableTextBox`es — `NameInputBox`, `EmailInputBox`, `PasswordInputBox`, `ConfirmPasswordInputBox` — read `visibility: Collapsed`, `realized: false` at **t=484.71 s** ⇒ correctly excluded. Per `TASK-1419`/`TASK-1420` an all-button ring in a non-form mode is **CORRECT, not a fence-(c) failure**. See §5 for why I could not enter a form. | **unobs** |
| (e) slider | — | no slider on this screen | n/a |
| (f) on close, where is focus | Accept on `BackButton` **t=606.00 s** | main menu **`Button_0`** ("Play (vs Bot)") **`focused: true`**, **t=607.52 s** | **pass** |
| (g3) `IA_MenuBack` closes? | screen still present | **NO — INERT.** Same positive line naming `AccountMenuWidget_0`, **t≈535.75 s** | **inert (predicted)** |

---

## §3 — 🚨 THE GRAPHICS RING, SCORED IN R4a's PRIORITY ORDER — **I USED THE PRIMARY ANCHOR**

**⭐ PRIMARY (the whole-ring walk).** From stop 0, **`IA_MenuDown` ×18 reached `BackButton`, and ×19
wrapped to `ShowFrameRateCounterCheckBox`** — the zero-collision, self-identifying name. The wrap is
corroborated **twice**: the log line, *and* a `ui_snapshot` rooted on `SiegeGraphicsMenuWidget` returning
that node with **`focused: true`** at **t=247.74 s**.

```
t≈168.87 → 247.12 s, nineteen consecutive IA_MenuDown, nothing else pressed:
 0 ->  1 of 19 ('AutoDetectButton')                 9 -> 10 of 19 ('EffectsQualitySlider')
 1 ->  2 of 19 ('OverallQualitySlider')            10 -> 11 of 19 ('FoliageQualitySlider')
 2 ->  3 of 19 ('ViewDistanceQualitySlider')       11 -> 12 of 19 ('ShadingQualitySlider')
 3 ->  4 of 19 ('AntiAliasingQualitySlider')       12 -> 13 of 19 ('ResolutionScaleSlider')
 4 ->  5 of 19 ('ShadowQualitySlider')             13 -> 14 of 19 ('ScreenResolutionPrevButton')
 5 ->  6 of 19 ('GlobalIlluminationQualitySlider') 14 -> 15 of 19 ('WindowModePrevButton')
 6 ->  7 of 19 ('ReflectionQualitySlider')         15 -> 16 of 19 ('VSyncCheckBox')
 7 ->  8 of 19 ('PostProcessQualitySlider')        16 -> 17 of 19 ('FrameRateLimitPrevButton')
 8 ->  9 of 19 ('TextureQualitySlider')            17 -> 18 of 19 ('BackButton')
                                                   18 ->  0 of 19 ('ShowFrameRateCounterCheckBox')  ⬅ WRAP
```

**SECONDARY:** every line reads **`of 19`**, and the registration line printed
`menu nav target registered -> 'SiegeGraphicsMenuWidget_0' (registered screen), **19 focus stop(s)**,
2 screen(s) registered.`
**TERTIARY — deliberately NOT promoted:** `('BackButton')` appears above but I am **not** resting
anything on it. It is not self-identifying (`WBP_SessionMenu`, `USettingsMenuWidget` and
`USiegeGraphicsMenuWidget` all carry one, and `MoveFocus` prints a bare `GetName()`).

**⛔ THE NEGATIVE INVARIANT — `no index repeats twice in a row` — HELD.** Across 19 Down presses, 4 Up
presses and every other navigation this session, **no `MoveFocus` line repeated its pair.** The
pre-Row-A bug printed `17 -> 18 of 24 ('KeepSettingsButton')` **six consecutive times**; this binary
prints a strictly advancing sequence and a clean wrap. **This is the anchor I scored the severed ring on.**

**Criterion (ii) — `Up` traverses the same set in reverse:** `3 -> 2 -> 1 -> 0 -> 18 of 19 ('BackButton')`
at **t≈429.65 → 431.01 s**, i.e. the reverse wrap lands on `BackButton`, not on a dead zone.

**⛔ R4b's TRAP WAS NOT SPRUNG, AND THAT WAS DELIBERATE.** I pressed **no** `IA_MenuLeft`/`IA_MenuRight`
during the ring walk; both Right presses came **after** it was complete. The confirm bar never
un-collapsed, the ring never grew to 21, and **every line read `of 19`**. Keystroke order is recorded
above so this is checkable rather than asserted.

**Row A's index map reproduced exactly.** All nineteen entries match `handoffs/TASK-1469-programmer.md`
§2's AFTER column, including the two things that make it attributable: `KeepSettingsButton` and
`RevertSettingsButton` are **absent** (limb 2 — they sat inside the `Collapsed` `VideoModeConfirmBorder`),
and the three `…NextButton`s are **absent** while the three `…PrevButton`s remain at 14, 15 and 17
(limb 3 — one member admitted per stepper row).

⚠️ **(b) for Graphics is a DERIVATION, said as one.** I took no `ui_snapshot` at the instant Graphics
opened. The ring's *liveness* is nonetheless proven by the walk itself: `MoveFocus` re-reads the current
index from Slate every press, so **had no stop actually held focus, every press would have printed
`0 -> 1`** (the cold branch). It printed a strictly increasing sequence and then wrapped — which is only
possible if each stop genuinely took focus.

---

## §4 — THE NEGATIVE CONTROL (`SC-§137`, `VER-§8` cl. 3(a)) — **IT DISCRIMINATED**

**Declared before it was run** (see the pre-registration file): the same verb, the same lane, the same
instrument, on a stop that must *not* move the facade. I chose `AutoDetectButton` only after reading
`FindStepperPair` (`SiegeMenuInputSubsystem.cpp:1641-1700`) and confirming it demands
`<Base>PrevButton`/`<Base>NextButton` with **equal bases** — a name `AutoDetectButton` cannot satisfy, so
the control provably cannot fire a stepper.

| t (PIE) | focused stop | action | instrument | facade `ViewDistanceQuality` |
|---|---|---|---|---|
| 359.58 | `AutoDetectButton` (plain `UButton`) | — | `MoveFocus(+1): focus moved 0 -> 1 of 19 ('AutoDetectButton')` | 2 |
| **360.10** | `AutoDetectButton` | **`IA_MenuRight`** | **`StepFocusedStop(+1) entered.`** and **no further `Log`-level line** | **2 — UNCHANGED** |
| **387.16** | `ViewDistanceQualitySlider` (`USlider`) | **`IA_MenuRight`** | `slider … 0.5000 -> 0.7500 …` + `OnControllerCaptureEnd.Broadcast()` | **3 — MOVED** |

**Why the control's silence is a reading and not a blind instrument.** The `entered` line printed at
`Log`, so the action **reached** `StepFocusedStop`. From there every branch except one logs at `Log`, and
**none of them appeared** — no `declined: nav target not actionable`, no `declined: no focus stop holds
focus`, no slider line, no check-box line, no *"has no Left/Right semantics"*. The **only** silent branch
at default verbosity is the plain-`UButton` one (`Verbose`). ⇒ By elimination the control took exactly
the intended branch, did nothing, and left the facade where it was — while the identical verb on a
slider moved it, and moved **only** the one group. **The control discriminated.**

Facade read at the object path `/Engine/Transient.UnrealEdEngine_0:GameInstance_0.SiegeGraphicsSettingsSubsystem_0`
— baseline `ViewDistance/AntiAliasing/Shadow/GlobalIllumination/Reflection/PostProcess/Texture/Effects/
Foliage = 2`, `Shading = 3`, `ResolutionScale = 87.0`.

---

## §5 — 🚨 THE TEXT-BOX PREDICTION **STILL DOES NOT GO LIVE**, AND THIS IS THE REPORT'S MAIN CARVE-OUT

R5 and the dispatch both said *"the text-box prediction GOES LIVE ON THIS SITTING"* and that Login's four
`UEditableTextBox`es would be its first real test. **They were not testable, and the reason is the live
account state, not a defect.**

The profile `JonBonWes` is **logged in** (from boot). The Login screen therefore opened in **`LoggedIn`**
mode, in which `ApplyMode` collapses the forms: at **t=484.71 s** all four boxes read
`visibility: Collapsed`, `realized: false`. Row A's limb 2 then correctly refuses them, so **no
`UEditableTextBox` ever became a focus stop.**

**Every route into a form was closed to me, and I checked each:**
- `LogoutButton` → would return to `Chooser` and expose the forms — ⛔ **refused.** It mutates his
  account, and logging back in needs a password I do not have and must never touch (`ACC-§`). Leaving
  Jonathan logged out is not a cost a verification run may impose.
- `LinkCloudButton` → `CloudLinkForm` (which has `EmailInputBox`) — **not reachable**: it reads
  `visibility: Collapsed`, **`enabled: false`**, with `CloudStatusText` = *"cloud session expired - sign
  in again to re-link"*.
- `WBP_SessionMenu`'s `AddressTextBox` → ⛔ **out of scope** — the session menu is `TASK-1427`'s.

⇒ **`TASK-1419` spec-(3) (egress from a focused text box) and `TASK-1469` limb 1's functional payout are
`UNOBSERVABLE` on this sitting — for the third consecutive attempt, and for a NEW reason each time.**
`TASK-1413` could not reach one because none existed in that binary; this run could not reach one because
the live profile is logged in. **Recorded as neither confirmed nor refuted.**

⭐ **WHERE IT GOES LIVE NEXT, NAMED SO IT IS NOT LOST:** `TASK-1427`'s session-menu sitting.
`qa/TASK-1426.md` puts `AddressTextBox` at **stop 3 of 4** on a screen with no mode machine and no login
precondition — it is reachable by four Down presses. **That sitting is now the first real test of Row A
limb 1, and it should be told so.**

**What limb 1 *was* able to show — its DIAGNOSTIC half, and it is a real reading.** R2 warned that
`HandleMenuAccept` (`:828`) and `StepFocusedStop` (`:904`) could log `'None' (none focused)` while the
ring is visibly in a field. Across the whole session: **zero lines matching `none focused` and zero
matching `declined`**, against 111+ `LogSiegeMenuInput` lines from a demonstrably live instrument.
⇒ **R2's rider is SPENT** — Row A repaired all three callers at the root, as its §4 claimed.

⚠️ **R1 is neither spent nor needed.** My `focused:` field (`ui_snapshot`) was **not** all-false on any
screen — it named the true holder every time. But I never took a census **inside a form**, so R1's
conditional risk (`HasUserFocus` being exact-widget) **was never exercised** and remains live for
`TASK-1427`.

---

## §6 — PREDICTED vs ACTUAL STOP COUNTS (spec (2))

| screen / mode | predicted (source) | actual, measured | agree? |
|---|---|---|---|
| Main menu | **7** (`TASK-1469` §2, non-regression) | **7** — every main-menu line reads `of 7`, `Button_0`…`Button_6` | ✅ |
| **Settings** | **3** (`TASK-1415`, stated before 5b) | **3** at registration **and** `of 3` on every walk line | ✅ |
| **Graphics** | **19** (`TASK-1469` count table, re-scored by `TASK-1470`; *not* the pre-Row-A 24) | **19** at registration **and** `of 19` on all 23 walk lines | ✅ |
| **Login — `LoggedIn`** | **2 or 3** (`TASK-1419`: 3 while the cloud block shows a button, else 2) | 🚨 **BOTH, at different instants — see below** | ✅ within prediction |
| Login — `Chooser` / `CreateForm` / `LoginForm` / `CloudLinkForm` | 3 / 5 / 4 / 5 | **not reached** (profile logged in) | unobs |
| Deck builder, Session | — | **not touched** — `TASK-1427`'s | out of scope |

🚨 **THE ONE NUMBER THAT NEEDS ITS OWN SENTENCE, AND IT IS ABOUT THE INSTRUMENT, NOT A DEFECT.**
The Login **registration line printed `2 focus stop(s)`** at t≈483.19 s; the **live ring measured `of 3`**
at t≈534 s, the third stop being `SyncNowButton`. Both numbers are real and both are inside `TASK-1419`'s
predicted range. The gap is the cloud block resolving **after** registration: at t=484.71 s
`SyncNowButton` had become `Visible`/`enabled`/`realized` while `LinkCloudButton` stayed
`Collapsed`/`disabled`. `TASK-1419` shipped `RefreshMenuNavRing()` at five sites for exactly this, and it
logs nothing unless the ring actually fell off — which it did not (`LogoutButton` kept focus throughout).

⚖️ **THE LESSON, OFFERED NOT RULED:** on a **mode machine**, the registration count line is a snapshot of
one instant, **not** a live count — so *"compare the predicted count against the (5) log line"* can read
a transient. **The walk is the authoritative instrument; the count line is corroboration.** On the two
static screens the two agreed exactly, which is what makes the Login divergence legible rather than
alarming. **Routed to the manager, not self-adjudicated** (`SC-§50`, `SC-§101`).

---

## ⛔ ANCHORING DECLARATION — SAID IN THE REQUIRED WORDS

**Anchored by object path** (root + full `name_path`, read from `ui_snapshot`):
- `SettingsMenuWidget_0` → `BackdropBorder/RootPanel/{ConfirmToggleCheckBox, GraphicsButton, BackButton}`
- `AccountMenuWidget_0` → `BackdropBorder/RootPanel/{LogoutButton, SyncNowButton, BackButton,
  NameInputBox, EmailInputBox, PasswordInputBox, ConfirmPasswordInputBox, LinkCloudButton,
  CreateAccountButton, LoginExistingButton, StatusText, CloudStatusText}`
- `WBP_MainMenu_C_0` → `Overlay_19/VerticalBox_0/Button_0` … `Button_6`
- the graphics facade → `/Engine/Transient.UnrealEdEngine_0:GameInstance_0.SiegeGraphicsSettingsSubsystem_0`

⛔ **UNANCHORED — stated in those words:** the **eighteen Graphics stop names in §3's walk are
`UNANCHORED` at the object-path level.** `MoveFocus` prints a **bare `GetName()` with no screen
qualifier**, and I did not snapshot each node's `name_path`. What they *are* anchored by is weaker but
explicit: (1) the registration line naming the active target `SiegeGraphicsMenuWidget_0` immediately
before the walk, (2) the invariant `of 19` on every line, and (3) the wrap target
`ShowFrameRateCounterCheckBox` — censused at **zero collisions project-wide** — independently confirmed
by a `ui_snapshot` **rooted on `SiegeGraphicsMenuWidget`**, which *is* a screen-qualified read.
⭐ `ShowFrameRateCounterCheckBox`, `KeepSettingsButton`, `RevertSettingsButton`, `GraphicsButton` are the
collision-free names in play and are cited bare by permission; `BackButton` is **not**, and is not
promoted anywhere in this report.

---

## §7 — THE PIXEL LIMB (`TWO-LIMB-CONDITIONED-BY-VER-11-CL-9-2026-09-24`)

**STATE limb and PIXEL limb are recorded side by side and are NOT merged.**

- 🤖 **STATE limb — the gate — PASSED** on all four subjects. See §1–§6.
- 🧑 **PIXEL limb — `UNOBSERVABLE`. The four-part bar is NOT specced on this row** (the row header says
  so in its own words), and `VER-§11` cl. 9 makes a ringless capture `UNOBSERVABLE`, never a
  `VERIFY-FAILED`.
  🚨 **AND A STRONGER, HONEST LIMIT I WILL NOT PAPER OVER: I captured three frames to disk, but no image
  content reached my context** — `attach_pie_frames` returned its manifest and statistics only. ⇒ **I did
  not see these frames, so I make NO claim about what is drawn in them** — not "a ring is visible", not
  "the ring is on `BackButton`". The Evidence section below reports only what I can attest: the file,
  its dimensions, and its measured luma statistics. 🧑 **Their adjudication is Jonathan's.**
- 🧑 **(2b) *"does it look right?"* remains his, forever.**

---

## Evidence (promoted)

- `.claude/pipeline/playtest-evidence/2026-09-25/VER-TASK-1421-t03m33s-graphics-backbutton-reached-by-down-x18.png`
  — composited PIE frame, **1280×725**, `mean_luma 13.7`, `pct_near_black 0.8996`. Captured at the
  instant the STATE limb recorded `17 -> 18 of 19 ('BackButton')`, i.e. the Graphics screen with the ring
  on `BackButton` after eighteen Downs. ⛔ **I did not see this image; no pixel claim is made.**
  🚨 **THE FILENAME'S TIME STAMP IS WRONG AND I CANNOT FIX IT.** The true stamp is **PIE `t = 220.858 s`
  (= 03m40s)**, not `03m33s` — I chose the name before the capture returned its stamp. I hold **no tool
  that renames or moves bytes** (`VER-§7` cl. 4), so the rename is **OWED to the commit host
  (`TASK-1422`)**; the correct name is
  `VER-TASK-1421-t03m40s-graphics-backbutton-reached-by-down-x18.png`. Recorded rather than left to pass
  as correct. **The two later frames deliberately omit the `-t` token** so the defect cannot recur.
- `.claude/pipeline/playtest-evidence/2026-09-25/VER-TASK-1421-graphics-ring-wrapped-to-showframeratecountercheckbox.png`
  — composited PIE frame, **1280×725**, `mean_luma 13.5`, `pct_near_black 0.9091`, **PIE `t = 292.010 s`**.
  Captured after the 19th Down, when the STATE limb recorded `18 -> 0 of 19
  ('ShowFrameRateCounterCheckBox')` and `ui_snapshot` read that node `focused: true`.
  ⛔ **Not seen by me; no pixel claim.**
- `.claude/pipeline/playtest-evidence/2026-09-25/VER-TASK-1421-login-loggedin-mode-ring-on-backbutton.png`
  — composited PIE frame, **1280×725**, `mean_luma 49.7`, `pct_near_black 0.0`, **PIE `t = 564.024 s`**.
  The Login screen in `LoggedIn` mode with the STATE limb recording `BackButton` focused.
  ⛔ **Not seen by me; no pixel claim.**

Non-promoted, cited above and left where they are (gitignored):
`Saved/Logs/GitClaudeUnrealTest.log` (every line quoted in this report, 16:36:58 → 16:47:xx) · the
auto-armed background recording at `Saved/AuraVerify/rec_1790354218583615100_40` (never collected — every
reading here is a log line, a node read or a facade read, not a frame).
Pre-registration (outside the repo, by design): `…\scratchpad\TASK-1421-prereg.md`.

---

## Hypotheses (NOT verdicts)

1. **The Login count divergence (§6)** — *hypothesis:* the registration line is emitted before
   `RefreshCloudBlock` settles the cloud row, so on a mode machine the printed count trails the live ring
   by one refresh. **Measured:** `2` at registration, `3` on the walk, `SyncNowButton` realized in
   between. **Not measured:** the ordering inside `NativeConstruct`/`RefreshCloudBlock`. Not my call to
   prescribe.
2. ⚠️ **A STALE CLAIM IN A SHIPPED LIMITATION, FOUND BY READING BEFORE PRESSING.** `TASK-1415`'s L7 says
   *"Accept on a checkbox is STILL a silent no-op."* **The shipped code disagrees:** `HandleMenuAccept`
   casts to `UCheckBox` and **toggles it** (`SiegeMenuInputSubsystem.cpp:1308-1312`, carrying the comment
   *"TASK-1409 (2): `UCheckBox` ⇒ Accept toggles"*). **I therefore did NOT fire Accept on
   `ConfirmToggleCheckBox`** — it would have flipped a real persisted setting to satisfy (g2), and
   (g2) was satisfied instead by the product's own `GraphicsButton` Accept. Its state is recorded
   untouched: `CheckedState: Checked`, `IsFocusable: true`, `bIsEnabled: true` at t=94.60 s. **This is a
   documentation discrepancy, not a defect** — routed, not self-adjudicated.
3. ⚠️ **A LAYOUT OBSERVATION I CANNOT EXPLAIN AND AM NOT ATTRIBUTING.** The Settings panel was **narrower
   after** Graphics closed than before it opened: `BackdropBorder/RootPanel` `abs_w` **959.65 → 460.65**,
   `ConfirmToggleCheckBox` `abs_h` **50 → 75**, and the hint text re-wrapped (`abs_w` 906 → 407), between
   t=42.68 s and t=433.05 s. `dpi_scale` (0.6706) and viewport (1280×725) were **identical** at both
   instants, so it is not a DPI or resolution artefact. **Mechanism unknown; no row blamed.** Flagged
   because it is cosmetic-visible and nobody asked me to look for it.

---

## `VER-§7` cl. 2 DECLARATION — every name reached THROUGH the runner

Inside `run_verification_sequence` I used exactly four step types: **`inject_input_action`**,
**`wait_pie_seconds`**, **`ui_snapshot`**, **`get_widget_property_in_pie`**. All four are **also
standalone tools on my own line** — no step reached a name I do not hold directly.
⛔ **I invoked NO PIE-world-mutation name: no `pie_scene_edit` (and therefore no `spawn_actor` /
`delete_actor` / `set_actor_property` / `set_actor_transform` / `set_actor_enabled` / `teleport_player`),
no `call_actor_function`, no `set_player_transform`, no `start_state_recording`.**

**`execute_unreal_python_readonly` — every call was a read**, enumerated so it can be audited:
`os.getpid()` · `sys.executable` · `SystemLibrary.get_command_line()` · `EditorLevelLibrary.get_editor_world()` ·
`get_editor_subsystem(UnrealEditorSubsystem)` + `get_game_world()` · `GameplayStatics.get_game_instance()` ·
`Paths.project_dir()` + `os.stat` on the DLL · `hasattr`/`dir` reflection · `unreal.find_object()` ·
`SiegeGraphicsSettingsSubsystem.get_quality_group_names()` (static) · the two `BlueprintPure` getters
`get_quality_group_level()` / `get_resolution_scale_percent()`.
**No property was written, no asset touched, no editor state mutated, nothing saved.**

Other granted verbs used: `load_level` · `start_pie` / `stop_pie` **on the session I started myself** ·
`is_pie_active` · `capture_pie_frame` · `attach_pie_frames` · `get_input_mapping_context_keys` ·
`get_unreal_output_logs` · `ui_snapshot` · `get_widget_property_in_pie` ·
`mcp__unreal_inspector__grep` · `Read`/`Grep`/`Write`/`Edit`.
⛔ **No code edit, no asset write, no compile, no git, no editor-lifecycle action. PID 18236 left UP.**
⛔ **NOT A COMMIT.** Nothing staged or committed; 🧑 Jonathan commits this project himself.

---

## Not examined / limitations this run

1. ⛔ **THE TEXT-BOX LIMB IS `UNOBSERVABLE` — THE HEADLINE CARVE-OUT.** See §5. `TASK-1419` spec-(3) and
   `TASK-1469` limb 1's functional payout were not reachable because the live profile is logged in, so
   all four `UEditableTextBox`es were `Collapsed`/`realized: false`. **Neither confirmed nor refuted.**
   ⭐ **It goes live at `TASK-1427` (session menu, `AddressTextBox` = stop 3 of 4) — that sitting should
   be told it is now the first real test.**
2. ⛔ **LOGIN'S OTHER FOUR MODES WERE NOT REACHED** — `Chooser`, `CreateForm`, `LoginForm`,
   `CloudLinkForm`. Their predicted counts (3 / 5 / 4 / 5) are **unfalsified**, not confirmed. Reaching
   them needs a logout I must not perform.
3. ⛔ **THE DECK BUILDER AND SESSION MENU WERE NOT TOUCHED** — `TASK-1427`'s sitting, excluded by the
   dispatch.
4. ⚠️ **A SETTING WAS CHANGED BY THE FEATURE UNDER TEST AND MAY HAVE PERSISTED.** `ViewDistanceQuality`
   went **2 → 3** through the product's real commit edge (`OnControllerCaptureEnd` → the screen's facade
   writer → its save path). If it reached `Saved/Config/…/GameUserSettings.ini` that file is
   machine-local and gitignored, but it is a **real state change**. **I did not revert it**: doing so
   means re-opening Settings → Graphics and pressing `IA_MenuLeft` on one exact slider, which re-enters
   R4b's arming trap for a cosmetic tidy-up. Declared rather than chased, per `TASK-1413`'s precedent.
   ⚠️ Also inherited and untouched: `ShadingQuality` was **already 3** at session start — `TASK-1413`'s
   own declared residue, corroborating its limitation 7.
5. ⚠️ **`Verbose` LINES DID NOT PRINT** and R6's four console writes were **not made** — I hold no console
   verb. No conclusion rests on a `Verbose` line; §4's control is argued by elimination over the
   `Log`-level branches.
6. ⚠️ **THE BUDGET WAS OVERRUN AND I AM SAYING SO.** The inherited budget (`TASK-1413` (4)) wants every
   observable before `t≈60 s`. Settings' registration landed at **t≈41 s**, but Graphics is reachable
   **only** through Settings (R7) and Login is a separate descent, so the last observable landed at
   **t=607 s**. It cannot be batched away — R7 makes the order serial by construction.
7. ⚠️ **`SC-§118` IDENTIFICATION IS PARTIAL** — `FCommandLine::Get()` returned empty, so the editor was
   identified by in-process PID + exe path + editor-world presence. No `-game` session of Jonathan's was
   present (`get_game_world()` was `None` before I started PIE) and **nothing was killed, closed or
   relaunched by me.**
8. ⚠️ **EIGHTEEN GRAPHICS STOP NAMES ARE `UNANCHORED`** at the object-path level — see the Anchoring
   declaration. The wrap target is the exception and is screen-qualified.
9. ⚠️ **PIXEL LIMB `UNOBSERVABLE`; THE THREE FRAMES WERE NOT SEEN BY ME.** See §7. One filename carries a
   wrong time stamp; the rename is **owed to `TASK-1422`**.
10. ⚠️ **I DID NOT CITE THE AUTOMATION SUITE** as evidence anywhere, per the dispatch — `GetMenuButtons()`
    and `IsNavFocusStop()` have diverged and a green suite covers neither of Row A's new limbs
    (`TASK-1475` is boarded to close it).
11. ⚠️ **ONE SLIDER, ONE DIRECTION, ONE PRESS.** I stepped `ViewDistanceQualitySlider` **+1** once. I did
    not test `IA_MenuLeft`, the other ten sliders, a range bound, or the three stepper rows (deliberately
    — R4b). `qa/TASK-1410.md` WARN-8's physical-pad scenario remains **untested, not cleared**.
12. ⛔ **(g3) DID NOT CLOSE ANY SCREEN**, on either Settings or Login. The practical half of the ceiling
    stands. **I amended no ceiling block** (`TASK-1454` still owns the close target).
