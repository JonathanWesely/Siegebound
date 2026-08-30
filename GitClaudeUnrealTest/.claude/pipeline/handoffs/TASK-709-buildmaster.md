# TASK-709 — THE INTEGRATION (build-master handoff)

**Date:** 2026-08-30 · **Batch:** TASK-704 + 705 + 706 + 707, gated by `qa/TASK-708.md` (PASS)
**Outcome:** ✅ **COMPILE PASSED** · ⚠️ **SUITE RED 155/156** · ⛔ **NOTHING COMMITTED** · ⛔ **NOTHING PUSHED**

---

## 1. THE HEADLINE

The compile is clean and the batch is one unsound test assertion away from landing. **The shipped feature code is correct** — the single red test is a fixture defect in the batch's own test file, and it is a defect that **could not pass no matter how correct the code is**. I authored no fix: a test is code and owes its own `SC-§27` diff-scoped verdict.

---

## 2. COMPILE — ✅ `Result: Succeeded`, PARSED

**`Result: Succeeded`** at **line 43** of the build log. ⛔ The exit code was **0**, which proves nothing here (Build.bat returns 0 on failure via the Live-Coding mutex) — the log line is the authority and it is what I acted on.

Corroborating scan of the full 45-line log: zero `error C####`, zero `error LNK`, zero `fatal error`, zero `0x800711C7`, and ⭐ **zero `C2248`, zero `C2445`**.

⇒ **QA's watch-items 4 and 5 never fired.** D-10's pragma-wrapped `IsFocusable` field write compiled exactly as QA ruled it would, and the documented `TSubclassOf`-vs-`UClass*` ambiguity stayed resolved. Nothing was reverted the wrong way.

All three batch translation units built, and the module linked:
`SiegeControlsHelpTest.cpp` · `SiegeControlsHelpWidget.cpp` · `SiegePlayerController.cpp` → `UnrealEditor-GitClaudeUnrealTest.dll`. 18 actions, 19.42 s.

**Preconditions measured, ⛔ not assumed:**
- Editor **down** (`Get-Process UnrealEditor*` → nothing) — and ⛔ I did not open it, before or after.
- **Smart App Control OFF** — `HKLM:\SYSTEM\CurrentControlSet\Control\CI\Policy\VerifiedAndReputablePolicyState = 0`. Checked *before* building, so the 2-second `0x800711C7` trap was pre-cleared rather than diagnosed after the fact.
- No cook, no UBT, no AutomationTool, no other gate in flight (QUIET-MODULE satisfied).

---

## 3. SUITE — ⚠️ 155 / 156

Ran headless: `-ExecCmds="Automation RunTests Siegebound;Quit" -nullrhi -unattended -nopause -nosplash`. No GUI, no PIE, no desktop interaction — safe with the desktop locked.

**The COUNT is 156 and QA's §7 figure is confirmed on two independent instruments:**

| Instrument | Result |
|---|---|
| Static `IMPLEMENT_*_AUTOMATION_TEST` sweep | **156** across **13** files, **13** in `SiegeControlsHelpTest.cpp` |
| Unique executed test paths in the run log | **156** |

⇒ **the total is right; the colour is not.**

```
Siegebound.ControlsHelp.RawLanesAreIdentity      Result={Fail}
  Error: Expected '...and it is EXACTLY ONE translation, never two' to be true.
  SiegeControlsHelpTest.cpp(632)
```

155 pass. ⭐ **Test 2, `MappedLaneIsNeverDoubleTranslated`, is GREEN** — so the batch's headline guarantee, the double-translate guard on the shipped Lane-A primary path, is now proven **at runtime**, not merely at source as QA had it.

### ⭐ THE SHIPPED CODE IS CORRECT — the defect is one assertion in the fixture

Measured at source:

`MakeDvorakTranslation()` (`:85-98`) injects **11 entries whose domain is entirely letters**, and maps the accept key `Z → Semicolon` (`:98`).

| Step | Value |
|---|---|
| `ZTranslated = GetPositionalKey(Z)` | `Semicolon` |
| `DerivedResolved[0]` | `Semicolon` ⇒ **exactly one translation happened — line 630 PASSED** |
| `GetPositionalKey(ZTranslated)` = `GetPositionalKey(Semicolon)` | `Semicolon` — **an identity: `Semicolon` is not a letter, so it is not in the map's domain** |
| The assertion at `:633` | `Semicolon != Semicolon` ⇒ **FALSE by construction** |

⇒ The probe **cannot pass regardless of code correctness**, and worse, at this position it **cannot distinguish one translation from two** — precisely the thing it claims to measure. Contrast the deliberate authoring at `:88-89`, where `F → U` **and** `U → G` both exist so that test 2's second hop lands somewhere distinct. Test 4 reuses that idiom on a key that has **no second hop**.

⛔ **The failing branch is not the shipped state.** It is the Lane-C **F-1 reversal** case (`bLiteralKeyLabel == false`). The shipped pin — literal `Z` on every layout, per `KBD-§8` — is asserted at `:613-619` and **passes**.

**Two candidate repairs — ⛔ neither authored here, for the programmer to choose and QA to rule on:**
- **(a)** give `Semicolon` a second hop in the fixture map, making the probe meaningful; or
- **(b)** drop/restate `:632-633` for Lane C, recording that `Z`'s translated position is a fixed point by construction and that test 2 already carries the real double-translate guarantee.

### ⚖️ The process note worth keeping

This is a defect **pre-compile QA structurally cannot catch**: QA correctly never runs the suite, and this batch had never been executed before tonight — this was its first execution. QA §7's "the tests assert behaviour, not the pin" is true of their *design*; this one assertion is simply unsound for the key it chose. ⛔ **No criticism of the PASS is implied** — it is the pipeline working as intended, one stage later.

---

## 4. ⭐ THE TASK-705 LFS TRAP WAS REAL — AND IS NOW DISARMED IN THE INDEX

TASK-705 declared it and **it was live.** Verified **oid-vs-sha256**, ⛔ never by size:

| File | Staged oid AS FOUND | Working-tree sha256 | Verdict |
|---|---|---|---|
| `Content/Input/Actions/IA_ControlsHelp.uasset` | `b3757ca0…3df2f` | `b3757ca0…3df2f` | ✅ already correct |
| `Content/Input/IMC_Hero.uasset` | **`9654fae4…2950`** — byte-identical to **HEAD**, the **PRE-append** blob | **`9ba4aeb0…46ab`** | ⛔ **MISMATCH** |

⇒ ⭐ **A commit as-found would have shipped the TAB mapping ABSENT while every file "looked" staged, and the loss would have been invisible until someone pressed TAB.** The size difference (13575 → 14099) corroborates but is *not* what I ruled on.

Both paths were re-`add`ed; both staged oids now equal their file sha256, and both match TASK-705's declared values.

**This staging is deliberately LEFT IN PLACE** so the next integrator inherits the disarmed state rather than rediscovering the trap. ⛔ Do not `git reset` these two paths.

---

## 5. `IA_ControlsHelp` — ✅ **IT EXISTS**

`Content/Input/Actions/IA_ControlsHelp.uasset`, sha256 `b3757ca0…3df2f`, matching TASK-705's declared value, with the `IMC_Hero` 25→26 append present in the working tree.

⇒ **TAB is NOT inert for design reasons** — the asset landed. It is unpressable tonight only because the batch is uncommitted and the editor is down. ⛔ Per QA's WARN-4 I did **not** use the `(not bound)` / "no key named" path as a diagnostic: it is unreachable, since Lane A's fallback always yields a chip from `QwertyReferenceKeys`.

---

## 6. STATE LEFT BEHIND — DECLARED

- ⛔ **No commit · no push · no editor · no MCP · no PIE.** `main` untouched at **`22728c8`**.
- ⚠️ **`main` is 0 ahead / 0 behind `origin/main` — the orchestrator's "several commits ahead" was stale.** Jonathan has evidently pushed since; consistent with his habit of self-committing and pushing without telling the pipeline. Measured, not assumed.
- **Index:** the two TASK-705 LFS paths staged (§4). Everything else left exactly as found.
- **Fences honoured:** ⛔ never touched `Tools/Packaging/ship.ps1` or the `packagedZIPofGame` tree; no zip; `git add -A` never used.
- ⛔ **Did NOT act on QA's Ctrl+Z finding** (`SiegeControlsHelpWidget.cpp:939`) — boarded as a follow-up, comment-only, not mine.

### Commit cargo, prepared and ready for the re-run (explicit paths only)

```
Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.h        (new)
Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp      (new)
Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp  (new)
Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h          (mod)
Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.cpp        (mod)
Content/Input/Actions/IA_ControlsHelp.uasset                           (new, LFS)
Content/Input/IMC_Hero.uasset                                          (mod, LFS)
.claude/pipeline/TASKBOARD.md
.claude/pipeline/CONVENTIONS.md
.claude/pipeline/handoffs/TASK-704-programmer.md
.claude/pipeline/handoffs/TASK-705-artist.md
.claude/pipeline/handoffs/TASK-706-programmer.md
.claude/pipeline/handoffs/TASK-707-programmer.md
.claude/pipeline/handoffs/TASK-709-buildmaster.md
.claude/pipeline/qa/TASK-708.md
```

**Explicitly EXCLUDED as other lanes' work** (verified by reading their diffs, not by guessing): `Config/DefaultEngine.ini` + `Config/DefaultGame.ini` (TASK-698/713 — the `PKG-8` identity and `PKG-9a-1` struck key), `Docs/setupdirections.md`, `.claude/commands/`, `Tools/Packaging/` (**TASK-718, fenced**), and handoffs `698`/`699`/`701`/`712`–`717` plus `qa/TASK-702.md`.

⚠️ **One declared deviation for the record:** `TASKBOARD.md` and `CONVENTIONS.md` are shared hub files whose working-tree diffs (+291 and +398 lines) carry **many** lanes' writes, not just this batch's. They cannot be split by task without surgery, so whichever commit lands them will sweep in neighbouring lanes' status lines. Flagging it rather than hiding it. **Secret scan over all cargo: CLEAN** (no `HF_TOKEN`, key, or JWT pattern).

---

## 7. WHAT I NEED

Route `SiegeControlsHelpTest.cpp:632-633` back to the **gameplay-programmer** (counts as a QA loop), then re-run TASK-709. **Everything else in this batch is ready to land the moment that one assertion is sound** — the compile is clean, the cargo is staged and verified, and the LFS trap is already disarmed.

---
---

# TASK-709 — THE RE-RUN (build-master handoff, second integrator)

**Date:** 2026-08-30 · **Batch:** TASK-704 + 705 + 706 + 707 + 719, gated by `qa/TASK-708.md` (PASS) **and** `qa/TASK-720.md` (PASS — 0 blocker · 0 warn · 3 nit)
**Outcome:** ✅ **COMPILE PASSED** · ✅ **SUITE 156 / 156 GREEN** · ✅ **COMMITTED (one commit, explicit paths)** · ⛔ **NOT PUSHED**

⛔ Nothing above this line was altered — the predecessor's record stands as written. Its four load-bearing findings (the clean compile, the unsound assertion, the LFS trap, the `IA_ControlsHelp` existence proof) were **inherited and re-verified**, not redone from scratch.

---

## 1. THE HEADLINE

**The batch landed.** The one red test is green, the compile is clean, and the LFS trap the predecessor disarmed was still disarmed when I checked it — ruled again on **oid-vs-sha256**, ⛔ never on size.

---

## 2. GATE 1 — ⭐ EXACTLY ONE SOURCE FILE CHANGED, MEASURED TWO WAYS

`qa/TASK-720.md` was git-fenced and asked the integrator to close this gap. I closed it on **two independent instruments**, because ⚠️ **`git diff --stat` alone structurally cannot see it**: the three new batch files are **untracked**, so a working-tree diff would silently omit the very file in question.

| Instrument | Result |
|---|---|
| `find Source -newermt '2026-08-30 01:35:00'` (the predecessor's build window) | **exactly one file** — `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeControlsHelpTest.cpp`, mtime `02:04:42` |
| **The compiler's own adaptive-unity decision** | `[1/4] Compile [x64] SiegeControlsHelpTest.cpp` — **one TU rebuilt**, then link + metadata. The predecessor's run built **three**. |

⇒ ⭐ **The build system independently corroborates the file-system evidence.** Had `SiegeControlsHelpWidget.cpp` or `SiegePlayerController.cpp` changed, UBT would have recompiled them; it did not. **Gate 1 PASSES.**

Tracked-file diff vs `HEAD`, for the record: `SiegePlayerController.h/.cpp` (this batch's, byte-unchanged since the predecessor compiled them) plus three **other lanes'** files excluded from cargo — `Config/DefaultEngine.ini`, `Config/DefaultGame.ini` (TASK-698/713) and `Docs/setupdirections.md`.

---

## 3. COMPILE — ✅ `Result: Succeeded`, PARSED FROM THE LOG

**`Result: Succeeded`** at **line 27**. ⛔ The exit code was **0** and I did not act on it — Build.bat returns 0 on a failed build via the Live-Coding mutex, so the log line is the only authority.

Corroborating scan of the full log: **0** hits across `error C####` · `error LNK` · `fatal error` · `C2248` · `C2445` · `0x800711C7` · `Result: Failed`.

**Preconditions measured before building, ⛔ not assumed:**
- Editor **DOWN** (`tasklist` → no `UnrealEditor*`). ⛔ I never opened it, before or after. ⛔ No MCP, ⛔ no PIE, ⛔ nothing interactive — the desktop is locked.
- **Smart App Control OFF** (`VerifiedAndReputablePolicyState = 0x0`) — the 2-second `0x800711C7` trap pre-cleared rather than diagnosed after the fact.
- 4 actions, 5.44 s. `UnrealEditor-GitClaudeUnrealTest.dll` relinked.

---

## 4. SUITE — ✅ **156 / 156**, AND THE COUNT IS RIGHT

Headless, no GUI: `-ExecCmds="Automation RunTests Siegebound;Quit" -nullrhi -unattended -nopause -nosplash -NoLiveCoding`.

| Instrument | Result |
|---|---|
| Static `IMPLEMENT_*_AUTOMATION_TEST` sweep | **156** across **13** files — **13** in `SiegeControlsHelpTest.cpp` (QA's §7 figure, confirmed) |
| `Test Completed` lines in the run log | **156** |
| Unique executed `Path={Siegebound.*}` values | **156** |
| `Result={Success}` | **156** |
| `Result={Fail}` | **0** |

⇒ ⭐ **The count held at 156** — not 155, not 157. A different count would have been a finding; it isn't one.

```
Siegebound.ControlsHelp.RawLanesAreIdentity      Result={Success}
```

**All 13 `Siegebound.ControlsHelp.*` tests are green**, including both double-translate guards: test 2 `MappedLaneIsNeverDoubleTranslated` (the Lane-A primary path) and the repaired test 4 (the Lane-C F-1 reversal).

### ⚖️ What the repair actually bought

The predecessor's failure was **never a code defect**. `MakeDvorakTranslation()` is a **letters-only** map, and the accept key translates `Z → Semicolon` — so the old `:632` assertion reduced to `Semicolon != Semicolon`, **false by construction**, and worse, at that position it **could not distinguish one translation from two** — precisely the thing it existed to measure. TASK-719 moved the claim onto `F → U → G`, where a second hop genuinely exists, and added a self-check at `:654-659` asserting `TwoHops != OneHop` **before** the claim, so the probe can never again silently collapse into a tautology.

⭐ **The test written to guard the batch's headline defect could not fail. It now can — and it passes.**

---

## 5. THE LFS TRAP — ✅ STILL DISARMED, RE-VERIFIED BY OID, ⛔ NEVER BY SIZE

The predecessor deliberately left both paths staged. I confirmed the staging survived — TASK-703's commit landed in between and could have disturbed the index; it did not:

| File | HEAD oid | Staged oid | Working-tree sha256 | Verdict |
|---|---|---|---|---|
| `Content/Input/IMC_Hero.uasset` | `9654fae4…2950` (**pre-append**) | `9ba4aeb0…46ab` | `9ba4aeb0…46ab` | ✅ **staged = tree ≠ HEAD** — the append lands |
| `Content/Input/Actions/IA_ControlsHelp.uasset` | *(absent)* | `b3757ca0…3df2f` | `b3757ca0…3df2f` | ✅ new file, exact match |

`git check-attr filter` confirms **`filter: lfs`** on both, so both entered as pointers with their blobs in LFS.

⇒ ⭐ **Committing as-the-predecessor-found-it would have shipped the TAB mapping ABSENT while every file "looked" staged** — invisible until someone pressed TAB. That did not happen.

---

## 6. ⭐ **TAB IS GENUINELY LIVE ONCE THIS LANDS**

`IA_ControlsHelp.uasset` **exists** (sha256 `b3757ca0…3df2f`, matching TASK-705's declared value) **and** the `IMC_Hero` 25→26 row append is in the committed blob. The controller's soft-ref resolve therefore has something real to resolve. ⛔ It is unpressable *tonight* only because the editor is down and the desktop is locked — **not** because anything is missing or inert. TASK-710 (Jonathan's sitting) can press it.

---

## 7. ⭐ THE BATCH'S REAL FINDING — WHY THIS SCREEN NEARLY LIED ON ITS FIRST FRAME

The action registry authored in TASK-704 had typed the **literal letters `T` and `E`** into four detail sentences. Those two letters are **exactly the two US-Dvorak relocates** — `T → Y` and `E → .`. A literal transfer would therefore have printed **the wrong keys on the very screen built to print the right ones**, and — because this machine is QWERTY, where the map is identity — **the defect would have been invisible to every check any of us can run here.** TASK-707 caught it in 704's own prose rather than inheriting it; every label now derives through the layout accessor at open time.

⇒ The two findings rhyme: **a screen that could have printed wrong keys unseeably, and a test that could not have caught it.** Both closed in this commit.

---

## 8. STATE LEFT BEHIND — DECLARED

- ✅ **One commit, explicit pathspec** (`git commit -F <msg> -- <paths>`, the TASK-703 idiom) — ⛔ `git add -A` never used, ⛔ **not pushed**.
- ⚠️ `main` is now **2 ahead of `origin/main`, unpushed** (it was **1** ahead at `2cc8213` when I started — Jonathan self-commits and pushes without telling the pipeline; measured, not assumed).
- **Board:** TASK-704/705/706/707/708/709/719 flipped to **done**. ⭐ **TASK-720 had no board row at all** — the re-QA was dispatched without one; I wrote the row rather than leave a PASS unrecorded.
- **Excluded as other lanes' work, verified by reading their diffs:** `Config/DefaultEngine.ini` + `Config/DefaultGame.ini` (**TASK-698/713**), `Docs/setupdirections.md`, and handoffs `698`/`699`/`699/`/`713`/`715`/`716`.
- ⛔ **`Tools/Packaging/` and the `packagedZIPofGame` tree never touched** (TASK-718 fence). No cook, no zip, no UAT.
- **Secret scan over all cargo: CLEAN** — no `HF_TOKEN`, key, JWT, or private-key pattern. (The single grep hit is the predecessor's own prose *about* scanning.)
- ⛔ **Still not acted on:** QA's Ctrl+Z finding at `SiegeControlsHelpWidget.cpp:939` — comment-only, boarded as a follow-up, not mine to author.

---

## 9. FOLLOW-UPS FOR THE MANAGER

1. **TASK-710** — Jonathan's acceptance sitting is now genuinely pressable: TAB is bound and the code is in `HEAD`.
2. **TASK-711** — the first real `/ship` was gated on "the thing worth re-shipping"; that thing now exists.
3. **The Ctrl+Z finding** (`SiegeControlsHelpWidget.cpp:939`) still wants its own task.
4. ⚠️ **A process note worth a law line:** `git diff --stat` **cannot** see a change to an untracked file, so "the diff must show exactly one source file" is unenforceable as literally written for a batch whose files are new. The sound instruments are **mtime vs the prior build window** and **which TUs the compiler chose to rebuild** — both used here.
