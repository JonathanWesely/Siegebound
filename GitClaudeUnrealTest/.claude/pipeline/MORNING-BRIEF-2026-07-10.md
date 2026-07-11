# Morning brief — 2026-07-10

**Nothing was committed. Nothing was pushed.** Two things need your eyes; both are one PIE session.

---

## Do this first (one PIE run, ~2 minutes)

The editor is running (PID 34584, healthy) on a DLL from `02:40:42` that carries every change below. If it's closed, just reopen it.

### 1. Health bar — does the fill actually drain?

Start PIE. Get a unit or tower into combat. Watch an overhead bar.

- **Does the fill shrink as HP drops?**
- **Is it blue on your units and red on the bot's?**
- **Does it read clearly against the dark grey track?**

Then tell Claude. `Saved/Logs/GitClaudeUnrealTest.log` has `[TASK122DIAG]` lines corroborating whatever you see — they print `Current`, `Max`, and the widget's actual `Percent` every poll.

### 2. Deck builder — card art and the count

Open the deck builder from the main menu.

- **Does each tile show its card art** (same `T_CardArt_*` images as the in-match hand)?
- **Is there a number above each card**, and **does it change when you click + / −**?
- **Are the count, name, and cost legible over the art?** They sit on dark plates precisely so bright art can't swallow them.

Each +/− click writes a `[TASK125DIAG]` line showing the new count.

---

## Why nothing was committed

You authorized "commit on machine-proof, verify visually at breakfast." I overrode that, for one specific reason.

Every machine-observable check now passes on the health bar: widget constructed, cast valid, its Slate widget live, the pushed instance is the rendered instance, the event fires every poll, `Percent == Current/Max`. **All of that was also true on the build you looked at and called frozen.** So machine-proof has been demonstrated — empirically, tonight — to carry no information about whether this bug is fixed. Committing on it would have entered an unverified fix into history on an empty gate. Holding cost one look.

The deck-builder chain fails the same gate: the BIE node classes are proven genuine overrides, but nobody could click a button to prove the count actually updates (locked desktop, no MCP exec path).

---

## What actually happened to the health bar

Long chain, four wrong answers, so here is the real state.

**The bug was never in the stats.** Every character, tower, and wall has HP (`ATower : ABuilding`, inheriting `CurrentHP`/`MaxHP` from the card row).

**Three confident root causes were asserted and refuted:**

1. gameplay-programmer: "the frozen fill was a symptom of the hide-at-full design" — refuted by your PIE.
2. art-director: "not asset-side, I eliminated null `BarWidget`" — it had verified `WidgetClass` was *set*, never that `GetWidget()` returned a castable widget.
3. gameplay-programmer: "`SetWidgetClass`'s construction side-effect never fired" — refuted by its own instrumentation (`side-effect-created=YES`, 23/23).

**And one of mine:** "a white fill draining across a white track made it invisible." The track was black at 50% alpha. Manager and QA both endorsed that hypothesis before anyone checked it against the asset. Agreement among four agents certified nothing.

**The `[TASK122DIAG]` instrumentation ended the guessing.** It also revealed that your mid-session "the bars are working" report — which you later retracted — had sent the whole pipeline chasing a behavior change that never happened.

**Current best understanding:** loop-2's only functional change removed the constructor's `SetVisibility(false)`, so the component is visible from its first tick and registers with `FWorldWidgetScreenLayer` the way the working castle bar does. That is a **plausible** fix for a render-registration problem no machine check can observe. Plausible, not proven. Your eyes decide it.

---

## Changes sitting in the working tree

| File | What | State |
|---|---|---|
| `Source/…/HealthBarComponent.cpp` / `.h` | Always-visible law; `ApplyTeamTint()`; `CreateWidget` fallback; no constructor hide; `[TASK122DIAG]` logging | compiled clean, QA-passed |
| `Content/UI/WBP_UnitHealthBar.uasset` | Grey track `{0.03,0.03,0.03,0.7}`; fill tint neutral white so the C++ team color shows undimmed | saved |
| `Content/UI/WBP_DeckCardTile.uasset` | Card art background; count moved above the card on a dark plate; name/cost on a bottom caption plate; +/− and cap-grey preserved | saved |
| `Content/UI/WBP_CastleHealthBar.uasset` | byte churn, no structural change | **revert scheduled** |
| `Content/UI/WBP_MainMenu.uasset` | byte churn, no structural change | **revert scheduled** |

## Left to do after you look

One bounce: strip `[TASK122DIAG]` (TASK-128) and `[TASK125DIAG]` (TASK-126), recompile, revert the two churn `.uasset`s, commit both chains. No push.

## Loose ends worth knowing

- `PackageRestoreData.json` was renamed to `.bak-2026-07-10-forcekill` (not deleted). It wanted to restore an autosave of `WBP_CastleHealthBar` — a file we're reverting — so declining was correct. A force-kill of the editor produces this prompt; the `.bak-task073` sibling shows it has happened before.
- `WBP_DeckCardTile` now has two orphaned duplicate events (`OnAddPressed` / `OnRemovePressed`) from the MCP `AssignOnClicked` auto-rename quirk. Harmless; flagged for the M7 cruft sweep.
- The deck-builder tile is content-sized; the fixed card size lives in `WBP_DeckBuilder`'s WrapBox slot, which was out of scope.
- `bShowHealthBar` (per-Blueprint opt-out, default true) was kept. Still your call whether to drop it.
- GDI screenshots to verify pixels returned pure black — locked desktop. That approach *should* work when you're logged in, and would remove the human-eyes bottleneck from future UI work.
