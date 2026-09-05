# TASK-1027 — HAND-ADVANCE-DIAG — programmer handoff

**DIAGNOSE-ONLY. Zero source edits. Zero compiles. Zero mutating Git. Editor left UP (PID 17916), `L_Arena` never saved.**

---

## ⭐ VERDICT

**Cause (1): the hand DID turn over on Fog cast #1, and the card drawn WAS another `Fog`.**

Not ambiguous. Not "probably". The `ConfirmPlayFromHand` call on cast #1 is proven to have been **reached** and to have **returned true**, by a log written by the exact run Jonathan recorded.

**There is no defect here.** `Next:` was never frozen — it advanced from one `Fog` instance to a *different* `Fog` instance, which is pixel-identical. The deck made that outcome ordinary, not lucky.

---

## ⛔ WHICH INSTRUMENT, WHICH CODE STATE — read this before any claim below

The row warned not to mix a runtime instrument (pre-`TASK-1018`) with a source read (post-`TASK-1018`). Measured:

| artifact | timestamp | code state |
|---|---|---|
| `Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll` | **2026-09-04 20:58:26** | the binary that ran |
| `Source/.../SiegePlayerController.cpp` (working tree) | **2026-09-04 22:36:06** | **post-`TASK-1018`**, uncompiled |
| `git show HEAD:...SiegePlayerController.cpp` | commit `84eec02` | **pre-`TASK-1018`** |

**Every runtime claim below comes from `Saved/Logs/GitClaudeUnrealTest-backup-2026.09.05-04.50.26.log` — the PRE-`TASK-1018` behaviour, i.e. exactly what Jonathan saw.**
**Every source claim below is quoted from `git show HEAD:` (pre-`TASK-1018`), never from the working tree**, and I proved the load-bearing region is untouched by the pending edit:

```
git diff -U0 hunk headers: @@ -1163 @@ -3239 @@ -3244 @@ -4663 @@ -4693 @@ -4695 @@ -4705 @@ -4724
```

There is **no hunk anywhere near the confirm block**. `TargetingHandSlot` has **9 sites in HEAD and 9 in the working tree**, same shape, only renumbered (+~68). The mechanism I reason about is byte-identical in both states, so the mixing hazard does not arise for this verdict.

---

## ⚠️ I DID NOT NEED THE TIMING WINDOW — AND A LIVE RE-RUN COULD NOT HAVE ANSWERED THIS

I did not burn the pre-compile window with a fresh PIE run. Two reasons, the second decisive:

1. The two lines the row asked for are **`Log`** and **`Error`** level — both on by default. `Verbose` was never required.
2. **A live re-run today literally cannot reproduce the observation.** `deck1` was **changed after his session**, measured log-to-log:

| session | line |
|---|---|
| 04:44:56 (**his recorded run**) | `pending override deck 'deck1' set (6 entries, 50 cards)` |
| 04:51:25 (next session) | `pending override deck 'deck1' set (**5 entries**, 50 cards)` |

The current `SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34.sav` (written **21:51**, *after* his 21:44:30–21:50:26 session) holds `WatchTower · Witch · BrightSun · Pickpocket · Wall` — **no `Fog` at all**. Casting Fog twice today would exercise a deck that cannot contain the card. ⛔ **The saved deck on disk is NOT the deck he played; do not read the played composition out of that file.**

Instead I found a **strictly better instrument than a re-run: the archived log of the run itself.** It is not a reconstruction — it is the event.

---

## Proof that the log IS the recorded run

Session: opened `09/04/26 21:44:30`, closed `21:50:26`. Video: `2026-09-04 21-45-11`, 73.1 s.

Three — and exactly three — `cast spell` lines, matching the video's three casts:

```
[2026.09.05-04.46.07:904] cast spell 'Fog'       for 50 gold at (-20088, 486, 0)
[2026.09.05-04.46.13:234] cast spell 'Fog'       for 50 gold at (-20083, 441, 0)
[2026.09.05-04.46.16:423] cast spell 'BrightSun' for 60 gold at (-20122, 470, 0)
```

Back-solving the recording epoch from VID-006's frame-accurate confirm times (00:58.92 / 01:04.30 / 01:07.40):

| cast | log (PDT) | video t | implied epoch |
|---|---|---|---|
| Fog #1 | 21:46:07.904 | 58.92 | **21:45:08.984** |
| Fog #2 | 21:46:13.234 | 64.30 | **21:45:08.934** |
| BrightSun | 21:46:16.423 | 67.40 | **21:45:09.023** |

**Spread: 89 ms across three independent casts.** Costs match the pixel gold deltas exactly (−50 / −50 / −60).

**Independent corroboration on a fourth event.** The analyst bracketed the Bright Sun reticle's *appearance* on pixels alone to `01:06.5 < t ≤ 01:06.8`. The log's `targeting mode entered for spell 'BrightSun'` sits at 21:46:15.707 ⇒ **video t = 66.73 s** — inside that 0.3 s bracket, from a completely different instrument.

**Exclusion:** a second editor instance (`GitClaudeUnrealTest_2*.log`) also cast spells, at 04:40:44 / 04:42:13 / 04:42:34 — all **before** the recording started. Cleanly ruled out.

---

## ⛔ THE POSITIVE CONTROL — run BEFORE interpreting any absence

The row's standing sentence: **an empty log reads as a false pass.** Three independent legs, all green:

1. **The category is always-on at Error.** `GitClaudeUnrealTest.h:8` — `DECLARE_LOG_CATEGORY_EXTERN(LogGitClaudeUnrealTest, Log, All)`. Default runtime verbosity `Log` ⇒ `Error` (more severe) is **always** compiled in and always enabled. No `Log …Verbose` was ever required to see the tripwire.
2. **The category was demonstrably emitting during that session**, including at the exact cast instants: **44 `LogGitClaudeUnrealTest` lines**, among them the fog-state and VFX lines timestamped `04:46:07.903` — one millisecond before the cast line.
3. **Error-level lines do render into that specific file**: **14 `: Error:` lines** of other categories are present in it (`LogPlayerController`, `LogLiveCoding`).

⇒ **The zero I report is a real zero, not "logging was off."**

⚠️ Recorded for the next person: **`LogGitClaudeUnrealTest: Verbose` lines in that session = 0.** Verbose genuinely *was* off. Had my verdict depended on a Verbose line, the absence would have been meaningless — which is precisely the trap the row named. It did not depend on one.

---

## The proof chain (pre-`TASK-1018` source, quoted from `HEAD`)

**Step 1 — `TargetingHandSlot` was set, and the rollback did not fire.** `HEAD:1174-1184`:

```cpp
else
{
    // the PendingHandSlot pattern: record the slot BEFORE entry, roll it
    // back if the entry refused internally
    TargetingHandSlot = Slot;
    EnterTargetingMode(CardID);
    if (!bInTargetingMode)
    {
        TargetingHandSlot = INDEX_NONE;
    }
}
```

`bInTargetingMode = true` is set at `HEAD:3263`; the line `targeting mode entered for spell '%s'` is the **final statement of `EnterTargetingMode`** (`HEAD:3296`), after every refusal early-out. That line **printed for cast #1** at `04:46:05.029`. ⇒ the function ran to completion ⇒ `bInTargetingMode == true` ⇒ **the rollback to `INDEX_NONE` did not execute.** (Corroborated on pixels: the reticle was visibly on screen.)

**Step 2 — the guard passed, so the call was made.** `HEAD:3483-3490`:

```cpp
if (TargetingHandSlot != INDEX_NONE && DeckComponent)
{
    if (!DeckComponent->ConfirmPlayFromHand(TargetingHandSlot))
    {
        UE_LOG(LogGitClaudeUnrealTest, Error,
            TEXT("... ConfirmPlayFromHand(%d) refused at spell confirm for '%s' — hand mutated mid-targeting (should be impossible)."), ...);
    }
}
```

This guard was the **one silent path** that could have produced cause (2) with no error line — `TargetingHandSlot == INDEX_NONE` skips the call entirely. Step 1 closes it. `DeckComponent` is a default subobject and is independently proven non-null in that same session, because casts #2 and #3 advanced the hand through the same pointer.

**Step 3 — it returned true.** `grep "ConfirmPlayFromHand"` over the session log: **0 hits.** `grep "LogGitClaudeUnrealTest: Error"`: **0 hits.** Given the positive control, the tripwire genuinely did not fire ⇒ the call did not return false ⇒ **card moved to discard and the replacement was drawn.**

---

## Why the pixels looked frozen — the mechanism, and why 3 Fogs was ordinary

`DeckComponent.h`: *"Playing/discarding moves the slot's card to the discard pile and **IMMEDIATELY draws its replacement into the same slot**"*, and `PeekNextCardID()` (the `Next:` widget) is the top of the draw pile — **the very card that will refill the slot.**

So the refill card *is* the card `Next:` was showing:

| | slot 6 | `Next:` |
|---|---|---|
| before cast 1 | Fog **A** | Fog **B** |
| after cast 1 | Fog **B** (A → discard) | Fog **C** ← *still reads "Fog"* |
| after cast 2 | Fog **C** (B → discard) | **Witch** |

Both pixel observations fall straight out: slot 6 reading `Fog` before *and* after is the **predicted** result of a successful draw, and `Next:` "not advancing" was **C replacing B** — a different card instance with the same `CardID`. It matches VID-006 finding 14 exactly: at 01:04.5 `Next:` flips to `Witch` while **slot 6 still reads `Fog 50g`** (that is C).

This needs three `Fog` instances. The deck had them easily:

- His deck was the **override** deck, not the `DeckCount` default: `built a 50-card draw pile from the pending OVERRIDE deck 'deck1' (6 entries) — DeckCount column bypassed`. **50 cards over 6 entries ≈ 8.3 copies each.**
- ⛔ **Do not reach for `DT_Cards.MaxCopies = 2` as a contradiction.** `UDeckLibrary::IsDeckLegal` carries: *"CARD-UNCAP 2026-08-28 (UNCAP-§3): the per-CardID aggregate MaxCopies check (and its RunningCounts map) is **DELETED** — a deck may hold any number of copies of any single card; MaxCopies is the hero-upgrade STACK cap only."* The legality line in his log — `active saved deck 'deck1' is legal (50 cards)` — is a **total-size** check.
- ⚠️ `Docs/Data/cards.csv` shows `DeckCount = 0` for `Fog`, `BrightSun`, `Witch`. That column was **bypassed** and is irrelevant to what he drew — a live trap for anyone re-deriving this from the CSV.

---

## ⚖️ Overlap with TASK-1026 — REPORTED, NOT ADJUDICATED

`TASK-1026` owns the three-simultaneous-Witches question. My evidence bears on it and I am **not ruling**:

1. `UDeckLibrary::IsDeckLegal` **deliberately deleted** the per-CardID `MaxCopies` cap (`CARD-UNCAP 2026-08-28`, `UNCAP-§3`). Three Witches from a `MaxCopies = 2` card is therefore **the documented shipped ruling**, not prima facie a bug.
2. The played `deck1` (6 entries / 50 cards) is **not recoverable from disk** — the save was overwritten at 21:51 and now holds 5 entries. Its Witch count is unmeasurable after the fact; the *only* surviving fact is `6 entries, 50 cards`.
3. Its `Witch` count is therefore unknown but plausibly ~8, which explains three concurrent Witches with no reshuffle argument needed.

**TASK-1026's row should be read against `UNCAP-§3` before it is treated as a defect.**

---

## ⛔ DRIVABILITY (clause 0) — answered plainly: **NO**

**No cheat exec can play a hand slot.** Censused by symbol, `SiegeCheatManager.h` — the complete `UFUNCTION(exec)` set is **five**:

`SummonTestUnit` · `ApplyTestDamage` · `AddTestGold` · `SetTestDamageBoost` · `DumpAssistantPrompt`

None plays, discards, or targets a hand slot. `PlayHandSlot` is not `exec` and has no console entry: `FAutoConsoleCommand` has **0 occurrences** in `Source/`, and the only `IConsoleManager` use is `TAutoConsoleVariable<int32> siege.Input.LayoutPollEnabled` — a cvar, not a command.

**MCP has no input lane and no console-exec lane either.** I enumerated the live toolsets: `EditorAppToolset` (PIE start/stop, cvars, capture, camera, selection), `LogsToolset` (verbosity + log reads), plus asset/actor/blueprint toolsets and a *sandboxed* `ProgrammaticToolset` that orchestrates registered tools only. **There is no "execute console command" and no synthetic-input tool.**

⇒ Per clause (0) this would normally hand off to Jonathan. **It did not need to** — the archived log answered the question completely and with better fidelity than a re-run could have. **No instruction for Jonathan is required for this row.** For completeness, had one been needed it could not have worked anyway: the current `deck1` contains no `Fog`.

---

## 🚨 ONE THING QA / build-master SHOULD ACT ON — the evidence is UNTRACKED and PRUNABLE

The sole record of Jonathan's recorded session is:

```
Saved/Logs/GitClaudeUnrealTest-backup-2026.09.05-04.50.26.log
```

`git check-ignore` ⇒ **`.gitignore:111  Saved/`** — untracked, and UE prunes old `-backup-` logs on startup. **Every editor launch risks deleting the only copy of the proof behind this verdict.** I did **not** copy it: my contract names the handoff as sole write. **Recommend a manager row to promote it (or its 40-line cast window) into `.claude/pipeline/playtest-evidence/2026-09-04/`.** This is a recommendation, not an action taken.

---

## Findings recorded (NOT fixed — per clause 5)

- **No defect in the hand-advance path.** Cast #1 behaved correctly. `TASK-1027` closes with no fix row.
- **VID-006 finding F3 is RESOLVED and can be struck** — its "one unresolved anomaly" is explained; the analyst was right to refuse to pick from pixels, and right that reading (i) was live.
- **F1 corroborated at runtime, and it is the whole of "nothing happened"** — his own log carries `spell VFX '/Game/VFX/NS_Spell_Fog…' not found` and `…NS_Spell_BrightSun… not found`, plus `Sound '/Game/Audio/S_SpellCast' unresolved`. Both fog resolvers **ran**: `Fog raised for 300 s (refresh, never stack)` twice, and `BrightSun: fog BURNED OFF and PREVENTED for 120 s`. **Every spell did its job; nothing rendered.** Already boarded — not re-boarding.

---

## Fences honoured

Source edits **0** · compiles **0** · mutating Git **0** (`show` / `status` / `diff` / `check-ignore` only) · MCP calls were **read-only metadata** (`list_toolsets` / `describe_toolset`; no PIE started, no level touched, no asset written) · editor left **UP**, `L_Arena` **never saved** · board write limited to `TASK-1027`'s own `status:` line.

## What QA should scrutinise

1. **Step 1 of the chain is the whole verdict.** If `targeting mode entered` could print on a path where `bInTargetingMode` is false, the `INDEX_NONE` hole reopens and the verdict degrades to ambiguous. I read it at `HEAD:3263` (set) vs `HEAD:3296` (logged, last statement). Re-read that ordering.
2. **The epoch correlation.** Three casts to 89 ms and a fourth event inside a 0.3 s pixel bracket. If you think the log is a different session, that is the number to attack.
3. **The positive control.** I claim the zero is real on three legs. One leg failing is not fatal; all three failing would make the tripwire absence meaningless.
4. **I asserted the deck had ≥3 Fogs from `6 entries / 50 cards` + `UNCAP-§3`, not from a measured Fog count** — the played deck's exact composition is **unrecoverable** (overwritten at 21:51). This is the softest link and I flag it as such: it is an *explanation* of the pixels, not part of the proof. **The verdict rests on steps 1–3, which are independent of it.**
