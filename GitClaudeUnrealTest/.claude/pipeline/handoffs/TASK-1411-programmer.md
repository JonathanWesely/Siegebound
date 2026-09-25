# TASK-1411 — [FOGVOLUME-CITATION-FIX] — programmer handoff

- marker: `TASK-1411-FOGVOLUME-CITATION-FIX`
- assignee: gameplay-programmer
- status on exit: `ready-for-qa`
- law: `SC-§39` · `SC-§101` · `SC-§138`
- file touched: `Source/GitClaudeUnrealTest/Siegebound/FogVolume.cpp` — **comment bytes only**
- ⛔ no compile · ⛔ no PIE · ⛔ no MCP · ⛔ no asset · ⛔ no git · ⛔ no editor lifecycle action
  (two editors were up throughout — PID 26992 GUI, PID 39980 `-game`, both 🧑 Jonathan's — **neither touched**;
  `TASK-1399` was driving PIE concurrently and this row stayed text-only start to finish)

---

## 1. The census was RE-MEASURED before it was acted on (`SC-§138`)

`TASK-1398` §3 F6 handed me a finding. I did not take it on trust. Four independent greps:

| claim from `TASK-1398` | my own measurement | verdict |
|---|---|---|
| `USettingsMenuWidget::CreateAndAddToViewport` does not exist | `grep -rn "CreateAndAddToViewport" Source/ --include=*.h --include=*.cpp` returns **five** declarations — `SiegeAssistantConsoleWidget.h:654`, `SiegeControlsHelpWidget.h:843`, `SiegeGraphicsMenuWidget.h:330`, `SiegeGraphicsMenuWidget.h:1305`, `WarMapWidget.h:1082`. **None is on `USettingsMenuWidget`**, whose only appearance in its own header is the class declaration at `SettingsMenuWidget.h:110`. | ✅ CONFIRMED |
| the real one is `USiegeGraphicsMenuWidget::CreateAndAddToViewport` at `SiegeGraphicsMenuWidget.h:330` | `330:	static USiegeGraphicsMenuWidget* CreateAndAddToViewport(` | ✅ CONFIRMED |
| exactly ONE C++ caller, `SettingsMenuWidget.cpp:555` | the only non-comment invocation in `Source/` is `SettingsMenuWidget.cpp:555`: `USiegeGraphicsMenuWidget* Panel = USiegeGraphicsMenuWidget::CreateAndAddToViewport(`. The **three** other spellings of the qualified name are the **definition** (`SiegeGraphicsMenuWidget.cpp:555`) and two **`//` comment lines** (`SiegeGraphicsMenuWidget.cpp:3263`, `SiegePlayerController.cpp:492`), checked individually. ⚠️ **AMENDED 2026-09-23 (`TASK-1412` NIT):** this originally read *"the two other spellings"* and omitted the definition. The claim is unaffected — a definition is not a call — but ⛔ this row's own subject is a census presented as exhaustive that was not, so the count is corrected rather than excused. `SiegePlayerController.cpp:497` is a call to `USiegeFrameRateCounterWidget::CreateAndAddToViewport` — a **different class**, not a caller of this one. | ✅ CONFIRMED |
| not `BlueprintCallable` ⇒ no BP graph reaches it | stronger than stated: the declaration carries **no `UFUNCTION` macro at all** (`SiegeGraphicsMenuWidget.h:315-333` — the doc comment closes at `:329`, the `static` begins at `:330`, nothing between). ⚠️ The `PLAYER ACTIONS (BlueprintCallable …)` banner at `h:336` opens the **next** section and does **not** cover the factory — a reader skimming could mis-read that, which is why it is called out here. No `UFUNCTION` ⇒ no reflection ⇒ **no DIRECT** BP or editor-Python entry point. 🚨 **AMENDED 2026-09-23 (`TASK-1412` WARN-1):** this row first concluded *"unreachable from BP"* **full stop, and that inference is FALSE** — see §8. | ✅ premise CONFIRMED; ⛔ inference CORRECTED |

## 2. The edit — both lines quoted in full

**BEFORE** (`FogVolume.cpp:1161`, one line):

```
// and USettingsMenuWidget::CreateAndAddToViewport has ZERO call sites. ⇒ THE PLAYER CANNOT OPEN
```

**AFTER, AS AMENDED — the shipping text** (`FogVolume.cpp:1161-1165`, five lines):

```
// and USiegeGraphicsMenuWidget::CreateAndAddToViewport (SiegeGraphicsMenuWidget.h:330) has EXACTLY
// ONE C++ caller — SettingsMenuWidget.cpp:555 — and carries no UFUNCTION at all, so it has no
// DIRECT BP or Python entry point; the ONLY route in is USettingsMenuWidget::GraphicsPressed()
// (SettingsMenuWidget.h:151), which IS BlueprintCallable but is a member of that same settings
// screen. Citation corrected TASK-1411. ⇒ THE PLAYER CANNOT OPEN
```

⚠️ The intermediate three-line version this row first wrote — the one `TASK-1412` reviewed — is quoted and
withdrawn in §8. It never left the working tree.

**CUMULATIVE shape vs `HEAD` (what `TASK-1414` will commit): 1 deletion, 5 insertions, net +4 lines,
ONE hunk, ONE file.** `git diff --stat` → `1 file changed, 5 insertions(+), 1 deletion(-)`.

## 3. ZERO EXECUTABLE BYTES CHANGED — stated, and shown

**No executable line changed.** Every added and every removed line begins with `//`. Mechanically:

```
git diff -U0 -- .../FogVolume.cpp | grep -E '^[+-]' | grep -vE '^(\+\+\+|---)' | grep -vE '^[+-][[:space:]]*//' | wc -l
→ 0
```

Zero changed lines survive the "strip everything that is a `//` comment" filter. The hunk sits inside a
free-standing `//` block; no statement, declaration, string literal or preprocessor directive is inside it.

⚠️ **RE-MEASURED after the §8 amendment, not inherited from the first pass** — still `0`. And a second
check `TASK-1412` named, which I had not run the first time: **trailing backslash line-continuation on any
added line**, which would silently swallow the line beneath it out of the build:

```
git diff -U0 -- .../FogVolume.cpp | grep -E '^\+' | grep -v '^+++' | grep -cE '\\[[:space:]]*$'   →   0
```
This row therefore contributes **nothing** to wave B's compile beyond a re-tokenised comment.

## 4. The conclusion is UNCHANGED — and was deliberately not re-argued

The sentence the citation serves is **byte-identical**, including its line wrap:
`⇒ THE PLAYER CANNOT OPEN` / `THE GRAPHICS MENU DURING A MATCH, so nothing here is built for a mid-match
settings change.` The old text broke after `CANNOT OPEN`; so does the new text. Untouched above it:
the `⚠️ SCOPE DELETION` opener (`:1158-1159`) and the `WBP_MainMenu` / `L_MainMenu` clause (`:1160`).
Untouched below it: the `THOSE TWO FACTS ARE THE LOAD-BEARING ONES` paragraph (`:1163` onward, now `:1165`),
which still counts **two** facts — the map fact and the call-site fact — so its arithmetic still holds.

I did **not** restate, soften or re-derive why the player cannot reach the panel. Per spec (3), a second
copy of that reasoning is a future divergence, so the corrected clause states the *facts* and hands off to
the conclusion that was already there.

## 5. One judgement call QA should scrutinise

**The corrected fact is `ONE` caller, not `ZERO` call sites** — a straight class-name swap would have
produced a *false* sentence (`USiegeGraphicsMenuWidget::CreateAndAddToViewport` plainly has a caller).
The conclusion survives anyway, and that is the whole point of the row: the single caller is
`SettingsMenuWidget.cpp:555`, i.e. the settings panel — which line `:1160` has already established is
reachable only from `WBP_MainMenu` on `L_MainMenu`. One caller, and it is on the other map.

**I deliberately did NOT write the old wrong symbol anywhere in the file** — not even as a "was previously
cited as …" note. Re-typing `USettingsMenuWidget::CreateAndAddToViewport` would put the fail-silent grep
target straight back into the file this row exists to clean, which is the exact defect `TASK-1398` found.
The provenance lives in this handoff and in `Citation corrected TASK-1411.` instead.

## 6. For QA (`TASK-1412`)

1. **Comment-only?** Read `FogVolume.cpp:1155-1170` directly rather than trusting §3 — the filter command
   above is reproducible, and the diff is one hunk.
2. **Does the new citation resolve?** `SiegeGraphicsMenuWidget.h:330` and `SettingsMenuWidget.cpp:555`.
   `grep -rc "USettingsMenuWidget::CreateAndAddToViewport" Source/` is **0 across every file in `Source/`** —
   the dead grep target is gone from the code, and it is gone because it was corrected, not deleted.
   ⚠️ **AMENDED 2026-09-23 (`TASK-1412` NIT):** this first said *"repo-wide"*, which is **wrong scope** — the
   symbol still appears in **7 files under `.claude/pipeline/`**, and that is the **desirable** provenance
   record, not a leak. ⛔ The fence is `Source/`.
3. **Conclusion re-argued?** No. §4 names the exact byte-identical span and the untouched neighbours.

## 7. Routing

`ready-for-qa` → `TASK-1412` **PASS (0 BLOCKER / 1 WARN / 3 NIT)** → `qa-passed` → §8 author-amendment at
`qa-passed` (⛔ **not** a re-flip to `ready-for-qa`: QA's fence was the citation and it passed the citation)
→ rides wave B's 5a compile → **no 5b** (a comment has no runtime criterion, and that is not an
`UNOBSERVABLE`) → commit `TASK-1414`. **Nothing committed here** — cargo for wave B's host.

---

## 8. AMENDMENT 2026-09-23 — the author fixing a WARN on its own new bytes, pre-commit

🚨 **This is NOT a QA bounce.** `TASK-1412` returned **PASS, 0 BLOCKER**, and the row stayed `qa-passed`
throughout. What follows is the author correcting **one clause of its own uncommitted new text** — the
cheapest possible moment, and the only one where it costs nothing.

### 8.1 What was wrong — ⛔ the premise was true, the INFERENCE was false

The clause this row shipped into the file at first read:

```
// ONE C++ caller — SettingsMenuWidget.cpp:555 — and is not BlueprintCallable (no UFUNCTION at
// all), so no BP graph reaches it either. Citation corrected TASK-1411. ⇒ THE PLAYER CANNOT OPEN
```

✅ **Premise TRUE and re-confirmed:** `SiegeGraphicsMenuWidget.h:330` carries no `UFUNCTION` (doc comment
`:314-329`, `static` at `:330`, nothing between).

⛔ **Inference FALSE.** I verified it at source myself rather than taking the report on trust:

- `SettingsMenuWidget.h:151` — `UFUNCTION(BlueprintCallable, Category = "Siegebound|Settings")` on
  `void GraphicsPressed();`
- `SettingsMenuWidget.cpp:543` — `void USettingsMenuWidget::GraphicsPressed()`, and its body at `:555`
  **is** the `USiegeGraphicsMenuWidget::CreateAndAddToViewport(GetOwningPlayer(), nullptr, 20)` call.

⇒ **a BP graph reaches the factory in ONE HOP through a `BlueprintCallable` wrapper.** *"No BP graph
reaches it either"* is simply not true. The absence of a `UFUNCTION` on the factory buys **no DIRECT**
entry point — it does not buy unreachability.

🚨 **Why this had to be fixed and not waved through: this row exists because a load-bearing comment in
this file cited something false.** Shipping a *new* over-strong inference inside the repair is the same
defect class the row was opened to remove — and comments in this block are read as evidence by later rows.

### 8.2 The amended clause

```
// ONE C++ caller — SettingsMenuWidget.cpp:555 — and carries no UFUNCTION at all, so it has no
// DIRECT BP or Python entry point; the ONLY route in is USettingsMenuWidget::GraphicsPressed()
// (SettingsMenuWidget.h:151), which IS BlueprintCallable but is a member of that same settings
// screen. Citation corrected TASK-1411. ⇒ THE PLAYER CANNOT OPEN
```

The stronger `UFUNCTION`-absence premise is **kept** and now buys only what it actually buys. The BP route
is **named**, not hidden — and named with the fact that defuses it: `GraphicsPressed()` is a **member** of
`USettingsMenuWidget`, so reaching it requires an instance of the settings screen, which `:1160` has
already established is main-menu-only. ⛔ I wrote *"that same settings screen"* rather than re-stating
"main-menu-only", deliberately: an anaphoric reference cannot drift from `:1160`; a second copy can.

### 8.3 Shapes — stated precisely, because `TASK-1414` is the FIRST actor who can check them

⚠️ QA never saw a whole-file diff (git is fenced for it), so these numbers have had no independent check yet.

| | deletions | insertions | net | hunks | files |
|---|---|---|---|---|---|
| **the amendment alone** (vs the reviewed 3-line text) | 2 | 4 | +2 | 1 | 1 |
| **CUMULATIVE vs `HEAD`** — what gets committed | **1** | **5** | **+4** | **1** | **1** |

`git diff --stat` → `1 file changed, 5 insertions(+), 1 deletion(-)`. The single hunk header is
`@@ -1159,5 +1159,9 @@`.

### 8.4 Fences re-verified after the amendment

- ⛔ **Zero executable bytes** — strip-comments filter re-run on the amended diff: **0** (§3).
- ⛔ **No trailing backslash continuation** on any added line: **0** (§3) — the trap QA named.
- ⛔ **Conclusion byte-identical**, wrap included: `⇒ THE PLAYER CANNOT OPEN` / `THE GRAPHICS MENU DURING A
  MATCH, so nothing here is built for a mid-match settings change.` Both the `⚠️ SCOPE DELETION` opener
  (`:1158-1159`) and the `THOSE TWO FACTS ARE THE LOAD-BEARING ONES` paragraph (now `:1167`) appear as
  **context lines** in the diff — mechanical proof they were not touched.
- ✅ **The two-facts arithmetic STILL HOLDS, re-checked after rewording as instructed.** Fact 1 = the panel
  is reachable only from `WBP_MainMenu` on `L_MainMenu` (`:1160`). Fact 2 = the factory's exposure, carried
  by two coordinate conjuncts that **share one elided subject** — *"…has EXACTLY ONE C++ caller"* and
  *"…carries no UFUNCTION at all"*. I replaced *"and is not BlueprintCallable"* with *"and carries no
  UFUNCTION at all"* **in the same grammatical slot**, so the elision QA measured is preserved. Everything
  after `, so it has no DIRECT BP or Python entry point;` is a consequence clause and its expansion, whose
  subject is the pronoun *"it"* — the same factory. ⛔ No third top-level conjunct was introduced, so
  *"THOSE TWO FACTS"* still counts correctly.
- ⛔ `grep -rn "USettingsMenuWidget::CreateAndAddToViewport" Source/` → **0**, unchanged. The dead symbol
  was not re-typed while rewording.
- ⛔ No compile, no PIE, no MCP, no git, no editor lifecycle action. Both of Jonathan's editors
  (PID 26992, PID 39980) untouched; `TASK-1399` still driving PIE.
- ⛔ One file, one hunk, one clause. Nothing else in `FogVolume.cpp`, no other file, no other row.
