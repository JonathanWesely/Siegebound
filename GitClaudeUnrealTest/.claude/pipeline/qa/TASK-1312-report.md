# QA Report — TASK-1312 (subject: TASK-1311) — Verdict: **PASS** (0 BLOCKERS · 0 WARN · 2 NIT · 3 NOTE)

**Verdict: PASS** — ROUTE **B**, and its Route-A evidence gate was genuinely **EMPTY**. **Unpinned ⇒ no coupling check applies.**
subject: TASK-1311 · gate: TASK-1312 · host: TASK-1313 · reviewer: qa-reviewer · 2026-09-19
Reads performed whole per `SC-§38a`: `TASKBOARD.md` `#### TASK-1312` + `#### TASK-1311` (full `status:` line) + `PLAYAGAIN-KEYBOARD-RULING` incl. **AMENDMENT A** · `handoffs/TASK-1311-programmer.md` · `SiegePlayerController.cpp:2135-2290` · `qa/TASK-1297-report.md` §E (cause chain, re-derived at engine source below rather than inherited).

---

## §0 — THE THREE RIDERS, DISCHARGED EXPLICITLY

- **RIDER 1 — the 6-line comment is ACCEPTED and is NOT bounced.** It stands at `:2262`–`:2267`, **zero executable lines**, one-block delete to undo, and the row **flagged** it under `SC-§121` cl. 5 rather than smuggling it. Recorded as **compliant**, not as a deviation. No finding is raised against its existence.
- **RIDER 2 — the stale last sentence: ONE-LINE NOTE, not a WARN, not a bounce.** See **NOTE-1**.
- **RIDER 3 — the reachability answer is written to be CITED.** See **§5**, which states what I measured and what I did not.

## §1 — CHECK (1): THE ROUTE WITH ITS COST — **PASS**

**Route taken: B (code side). No `.uasset` in the diff ⇒ 🧑 Jonathan is not needed for a hash discharge on this row.**

Deliverable (0)'s four reads are quoted in the handoff **with actual values** (no *"I checked"* anywhere). I re-measured three of the four at my own instant (`SC-§91`); all three agree:

| (0) read | Handoff's value | **My independent re-measurement** | Agrees |
|---|---|---|---|
| (a) `WBP_VictoryScreen` CDO `bIsFocusable` | `False` | `get_asset_meta /Game/UI/WBP_VictoryScreen.WBP_VictoryScreen PropertyValues` → `uint8 bIsFocusable = False` | ✅ |
| (a′) `DesiredFocusWidget` | `()` empty | same call → `FWidgetChild DesiredFocusWidget = ()` | ✅ (corroborates **AMENDMENT A2**) |
| (c) `SObjectWidget.cpp:175` | `bool SObjectWidget::SupportsKeyboardFocus() const` | engine read at `:170-183` → `:175` is exactly that signature | ✅ **exact** |
| (c) `PlayerController.cpp:6345` | the `UE_LOGF(... Error ...)` emitter | engine read at `:6336-6351` → `:6340` is `FInputModeUIOnly::SetWidgetToFocus`, **`:6345` is the `Error` line**, inside `#if !(UE_BUILD_SHIPPING \|\| UE_BUILD_TEST)` | ✅ **exact** |
| (b) `:2261-2270` pre-edit block | quoted verbatim | post-edit state read directly (below); pre-image accepted as declared (no `Bash`) | see §7 |
| (d) pin census | 1 raw hit, a comment | reproduced — see §3 | ✅ |

**Route A's gate was empty and I checked its strongest limb myself.** The row's positive-evidence test (a `KBD-§` row, an `IMC_`/`IA_` binding, or a GDD sentence at `file:line`) returned nothing, and the structural reason is measured, not asserted: `SiegeMenuInputSubsystem.cpp:25` pins `MenuMapName = TEXT("L_MainMenu")` and `:47` hard-returns on any other map, while the victory screen is on `L_Arena` — the manager records the same measurement at **AMENDMENT A1**. ⇒ **Route B was not merely cheaper; Route A was unauthorised.** And per **RULING 1(d)**, 🧑 his 2026-09-19 *"board it"* is evidence for the **product** and **not** a licence for this row to switch routes — the diff correctly does **not** switch.

**The `.uasset` limbs of check (1) do not fire** (no `.uasset` in the diff): (1)(i) positive evidence, (1)(ii) behaviour-change flag, (1)(iii) the `SC-§125` hand-save declaration are all **N/A**. The handoff nonetheless declares §2 *"no `.uasset`, no keystroke"* and makes **no** claim that a programmatic save discharged anything — the shape that would have been a BLOCKER on sight is absent.

**Route-choice cost, correctly declared:** `TASK-1313` owes 🧑 him nothing at 5a.

## §2 — CHECK (2): NO SUPPRESSION SUBSTITUTED — **PASS**

Measured by me, repo-wide over `Source/`:

```
Grep 'SetWidgetToFocus|SetSuppressLogErrors|Non-Focusable|NonFocusable'  Source/   ->  No matches found  (0)
```

⇒ `AddExpectedError` diff = **0** (§3) · **no** `SetSuppressLogErrors` anywhere · **no** `#if` added around the engine site (the post-edit block `:2261-2272` contains none — I read it) · **no** log-verbosity change in the diff (the working tree carries no `Config/**` modification). **The absence of the `Error` is the assertion**, and the row did not buy that absence with a silencer.

## §3 — CHECK (3): **UNPINNED ⇒ NO COUPLING CHECK APPLIES** — **PASS**, verified as a fact with a positive control

```
Grep 'AddExpectedError'  Source/GitClaudeUnrealTest/Siegebound/Tests/
Source\GitClaudeUnrealTest\Siegebound\Tests\SiegeFogVisualTest.cpp:1475://  ⛔ ALL now suite-visible. ⛔ DO NOT ADD `AddExpectedError` TO THIS TEST — that would restore the
```

**1 raw hit; it is a COMMENT forbidding the construct ⇒ 0 executable `AddExpectedError` calls in `Tests/`, and 0 pinning this or any Error string.** That single hit is the **positive control** `SC-§39` requires: the needle fires where the token exists, so the zero is a **measured zero**, not a broken pattern.

⇒ **This emitter is UNPINNED. There is no coupled test deletion in this row and none was hunted for** (`SC-§59` cl. 5 — a gate that invents the missing half of a coupling fails a correct row). Confirmed that no half is missing: the diff deletes **no** test file and the suite total is baseline ±0 by construction (no test added, none removed).

The hazard the row names is real and is the reason this is a defect rather than tidiness: unpinned means any future automation test that ends a match inside a capture window reds on an **undeclared** `Error` with nothing to absorb it (`SC-§70`).

## §4 — CHECK (4): FENCES — **ALL PASS**

| Fence | Verdict | How I established it |
|---|---|---|
| Play Again button / `OnClicked` | ✅ untouched | lives in `WBP_VictoryScreen` — **no `.uasset` in the diff**; the code-side twin `ASiegeGameMode::PlayAgain()` is at `SiegeGameMode.cpp:1305`, **a file that is not modified in the tree** |
| `SetWinner` / `SetLocalVictory` | ✅ untouched | read in place at `:2202-2216` and `:2229-2243`; both sit **above** the edited hunk and are structurally intact (by-name `FindFunction` + `ParmsSize` guard + `ProcessEvent`, both `else` log branches present) |
| No WidgetBlueprint duplicated-and-reparented | ✅ | the handoff **answers directly** (§5 fence (3)(b)): it opened, duplicated, reparented, edited and saved **no** WidgetBlueprint. Corroborated independently — the working tree carries **no** `Content/**` modification, and my own asset contact was one **read-only** `get_asset_meta` |
| `FInputModeUIOnly` posture byte-identical | ✅ | read post-edit at `:2268-2272`: `bShowMouseCursor = true;` · `bEnableClickEvents = true;` · `FInputModeUIOnly InputMode;` · `SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);` · `SetInputMode(InputMode);` — all five present, in order, unchanged |
| `Saved/**` absent | ✅ | not in the tree's modified set |
| `TASKBOARD.md` / `CONVENTIONS.md` absent **from this row's diff** | ✅ | both **are** modified in the tree, and both are **attributable to the manager, not to `TASK-1311`**: the `PLAYAGAIN-KEYBOARD-RULING` section + `TASK-1314`–`TASK-1319` rows are dated 2026-09-19 and uncommitted at `d818b5e`. The handoff §7 further records that the row **declined to flip its own board line** because fence (3)(e) put the file out of bounds — correct, and the flip is discharged by me below |
| **ZERO files from `TASK-1298`'s pathspec in this diff** | ✅ | `Tests/SiegeMenuInputTest.cpp` · `DeckBuilderWidget.{h,cpp}` · `Content/Blueprints/BP_MenuGameMode.uasset` are all unmodified. `Tools/run_suite_bounded.ps1` **is** modified in the tree — **and it is `TASK-1310`'s, attributed by its own content**, not by assumption: its header reads `CORRECTED 2026-09-19 by TASK-1310` at `:64`, with `TASK-1310`'s census at `:89` and its required `NOT MEASURED` wording at `:117`. ⇒ no contamination of `TASK-1311`'s diff; see **NOTE-3** for what this obliges `TASK-1313` to do |

## §5 — CHECK (5): REACHABILITY — **ANSWERED, NOT ASSUMED** (RIDER 3: written to be cited by `TASK-1314` (5))

`HandleMatchEnd` is declared at `SiegePlayerController.h:1218` and defined at `SiegePlayerController.cpp:2135`. **I reproduced the full trigger chain at source; I did not accept the handoff's version of it.**

**The live path (M8 state route) — three hops, all ours:**
1. `ACastle.cpp:1234` — `OnCastleDestroyed.Broadcast(this, Team);`, fired from the castle's destruction handler after `ApplyDestroyedState(true)` at `:1226`. **Precondition: a castle reaches 0 HP.**
2. bound at `SiegeGameMode.cpp:134` — `It->OnCastleDestroyed.AddUniqueDynamic(this, &ASiegeGameMode::OnCastleDestroyedHandler);` → handler at `SiegeGameMode.cpp:523`, which freezes the world (`:581`) and then calls `SiegeGameState->SetMatchResult(Winner)` at **`SiegeGameMode.cpp:593`**.
3. `SiegeGameState.cpp:204` `SetMatchResult` — guarded `if (!HasAuthority() || bMatchEnded) return;` at `:208`, latches `WinningTeam`/`bMatchEnded`, then calls `NotifyLocalControllersMatchEnd()` at **`:219`** → `SiegeGameState.cpp:248`, whose loop calls **`SiegePC->HandleMatchEnd(WinningTeam)` at `SiegeGameState.cpp:266`**, gated on `SiegePC && SiegePC->IsLocalController()` at `:264`.

**The fallback path:** with no `ASiegeGameState` (defensive mis-config) the retired direct push runs — **`SiegeGameMode.cpp:606`**, `SiegePC->HandleMatchEnd(Winner)`, inside the `else` at `:595`.

**Preconditions on the emitter itself, inside `HandleMatchEnd`:** the `bMatchEnded` latch early-outs at `:2162-2168` (so the block runs **once** per match), and the M8 local-UI guard `if (!IsLocalController()) return;` at **`:2177`** gates everything below it. The removed focus call sat at `:2267`, i.e. **after** both — ⇒ **the `Error` fired once per match end, on the local controller only.**

⇒ **The defect claim stands: the site is reachable in ordinary play** (any match that ends by castle destruction, standalone or host), and `qa/TASK-1297-report.md` proved the cause chain while leaving this trigger unproven — it is now proven.

**What I did NOT measure (state it, do not let `TASK-1314` inherit it as more):** I did not drive PIE, so I have **no runtime observation** that the `Error` actually appeared in a live log, nor that it is now absent. Everything above is a **static call-graph + engine-source** read.

### ⭐ A measured input for `TASK-1314` (0)(e) that falls straight out of this check — **a LEAD, not a finding (`SC-§101`)**

`FInputModeUIOnly::ApplyInputMode` (`PlayerController.cpp:6372`) passes `WidgetToFocus` to `FInputModeDataBase::SetFocusAndLocking` (`:6313`), which issues `SlateOperations.SetUserFocus(...)` **only when the pointer is valid** (`:6315-6318`). And `FSlateApplication::SetUserFocus` does **not** abort on a non-focusable target — it walks the focus path **from the leaf upward** and focuses the first ancestor whose `SupportsKeyboardFocus()` is true (`SlateApplication.cpp:~3021-3038`), returning `false` with **no change at all** if that ancestor is already the focused widget. `SViewport::SupportsKeyboardFocus()` is `override { return true; }` (`SViewport.h:106`).

⇒ In the shipped flow (focus already on the game viewport at match end) the deleted call was a **true no-op** — the loop would have found `SViewport`, seen it was already focused, and returned `false`. **Route B is behaviour-preserving, confirmed at mechanism level, not merely asserted.** The only divergence is a corner: if some *other* widget held Slate focus at match end, the old call would have re-homed focus up to `SViewport` and now does not. **Unmeasured at runtime** (I read code, not a live focus holder), and **not player-visible via the acceptance criterion**, because `SetIgnoreInput(true)` at `:6384` is applied regardless and a Play Again **click** is hit-test-driven, not focus-driven.

**For `TASK-1314`:** this is why **Route K-1 must focus the BUTTON.** Post-Route-B the focused widget at a match end is `SViewport` (unchanged from in-match), and `SViewport` routes keys to the viewport client / PlayerController / Enhanced Input — **not** to `SButton::OnKeyDown` or the navigation config's Accept on the victory screen. Focusing the root would land on a widget that is not the button; focusing the button hands Slate a specific `SWidget` and **bypasses the empty `DesiredFocusWidget` entirely** (as AMENDMENT A3 already reasons). Treat the `SViewport` sentence as a **lead to re-measure**, per `SC-§101`.

## §6 — CHECK (6): `.uasset` DISCIPLINE — **PASS (vacuously, and that is the point)**

**No `.uasset` is edited by this row.** `Content/UI/WBP_VictoryScreen.uasset` is **read-only** to it and is named in the handoff with sha256 `7817dd7f19bcf1028bd21ddfc4d92731c01c14d567c19641cc10e9f1d12bd6ee` / 153,489 bytes / mtime unchanged (`SC-§68` satisfied in the form the law wants for an untouched file). I hold no `Bash` and did **not** re-hash it; instead I re-read its CDO through the read-only inspector and the values match. **`TASK-1313` re-measures.**

## §7 — CHECK (7): ACCEPTED AS DECLARED (`SC-§71b`) — NAMED, NOT WAVED THROUGH

I hold **no `Bash`**, **no compile**, **no Git**, and **no PIE**. The following are **accepted as declared** and are **`TASK-1313`'s to measure**:

1. **`git diff` / `git diff --stat` = 1 file, 6 insertions / 4 deletions** — I verified the **post-edit STATE** by direct read (`SC-§104`: assert state, not tallies) and it is exactly what a correct application of that diff produces; I could not read the **pre-image**.
2. **Pre-edit sha256 `bae4bcf7…` → post-edit `6a4d107b…`, 7,433 → 7,439 lines.**
3. **`git status --short` = one `M` line at the row's instant.** ⚠️ At **my** instant the tree also carries `Tools/run_suite_bounded.ps1` (`TASK-1310`), `.claude/pipeline/TASKBOARD.md` and `.claude/pipeline/CONVENTIONS.md` (manager) — **both instants can be true**; attribution is established in §4 and this is **not** a finding against `TASK-1311`. See **NOTE-3**.
4. **Compile `Result: Succeeded`, read FROM THE LOG** (never `$LASTEXITCODE` — `UE-§ exit-code-lies`) and **suite total = baseline ±0, reconciled BY NAME** (`SC-§104`).
5. **`Content/UI/WBP_VictoryScreen.uasset` hash unchanged.**

**⭐ THE ONE THING ONLY A RUNTIME LEG CAN ANSWER, in one sentence:** whether the engine string `InputMode:UIOnly - Attempting to focus Non-Focusable widget` is **GONE at a real match end on `L_Arena`** — that is the `playtest-verifier`'s 5b criterion (`TASK-1311` (4b)), **and this PASS does not claim it**; a `0` counted in a log from a PIE run that never ended a match is `SC-§39`'s pin-that-cannot-fail, and `UNOBSERVABLE` with the reason is a legitimate verdict here (ceiling declared at boarding, `VER-§5`).

---

## Findings

- **[NIT-1]** `SiegePlayerController.cpp:2265` — the comment's clause *"asking for it was already a no-op"* is **true in the shipped path but imprecise as stated**: `FSlateApplication::SetUserFocus` walks up to the nearest focusable ancestor rather than ignoring a non-focusable target, so the call was a no-op **because `SViewport` already held focus**, not because Slate discarded the request. — **Non-blocking, and it contradicts neither RIDER 1 nor RIDER 2.** Suggested sharpening, for `TASK-1314` (6d) which is already rewriting this block: *"…so focus never lands **on this widget** — Slate re-homes to the nearest focusable ancestor (`SViewport`), which already holds focus, making the call a no-op."*
- **[NIT-2]** `SiegePlayerController.cpp:2266` — *"Do not re-add it unless the widget root is made focusable first"* is **good and must be preserved** through `TASK-1314`'s edit, but it can be **sharpened**: the root is **necessary and NOT sufficient** — `DesiredFocusWidget` is empty on the CDO (**I re-measured it: `FWidgetChild DesiredFocusWidget = ()`**), so a focusable root focuses the root and activates nothing. Owner: `TASK-1314` (6d). Per **AMENDMENT A2/A3** this is already the manager's reading; recorded here so the code comment and the board agree.
- **[NOTE-1] (RIDER 2 — one line, as ruled)** `SiegePlayerController.cpp:2266-2267` — the sentence *"whether Play Again should be key/pad-reachable at all is a product question owed to Jonathan"* is **now FALSE** (🧑 he answered it 2026-09-19); it went false **mid-flight, after the row wrote it ⇒ no fault of `TASK-1311`**, and its owner is **`TASK-1314` (6d)**. **Not a WARN, not a blocker, not a bounce.**
- **[NOTE-2]** Credit on the record: the row's **first draft of that comment spelled `SetWidgetToFocus` twice**, which made its own acceptance (4a) `grep -c` return **2** instead of **0**. It **caught that itself and rewrote the comment rather than relax the acceptance clause** — the correct instinct, and the acceptance now passes on its own literal terms (**I reproduced it: 0 repo-wide**).
- **[NOTE-3] For `TASK-1313`, and it is the one thing that could still go wrong** — the working tree is **not** clean of other rows' work: `Tools/run_suite_bounded.ps1` (`TASK-1310`, attributed at `:64`), `.claude/pipeline/TASKBOARD.md` and `.claude/pipeline/CONVENTIONS.md` (manager). ⇒ **commit BY EXPLICIT PATHSPEC, never `-a` and never `git commit` on a staged-by-the-editor index** (`UE-§ git-plugin-autostages-index` — the editor's `Provider=Git` auto-stages; verify the **COMMIT** with `git show --stat HEAD`, never the index). A `-a` here would sweep `TASK-1310`'s ungated tooling diff into a `qa-passed` commit.
- **[NOTE-4]** `qa/TASK-007-report.md`'s older WARN — `HandleMatchEnd` goes `FInputModeUIOnly` even when `VictoryWidget` is null — is **named and left alone**, exactly as fence (3)(f) instructs. It is **now boarded as `TASK-1319`** (RULING 3), so it is no longer an orphan. Confirmed the diff neither creates nor worsens it: the posture lines at `:2268-2272` were always unconditional, and the deleted `if (VictoryWidget)` wrapper only ever guarded the focus call.

**BLOCKERS: 0.**

---

## Notes for build-master (PASS)

1. **Diff under gate:** `Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp` **only** (+ `handoffs/TASK-1311-programmer.md`, + this report). **No `.uasset`, no test file, no `Tools/`, no `Saved/**`.**
2. **🧑 Jonathan is NOT needed for a hash discharge on this row** — Route B carries no package write, so `SC-§125` cl. 3's BP-editor-save cost does **not** apply at 5a.
3. **Commit by pathspec — see NOTE-3.** Three unrelated modified files are live in the tree.
4. **Compile risk: negligible.** The change deletes one call and one `if` wrapper and adds comment lines. `VictoryWidget` is a **UPROPERTY member** (`SiegePlayerController.h:3590`), so no unused-variable diagnostic is possible; `FInputModeUIOnly` is still constructed and used, so no include becomes load-bearing-absent. The removed `VictoryWidget->TakeWidget()` had a real side effect (it builds/caches the `SObjectWidget`) — **already discharged** by `VictoryWidget->AddToViewport(10)` at **`:2245`**, which runs **before** the edited block and takes the widget internally. **I re-checked that ordering myself; it holds.**
5. **Read `Result: Succeeded` from the log, not `$LASTEXITCODE`** (`UE-§ exit-code-lies`), and reconcile the suite **by name** (`SC-§104`), expecting **baseline ±0**.
6. **5b is owed and this PASS does not substitute for it:** the runtime criterion is the engine `Error` string absent from a PIE log that **actually reached a match end on `L_Arena`**; `UNOBSERVABLE` with a reason is legitimate here, and 🧑 his hand check is recorded **alongside** the verdict, never merged into it (`VER-§8` cl. 4).
7. **`TASK-1314` is `blocked-by` `TASK-1313` COMMITTED** (RULING 1(e)) — it rewrites the very comment block this row authored, in the same function, within ten lines.
