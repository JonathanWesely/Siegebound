# Handoff — TASK-1306 (build-master, legs 5a + 5b + 5c)

**Subject:** `TASK-1304` — the deck-builder focus-entry fix, three blocks, one diff.
**Outcome:** committed by pathspec on top of `6bd1aa5`. ⛔ **NOT PUSHED** — Jonathan has not asked.

---

## 🧑 THE DECIDING FACT — his own hands

The verifier returned `Verdict: UNOBSERVABLE` (qualified) because `simulate_key_press` injects **below Slate**
and cannot arm the card grid. The commit was held and the row's one sentence was put to Jonathan directly.

> 🧑 **Jonathan, 2026-09-18, verbatim: *"Yes — the total went up by one."***
> (Relayed by the orchestrator in Claude Code, `SC-§97` — the same provenance path as his `TASK-1300` ruling.)

⇒ The acceptance criterion — *with a card outlined, `Enter` raises the deck total by one* — is **MET**,
by the only instrument that could measure it. `TASK-1286` works end to end and is flipped `done`.

⛔ **The written report was NOT touched.** `qa/TASK-1306-verify.md` still reads `Verdict: UNOBSERVABLE`;
it was not edited, not re-verdicted, not overwritten. His hand check is a **separate, stronger** instrument
recorded **alongside** it — not a promotion of an `UNOBSERVABLE` to a pass. `VER-§5` stands unbent.

## What the automated rig did and did not measure

| half | instrument | result |
|---|---|---|
| focus path (block A) | `ui_snapshot` root `focused`, read 3× (PIE t=43.51 / 78.59 / 111.58 s) | ✅ **`WBP_DeckBuilder_C_0` `"focused": true`**, against a loop-0 negative control of `false` on the identical field |
| key routing → `Enter` | `simulate_key_press` | ⛔ **never reached** — injects below Slate; `FocusedCardIndex` stayed `-1` |
| the acceptance | 🧑 **his hands** | ✅ **"the total went up by one"** |

⛔ The automated rig **never measured `Enter`**. He did. The commit message says exactly that.

## 5a (already reported)

Compile `Result: Succeeded` (parsed from the log, never `$LASTEXITCODE`). **Warning delta 2 → 0, C4996 count 0.**
Suite **561/561 twice**, delta 0 vs baseline 561, reconciled **by name** (`SC-§104`). Editor relaunched PID 1812.

## 5c — clause work

- **(6) `CONVENTIONS.md` fence — 5/5 markers.** Command: `git show HEAD:<path> | grep -c -F -- "<marker>"`
  vs `grep -c -F -- "<marker>" <path>`. (a) `A-2 SCOPE` 0→2 (one is the `AS-§6` bullet head at `:797`)
  · (b) `Scoped — Escape may exit the card grid` 0→1 · (c) the old "read at its widest" sentence **1→1,
  present INSIDE a `~~…~~` strike** ⇒ the STOP condition did **not** fire · (d) `EXERCISED END-TO-END` 0→1
  · (e) `did a gate read the migrated bytes` 0→1. `CONVENTIONS.md` was **staged and verified, never edited** (`SC-§82`).
- **(7) Inherited debt DISCHARGED.** `Saved/AuraVerify/VER-TASK-1286-a3-…png` promoted to
  `playtest-evidence/2026-09-17/` and committed, byte-identical (`sha256 2876957204ec95c7…defdb233`, 398,380 B).
  `TASK-1286`'s status no longer cites a file that does not exist. `TASK-1306`'s own PNG needed no promotion.
- **(8) Flips:** `TASK-1304` → `done` · `TASK-1305` confirmed already `done — PASS` (not rewritten)
  · `TASK-1286` → **`done`** · `TASK-1306` → `done`. `Edit` only, never a truncating write (`SC-§120`);
  the board is **36,460 lines before and after**.

## 🙋 OWED TO THE MANAGER — the clause that was deliberately withheld

**The commit landed ⇒ `TASK-1286` spec block (I)'s `DECK-§` key-table clause in `CONVENTIONS.md` is now owed,
and it is the manager's to write.** It was left unwritten on purpose while the behaviour was unreachable —
`SC-§101` in its purest form, a law describing behaviour that cannot execute. **That condition has ended:**
the entry path is proven at runtime and the acceptance is confirmed by Jonathan's own hands.

## Follow-ups found, not fixed (for the manager to board)

1. ⚠️ **The `AcquireBuilderFocus` read-back log line is one frame too early to witness its own effect**
   (verifier §2): it runs synchronously inside `NativeConstruct`, before the deferred focus flush, so it can
   only ever print the *pre-existing* focus holder and is structurally incapable of printing
   `SObjectWidget == SObjectWidget`. **An instrument defect, not a behavioural one** — the post-flush
   `ui_snapshot` says the focus landed. A next-tick read-back would make the line say what it was built to say.
2. ⚠️ **`simulate_key_press` cannot reach Slate-native widgets** — measured twice now, with a firing control.
   Any future row whose acceptance is a Slate keypress is **unverifiable by the current rig** and should say so
   at boarding time rather than discovering it at 5b.
3. ⚠️ `TASK-1294`'s Aura 401 fired in suite pass 1 and claimed no victim (0 in pass 2) — external, live,
   non-deterministic, nothing attributable to this diff.
