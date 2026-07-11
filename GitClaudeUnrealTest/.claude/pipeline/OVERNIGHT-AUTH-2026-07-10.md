# Overnight authorizations — 2026-07-10 (granted by Jonathan before going to bed)

Standing grants for the unattended run. **These expire when Jonathan is next active.** If a situation is not covered here, STOP and escalate — do not improvise a permission.

## 1. Editor bounce

**REVOKED 2026-07-10 when Jonathan returned to the machine:** *"do not force editor reclose the next time we need to close, I need to choose which assets to save."* The force-close grant below was valid ONLY while Jonathan was asleep with a locked desktop and had authorized "never save" (a force-kill was lossless because nothing was to be saved). **That condition is over.**

**NOW — Jonathan is present and actively working (unsaved editor state he cares about):**
- **NOBODY force-closes the editor** — not agents, not the orchestrator. No `Stop-Process -Force` on `UnrealEditor.exe`.
- **When the editor must close (for a compile, churn revert, etc.), ASK Jonathan to close it himself** so he chooses which assets to save. Wait for his confirmation that it's closed before proceeding.
- A graceful `CloseMainWindow()` is also not the orchestrator's to issue unprompted — it pops the save dialog on Jonathan's screen, which is his to answer, but do not drive it for him. Just ask.
- Relaunching a closed editor (detached) remains fine for build-master once Jonathan confirms it's down.

--- superseded overnight text (retained for context) ---
**AMENDED 2026-07-10 after build-master was permission-denied on process termination.** It was right to stop rather than work around the denial.

- **Subagents MUST NOT terminate `UnrealEditor.exe`.** The permission system denies it and no agent may route around that. If you find the editor running when your task needs it closed: STOP, report to the orchestrator, do not kill it. This supersedes nothing — it is the standing rule.
- **The ORCHESTRATOR performs the close**, acting on Jonathan's direct grant (given via AskUserQuestion before he went to bed: *"Yes — bounce it, never save"*). Procedure that worked: `CloseMainWindow()`, poll ~75 s; if it hangs (a save-prompt modal blocks it — this happened), `Stop-Process -Force`. **A forced exit is LOSSLESS here precisely because nothing is to be saved.** Verified after the first such kill: exactly the 3 known churn files dirty, no new residue, level untouched.
- Orchestrator then confirms **editor down + DLL unlocked** and re-dispatches build-master with that precondition *actually verified* — not asserted from a stale template. (I asserted it once from a template while PID 31836 was live; build-master caught it. Verify, then state.)
- build-master's remaining scope: `Build.bat` → relaunch `UnrealEditor.exe` detached → poll MCP (`http://127.0.0.1:8000/mcp`) until it answers (~45–140 s boot). Relaunching is fine; only *terminating* is orchestrator-only.
- **Never save-then-quit.** Saving bakes fresh GUID churn into `.uasset` files every bounce — that residue is exactly what we spent tonight untangling.

## 2. Working-tree residue — REVERT BOTH

- `Content/UI/WBP_CastleHealthBar.uasset` → **revert to HEAD**
- `Content/UI/WBP_MainMenu.uasset` → **revert to HEAD**

Both are same-content resaves (byte churn, no structural change; MainMenu verified against its LFS object: identical `WBP_DeckBuilder`/`CreateWidget` FNames, 1,460 B of GUID churn). Reverting keeps meaningless churn out of history.

- `Content/UI/WBP_UnitHealthBar.uasset` is **NOT residue** — TASK-127/123 legitimately modify it. Commit it.
- **Do the reverts while the editor is CLOSED** (inside a bounce window). A running editor holds streaming locks on `.uasset` files and `git checkout --` fails with "unable to unlink: Invalid argument".

## 3. Commit policy — COMMIT ON MACHINE-PROOF, NEVER PUSH

build-master may commit the health-bar chain, and later the deck-builder chain, **locally**, once ALL of these hold:

- The BP event path is proven to **EXECUTE at runtime** (a `Print String` inside the event, or a log line) — not merely to exist in the graph. This is the exact error that cost tonight three wrong root causes.
- `SetPercent` receives correct `Current`/`Max`.
- All `[TASK122DIAG]` lines AND the orphaned `DiagPollCount` member are stripped (TASK-128).
- QA has PASSED the stripped code.
- The build compiles clean.

**NEVER `git push`.** No exceptions, no matter how clean the result.

### AMENDED 2026-07-10 03:xx — the health-bar commit is HELD, and the machine gate above is UNSOUND for it

The 3rd-pass diagnostic PIE returned `BarSlate=1`, `identity=1`, `widgetSlate=1`, cast VALID, `OnHPChanged PUSHED` every poll, `BarPercent == Current/Max` — hero, unit, and building, across 708 `[TASK122DIAG]` lines. Both remaining hypotheses (unpainted UPROPERTY; wrong widget instance) are REFUTED.

**But every one of those checks ALSO passed on the loop-1 build** — art-director measured `BarPercent` falling 0.85 → 0.7 on it — **and Jonathan looked at that build and saw a frozen bar.** Therefore *machine-proof does not imply the user-visible bug is fixed*, and the §3 gate provides no evidence for this particular fix.

Jonathan granted commit-on-machine-proof on the reasonable assumption that it implied a working bar. That assumption is now falsified by evidence he did not have. **Do NOT commit the health-bar chain.** Hold it for his visual confirmation. This costs him one look and costs us nothing; committing would enter an unverified fix into history on a gate we now know is empty.

Consequently **TASK-128 (strip `[TASK122DIAG]`) stays DEFERRED** — the instrumentation should still be live when Jonathan retests, so he gets a log alongside whatever he sees.

### AMENDED again 2026-07-10 — the DECK-BUILDER commit is ALSO held, for the same reason

TASK-125 shipped the card art + above-card count. art-director proved the **node class** (both `WBP_DeckBuilder` BIEs, `OnDeckModelChanged` / `OnDeckSlotCountChanged`, are genuine `K2Node_Event` overrides — not custom events, so C++ `AddCopy`/`RemoveCopy` will drive `RefreshAll → RefreshCell`). It could **not** prove **execution**: the deck builder opens only via a menu-button click, the desktop is locked (no `SendInput`), and MCP exposes no UFUNCTION/exec to invoke `AddCopy`. It said so plainly instead of inferring it.

So the deck-builder chain fails the same §3 gate — *"the BP event path is proven to EXECUTE at runtime"* — that the health-bar chain fails. **Hold TASK-126's commit.** `[TASK125DIAG]` stays LIVE in `RefreshCell` so Jonathan's first +/− click writes a log.

**Both chains now converge on one morning pass:** Jonathan opens PIE once, checks the bar and the deck-builder tiles, and the logs corroborate whatever he sees. Then, in a single bounce: strip `[TASK122DIAG]` (TASK-128) **and** `[TASK125DIAG]` (TASK-126), recompile, revert the two churn `.uasset`s, and make the commits. Nothing is committed before that.

Rationale, recorded so it isn't re-litigated: committing on a gate we have empirically shown to be empty buys nothing and costs a dirty history. Holding costs one look.

Loop-2's only functional change is removing the constructor's `SetVisibility(false)`, so the component is visible from its first tick and gets added to `FWorldWidgetScreenLayer` the way the working castle bar does. That is a *plausible* fix for a render-registration problem that no machine check tonight could observe. Plausible — not proven. Say so.

The final *visual* gate — "fill visibly drains, blue on friendly, red on enemy, grey track, legible" — **cannot be machine-verified** (screen-space Slate is uncapturable; live `FillColorAndOpacity` won't serialize). It is a **human WATCH for Jonathan at breakfast.** Commit anyway per his ruling; a bad result is one local revert. **Do not fake, infer, or quietly downgrade this check — record it explicitly as an open WATCH in the handoff and in Slack.**

## 4. Scope tonight

1. TASK-123 (reopened) — repair `WBP_UnitHealthBar`
2. TASK-128 — strip the diagnostics
3. QA the stripped code
4. TASK-124 phase-B — bounce, compile, PIE machine-checks, revert the two residue files, **one commit**, no push
5. TASK-125 — deck-builder card art + per-card count
6. TASK-126 — integration + commit

TASK-125 and TASK-123 are both editor-mutating: **serialize them.** One editor-mutating agent at a time.

## 5. Escalation — STOP, don't improvise

- **Max 3 QA loops per task.** On the 3rd failure, STOP that chain, post to 🚨 Blockers (`C0BF0QZP3CN`, thread_ts `1783116296.221319`), and leave it for Jonathan.
- Editor won't boot, MCP won't answer, compile fails twice on the same error, or anything requires a decision not granted above → STOP and escalate. Do not guess at Jonathan's intent.
- **Never fabricate a verification.** "I could not check X" is always an acceptable report. Claiming a check you did not run is not.

## 6. The lesson from tonight, binding on every agent

Three specialists asserted confident root causes from static inspection; every one was refuted by reality. A graph can be perfectly wired and never execute. `WidgetClass` being set does not prove `GetWidget()` returns a widget. **Report the observation, not the conclusion. Prove execution, not structure.**
