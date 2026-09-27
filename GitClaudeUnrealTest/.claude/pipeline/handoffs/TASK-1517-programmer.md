# TASK-1517 — handoff (gameplay-programmer) — VERIFIER-BODY-LAW-ALIGN

Marker `TASK-1517-VERIFIER-BODY-LAW-ALIGN`. Gate: `TASK-1518`. Host: `TASK-1520` (docs-only).

## Files touched
- `.claude/agents/playtest-verifier.md`: BODY only (every hunk is at line 34 or below; the closing `---` is line 7).
- `.claude/pipeline/TASKBOARD.md`: this row's `status:` line only.
- This handoff.
- NOT touched: `CLAUDE.md` (never authored, never staged), `CONVENTIONS.md`, anything under `Tools/`, the frontmatter.
- No live `playtest-verifier` ran during the edit (per the dispatch).

## Frontmatter byte-identity proof
| measurement | before my first edit | after my last edit |
|---|---|---|
| `head -7 … \| sha256sum` (working tree, CRLF) | `653fc50c2a29d071cd171daf0c2625e8b09ec58d0ce8591dc174e47faaa4997e` | `653fc50c2a29d071cd171daf0c2625e8b09ec58d0ce8591dc174e47faaa4997e` |
| first 7 lines, raw byte compare vs the pre-edit copy (python `rb`) | n/a | `True` (identical) |
| whole file sha256 | `4db4ee6a0c5fb4ac6055a47de9de7d0a838e23d5fb7c03e7d76bc4d5d2fa177b` (= `qa/TASK-1505.md` §3.2) | `1dffc539d7ff16bd0a4092fb4ce39d767209ef5c54275390500ff27b2d843558` |
| bytes / CR / LF | 17513 / 183 / 183 | 19794 / 216 / 216 (CRLF pairs 216: uniform CRLF, `VER-§12` cl. 7d byte count) |

Why `git show HEAD:… | head -7 | sha256sum` gives a different number (`5c2b409f…4660749`): `core.autocrlf=true`, so the HEAD blob is stored LF (0 CR / 7 LF in its first 7 lines). The working-tree `head -7` with CR stripped hashes to that same `5c2b409f…`. The frontmatter therefore matches HEAD as well as the pre-edit working tree.

## Diff shape
`git diff --numstat`: `58  25  GitClaudeUnrealTest/.claude/agents/playtest-verifier.md`. Line count 183 → 216 (+33 = 58 − 25).
`git diff -U0` hunk headers (none at or above line 7):
```
@@ -34 +34,2 @@
@@ -37,9 +38,19 @@
@@ -98 +109 @@
@@ -127,2 +138,5 @@
@@ -130 +144,4 @@
@@ -134 +151,4 @@
@@ -160,7 +180,13 @@
@@ -172,2 +198,3 @@
@@ -175 +202,2 @@
@@ -177,0 +206,5 @@
```

## (1)–(5): the new text, with every pre-existing sentence I changed named
Line numbers are new-file lines.

### (1) `VER-§3` cl. 6: the standing PIE grant
- **Inputs bullet (old 32–34 → new 32–35).** Before: *"… (if yes, the orchestrator has already announced that PIE will be driven — do not start until told "go")."* After: *"… (if yes, the orchestrator has already announced that PIE will be driven — no per-run "go" is owed: his standing grant, `VER-§3` cl. 6)."*
- **Heading (old 37 → new 38).** Before: `## Jonathan present ⇒ do not start until told "go"`. After: ``## Jonathan present ⇒ announced and reported, no wait for "go" (`VER-§3` cl. 6)``.
- **Section body (old 38–45 → new 39–56).**
  - *"Aura verification drives PIE in the editor he may be looking at."*: kept verbatim (39).
  - REMOVED: *"If the dispatch says Jonathan is present, the orchestrator has already announced the run; you still do NOT touch PIE, input simulation, or the viewport until the dispatch (or a follow-up message from the orchestrator) says "go"."* REPLACED BY (39–42): *"His standing grant (2026-09-20, quoted verbatim in `VER-§3` cl. 6) discharged the WAIT for a per-run "go": a verifier dispatched without one DOES start PIE, and a row still carrying `blocked-by: 🧑 his PIE go` is read as discharged, not pending."*
  - CHANGED: *"If the dispatch is silent on his presence, treat him as present and ask before the first PIE call."* → (42–43) *"If the dispatch is silent on his presence, treat him as present (`VER-§3` cl. 2: when in doubt, present)."*
  - ADDED (44–53), the surviving fences the row names:
    - 44 *"The grant removes the wait and nothing else:"*
    - 45–48 announcement + report: *"The orchestrator announces the run before it dispatches you, and you REPORT it: your ⚙️ Dev & QA post and your `qa/TASK-###-verify.md` report (see Output), with the orchestrator's checkpoint, are how he is told a PIE run happened. ⛔ Never drive PIE silently."*
    - 49–50 census first: *"The editor census runs FIRST, by command line, every PID classified (`SC-§118`). The grant removes a wait, not an identification."*
    - 51–52 `-game`: *"Any `-game` instance is his: ⛔ never driven, never PIE'd into, never closed (`SC-§118`, unrelaxed)."*
    - 53 `.sav`: *"`.sav` net zero is proven (sha256 AND mtime, `SC-§125`), never asserted."*
  - *"His editor state is never collateral — …whatever it observed."* and *"If the editor is already in PIE when you look, that is his session — ⛔ never stop it; report and wait (`VER-§3` cl. 4)."*: text verbatim, only re-wrapped (54–56). cl. 4 survives.
- **Speed preamble (old 98 → new 109), N6.** *"the Jonathan-present wait"* → *"the Jonathan-present rules"*. Nothing else on the line changed.
- **Output "could not start" sentence (old 172–173 → new 198–200).** Before: *"A run that could not start (editor down, Aura disconnected, row not `built`/`qa-passed`, Jonathan present without a "go") produces NO verdict — report the blocker instead."* After: *"A run that could not start (editor down, Aura disconnected, row not `built`/`qa-passed`) produces NO verdict — report the blocker instead. A missing per-run "go" is not a blocker (`VER-§3` cl. 6)."*
- Consistency with `CLAUDE.md` line 67 as the orchestrator changed it on his word (*"… announces it first and reports it — no wait for a go (his standing grant, 2026-09-20, `VER-§3` cl. 6 …)"*): the body now says the same three things (announce, report, no wait). I read `CLAUDE.md`; I did not edit it.

### (2) `VER-§10` cl. 1 (F2): the `blocked` flip
- CHANGED (old 175 → new 202–203): *"Then flip ONLY your own row's `status:` to `verified` or `verify-failed`"* → *"Then flip ONLY your own row's `status:` to one of three words (`VER-§10` cl. 1): `verified`, `verify-failed` or `blocked`"*. The UNOBSERVABLE parenthetical after it is unchanged (204–205).
- ADDED (206–210): *"A `🚧` outage (editor down, Aura not connected, a needed tool not granted — `VER-§1` cl. 6, `VER-§5` cl. 3) writes no verdict line and flips the row to `blocked`: the status line carries the report path + section, and the blocker's substance goes in the report's prose. ⛔ Not `backlog` (that erases that the row ran and spent an attempt). ⛔ Not `verify-failed` (nothing failed; nothing was measured)."*

### (3) `VER-§1` cl. 5/5a (F3): the verdict derivation
- REPLACED (old 160–166 → new 180–192). Before: *"Verdict rules: `VERIFIED` only when every acceptance line with a runtime signal was observed passing; `VERIFY-FAILED` when at least one such line was observed failing (a verifier that cannot fail is not a gate — write the failing observation, with its evidence path, first); `UNOBSERVABLE` when no acceptance line has a runtime signal. `MEASURED` when no line was observed passing or failing and at least one is a controlled negative — the probe fired, a NAMED control discriminated, and the observable did not move (cell rule `VER-§1` cl. 3a, derivation order cl. 5a)."* After:
  ```
  Verdict rules: derive line 1 from the table's last column in precedence order, first
  match wins (`VER-§1` cl. 5/5a):
  1. any `fail` ⇒ `VERIFY-FAILED` (a verifier that cannot fail is not a gate — write the
     failing observation, with its evidence path, first);
  2. else ≥1 `pass` ⇒ `VERIFIED`. The `unobs` and `measured` lines are listed under
     `## Not examined / limitations this run` (each `measured` line with its control), and
     when n<m (n of the m acceptance lines observable) your row's status line also gets
     `verify: partial (n/m observable)` (`VER-§5` cl. 4: a partial row is not
     `UNOBSERVABLE`);
  3. else ≥1 `measured` with a NAMED control ⇒ `MEASURED`;
  4. else ⇒ `UNOBSERVABLE`.
  A `measured` cell means the probe fired, a NAMED control discriminated, and the
  observable did not move (cell rule `VER-§1` cl. 3a).
  ```
- KEPT verbatim (192–197): *"A `MEASURED` that names no control is read as `UNOBSERVABLE` (`VER-§10` cl. 4). It never blocks and never bounces, and it is never a pass. Its board flip is `VER-§10` cl. 2: … `verify: measured — <the one-line finding>` (`VER-§1` cl. 5a)."* This is the MEASURED paragraph's substance, unchanged.
- F3's gap is closed: a report with ≥1 `pass` + ≥1 `unobs` now lands in branch 2 (`VERIFIED` + `verify: partial`), as the law has it.

### (4) S4 (N5): right-button scope + the keyboard set-active door
- CHANGED (old 127–128 → new 138–139): *"There is no right mouse button on this lane (`VER-§8` cl. 12)."* → *"`ui_perform` has no right mouse button (`VER-§8` cl. 12; `simulate_key_press` is being re-measured by `TASK-1519`)."* (the row's words, verbatim).
- ADDED (140–142): *"Set-active is actuable without it: `IA_MenuSecondary` on a focused deck-bar slot (`VER-§8` cl. 12 amendment; recipe `RCP-deckbuilder-set-active-by-keyboard.md` once `TASK-1515` ships it)."*
- *"Resolve every target with `ui_snapshot` … (`VER-§12` cl. 7a)."*: text unchanged (142–144).

### (5) Pointers
- S5, ADDED after *"… because you hold no shell."* (151–154): *"A film armed before `start_pie` may not survive a level travel (`VER-§12` cl. 7b amendment): when acceptance happens after a travel, arm a recorder AFTER the travel (an in-batch `record_burst` is the measured route), and name every film you find by path."*
- S4, ADDED after the 7a sentence (144–147): *"Capture a frame meant to show thin UI (a thin line, a small glyph) at `max_dim` ≥ 1280 (`VER-§12` cl. 7e). A parameter missing from `run_verification_sequence`'s schema is not an absent capability; a measured run decides (`VER-§12` cl. 7c)."*

## FORBIDDEN list: what I did not touch
Frontmatter (hash above) · `CLAUDE.md` · `CONVENTIONS.md` · `Tools/` · the `Edit`-scope paragraph (lines 20–29, byte-unchanged, outside every hunk) · the `SC-§118` census text in the template's `Editor/Aura state:` line (165, unchanged) · STEP 6 (lines 78–94, unchanged) · S1/S2/S3/S6, steps 1–7, the template block and the Slack paragraph (all outside every hunk).

## For QA (`TASK-1518`) to check closely
1. **"Treat him as present" (42–43).** Under cl. 6, "present" no longer means "wait". It still means announce-and-report, as opposed to cl. 5's `unattended` state line. I kept only the cl. 2 pointer and did not spell out the `unattended` consequence, because the row did not name it.
2. **The `blocked` flip covers the `🚧` outage only** (editor down / Aura not connected / tool not granted: `VER-§1` cl. 6, `VER-§5` cl. 3). I did NOT extend it to "row not `built`/`qa-passed`" (step 2 says report the mismatch and stop; flipping another stage's row to `blocked` is not in `VER-§10` cl. 1). I did NOT extend it to "editor already in PIE" either, because cl. 4 says report and wait and no clause rules a board word for that case. Both are left as the law leaves them.
3. **`n/m`** uses the law's own word ("observable"); I added no finer definition of whether a `measured` line counts toward n.
4. **`VER-§1` cl. 7 (two-limb) is not referenced** by the new derivation. The old body did not reference it either, and the row did not name it, so I did not add it. A later row may want a one-line pointer, because "first match wins" is suspended when cl. 7 applies.
5. **Two forward references will age:** "`simulate_key_press` is being re-measured by `TASK-1519`" (the row's verbatim words) and "recipe … once `TASK-1515` ships it". When those rows land, a follow-up may want to restate them as facts.
6. Line endings: the Edit tool preserved uniform CRLF (216/216 measured by byte count, not by regex).
