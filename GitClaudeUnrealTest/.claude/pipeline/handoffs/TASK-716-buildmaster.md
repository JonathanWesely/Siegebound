# TASK-716 — build-master handoff — THE ARENA ADJUDICATION

> ## ⛔ **VERDICT: `BLOCKED — desktop unavailable`.**
> **The go/no-go probe failed. The game was NEVER LAUNCHED, no game capture exists, and ⛔ NO ADJUDICATION WAS PERFORMED.**
> ⛔ **This is ⛔ NOT a `PASS`. ⛔ It is ⛔ NOT a `FAIL` of the artifact either — it is a NON-EXECUTION OF THE INSTRUMENT.** ⚠️ **TASK-700 REMAINS BLOCKED and the zip does not ship**, exactly as if this were a FAIL. The open question `PKG-§6a` asks — *does the Shipping exe render an arena* — is **still open**, unchanged from TASK-699.

Law: `PKG-§9f` (the click route) · `PKG-§6a` (the bar) · `SHIP-§8b` (the verdict discipline) · `PKG-§10` (stage hygiene, re-verified) · standing `TASK-076` (no simulated input on a locked desktop).
Date 2026-08-30, probe window 01:03–01:09.
Predecessor read in full: `handoffs/TASK-715-buildmaster.md`.

**Write surface this task touched: this handoff, five read-only probe scripts + two captures in the session scratchpad, and one `TASKBOARD.md` status line. ⛔ Nothing else.**

---

## 1. ⛔ THE GO/NO-GO PROBE — RUN FIRST, BEFORE ANYTHING WAS LAUNCHED

The task ordered the desktop condition probed **before** launching anything. It was, and it failed. **The exe was never started.**

### 1a. The first probe returned a genuinely AMBIGUOUS reading — and ambiguity was resolved by measurement, not by assumption

```
INPUT_DESKTOP        = Default                          <- looks UNLOCKED
LOGONUI_PROCS        = 1  (pid 36160, session 5)        <- looks LOCKED
FOREGROUND_HWND      = 196808  TITLE='Windows Default Lock Screen'   <- looks LOCKED
CURSOR_ROUNDTRIP_OK  = True   (SetCursorPos honoured)   <- looks UNLOCKED
SCREEN_CAPTURE_OK    = True                             <- inconclusive
SCREEN               = 1536x960 primary
```

⚠️ **Two facts pointed each way, and a third muddied it further: `LockApp.exe` (pid 20988) owns the foreground window, but ALL 13 of its threads read `Wait/Suspended`** — which is normally the *post-unlock* idle state. ⛔ **A go/no-go answered from any single one of these signals would have been wrong.** ⇒ escalated to the only instrument that cannot be argued with: **pixels**, then **window ownership**.

### 1b. THE PIXELS — the primary monitor is a lock screen, and the shell is nowhere on it

Full virtual-screen grab (`VirtualScreen` = X −1280, W 5120, H 960; three monitors) then cropped to primary `DISPLAY1` (0,0)-(1536,960):

- **What is on screen:** a **Windows Spotlight lock-screen photograph** (aerial view of a baroque castle) filling the entire primary monitor. The two secondary monitors are **black**.
- ⛔ **No taskbar. No desktop icons. No window of any kind.**
- ⚠️⚠️ **AND THAT IS THE PROOF, because five live top-level windows EXIST in session 5 and NONE of them is visible:** `UnrealEditor` ("GitClaudeUnrealTest - Unreal Editor"), `WindowsTerminal`, `claude`, `ApplicationFrameHost` ("Settings"), `SystemSettings`. **Windows that exist, are not minimised, and cannot be seen ⇒ something full-screen is occluding the entire shell.**

### 1c. WINDOW OWNERSHIP — the click rig's own abort condition, proven mechanically

`t669_topclick.ps1` aborts (`ABORT_not_topmost_at_point`, exit 3) unless `GetAncestor(WindowFromPoint(pt), GA_ROOT)` **equals the game's hwnd**. I evaluated exactly that expression, **read-only, injecting no click**, at five candidate menu points:

| Client point | Root window under cursor | Owner | Class |
|---|---|---|---|
| (768,480) | 9570386 | `explorer` (9124) | **`LockScreenBackstopFrame`** |
| (768,300) | 9570386 | `explorer` | **`LockScreenBackstopFrame`** |
| (768,600) | 9570386 | `explorer` | **`LockScreenBackstopFrame`** |
| (200,100) | 9570386 | `explorer` | **`LockScreenBackstopFrame`** |
| (1400,900) | 9570386 | `explorer` | **`LockScreenBackstopFrame`** |

⇒ ⛔⛔ **EVERY point on the primary monitor is owned by the lock-screen backstop.** The rig would abort at every candidate coordinate, and **`SetWindowPos(HWND_TOPMOST)` cannot raise a game window above the lock-screen backstop** — that is what the backstop is for. ⇒ **The click rig CANNOT DELIVER INPUT, established by the rig's own predicate rather than by inference from the wallpaper.**

📌 **The `INPUT_DESKTOP = Default` reading is a red herring and is recorded so the next agent is not fooled by it:** the Windows 11 lock screen renders on the **Default** desktop via `LockApp` + explorer's `LockScreenBackstopFrame`; only the *credential* stage switches to `Winlogon`. ⛔ **`OpenInputDesktop() == "Default"` is NOT a proof of an unlocked session.** Likewise **`SetCursorPos` succeeding proves nothing** — the pointer moves while the backstop eats every button event, which is precisely the `TASK-076` silent no-op.

### 1d. ⛔ NOT A ONE-SAMPLE CONCLUSION — the lock is SUSTAINED

Rather than declare BLOCKED on one reading (and to give an at-the-machine Jonathan a window to unlock), I polled **read-only, 12 samples over 4 minutes, 01:05:43 → 01:09:23**:

```
[1]..[12]  centerClass=LockScreenBackstopFrame  fgTitle='Windows Default Lock Screen'
           LockApp=1  LogonUI=1  LOCKED=True
RESULT=STILL_LOCKED
```

**12/12 locked. No transient, no race, no flicker.** ⇒ ⛔ **STOP.**

---

## 2. THE ROUTE I WOULD HAVE USED — recorded so the re-run is a re-run, not a re-design

Fully prepared and **not executed**. `PKG-§9f` is followed exactly; ⛔ **no map argument and no `-ExecCmds` appear anywhere in it:**

1. Launch **`…\packagedZIPofGame\Windows\GitClaudeUnrealTest.exe`** (the root shim — the player's double-click target) with **NO ARGUMENTS**. 715 proved the shim resolves to `…\Binaries\Win64\GitClaudeUnrealTest-Win64-Shipping.exe`.
2. Settle, resolve the Shipping pid, read the client rect via `t669_wincap.ps1 -Mode rect`, capture the menu.
3. Drive the game's **own** 7-entry menu with `t669_topclick.ps1 -ProcId <shipping pid> -X <x> -Y <y>` to reach a live match — the real **menu → level-travel → HUD** path.
4. `t669_wincap.ps1 -Mode cap` after travel settles, then adjudicate the capture against the `PKG-§6a` bar under `SHIP-§8b`.
5. Close **my own** game process via `CloseMainWindow`, confirm 0 game processes.

⛔ **I did not run step 1.** A launch would have put a stolen-focus fullscreen window behind a lock screen, produced only lock-screen or occluded pixels, and risked leaving a process on Jonathan's locked machine — **all cost, zero evidence.**

---

## 3. ⛔ PER-CRITERION OBSERVATIONS — **ZERO OF FOUR OBSERVED**

`SHIP-§8b(2)` demands affirmative, per-criterion statements of **what was seen**. **I saw none of it, and I will not manufacture any.**

| # | `PKG-§6a` criterion | What I actually saw | Status |
|---|---|---|---|
| 1 | a real card deck built from `DT_Cards` rows (⛔ not blank slots) | **NOTHING — the game was never launched.** | ⛔ **NOT OBSERVED** |
| 2 | HUD present | **NOTHING — the game was never launched.** | ⛔ **NOT OBSERVED** |
| 3 | hero / arena rendering | **NOTHING — the game was never launched.** | ⛔ **NOT OBSERVED** |
| 4 | ⛔ not a menu | **NOTHING — not even a menu was reached this run.** | ⛔ **NOT OBSERVED** |

⚖️ **Under `SHIP-§8b(4)` an ambiguous or absent capture is a `FAIL`, never a pass — so the ONE thing this outcome is definitely not is a `PASS`.** I am reporting it as **`BLOCKED`** rather than `FAIL` because the task's own go/no-go clause names that word for exactly this condition, and because **the distinction is load-bearing for the manager:** a `FAIL` would be a measured statement *about the artifact* and would send work back to the programmer/artist; **`BLOCKED` is a statement about the INSTRUMENT and sends the task back to the scheduler.** ⛔ **Both stop the zip identically.**

---

## 4. ⛔ WHAT I REFUSED TO DO — named, because each was available and each would have been a lie

- ⛔ **Did not launch the exe and report process liveness / window title as a pass.** 715 and 699 already have liveness + title `Siegebound`; repeating it buys **nothing** and dressing it as an arena verdict is the `PKG-§5a` failure with our own gate as the false witness.
- ⛔ **Did not fall back to the map argument or `-ExecCmds`.** `PKG-§9f` measured that Shipping ignores both. It would have produced a menu, and **a menu is never a pass.**
- ⛔ **Did not substitute the Development exe against the package's cooked bytes.** That instrument exists and still works (715 §1c), but it is **699's already-ratified CONTENT half** — ⛔ it proves nothing about **rendering**, which is the only thing this task was buying. `PKG-§9f`: *neither half substitutes for the other.*
- ⛔ **Did not use `PrintWindow` on an occluded window to fake a capture.** `PW_RENDERFULLCONTENT` can sometimes pull pixels from a covered window; a capture obtained that way, of a game that never got a single click through the backstop, would show a menu at best — **and would be an unattended boot-verify silently degraded to "the menu came up", the exact thing `PKG-§9f` forbids by name.**
- ⛔ **Did not dismiss the lock screen, click through it, or touch ANY window of Jonathan's** — not the Unreal Editor (pid 17044, still up), not the terminal, not Settings. **No input event of any kind was injected this task.** The only pointer motion was the probe's own round-trip test, **and the cursor was restored to its original position (768,480).**
- ⛔ **Did not lower the bar.** *"The desktop was locked"* is an argument about the **instrument**, never about the **bar**.

---

## 5. ✅ COLLATERAL: TASK-715's PRUNED STAGE RE-VERIFIED — ⛔ NO FINDING, IT IS UNALTERED

Read-only. The task told me to report any alteration as a finding rather than fix it. **There is none — it reconciles to the byte:**

| Check | 715 recorded | Measured now | |
|---|---|---|---|
| Stage files | 70 | **70** | ✅ |
| Stage bytes | 1,731,428,706 | **1,731,428,706** | ✅ exact |
| Shipping exe SHA-256 | `A853A1E5…B8472` | **`A853A1E5369EDFDD31A3346DB8FDF3367E1F7FE97806C331EE05D49DC6AB8472`** | ✅ byte-identical |
| Shipping exe size | 177,716,736 B | **177,716,736 B** | ✅ |
| Root shim SHA-256 | `7F2C8FD6…F4B5` | **`7F2C8FD657C18E6031F0C1EEF8DACB86A4134630C99A45BEE61BD99B3300F4B5`** | ✅ |
| ProductName / Company | `Siegebound` / `Jonathan Wesely` | **`Siegebound` / `Jonathan Wesely`** | ✅ |
| Deleted orphan `.exe` / `.pdb` | absent | **both still absent** | ✅ |
| `PKG-§10` invariant | 1 game exe + shim | **1 game exe + shim** (+2 VC redist *installers*) | ✅ holding |
| Game processes running | 0 | **0** | ✅ I launched none |

⇒ ⭐ **The build a re-run will adjudicate is provably the same build 715 handed over.** `PKG-§10`'s ordering (*prune → measure → boot-verify → zip*) is intact and the boot-verify slot is simply still empty.

---

## 6. HOW TO DISCHARGE THIS — it is one action, and it is Jonathan's

⚠️ `PKG-§9f` states the cost honestly: **Shipping boot-verify is SCHEDULABLE, ⛔ not unattended.** This run is that constraint arriving in practice, on schedule, and the gate behaved correctly.

- 🙋 **FOR JONATHAN — EITHER OF THESE CLOSES IT:**
  1. **Unlock the machine and say go.** Re-dispatch TASK-716; §2 above is a ready-to-run route and nothing needs re-designing. Expect a **stolen-focus fullscreen window** for roughly a minute — that is the rig working. It is fully automated from there.
  2. ⭐ **Or adjudicate it yourself — `SHIP-§8b(7)`: your eye wins and it is faster than anything I can do.** Double-click
     `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\Windows\GitClaudeUnrealTest.exe`
     click **Play**, and tell me the four things: **is there a real card deck (not blank slots) · is the HUD there · is the hero/arena rendering · is it not a menu.** Four yeses is the PASS this gate needs.
- ⛔ **Until one of those happens, TASK-700 does not zip.** ⚖️ **That is the gate working as designed, ⛔ not an obstruction:** the last Development cook booted a flawless menu on a game with **no deck, no HUD and no hero**, and that is the exact failure this checkpoint exists to catch.

---

## 7. CAPTURES RETAINED (`SHIP-§8b(6)`)

⚠️ **Both are lock-screen evidence of the BLOCK. ⛔ NEITHER is a game capture, and ⛔ neither may ever be cited as boot-verify evidence.**

- `C:\Users\wesel\AppData\Local\Temp\claude\C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest\10fcb540-8a89-457a-9798-070dabfaf278\scratchpad\t716_probe_screen.png` — full virtual screen, **5120x960**, all three monitors.
- `C:\Users\wesel\AppData\Local\Temp\claude\C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest\10fcb540-8a89-457a-9798-070dabfaf278\scratchpad\t716_probe_primary.png` — primary `DISPLAY1` crop, **1536x960** — the lock screen with the entire shell occluded.

**Probe scripts (all read-only, reusable at re-dispatch), same scratchpad directory:**
`t716_desktopprobe.ps1` (input desktop / cursor / capture) · `t716_probe2.ps1` (foreground-window ownership) · `t716_probe3.ps1` (**the click-rig abort predicate — run this one first next time**) · `t716_lockpoll.ps1` (bounded unlock poll) · `t716_stagecheck.ps1` (stage integrity) · `t716_screencap.ps1` (multi-monitor grab).

---

## 8. FENCES — ALL HELD

⛔ No zip · ⛔ no commit · ⛔ no push · ⛔ **zero git writes of any kind** · ⛔ no editor contact, no MCP call, no cook, no compile · ⛔ the staged tree **not modified** (verified byte-identical, §5) · ⛔ nothing under `Source/`, `Content/`, `Config/` touched · ⛔ **no process of Jonathan's closed, killed or disturbed** (his Unreal Editor pid 17044 is still up, untouched) · ⛔ **no game process launched, so none to shut down** (count 0, before and after).

**Deviation from spec:** exactly one, and it is the spec's own instruction — **the job was not performed, because the go/no-go probe that the spec ordered run first returned NO-GO.** ⛔ No weaker instrument was substituted, and no pass was claimed from process liveness.

## 9. FINDINGS FOR THE MANAGER

1. ⛔⛔ **`PKG-§6a`'s arena question is STILL OPEN. Nothing in this task advanced it.** ⚠️ **Do not let TASK-716's completion-as-a-task be mistaken for its completion-as-a-gate** — the handoff exists, the adjudication does not.
2. 📌 **NEW, WORTH WRITING INTO LAW BESIDE `PKG-§9f`'s unlocked-desktop cost — THE LOCK PROBE MUST BE `WindowFromPoint`, ⛔ NOT `OpenInputDesktop`.** On this locked machine, `OpenInputDesktop()` returned **`"Default"`** and `SetCursorPos` **succeeded** — ⛔ **both of the obvious probes said UNLOCKED while the session was locked.** The two that told the truth are **the class of the window under the click point (`LockScreenBackstopFrame`)** and **the pixels**. ⇒ ⭐ **A future `ship.ps1` desktop gate that checks `OpenInputDesktop` will pass on a locked machine and then boot-verify into a backstop** — the fake-gate family (`SHIP-§8c`) again. **The correct mechanical check is the click rig's own predicate**, and `t716_probe3.ps1` is it.
3. ⚠️ **This is a standing, recurring tax on every Shipping `/ship`, not a one-off.** `PKG-§9f` already says *schedulable, not unattended*; **this run is the first time it actually stopped work**, and it will happen again on any overnight or unattended ship. Worth deciding deliberately whether `/ship` should **detect and refuse early** (cheap — before a ~30 min cook) rather than discover it at `C3` after the cook is spent. ⇒ ⭐ **Suggest a phase-A gate using finding 2's predicate**, sibling to TASK-712's `A6` machine-class check.
4. ✅ **715's stage is verified intact** (§5) — TASK-700's rider R2 paths are unchanged and still correct.
5. ⛔ **TASK-700 stays blocked.** ⚖️ **A BLOCKED boot-verify and a FAILED boot-verify are equally unshipped.**
