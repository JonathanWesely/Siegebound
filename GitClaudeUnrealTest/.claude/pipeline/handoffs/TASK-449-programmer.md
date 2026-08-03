# TASK-449 — [W1-WIRE] The open-key seam: bind `IA_AssistantConsole`, create/show the console, drive the posture

- **assignee:** gameplay-programmer · **date:** 2026-08-03 · **status on completion:** `ready-for-qa`
- **QA gate:** TASK-446 (item **(f)**) · **compile gate:** TASK-447 — **the ONE gate, quiet-module law**
- **files touched (EXCLUSIVE owner, sequential re-ownership after TASK-440):**
  - `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegePlayerController.h`
  - `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegePlayerController.cpp`
- **files NOT touched:** everything else. ⛔ No component edit, no widget edit, no `Build.cs`, no `Content/`, no `.ini`, no Git, no editor, no MCP, no PIE, **no compile.**

> **M8 DECLARATION, verbatim: adds no replicated property, no new replicated class, no new relevancy tier.**
> And *why* it is true here rather than only asserted: everything this task adds is **client-local input plumbing**. `AssistantConsoleAction` / `AssistantConsoleActionAsset` are input assets resolved on the locally-controlled controller; `AssistantConsoleWidget` is a `Transient` viewport widget that exists only on the machine that pressed the key; `bAssistantConsoleOpen` (TASK-440's, unchanged here) is a plain non-`UPROPERTY` bool describing local UI focus. **No RPC, no `GetLifetimeReplicatedProps` entry added or changed, no authoritative outcome reachable from this file.** Opening a text box decides nothing about the match.

---

## 0. ⚠️ TASK-440's WORK WAS EDITED ADDITIVELY AND NOTHING OF IT WAS REVERTED — HERE IS THE PROOF, NOT THE ASSERTION

`git diff --stat` on the two files, before and after my session:

| | insertions | deletions |
|---|---|---|
| after TASK-440 (its handoff §6, its own reading) | 425 | 94 |
| after TASK-449 (now) | **715** | **94** |

⇒ **+290 insertions, and the deletion count is UNCHANGED at 94.** Every one of those 94 deletions is TASK-440's (the `SpawnGroupCircleDecal` private→public move, verifiable as the only `-` hunk in the header). **I deleted nothing, reformatted nothing, and moved nothing.** The posture owner at `:4118`, `CanOpenAssistantConsole()`, `SetAssistantConsoleOpen()` and all four mutual-exclusion mirrors are byte-identical to how TASK-440 left them — **I read them and called them; I did not touch them.**

📌 **The board's correction is confirmed at the artifact, first-hand, not relayed.** TASK-444's handoff §5(a) claims the `ApplyCursorInputState` posture term is missing. **It is present**, `SiegePlayerController.cpp:4118`:

```cpp
const bool bWantCursor = bInPlacementMode || bInTargetingMode || (GroupPickStage != EGroupPickStage::None) || bAssistantConsoleOpen || bUICursorHeld;
```

Of TASK-444 flag (a)'s three items — *(i)* bind the key, *(ii)* create the widget, *(iii)* the posture term — **(iii) was already done by TASK-440 and this task delivers (i) and (ii).**

---

## 1. WHAT I VERIFIED FIRST-HAND ABOUT THE GAP (the relayed-diagnosis law applies to the spec's premise too)

Repo-wide over `Source/**.{h,cpp}`, **before** my edits:

| symbol | call sites outside its own definition | after this task |
|---|---|---|
| `USiegeAssistantConsoleWidget::CreateAndAddToViewport` | **0** | **1** (`SiegePlayerController.cpp:4355`) |
| `SetAssistantConsoleOpen` | **0** | **6** |
| `OnConsoleOpenChanged` `AddDynamic` | **0** | **1** (+ the matching `RemoveDynamic` in `EndPlay`) |
| any `BindAction` for an assistant action | **0** | **1** |
| `ToggleConsole` | **0** | ⚠️ **still 0 — DELIBERATE, see §4** |

**The gap was real and it is now closed for the OPEN direction.** ⛔ **It is NOT closed for the SUBMIT direction — see §6, which is the single most important thing in this handoff.**

---

## 2. THE BINDING (spec item 1) — the shipped idiom, mirrored character-for-character

Three additions, each cloned from `IA_CmdFollow` / `IA_CmdAmbush`:

```cpp
// constructor (.cpp, immediately after CmdFollowActionAsset)
AssistantConsoleActionAsset = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_AssistantConsole.IA_AssistantConsole")));

// SetupInputComponent (.cpp, after the CmdFollow resolve)
AssistantConsoleAction = ResolveInputAction(AssistantConsoleAction, AssistantConsoleActionAsset, TEXT("IA_AssistantConsole"), TEXT("TASK-445"));

// SetupInputComponent (.cpp, after the CmdFollow binding, inside the same UEnhancedInputComponent block)
if (AssistantConsoleAction)
{
    EnhancedInputComponent->BindAction(AssistantConsoleAction, ETriggerEvent::Started, this, &ASiegePlayerController::OnAssistantConsolePressed);
}
```

- Header: `TObjectPtr<UInputAction> AssistantConsoleAction` — `UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")`, placed directly after `CmdFollowAction`; `TSoftObjectPtr<UInputAction> AssistantConsoleActionAsset` — `UPROPERTY(EditDefaultsOnly, Category = "Input")`, directly after `CmdFollowActionAsset`.
- ✅ **INERT-NULL-SAFE IS THE DESIGNED STATE AND IT IS WHY I AM NOT BLOCKED ON TASK-445.** With `/Game/Input/Actions/IA_AssistantConsole` absent, `ResolveInputAction` returns null after **one** `Warning` naming `TASK-445` and the asset path, the `if` skips the binding, and the key is **inert**. **Never a crash. Zero effect on any other action.**
- ⚠️ **A `UInputAction` reference is not the input MAPPING.** TASK-445 still owns creating the asset **and** mapping it in `/Game/Input/IMC_Hero`, and still **FLAGS-never-stomps** if its proposed key (`Enter`) is taken. **Nothing in this task chooses or asserts a key** — the word `Enter` appears in no code I wrote.

### ⛔ §2 COMPLIANCE — I ADDED A BINDING AND RE-ROUTED NONE

Every pre-existing `BindAction` in `SetupInputComponent` is **untouched**: `IA_Card1`, the `IA_Card2..6` payload loop, `IA_UICursor` (×3 trigger events), `IA_CancelPlace`, `IA_CmdAttack`, `IA_CmdHold`, `IA_CmdDefend`, `IA_CmdAmbush`, `IA_CmdFollow`. **No key is made to pass through the assistant**, no binding was moved, re-ordered, wrapped or conditioned. My `if (AssistantConsoleAction)` block is appended **last**, inside the existing `UEnhancedInputComponent` cast, so the `else` warning path is unchanged too.

---

## 3. ⛔ THE HANDLER'S EXACT ORDERING — GUARD FIRST, UI SECOND (spec item 2, the load-bearing part)

`ASiegePlayerController::OnAssistantConsolePressed()`, statement by statement, in order:

| # | statement | why it is where it is |
|---|---|---|
| 1 | `if (bAssistantConsoleOpen \|\| (AssistantConsoleWidget && AssistantConsoleWidget->IsConsoleOpen()))` | **The toggle asks "is it open?" BEFORE "may it open?"** The close path must never be gateable — a refusable close is a close that can strand the cursor in `GameAndUI`. **Either half counting as open** is deliberate: they can only disagree through a bug, and the key is then what repairs it. |
| 2 | → `AssistantConsoleWidget->CloseConsole()` (null-checked) | Hides the widget, lowers any prompt, hands focus back to the viewport, broadcasts `OnConsoleOpenChanged(false)`. |
| 3 | → `SetAssistantConsoleOpen(false); return;` | **Spec item 2's literal requirement.** Usually a no-op (statement 2's broadcast already cleared it through my handler) and **kept anyway**: it is the one line that guarantees the posture cannot survive the key even if the widget is null, already closed, or a future change drops the broadcast. Closing is never refused. |
| 4 | `if (!SetAssistantConsoleOpen(true)) { return; }` | ⛔ **THE POSTURE IS TAKEN BEFORE ANYTHING EXISTS.** This is the whole task. Re-gates on `CanOpenAssistantConsole()` — placement / targeting / group pick / match ended. |
| 5 | `Console = GetOrCreateAssistantConsoleWidget()` | **First** construction happens **only after** the grant, so a refused open never even allocates the widget. |
| 6 | `if (Console == nullptr) { SetAssistantConsoleOpen(false); return; }` | Creation failed ⇒ **roll the posture back.** A cursor owner with no UI behind it is the soft-lock. |
| 7 | `Console->OpenConsole();` | Shows it and focuses the box. Posture is already `GameAndUI` at this point — **focus lands into a posture that already accepts it**, not the reverse. |
| 8 | `if (!Console->IsConsoleOpen()) { SetAssistantConsoleOpen(false); UE_LOG(...); }` | ⚠️ **`OpenConsole()` CAN REFUSE SILENTLY AND HAS NO RETURN VALUE** (`SiegeAssistantConsoleWidget.cpp:451-461`: a **disabled** console — the assistant fault latch — returns having shown nothing). **I ask the widget instead of assuming**, and release the posture. |

### ⛔ WHAT HAPPENS ON A REFUSAL — LITERALLY NOTHING VISIBLE

On the statement-4 refusal (a group pick, placement or targeting is live, or the match ended):

- **no widget is constructed** — `GetOrCreateAssistantConsoleWidget()` is not reached;
- **nothing is added to the viewport**, so nothing can flash;
- **no keyboard focus moves** — `FocusInputBox()` is inside `OpenConsole()`, which is never called;
- **no transcript line, no BIE, no delegate broadcast**;
- **one log line, and it is `SetAssistantConsoleOpen`'s own** (it already names which owner refused). I deliberately add **no second log** — one refusal, one line.

On the statement-8 refusal (widget disabled) the widget itself showed nothing, and I **release the posture and log once at `Log` verbosity**.

---

## 4. ⚠️ `ToggleConsole()` IS DELIBERATELY NOT CALLED, AND THIS IS THE ONE DECISION QA SHOULD RULE ON

The spec's `names` list includes `USiegeAssistantConsoleWidget::ToggleConsole`. **I did not call it, and calling it would have broken the ordering this task exists to enforce.**

`ToggleConsole()` is `if (bConsoleOpen) CloseConsole(); else OpenConsole();` — **it shows the console itself, before any controller code can gate it.** Binding the key straight to it would produce exactly the forbidden sequence: *widget appears → controller then discovers it was not permitted → widget is yanked away*, having already seized keyboard focus. The spec's ⛔ clause and `ToggleConsole()` are **mutually exclusive**, and the ⛔ clause wins.

⇒ The controller performs the toggle itself (§3 statements 1–8) and calls the **primitive** `OpenConsole()` / `CloseConsole()`. `ToggleConsole()` remains correct, remains public, and remains **uncalled in the game module** — it is the natural binding for a Blueprint or a future FSM-driven toggle that does not need the posture gate. **If QA prefers it deleted or documented as controller-only, that is a widget-file change and TASK-444 owns that file, not me.**

---

## 5. THE POSTURE BINDING + LIFETIME (spec items 3 and 4)

**Creation is LAZY, exactly as specced** — `GetOrCreateAssistantConsoleWidget()` is the only construction site:

```cpp
AssistantConsoleWidget = USiegeAssistantConsoleWidget::CreateAndAddToViewport(this);   // adds it CLOSED
if (AssistantConsoleWidget)
{
    AssistantConsoleWidget->OnConsoleOpenChanged.AddDynamic(this, &ASiegePlayerController::HandleAssistantConsoleOpenChanged);
}
```

- The function early-outs when the widget already exists, so **the delegate can never be double-bound** and can never fire twice per change.
- Held as `UPROPERTY(Transient) TObjectPtr<USiegeAssistantConsoleWidget> AssistantConsoleWidget` (declared beside `HUDWidget`), exposed by `UFUNCTION(BlueprintPure) GetAssistantConsoleWidget()`.
- Once created it **stays on the viewport, closed**, for the rest of the match — the widget collapses itself rather than being removed, so the transcript survives a close/reopen.

**`HandleAssistantConsoleOpenChanged(bool bOpen)` — `UFUNCTION()`, the posture-honesty half:**

- **`bOpen == false` ⇒ `SetAssistantConsoleOpen(false)`.** ⛔ **This is the whole reason the binding exists.** The console can close by routes this controller never sees: `CancelPressed()` with no prompt up, `SetConsoleEnabled(false)` when the assistant faults, or any later FSM-driven close (TASK-443). **A posture flag stuck true is a cursor soft-locked in `GameAndUI` with nobody willing to release it**, and `ApplyCursorInputState` is exactly where this project's input bugs have lived.
- **`bOpen == true` ⇒ take the posture; if the guard refuses, close the widget** rather than let the two disagree.
- **Termination is by construction, not by luck.** On the key path the flag is already true, so `SetAssistantConsoleOpen(true)` takes its no-op early-out. On the refusal path `CloseConsole()` sets `bConsoleOpen = false` **before** it re-broadcasts, so the nested call lands on the `!bOpen` branch and stops. (`FMulticastScriptDelegate` also copies its invocation list before invoking, so the re-entrant broadcast is safe on the engine side — ⚠️ **read, not executed**.)

**Teardown, added to `EndPlay` beside the four shipped symmetric teardowns:** `RemoveDynamic` **first** (making the release deterministic — one explicit call rather than a broadcast arriving during teardown, and no dynamic delegate left pointing at a controller that is going away), then `CloseConsole()`, `RemoveFromParent()`, null the member, then `SetAssistantConsoleOpen(false)`.

⚠️ **`bAssistantConsoleOpen` is still written by exactly ONE function.** I never assign it directly anywhere — every clear goes through `SetAssistantConsoleOpen`, including in `EndPlay`. TASK-440's invariant is preserved.

---

## 6. ⛔ THE RESIDUAL SEAM — THE **SUBMIT** DIRECTION IS STILL UNWIRED, AND IT IS TASK-443's

**Read this before anyone reports "the console works".** This task closes the **OPEN** seam. The **widget → component** forwards are **not** taken by anyone yet:

| widget (out) | component (in) | bound today? |
|---|---|---|
| `OnConsoleSubmitted(const FString&)` | `SubmitUtterance(const FString&)` | ⛔ **NO** |
| `OnConsoleConfirmed()` | `ConfirmPressed()` | ⛔ **NO** |
| `OnConsoleCancelled()` | `CancelPressed()` | ⛔ **NO** |
| `OnConsoleOpenChanged(bool)` | `NotifyConsoleOpened` / `NotifyConsoleClosed` | ⛔ **NO** (my binding on this delegate is the **controller's posture** only) |
| component (out) `OnAssistantStateChanged` / `OnAssistantMessage` / `OnAssistantAvailabilityChanged` | `SetAssistantState` / `ShowTranscriptLine` / `SetConsoleEnabled` | ⛔ **NO** |

⇒ **As of this commit, pressing the key opens a console you can type into and whose Enter goes nowhere.** That is **by design for this task** — the spec's item 3 ruling is explicit: *"The controller owns creation, visibility and posture; the COMPONENT (TASK-443) binds the widget's delegates and owns the FSM."* I deliberately did **not** take those bindings: doing so would (a) exceed an explicit ⚖️ ruling and (b) risk **double-binding** if TASK-443 also binds — and a doubly-bound `OnConsoleSubmitted` spends **two model turns per sentence**, which the queue-depth-1 design would surface as a spurious "busy" refusal rather than an obvious bug.

### 🚩 FLAGGED FOR TASK-443 AND FOR THE MANAGER — A REAL ORDERING HAZARD IN THE HANDOFF I AM GIVING YOU

**`GetAssistantConsoleWidget()` returns NULL until the player's first successful open.** Creation is lazy by spec. ⇒ **A consumer that binds once at `BeginPlay` binds to nothing and no-ops forever** — the same silent-failure shape as CONVENTIONS §11's `NativeOnInitialized` trap (no crash, no log, no ensure). `USiegeAssistantComponent`'s own header already flags (`:225`) that *"no task in this batch is EXPLICITLY assigned the widget's creation and this binding"*.

**Three shapes that work, so TASK-443 has nothing left to decide:**
1. **Push by pulling** — for the component → widget direction, call `GetAssistantConsoleWidget()` **at push time** and no-op on null. Works today with zero change to any file.
2. **Bind at the creation moment** — the landing site is exactly four lines, in `ASiegePlayerController::GetOrCreateAssistantConsoleWidget()` immediately after my `AddDynamic`. ⚠️ **That is a controller edit and TASK-443's spec says "no controller edits"** — so it needs a manager ruling, not a programmer's initiative.
3. **A creation broadcast** — a new controller-side delegate. I did **not** add one: it is public surface nothing binds yet, and inventing it would pre-empt the ruling.

⚠️ **This does not block TASK-448 by itself** (TASK-443 lands before it), but **if TASK-443 slips, Jonathan gets a console that opens and does nothing.** That is a materially different failure from the one this task fixed, and it should be said out loud rather than discovered at the playtest.

---

## 7. ⛔ WHAT I COULD NOT VERIFY — NOTHING HERE HAS BEEN COMPILED OR RUN

**No compiler, no UHT, no linker, no editor and no engine has seen a line of this.** TASK-447 is the one gate (quiet-module law). Concretely unchecked:

1. **UHT has not run** on: `UFUNCTION(BlueprintPure) GetAssistantConsoleWidget()` returning a pointer to the **forward-declared** `USiegeAssistantConsoleWidget`; the `UPROPERTY(Transient) TObjectPtr<USiegeAssistantConsoleWidget>`; the private `UFUNCTION() HandleAssistantConsoleOpenChanged(bool)`; the two new input `UPROPERTY`s. All four mirror shipped members in the same file (`GetAssistantComponent()` / `AssistantComponent` / `HandleHeroDied` / `CmdFollowAction*`) — **a reading, not a build.**
2. **`AddDynamic` / `RemoveDynamic` are macro-expanded name lookups.** A typo in the function name is a **compile** error, not a runtime one — and the compiler is the only thing that can tell you. **It has not run.**
3. **This file still cannot compile alone**, and that is expected, not a defect: TASK-440's `#include "Siegebound/SiegeAssistantComponent.h"` needs TASK-442 (landed) and my `#include "Siegebound/SiegeAssistantConsoleWidget.h"` needs TASK-444 (landed, uncommitted). **Both headers exist on disk now** — I verified the paths and the class names by reading them — but **nothing has linked them.** ⛔ **Do not open a QA loop over include order** (the TASK-416/417 precedent).
4. **`BindAction`'s overload resolution is unverified.** `&ASiegePlayerController::OnAssistantConsolePressed` is a `void()` member — identical in shape to `OnCmdFollowPressed`, which compiles today. Unproven here.
5. **ZERO runtime verification. Nobody has pressed this key.** The ordering argument in §3 is a **read-through** argument from source: the guard-then-create sequence is *proven by the statement order*, but **that it produces the right screen is not observed.** No PIE, no MCP, no editor — Jonathan is at the keyboard with the model loaded.
6. **The re-entrancy argument in §5 is read from engine source semantics, not executed.** If `FMulticastScriptDelegate` did not copy its invocation list, the nested `CloseConsole()` would still terminate (the `bConsoleOpen` early-out), but the iteration would be the thing at risk — **stated so the claim is checkable rather than trusted.**
7. **The key is inert until TASK-445 lands.** Even after a successful compile, **nothing happens when Jonathan presses anything** until `IA_AssistantConsole` exists and is mapped in `IMC_Hero`. ⚠️ **TASK-448 cannot test this task without TASK-445.**

---

## 8. NOT DONE — DELIBERATELY, AND WHY

- ⛔ **No `BeginPlay` change.** Creation is lazy by spec, so `BeginPlay` needed nothing — and not touching it keeps the HUD/PS-retry path byte-identical.
- ⛔ **No `HandleMatchEnd` / `HandleMatchReset` change.** ⚠️ **Consequence, flagged rather than silently fixed: a console left OPEN when the match ends stays open over the victory screen.** `CanOpenAssistantConsole()` refuses to *open* after match end, but nothing *closes* an already-open one, and `ApplyCursorInputState` early-outs while `bMatchEnded` is latched (so `HandleMatchEnd`'s `UIOnly` stands and the flag simply stops mattering). **This is consistent, not a soft-lock** — flag true + widget visible + the key still closes it. **A one-line close in `HandleMatchEnd` would tidy it; it is outside "you are writing the JOIN, nothing else", so it is a QA/manager call.** Added to the TASK-448 checklist below.
- ⛔ **No `HasAuthority()` check on the open path.** TASK-440's contract #5 and TASK-442 spec item 7 own the assistant's authority refusal **with the keys' approved wording**. Gating the *console* on authority would refuse a client the UI instead of refusing the *order* with the approved sentence — a different, worse behaviour.
- ⛔ **No `PlayerTick` change, no polled fallback key.** `IA_CancelPlace` has one because a stuck placement mode is a soft-lock; a console that will not open is not.
- ⛔ **No `USiegeSettingsSubsystem` read, no confirm-step code, no executor, no `Content/` asset, no `.uasset` reference.** `/Game/UI/WBP_AssistantConsole` stays **RESERVED and unreferenced** — `CreateAndAddToViewport(this)` passes no class, so a future WBP is adopted with zero change here.
- ⛔ **No cleanup, reformat or "tidy" anywhere in either file.**

---

## 9. WHAT QA SHOULD SCRUTINISE HARDEST

1. **§3's ordering table, statement by statement** — specifically that **statement 5 (creation) is unreachable when statement 4 refuses.** That is the entire acceptance criterion of this task.
2. **§4 — that `ToggleConsole()` is uncalled ON PURPOSE.** A reviewer checking the `names` list mechanically will read its absence as an omission. It is the opposite: calling it would violate the ⛔ ordering clause.
3. **Statement 8** — that I detect a **silent** `OpenConsole()` refusal by asking `IsConsoleOpen()` rather than assuming the void call worked. If you disagree that `OpenConsole()` can refuse, read `SiegeAssistantConsoleWidget.cpp:451-461` — **the artifact settles it.**
4. **§6 — the SUBMIT seam is still open**, and whether the lazy-creation/null-until-first-open hazard needs a manager ruling before TASK-443 is dispatched. **I consider this the most consequential thing in this handoff.**
5. **That `bAssistantConsoleOpen` is written by exactly one function** and that my `EndPlay` clear goes through `SetAssistantConsoleOpen(false)` rather than assigning the member.
6. **§2 — that no shipped `BindAction` was re-routed.** The diff in `SetupInputComponent` should be one resolve line + one guarded binding block, both appended.
7. ⚠️ **`Grep` mangles comment syntax on this machine** (CONVENTIONS §10) — my new comments are `//` line comments in the `.cpp` and `/** */` blocks in the `.h` with no nested `*/`. **Confirm structural/comment findings from raw `Read` output only.**

### TASK-448 (Jonathan, playtest) — what this task adds to the checklist
1. **The key opens the console** — *requires TASK-445's asset + `IMC_Hero` mapping; inert without it, by design.*
2. **Pressing it again closes it**, and the cursor returns to hidden free-look.
3. **Press it while a group pick (R/F/C), placement or spell targeting is live ⇒ ABSOLUTELY NOTHING APPEARS.** No flash, no focus steal, no half-frame. *(This is the acceptance test for the ordering.)*
4. **Cancel-with-no-prompt closes the console AND the cursor is released** — the `OnConsoleOpenChanged` posture path, the one that is invisible until it is broken.
5. **Every keyboard command still works identically with the console closed** — Jonathan's own standing criterion.
6. **End the match with the console open** — see §8: it currently stays on screen over the victory screen. **Is that acceptable, or is the one-line close owed?**

---

## Compliance

No Git · no compile · no editor / MCP / PIE · no `Content/` · no `.ini` · sealed holdout untouched · `L_Arena` untouched · stayed inside my two files · TASK-440's work edited **additively**, with the diff-stat proof in §0.
