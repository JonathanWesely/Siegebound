Verdict: MEASURED
# Verification — TASK-1399 — [MENU-NAV-RUNTIME-CENSUS]

⭐ **THE RUN THAT WAS BLOCKED IS DONE.** PIE started clean on the restarted editor, the **control arm
fired in every session**, and **all ten screens now have an answer**. ⛔ The previous report at this path
(`Verdict: 🚧 blocked`) is **superseded**; its §2 measurements are carried forward and extended, its §3
table is now filled in.

**Editor/Aura state:** Aura **connected** (`editor_connected`). Editor identified **by command line**
(`SC-§118`), measured by me, not borrowed: `Saved/Logs/GitClaudeUnrealTest.log` line 1 =
`Log file open, 09/24/26 13:38:43` (a **fresh** log — the wedged session's opened `09/21/26 13:00:14`),
line **538** = `LogInit: Command Line: ` — **empty** ⇒ a **GUI editor**, not `-game`, not a commandlet.
🧑 No `-game` instance was seen, touched, read or closed.
**Serialization (`VER-§2` cl. 1):** re-measured myself — `grep '^- status: `(integrating|verifying|compiling)`'`
over `TASKBOARD.md` ⇒ **0 matches**.
**Map:** editor opened on **`/Game/Maps/L_Arena`** (measured by me, as the dispatch predicted) ⇒ I called
`load_level` to `/Game/Maps/L_MainMenu` (`already_open: false`, `discarded_unsaved: false` — nothing was
discarded because nothing was dirty). ⚠️ **Reported as asked.**
**PIE:** `is_pie_active` = `false` before I began. **No session of his existed; nothing was stopped that I
did not start.** Standalone, 1280×725, `recommended_max_pie_instances: 2`.
**Attempts used: 1 of 3** — one continuous attempt containing **six PIE sessions** (a sub-screen cannot be
closed by any live instrument, see §4, so each one-way screen costs a session).
**Wall time ≈ 65 min.**
**Dirty state, measured BEFORE and AFTER (this discharges what the last report correctly refused to claim):**
before — `DIRTY_CONTENT=[]`, `DIRTY_MAPS=[]`; after — `DIRTY_CONTENT=[]`, `DIRTY_MAPS=[]`.
⇒ **Nothing reached disk. Measured, not asserted.**
**Left behind:** editor **up**, on `/Game/Maps/L_MainMenu` (⚠️ **changed from `L_Arena`**, which was where it
opened), **PIE stopped**, nothing dirty.
**The blocker did not recur.** `BP_Basic_Movement` was **not resident** at start (measured:
`BROKEN_BP_PACKAGE_RESIDENT=[('/Game/sA_ArcheryVfxPack/Blueprints/BP_Basic_Movement', False)]`) and
**no Blueprint sweep was performed** — the remaining graphs were read **one at a time, by name, AFTER all
PIE work**, per the dispatch.

---

## 1. 🚨 THE HEADLINE — AND IT IS **NOT** THE ONE THE ROW EXPECTED

**1 screen of 10 answers `IA_Menu*` injection. 6 answer nothing. 3 could not be reached at all.**

🚨 **BUT THE MOST CONSEQUENTIAL READING OF THE RUN IS §6: on the ONE screen that DOES answer injection,
the focus moves and NOTHING VISIBLE MOVES WITH IT.** Two composited frames — one with `Button_0` focused,
one with `Button_4` focused, structured read taken **16 ms** before the second capture — are, to my eye,
**pixel-indistinguishable in the button column**. 🧑 **His entire ask is about an outline.** Read §6 before
sizing any row.

---

## 2. THE PER-SCREEN (a)–(e) TABLE — EVERY READING CITED BY NODE NAME + PIE `t=`

Legend: **(a)** reached by injection · **(b)** any node `focused: true` / named index ≠ `-1` on open ·
**(c)** does that reading MOVE under `IA_MenuDown`/`IA_MenuUp` · **(d)** does `IA_MenuAccept` actuate ·
**(e)** focus on closing back to the parent.

| # | Screen | (a) reached | (b) focus on open | (c) moves on `IA_MenuDown`/`Up` | (d) `IA_MenuAccept` | (e) focus on close | split (`VER-§8` 3(a)) |
|---|---|---|---|---|---|---|---|
| 1 | **Main menu** ⭐ **CONTROL** | n/a — it is the entry point | ✅ **YES** `Button_0` `focused:true` — **six independent sessions**: t=19.439 · 31.806 · 19.841 · 15.205 · 13.588 · 16.138 | ✅ **YES** `Button_0`→`Button_1` t=19.872 →`Button_2` t=20.306; →`Button_4` t=33.289 & t=18.184; →`Button_5` t=21.641; →`Button_3` t=16.372 | ✅ **YES ×5**, each logged by the engine with its own button name (§3) | ⛔ not measured (no screen could be closed) | **POSITIVE** |
| 2 | **Deck builder** | ✅ **YES** — `IA_MenuAccept` on `Button_2` t=53.909 ⇒ `WBP_DeckBuilder_C_0` present t=55.923 | ⚠️ **PARTIAL** — root `WBP_DeckBuilder_C_0` `focused:true` t=83.290, but `FocusedCardIndex` = **-1** t=55.923 ⇒ **the grid is not armed** | ⛔ **NO** — `FocusedCardIndex` **-1 → -1 → -1** at t=55.923 / 56.634 / 57.267 across two injected Downs | ⛔ not separable — no focusable target to actuate | ⛔ **UNOBSERVABLE** — could not be closed (§4) | **MEASURED negative** |
| 3 | **Settings** 🧑 *his named screen* | ✅ **YES** — `IA_MenuAccept` on `Button_4` t=33.306 ⇒ `SettingsMenuWidget_0` present t=35.339 | ⛔ **NO** — **all 9 nodes** `focused:false` t=57.121, incl. `ConfirmToggleCheckBox`, `GraphicsButton`, `BackButton` | ⛔ **NO** — tree **byte-identical** t=58.371 after `IA_MenuDown` t=57.138 **and** `IA_MenuUp` t=57.754 | ⛔ **NO** — `IA_MenuAccept` t=58.388; `SiegeGraphicsMenuWidget` **absent** t=60.404 | ⛔ **UNOBSERVABLE** — could not be closed | **MEASURED negative** |
| 4 | **Graphics** | ⛔ **NO** — its only route is `GraphicsButton`, and Accept is inert on Settings (row 3 (d)) | ⛔ | ⛔ | ⛔ | ⛔ | ⛔ **UNOBSERVABLE (this screen only)** |
| 5 | **Login / Account** | ✅ **YES** — `IA_MenuAccept` on `Button_5` t=21.658 ⇒ `AccountMenuWidget_0` present t=23.674 | ⛔ **NO** — **all 24 nodes** `focused:false` t=23.674, incl. `LogoutButton`, `SyncNowButton`, `BackButton` | ⛔ **NO** — tree **byte-identical** t=24.329 after `IA_MenuDown` t=23.696 | ⛔ **not attempted** (see §8) | ⛔ **UNOBSERVABLE** — could not be closed | **MEASURED negative** |
| 6 | **Multiplayer / Session** | ✅ **YES** — `IA_MenuAccept` on `Button_3` t=16.388 ⇒ `WBP_SessionMenu_C_0` present t=18.405 | ⛔ **NO** — **all 13 nodes** `focused:false` t=18.405, incl. `HostButton`, `JoinButton`, `BackButton`, `AddressTextBox` | ⛔ **NO** — tree **byte-identical** t=19.038 after `IA_MenuDown` t=18.422 | ⛔ **not attempted** | ⛔ **UNOBSERVABLE** — could not be closed | **MEASURED negative** |
| 7 | **Victory / defeat** | ⛔ **NO** — requires a match to conclude; not reachable inside this budget | ⛔ | ⛔ | ⛔ | ⛔ | ⛔ **UNOBSERVABLE (this screen only)** |
| 8 | **War map** | ⛔ **NOT RESOLVED** — `IA_WarMap` injected **3×**; `ui_snapshot` missed on **three distinct names**: `WBP_WarMap` (t=17.517, t=18.149), `WarMapWidget` (×2), `WarMap` (t=180.072) | ⛔ | ⛔ | ⛔ | ⛔ | ⛔ **UNOBSERVABLE (this screen only)** — ⚠️ and see §5, the arena instrument was **alive** |
| 9 | **Controls help (Tab)** ⭐ **ARENA CONTROL** | ✅ **YES** — root `SiegeControlsHelpWidget` reads **`SelfHitTestInvisible`** (= open, `.cpp:3011`) and flips to **`Collapsed`** (= shut, `.cpp:2928`) on a second `IA_ControlsHelp` ⇒ **the toggle demonstrably fired** | ⛔ **NO** — zero `focused: true` in the whole payload (⚠️ depth-1 read, §8) | ⛔ **NO** — no change under `IA_MenuDown` | ⛔ not attempted | n/a | **MEASURED negative** |
| 10 | **Assistant console** | ✅ **YES** — root `SiegeAssistantConsoleWidget` reads **`SelfHitTestInvisible`** after `IA_AssistantConsole` | ⛔ **NO** at time of read — `InputBox` (`EditableTextBox`) `focused:false` t=177.393 ⚠️ **NOT read at the instant of opening** (§8) | ⛔ **NO** — `InputBox` `focused:false` again t=178.021 after `IA_MenuUp`; root visibility unchanged | ⛔ not attempted | n/a | **MEASURED negative** |

**TALLY — the number the row exists to produce:**
**1 answers `IA_Menu*` · 6 measured NOT answering · 3 unreachable (UNOBSERVABLE, per-screen only).**

🚨 **THE CONTROL ARM FIRED, IN EVERY SESSION, BEFORE EVERY NEGATIVE** (`SC-§137`). No negative in this
report stands alone: each sub-screen was **reached by the very injection lane** that is then observed to do
nothing inside it — the instrument is proven alive **in the same session, seconds earlier, by the act of
opening the screen being tested**. That is a stronger control than the row asked for.

---

## 3. ⭐ INDEPENDENT CORROBORATION FROM THE PROJECT'S OWN LOG — AND WHAT IS **ABSENT** FROM IT

`LogSiegeMenuInput`, **12 lines total** for the whole run, read from the live log:

```
[20.42.22:737][218] L_MainMenu: IMC_MainMenu applied at priority 0 on 'PlayerController_0'; IA_MenuUp / IA_MenuDown / IA_MenuAccept bound (Started).
[20.43.16:619][393] IA_MenuAccept -> OnClicked.Broadcast() on 'Button_2' ("Deck Builder").
[20.44.52:137][ 65] L_MainMenu: IMC_MainMenu applied ... (Started).
[20.45.25:392][ 14] IA_MenuAccept -> OnClicked.Broadcast() on 'Button_4' ("Settings").
[20.46.17:653][100] L_MainMenu: IMC_MainMenu applied ... (Started).
[20.46.39:249][351] IA_MenuAccept -> OnClicked.Broadcast() on 'Button_5' ("Login").
[20.46.58:566][461] L_MainMenu: IMC_MainMenu applied ... (Started).
[20.47.14:904][397] IA_MenuAccept -> OnClicked.Broadcast() on 'Button_3' ("Multiplayer").
[20.47.52:907][626] L_MainMenu: IMC_MainMenu applied ... (Started).
[20.48.06:463][394] IA_MenuAccept -> OnClicked.Broadcast() on 'Button_0' ("Play (vs Bot)").
[20.53.36:280][805] L_MainMenu: IMC_MainMenu applied ... (Started).
[20.53.57:432][ 18] IA_MenuAccept -> OnClicked.Broadcast() on 'Button_4' ("Settings").
```

Two things, and the **second is the load-bearing one**:
1. ✅ Every Accept the table claims is **named by the engine itself**, button by button. The `ui_snapshot`
   focus reads and the subsystem's own log **agree on every single one**.
2. 🚨 ⛔ **THERE IS NOT ONE LINE FOR ANY INJECTION MADE WHILE A SUB-SCREEN WAS OPEN**, and ⛔ **NOT ONE
   `IMC_MainMenu applied` LINE FOR THE `L_Arena` SESSION AT ALL.** The map gate and the coverage gate are
   therefore **observed at runtime**, not merely read in source.
   ⚠️ **Stated honestly:** the subsystem logs a *successful* Accept. An early-out logs nothing. So the
   absence is **consistent with** a decline, it does not **prove** which line declined (§7).

---

## 4. ⛔ A ONE-WAY DOOR — WHY (e) IS UNOBSERVABLE ON FOUR SCREENS, AND WHY THE RUN COST SIX SESSIONS

Once a sub-screen is open, **no instrument available to this lane can close it**:
- `IA_MenuAccept` / `IA_MenuDown` are inert (that is the finding itself);
- `simulate_key_press "Escape"` was taken **as a measurement** at t=83.307 — and the **observable was not
  `binding_found`**, it was that **`WBP_DeckBuilder_C_0` was still alive at t=84.824**. ⇒ `VER-§5` cl. 5's
  named dead end is **re-affirmed**, on an outcome rather than on a flag.
- `ui_perform` clicks are the other named dead end and were **deliberately not re-bought**.

⇒ each one-way screen costs a whole PIE session, and **(e) is honestly UNOBSERVABLE for rows 2/3/5/6** —
⛔ **not a claim that focus is lost on close.**

⭐ **BUT (e)'s MECHANISM IS ANSWERED STATICALLY, AND IT IS A REAL DELIVERABLE — see §5.3.**

---

## 5. ⭐ THE SECOND DELIVERABLE — THE BLUEPRINT BLIND SPOT IS **CLOSED**

### 5.1 Every remaining graph read — **one at a time, by name, after the PIE work**

| asset | Functions | **Key-handler override?** |
|---|---|---|
| `WBP_HUD` | 13, all `Update*`/`Setup*` | ⛔ **NONE.** Events = `Construct`, `Tick`, + delegate handlers only |
| `WBP_CardHand` | 7 layout/art builders | ⛔ **NONE.** Events = `Construct` + delegate handlers |
| `WBP_SessionMenu` | ⭐ **ZERO** | ⛔ **NONE — a genuinely empty graph.** Corroborates `TASK-1398` §4 with a **second, independent instrument** |
| `WBP_WarMap` | ⭐ **ZERO** | ⛔ **NONE — empty graph.** Same corroboration |
| `WBP_DeckBuilder` | `BuildSandboxButton`, `RefreshAll`, `BuildDetailsPanel`, `RefreshDetailsPanel`, `SplitGrid` | ⛔ **NONE** in the graph (its keys are the C++ Slate handlers `TASK-1398` already cited) |
| `BP_MenuGameMode` | `Construction Script` | ⛔ **NONE.** Events = `BeginPlay` only |

⇒ ⛔ **Across all ten screens' Blueprint assets, ZERO key-handler overrides exist.** Combined with the
previous run's `WBP_MainMenu` / `WBP_VictoryScreen` / `WBP_DeckCardTile` reads, **the blind spot
`TASK-1398` declared is now fully discharged.**
⚠️ The control limit from the previous report **stands unrelaxed**: no Blueprint in this project has an
`On Key Down` override to serve as a **direct positive control**, so this is "none is listed", not "none can
exist unlisted" (`SC-§39`).

### 5.2 ✅ `BP_MenuGameMode`'s REAL PATH — the previous run's unresolved item

⛔ **`/Game/Maps/BP_MenuGameMode` does not exist.** The asset is **`/Game/Blueprints/BP_MenuGameMode`**
(`Content\Blueprints\BP_MenuGameMode.uasset`), parent `GameModeBase`. **Resolved and read.**

### 5.3 🚨⭐⭐ `TASK-1398` §7 ITEM 5 — **ANSWERED, AND IT CHANGES `TASK-1400`'s SHAPE**

The deck builder's **Exit** button (`Conv_StringToText "Exit"`) is wired to `OnBackClicked`, whose graph is:

```
Custom Event: OnBackClicked
  -> CreateWidget (Class = WBP_MainMenu_C)
  -> Is Valid
  -> AddToViewport (ZOrder = 0)
  -> RemoveFromParent (self)
```

⇒ 🚨 **THE DECK BUILDER'S EXIT CREATES A BRAND-NEW `WBP_MainMenu` INSTANCE — it does NOT re-show a
remembered one — and there is NO focus call of any kind in the chain.**
⇒ ⭐ **This is the same shape as `USessionMenuWidget::BackPressed` (`.cpp:151-165`).** `TASK-1400`'s census
note said the re-arm "must work against a FRESH widget, not a remembered pointer" and flagged the deck
builder as BP-opaque. **It is no longer opaque, and it confirms the harder of the two cases for BOTH
exits.** ⛔ Handed to the manager as a measurement; ⛔ I amend nothing (`SC-§101`).

### 5.4 ✅ `WBP_MainMenu`'s SEVEN BUTTONS, IN ORDER (`TASK-1398` §7 item 2 — owed since boarding)

Read from a **live** `ui_snapshot` (the design-time tree does not contain them; `Construct` builds them):

| index | node | label |
|---|---|---|
| 0 | `Button_0` | **"Play (vs Bot)"** |
| 1 | `Button_1` | "Sandbox (No Bot)" |
| 2 | `Button_2` | "Deck Builder" |
| 3 | `Button_3` | "Multiplayer" |
| 4 | `Button_4` | "Settings" |
| 5 | `Button_5` | "Login" |
| 6 | `Button_6` | "Quit" |

⭐ Index 2 = "Deck Builder" **matches `TASK-1274`** exactly; the other six are new. ⭐ `Buttons[0]` —
`TASK-1400`'s required re-arm target — is confirmed to be **"Play (vs Bot)"**, 🧑 the top option he named.

---

## 6. 🚨🚨 THE SURPRISE — THE FOCUS MOVES AND THE PIXELS DO NOT

The row asked to be embarrassed by a surprise. **This is it, and it is the run's most important line.**

**The measurement.** Two composited frames of the same menu, 2.06 s apart, same session:
- **t=16.138** — before any injection. Structured read (six sessions, §2 row 1) says `Button_0` holds focus.
- **t=18.200** — after 4× `IA_MenuDown`. A `ui_snapshot` **16 ms earlier**, at **t=18.184**, reads
  `Button_4` `focused: true` and all six others `false`. The engine's own log then fires Accept
  `on 'Button_4' ("Settings")`.

**What the pixels show:** ⛔ **I can identify no difference between the two frames in the button column.**
In **both**, exactly one row renders wider and lighter — **`Button_3` "Multiplayer"** — and that row is
**NOT the focused row in either frame**. The focused row (`Button_0`, then `Button_4`) carries **no outline,
no tint change, nothing I can resolve** in either capture.

**Why the obvious escape is closed:** the capture is 1086×615 from a 1280×725 viewport — a **1.18×**
downscale. It resolves `Button_3`'s style difference perfectly well, so it is **not too coarse to show a
row-level change**. I re-took the pair explicitly to test this; `max_dim: 0` does not exceed 1086.

⚠️ **HYPOTHESES, NOT A VERDICT (`SC-§101`) — I did not determine the cause:**
- **H1 (leading):** `WBP_MainMenu`'s buttons are built at runtime by `Construct` via
  `GenericCreateObject Class=Button` with **default styling**, and a default `UButton` style defines
  Hovered/Pressed/Disabled but **no distinct focused appearance** ⇒ there may be **nothing authored to draw**.
- **H2:** `Button_3`'s constant lighter render is the **Hovered** brush from a stale mouse position, which
  would explain why it never moves.
- **H3:** a capture-path artifact. **Weakened** by the fact that a row-level difference *is* resolvable.

🚨 ⛔ **WHY THIS MATTERS ENOUGH TO STOP AND READ: `TASK-1400` is boarded to re-fire `ApplyInitialFocus()`
so that 🧑 "that outline appears at the top option." If H1 holds, `TASK-1400` can succeed COMPLETELY —
focus genuinely placed on `Buttons[0]` — AND HE WILL STILL SEE NO RING**, because the ring is not the same
thing as the focus. ⛔ A row that asserts focus and never looks at a pixel would pass and not fix his
complaint.
⚠️ ⛔ **AND THE HONEST COUNTERWEIGHT: 🧑 his own words presuppose an outline he has SEEN.** So either it
renders in a context I did not capture, or it renders too subtly for me to resolve, or it does not render.
⛔ **I am not calling it. This needs his eye, or a row that measures the button's style asset directly.**

---

## 7. THE (3) PREDICTION — **SCORED, AND HALF OF IT IS REFUTED**

The row predicted: *"most sub-screens read **(b) yes** / **(c) NO**."*

| half | verdict | evidence |
|---|---|---|
| **(c) NO** | ✅ **CONFIRMED, 6 of 6 reached sub-screens** | every (c) cell in §2 |
| **(b) yes** | ⛔ **REFUTED, 5 of 6** | Settings (9 nodes), Login (24), Session (13), Controls help, Console all read **no focused node at all**. Only the deck builder read any focus — and its grid index was still `-1` |

⇒ 🚨 **The sub-screens are in WORSE shape than the row predicted: they do not have focus sitting in the
wrong place, they have NO FOCUS AT ALL.** This **corroborates `TASK-1398` F1 at runtime** — *"for 8 screens
the work is AUTHORING focus logic, not routing it"* — which was a static claim and is now an observed one.
⛔ **Sizing implication (handed to the manager, not decided here): every one of these screens needs initial
focus authored, a focus-visual authored, and a walker that handles `UCheckBox` / `USlider` /
`UEditableTextBox` (`TASK-1398` F4) — not a re-route of an existing handler.**

---

## 8. `TASK-1405` LEG 1 — **DISCHARGEABLE** (⛔ I close nothing)

LEG 1 needs `WBP_MainMenu`'s own graph enumerated for its `Enter` handling. It **is** enumerated (previous
run, §2.2 there): `Construct`, five `*BtnClicked`, 21 `OnClicked_Event*`, four touch-template bound events,
and **no key handler of any kind**. §5.1 now adds that **no Blueprint anywhere in the ten screens carries
one**. ⇒ **the enumeration LEG 1 was waiting on exists.** ⛔ **Closing LEG 1 is the manager's act
(`SC-§101`); I only report that the input is available.**

---

## Evidence (promoted)

`.claude/pipeline/playtest-evidence/2026-09-24/` — written **directly** by the capture tool (verified
present on disk by `Glob`; no path is claimed that does not exist):

- `VER-TASK-1399-mainmenu-focus-state-before-injection.png` — PIE t=16.138. The seven-button menu over the
  blue-sky menu backdrop; labels top-to-bottom Play (vs Bot) / Sandbox (No Bot) / Deck Builder /
  Multiplayer / Settings / Login / Quit. `Button_0` holds keyboard focus; **the only row rendering
  differently is "Multiplayer" (`Button_3`)**, which is not it.
- `VER-TASK-1399-mainmenu-focus-state-after-4-menudown.png` — PIE t=18.200, `Button_4` "Settings" focused
  per a structured read 16 ms earlier. **Indistinguishable from the frame above**; "Multiplayer" is still
  the only differently-rendered row. ⭐ **These two are the §6 pair.**
- `VER-TASK-1399-settings-no-focus-after-menudown.png` — PIE t=24.485. The Settings panel: title
  "Settings", the "Confirm AI orders before they execute" checkbox and hint, and **"Graphics" and "Back"
  rendered as two flat, identically-styled light bars with no outline on either**, after an injected
  `IA_MenuDown`. The dimmed main menu shows through behind it.

⚠️ **TWO FILES IN THAT FOLDER ARE MISNAMED BY ME, AND I AM DECLARING IT RATHER THAN LEAVING IT TO BE FOUND:**
`VER-TASK-1399-mainmenu-focus-ring-control.png` and `VER-TASK-1399-mainmenu-ring-moved-to-settings.png`
were captured **before** §6 was discovered, and their slugs assert a **"ring"** that the pixels **do not
show**. They are the same two states as the first two files above and are **superseded by them**.
⛔ **Do not cite them.** I have **no shell and cannot delete or rename a file**, so **their removal is owed
to the host row.**

Re-greppable primary sources (all read-only; none copied, none modified):
- `Saved/Logs/GitClaudeUnrealTest.log` — line 1 (open stamp), line 538 (empty command line), and the
  12 `LogSiegeMenuInput` lines quoted verbatim in §3
- `Saved/AuraVerify/t1399_*.png` (the in-flight captures of the deck builder, session menu and login
  panels — **not promoted**, since §2 cites their structured reads rather than their pixels)
- `Saved/AuraVerify/rec_1790282538094210500_14/recording.h264` — session 1's auto-collected film (nvenc,
  3,894 frames)

## Hypotheses (not verdicts)

- ⚠️ **§6's H1/H2/H3** — the focus-visual question. **The pixel observation is measured; the cause is not.**
- ⚠️ **The mechanism of every (c) negative** is presumed to be `TASK-1398` F2's coverage gate
  (`IsMenuUncovered()` → false the moment any other top-level widget is visible). **Consistent with**
  §3's silence and with `applied_mapping_contexts: ["IMC_MainMenu"]` still being applied while the deck
  builder was open — i.e. **the context is NOT withdrawn; the subsystem declines.** ⛔ Not proven: I never
  observed the early-out itself.
- ⚠️ **The war map's non-resolution** (§2 row 8) has **two** readings I cannot separate: `IA_WarMap` did not
  fire, **or** the widget exists but is not resolvable by the snapshot matcher. ⛔ The arena instrument was
  **demonstrably alive** in that same session (row 9's visibility flip), which makes "the lane is dead"
  the *least* likely of the three — but that is inference, not measurement.

## Not examined / limitations this run

- ⛔ **Screens NOT reached, named individually:** **Graphics** (route blocked by row 3's inert Accept),
  **Victory / defeat** (needs a match to conclude), **War map** (three names, three misses).
- ⛔ **(e) on rows 2/3/5/6** — no instrument can close a sub-screen (§4). **No claim is made about focus on
  close.** §5.3 answers the *mechanism* statically; it does not observe the *focus*.
- ⛔ **(d) not attempted on Login, Session, Controls help, Console** — budget went to (a)/(b)/(c), which
  size the epic. Named, not glossed.
- ⚠️ **Row 9's (b) is a DEPTH-1 read.** Zero `focused: true` appeared in the payload, but I did not walk the
  controls-help subtree to full depth. **Weaker than rows 3/5/6, and marked as such.**
- ⚠️ **Row 10's (b) is NOT a read at the instant of opening** — `InputBox` was sampled at t=177.393,
  ≈160 s after the console opened and after several intervening injections. ⛔ **This report does NOT claim
  the console fails to focus on open** (`TASK-1398` cites `InputBox->SetKeyboardFocus()` at `.cpp:1400`); it
  claims only that the box read `focused:false` at that later instant and did not change under `IA_MenuUp`.
- ⛔ **No direct positive control for an `On Key Down` override** exists in this project (§5.1).
- ⛔ **`WidgetBlueprintLibrary` is not exposed to the read-only Python lane** (`module 'unreal' has no
  attribute 'WidgetBlueprintLibrary'`) — so a live widget census had to be done by name-guessing through
  `ui_snapshot`. **Named so no later row re-buys it.** The previous run's two dead instruments
  (`function_graphs`/`ubergraph_pages`, CDO reflection) were **not** re-bought.
- ⛔ **`start_pie` returns a recovered film manifest that overflows the tool-result limit** (4 occurrences,
  114k–469k chars). I read the **head** of one to confirm `status: pie_requested` and did **not** read any
  of them in full. ⛔ **No finding in this report rests on their contents.**
- ⛔ **`VER-§5` cl. 5's dead ends were not re-traced** beyond the single Escape measurement in §4.
- ⛔ **No** code, asset, compile, git, or editor-lifecycle action. `CONVENTIONS.md` untouched; no other
  row's line and no other agent's file written. **No clause amended, no row boarded** (`SC-§101`).

### `VER-§7` cl. 2 declaration (census §5 names reached through the runner)

**Through `run_verification_sequence` (8 invocations):** `ui_snapshot`, `inject_input_action`,
`wait_pie_seconds`, `get_widget_property_in_pie`, `capture_pie_frame`, `simulate_key_press`.
**Direct:** `load_level` ×2, `start_pie` ×6, `stop_pie` ×6 (**every one on a session I started myself**),
`is_pie_active` ×2, `start_pie_recording` ×1 (armed pre-PIE).
**Inspector reads:** `get_headless_status`, `get_text_file_contents`, `grep`, `get_unreal_output_logs` ×2,
`get_asset_meta` ×6 (**by name, one at a time, after all PIE work**), `get_asset_graph` ×1,
`execute_unreal_python_readonly` ×3 (all read-only).
⛔ **NO `pie_scene_edit`, NO `call_actor_function`, NO `spawn_actor`, NO mutating op of any kind.**
⛔ **`binding_found` is cited nowhere as an observable** (`VER-§8` cl. 10). It appeared once, in the Escape
result, and §4 states explicitly that the observable used instead was the widget's continued existence.
**Budget (cl. 2(b)):** the `t≈60 s` fence drove the batching — 8 sequences carried ~60 actions.
