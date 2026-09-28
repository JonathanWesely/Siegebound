# TASK-1548 — programmer handoff: [UI-PERFORM-FOCUS-POINTERS]

- **Row:** `TASK-1548` (marker `TASK-1548-UI-PERFORM-FOCUS-POINTERS`), 2026-09-27, gameplay-programmer. **Gate:** `TASK-1549`. **Host:** `TASK-1550` (docs only).
- **Law pointed at:** `VER-§12` cl. 7f, marker `VER-12-7F-UI-PERFORM-CLEARS-FOCUS` (`CONVENTIONS.md`, read, not written). Source: `.claude/pipeline/qa/TASK-1493-verify.md` (`Verdict: VERIFIED`, H1 HOLDS). Disposition context: `TASK-1539` status line, marker `UI-PERFORM-FOCUS-APPENDS-2026-09-27`.
- **Text only.** No `Source/`, `Content/`, `CONVENTIONS.md` or `CLAUDE.md` byte. No compile, no git write (read-only git with `--no-optional-locks`, `SC-§71a`). No `playtest-verifier` was live while I edited (dispatch prompt).
- **Every change is an insert or an append.** No pre-existing sentence was reworded, struck or deleted. Three pre-existing bullets were extended by appended sentences (named in §4); the old text of each is a byte prefix of the new line (checked with `difflib` against `HEAD`, CR-stripped: `prefix-preserved: True` for all three).

## §1 — Start state, re-measured at my instant (read `rb` + `hashlib`)

| file | sha256 at start | bytes | CR / LF | anchor it matches |
|---|---|---|---|---|
| `.claude/agents/playtest-verifier.md` | `e6dcf6dc646e1240db6a53a5ee2ed0be20a057f68527023d0d72cd98337536fd` | 20883 | 227 / 227 | dispatch: working tree `e6dcf6dc…37536fd`; CR-stripped `8207de496ad8e384a6ea2e07dc478913743faacf2b737c2763199478d949358f` = dispatch `8207de49…949358f` |
| `Tools/Verify/recipes/RCP-menu-to-deckbuilder.md` | `d814ea2c2624f38ed53d9a28536c8621f350cc3ab605bb8f9af95c2e08bebfa8` | 19704 | 0 / 132 | `qa/TASK-1535.md` §5 |
| `Tools/Verify/recipes/RCP-vsbot-capture-center-and-summon.md` | `e1ae86a6acb7cf69843ed60165f47cbb571a86eb919cbb8071274f5f638d8f13` | 32433 | 0 / 248 | `qa/TASK-1535.md` §5 |
| `Tools/Verify/recipes/RCP-deckbuilder-set-active-by-keyboard.md` | `201409670a0da22730a64edb8e551bb7e52785dc91fe4943e6a07d6ca57d3a47` | 36589 | 0 / 204 | `qa/TASK-1535.md` §5 |
| `Tools/Verify/recipes/README.md` | `fb7900d90767852f094e689370d5c531cda928f3f827e3de129e6b7e1c3fe539` | 15943 | 0 / 99 | `qa/TASK-1535.md` §5 |
| `Tools/Verify/recipes/RCP-deckbuilder-slot-and-card-edit.md` | `550ed8b34e2b0a0ec941496ebfd35a2b69e3b10dbf53c3fd047f736af36f0a6a` | 20084 | 0 / 167 | `qa/TASK-1528.md` §5 |
| `Tools/Verify/recipes/RCP-play-unit-card-from-hand.md` | `3eb2d019c372f1d786967e7ba8ad03b15929bae1d5982bf77d56aa8fe44f425f` | 49130 | 0 / 298 | `qa/TASK-1528.md` §5 |

All seven matched. `git status` showed all seven clean against `HEAD` (`9a67a27`) before I started.

## §2 — Frontmatter proof (the `TASK-1534` / `TASK-1517` method)

`head -7` = the first seven lines of the agent file, through the closing `---`, each with its CRLF.

| | raw (CRLF) sha256 | CR-stripped sha256 | bytes |
|---|---|---|---|
| before | `653fc50c2a29d071cd171daf0c2625e8b09ec58d0ce8591dc174e47faaa4997e` | `5c2b409f16987def46a9acd142c60f1c928e37a649395c401e931f11a4660749` | 4326 |
| after | `653fc50c2a29d071cd171daf0c2625e8b09ec58d0ce8591dc174e47faaa4997e` | `5c2b409f16987def46a9acd142c60f1c928e37a649395c401e931f11a4660749` | 4326 |

Both equal the dispatch's `653fc50c…a4997e` / `5c2b409f…4660749`. The only hunk is `@@ -155,0 +156,11 @@`, an insert after line 155, far below line 7. `model: claude-opus-5-5[1m]` and `effort: medium` are untouched. The file stays CRLF: after the edit CR = 238, LF = 238, zero bare LF (counted by byte, `VER-§12` cl. 7d). The edit was made with a Python byte write that asserted the start sha256 and a single occurrence of its anchor line (`  read a row's wording as a grant.`) before inserting.

## §3 — What changed, per file

### (1) `.claude/agents/playtest-verifier.md` (BODY only)

- **Where:** inside S4 ("Pointer facts; the law holds the detail."), appended after its last sentence. S4 is where the body teaches `ui_perform` (`double_click` / `click`, target resolution) and an injected action that acts on "a focused deck-bar slot" (`IA_MenuSecondary`). The body teaches no explicit focus reader and never uses `ui_perform`'s `focused` fields, so S4 is the one place a verifier learns `ui_perform` from the body. No new S-label was added, so the "labelled S1–S6" sentence stays true and unedited.
- **What the 11 new lines say:** a `ui_perform` call clears Slate keyboard focus (cl. 7f + marker; 8 of 8; `type`, `scroll`, `move`, 1-frame `wait`; standalone and inside `run_verification_sequence`) · a focus reading after one, with no focus-setting input between, is void, and so is every `focused` field in its own `before_snapshot` / `after_snapshot` · re-establish first (`IA_MenuDown` / `IA_MenuAccept`; console `IA_AssistantConsole` ×2), then read · an injected `Down` / `Accept` straight after one may start from an unread focus (labelled a sighting) · the other steps (`click`, `double_click`, `press`/`release`, `drag`, `assert`, `wait_for`, `snapshot` as a step) are not measured and the clause may not be cited for them · the mechanism is a HYPOTHESIS.
- **`UEditableTextBox` belief:** the body does NOT carry it (no `EditableText`, `HasKeyboardFocus` or text-box sentence anywhere in the body, grep). Spec (1)'s conditional therefore did not fire and nothing was marked. I added no text-box sentence, since that would be a sentence (1) does not reach.
- **Untouched, checked by the diff:** the `Edit`-scope paragraph, the Jonathan-present / `SC-§118` census text, STEP 6, S1–S3, S5, S6, Output, the verdict rules.

### (2) The recipes

- **`RCP-deckbuilder-slot-and-card-edit.md`** (the recipe `qa/TASK-1493-verify.md` named):
  - Known hazards, the `Tab` bullet: appended a dated note marking its "no `SlotButton` focused" read **UNSUPPORTED, not refuted** (it came after the §1(b) `ui_perform` `click` rows with no focus-setting input between, `TASK-1493` *Blast radius* item 3). It says `click` is outside cl. 7f's measured shapes (so whether those calls cleared focus is `NOT MEASURED`), quotes the manager's "corroboration only" ruling (`TASK-1539`, marker `UI-PERFORM-FOCUS-APPENDS-2026-09-27`), and names what the hazard still rests on: the tool's reply and `FocusedCardIndex -1`, which is a widget member (`UPROPERTY(Transient) int32` on `UDeckBuilderWidget`, `[D]`), not Slate focus. The old sentence is kept whole, not re-verdicted.
  - Known hazards, a new last bullet: the cl. 7f hazard, with two sub-bullets: **inside this recipe, no hit** (with the reasons and the "no re-verified record" statement) and **after it, a fence** (re-establish before a focus read or an injected `Down` / `Accept`, on the returned menu or the builder). It states that none of this recipe's calls is inside cl. 7f's measured scope (`double_click`-only calls; Step 4 mixes `double_click`s with 20-frame `wait`s, the clause measured a 1-frame `wait` alone), so the re-establish after them is caution, not the clause.
- **`RCP-menu-to-deckbuilder.md`:** Known hazards, the two-`WBP_MainMenu` bullet: appended a dated note. Its existing advice "re-read focus first" on a returned menu is a focus read after a `ui_perform` (`double_click` on Exit, the slot recipe's Step 6). The note says `double_click` is outside cl. 7f's measured shapes, tells the run to send a focus-setting input before that re-read (caution, not the clause), keeps the path `NOT MEASURED`, and states that no record in this recipe rests on such a read.
- **`RCP-deckbuilder-set-active-by-keyboard.md`:** Precondition 5 ("No mouse input inside the builder before the walk"): appended "nor any `ui_perform` call before the walk, including one with no pointer step", pointing at cl. 7f (a `type` or a `wait` step is not mouse input, so the existing text did not cover them), and states that no record in this recipe rests on such a read.
- **`README.md`:** the parked list for (3), inserted after the Pending table (§5 below). The README carries no steps, so its (2) census is zero.
- **`RCP-vsbot-capture-center-and-summon.md` and `RCP-play-unit-card-from-hand.md`:** zero hits, no change. Their bytes equal the §1 start anchors.

## §4 — Every pre-existing sentence changed, named

None was reworded. Three bullets gained appended sentences after their last existing sentence:
1. `RCP-deckbuilder-slot-and-card-edit.md`, Known hazards, the bullet that begins "**`simulate_key_press` does not reach this screen's widgets.**". Before (unchanged, still the line's prefix): `… no \`SlotButton\` focused; \`FocusedCardIndex -1\`. \`[M: archer50 §1(b)]\``. After: the same, then "⚠️ **Added 2026-09-27 (`TASK-1548`): the "no `SlotButton` focused" read is UNSUPPORTED, not refuted.** …".
2. `RCP-menu-to-deckbuilder.md`, Known hazards, the bullet that begins "**Two `WBP_MainMenu` instances after returning from the builder.**". Before (unchanged): `… re-read focus first; that path is \`NOT MEASURED\`.` After: the same, then "⚠️ **Added 2026-09-27 (`TASK-1548`): the return is itself a `ui_perform` `double_click`** …".
3. `RCP-deckbuilder-set-active-by-keyboard.md`, Preconditions item 5, which begins "**No mouse input inside the builder before the walk.**". Before (unchanged): `… \`1511\` sent no mouse input at all. \`[M: 1511 Not examined]\`` After: the same, then "⚠️ **Added 2026-09-27 (`TASK-1548`): nor any `ui_perform` call before the walk,** …".

The agent file and the README gained inserted lines only.

## §5 — The (2) census table

Criterion (spec (2)): a step that (a) reads Slate focus, or (b) injects navigation, after a `ui_perform` with no focus-setting input in between. "Chained" means the hit is in the step that comes after this recipe when a run composes recipes. Model reads (`EditingDeckIndex`, `WorkingDeck`, `FocusedCardIndex`, `BrushColor`, `GhostActor`, `CaptureOwner`), log lines and disk reads are not Slate focus readings.

| recipe | step | hit y/n | change |
|---|---|---|---|
| `RCP-deckbuilder-slot-and-card-edit.md` | Step 1 (`ui_snapshot` resolve) | n: resolves targets, reads no focus; follows the menu recipe's injected steps, no `ui_perform` before it | none |
| same | Steps 2, 3 (`ui_perform` `double_click`) + read-backs | n: the steps ARE `ui_perform`s; their read-backs are `EditingDeckIndex` / `WorkingDeck` | none |
| same | Steps 4–5 (`ui_perform` `double_click` + `wait` batch; `WorkingDeck` read-back / top-up) | n: model reads only | none |
| same | Step 6 (`ui_snapshot` by text, then `double_click` Exit; read-back a frame + `ui_snapshot "WBP_MainMenu"`) | n inside the recipe: the snapshots resolve a target and a screen, not focus | none |
| same | after any step: a focus read or injected `Down` / `Accept` on the returned menu or the builder | **y (chained)** | new Known-hazards bullet: the fence + re-establish, scope-labelled (caution for `double_click`) |
| same | Known hazards, the `Tab` record ("no `SlotButton` focused", archer50 §1(b) t=85.11, after `ui_perform` `click`s t=43.63 / 57.13) | **y (a cited record rests on such a read)** | appended: that read marked UNSUPPORTED, not refuted; the hazard's other legs named |
| same | "re-verified" record | none exists ("Not re-verified since") | stated in the new bullet |
| `RCP-menu-to-deckbuilder.md` | Steps 0–4 (focus read, Downs, focus read / top-up, Accept, builder read) | n: no `ui_perform` in the recipe; every cited focus read precedes any `ui_perform` in its run (archer50's first `ui_perform` row is §1(b) t=43.63, after §1(a)'s reads at 4.84 / 17.38; `1511`, `1512`, `1524`, `1519` record no `ui_perform`) | none |
| same | Known hazards: "re-read focus first" on a menu returned by Exit (a `ui_perform` `double_click`) | **y (chained)** | appended: re-establish before that read (caution, `double_click` out of scope); path stays `NOT MEASURED` |
| same | "re-verified" records (`1511`, `1512`, `1524`, `1519`) | did not rest on such a read (same reasons as Steps 0–4) | stated in the appended note |
| `RCP-deckbuilder-set-active-by-keyboard.md` | Steps 0–5, Variation, Controls C1–C3 | n: injected actions only, no `ui_perform` in the recipe; Precondition 5 already barred mouse input | none |
| same | Precondition 5: Steps 1–3 (`IA_MenuDown`, `IA_MenuBack`, `IA_MenuRight` + `DeckBar` snapshots) if a `ui_perform` ran before the walk | **y (chained)**: the existing "no mouse input" did not cover a `type` or `wait` `ui_perform` | appended: no `ui_perform` of any kind before the walk, with the cl. 7f pointer |
| same | "re-verified" records (`1524`, `1519`) and the source (`1511`) | did not rest on such a read: `1511` and `1524` record no `ui_perform`; `1519` records its `ui_perform` right-button shapes as not re-traced | stated in the appended note |
| `RCP-vsbot-capture-center-and-summon.md` | row 1 (`Button_0` focus read), row 2 (`IA_MenuAccept`), rows 3–15 | n: first actions of a fresh session, no `ui_perform` in the recipe, `1512` records none. `TASK-1493`'s own use (rows 1–4, menu t=3.85) preceded its first `ui_perform` (arena t=9.52) | **none (zero recorded here)** |
| `RCP-play-unit-card-from-hand.md` | all steps (hand / ghost reads, `IA_Card<N>`, aim, `simulate_key_press` confirm) | n: no `ui_perform`, no Slate focus read, and `IA_Card<N>` is a controller action, not menu navigation | **none (zero recorded here)** |
| `README.md` | — | n: an index, it carries no steps | (3) parked list inserted |

**Hit count: 4** (slot: 2, the chained fence and the `Tab` record · menu: 1, chained · set-active: 1, chained). **Recipes with a hit: 3 of 5.** Zero: vsbot, play, README.

⚠️ **One correction to the premise, for QA:** `qa/TASK-1493-verify.md` says the slot recipe "chains `ui_perform` `double_click` batches with injected navigation". Read against its bytes, the recipe itself injects no navigation after a `ui_perform`: the menu recipe's injected steps run before its Step 2, and its own reads are model reads. The navigation-after-`ui_perform` exposure is in what a run does NEXT (set-active, or the returned menu), which is where the fences went. The recipe's own step sequence was not changed.

## §6 — The (3) disposition: candidates 1–3 PARKED, candidate 4 is law

Listed in `README.md` (a "Parked 2026-09-27 (`TASK-1548`)" block after the Pending table) with these reasons, each against `VER-§13` cl. 3's bar ("exact steps … the read-back that proves each step landed … preconditions") and the README's format item 4 ("exact tool + parameters, copy-paste ready, each citing its source section"):
1. **The Slate focus reader: parked.** Neither `qa/TASK-1493-verify.md` (*Instrument*, *Recipe candidates* 1) nor the reader's origin `qa/TASK-1489-verify.md` §2.2 quotes the Python script; both name the calls (`HasKeyboardFocus()`, `HasFocusedDescendants()`, `HasUserFocusedDescendants(pc)`, `unreal.ObjectIterator`) and the object paths. The candidate's `unreal.ObjectIterator(unreal.EditableTextBox | unreal.Button)` is shorthand, not a call quoted as run. A copy-paste-ready step would be my script, unmeasured. Unparks when a run quotes its reader verbatim.
2. **The console re-establish (`IA_AssistantConsole` ×2, 0.5 s): parked.** Its read-back is reader 1 (parked), and the report quotes neither the call envelope nor how the console was first opened (the only `IA_AssistantConsole` lines in the report are the re-establish itself and H-M2).
3. **The detail-page re-establish (`IA_MenuDown` on the one-stop `BackButton` ring): parked.** The report does not give the route to the Controls help detail page (no preconditions), and its read-back is reader 1.
4. **Law already** (`VER-§12` cl. 7f).

The re-establish inputs and the reader are named in cl. 7f, so a run needs no recipe to use them; the README says so. No row is boarded to unpark them.

## §7 — Final bytes (read `rb` + `hashlib` after my last edit)

| file | sha256 | bytes | CR / LF |
|---|---|---|---|
| `.claude/agents/playtest-verifier.md` | `9b4976d65a5b58e969a1df9a0758fcafbed62d306eeb9ff1023712b95d347723` (CR-stripped `641142995fb44f8437f196be7c25dd0ef8614c13b0a20a9a18e555061cd5b903`) | 21775 | 238 / 238 |
| `Tools/Verify/recipes/RCP-deckbuilder-slot-and-card-edit.md` | `f72c9530c0633fb002ec7231cdda8e76cde20831addaedeee88cadde60b2e2d7` | 22547 | 0 / 170 |
| `Tools/Verify/recipes/RCP-menu-to-deckbuilder.md` | `aef9083b0c326409c8584bd890525bc15a3cdaf9ce47ae124cac154a28fc436c` | 20629 | 0 / 132 |
| `Tools/Verify/recipes/RCP-deckbuilder-set-active-by-keyboard.md` | `6167149ca6bf5885fa1cc66dc522eaf49322af29a82632f8a1873a88240965fd` | 37186 | 0 / 204 |
| `Tools/Verify/recipes/README.md` | `89347e54082573dd4b5ea84ed18027d4ecee0cf1d275a008e2ded3e2a56d7bd7` | 17367 | 0 / 105 |
| `Tools/Verify/recipes/RCP-vsbot-capture-center-and-summon.md` (untouched) | `e1ae86a6acb7cf69843ed60165f47cbb571a86eb919cbb8071274f5f638d8f13` | 32433 | 0 / 248 |
| `Tools/Verify/recipes/RCP-play-unit-card-from-hand.md` (untouched) | `3eb2d019c372f1d786967e7ba8ad03b15929bae1d5982bf77d56aa8fe44f425f` | 49130 | 0 / 298 |

Recipes stay LF (CR = 0). Git prints its usual "LF will be replaced by CRLF the next time Git touches it" warning on the four touched recipes; I changed no line ending, and the host's byte-identity check is the host's (`TASK-1550` (3)).

Also touched: `TASKBOARD.md`, this row's `status:` line only (→ `in-progress`, then → `ready-for-qa`), found by the unique row header each time.

## §8 — For QA to scrutinise

1. **Scope wording.** Every new line names cl. 7f's four shapes or says a shape is outside them. Where the recipes recommend a re-establish after a `double_click` (out of scope), they call it "caution, not the clause". Please check none of the new lines reads as "every `ui_perform` shape clears focus". The set-active precondition bars "any `ui_perform` call" as a precondition (a rule of use), and cites the measured shapes as its reason, not a claim about the unmeasured ones.
2. **The `Tab` marking.** I marked only the focus half of that hazard. `FocusedCardIndex` is a model member (`DeckBuilderWidget.h`, the `FocusedCardIndex` `UPROPERTY(Transient)`; the header's own comment separates "the MODEL" from Slate). If QA reads it as Slate focus, the marking would need to widen.
3. **The census premise correction** in §5.
4. **The (3) parking** is a judgement against cl. 3's bar. If QA or the manager reads the bar more loosely for a single-call reader, candidate 1 is the one closest to promotable.

## §9 — For the manager (flags, not changes: outside (1)–(3))

- `TASK-1493` used `RCP-vsbot-capture-center-and-summon.md` Step 1 rows 1–4 and wrote "Re-verified in-run: y" (its *Recipes used*). The README's "last verified" cell for that recipe still reads "2026-09-26 (source run; not re-verified since)", and the recipe's Source does not record the use. Recording it is a recipe-maintenance row, not this one.

## §10 — `git diff -U0`, all files touched (CR stripped for display; the agent file's bytes are CRLF, §2 / §7)

```diff
diff --git a/GitClaudeUnrealTest/.claude/agents/playtest-verifier.md b/GitClaudeUnrealTest/.claude/agents/playtest-verifier.md
index fb5c076..7da648a 100644
--- a/GitClaudeUnrealTest/.claude/agents/playtest-verifier.md
+++ b/GitClaudeUnrealTest/.claude/agents/playtest-verifier.md
@@ -155,0 +156,11 @@ S3 before you plan (step 3); apply S1 and S2 to every input you send.
+  ⛔ A `ui_perform` call clears Slate keyboard focus (`VER-§12` cl. 7f, marker
+  `VER-12-7F-UI-PERFORM-CLEARS-FOCUS`: 8 of 8 for its `type`, `scroll`, `move` and
+  1-frame `wait` steps, standalone and inside `run_verification_sequence`). A Slate
+  focus reading taken after one, with no focus-setting input in between, is void, and
+  so is every `focused` field in its own `before_snapshot` / `after_snapshot`.
+  Re-establish first (an injected `IA_MenuDown` / `IA_MenuAccept`; for the assistant
+  console, `IA_AssistantConsole` ×2), then read. An injected `Down` / `Accept` sent
+  straight after one may start from a focus you did not read (a sighting, same
+  clause). Its other steps (`click`, `double_click`, `press`/`release`, `drag`,
+  `assert`, `wait_for`, `snapshot` as a step) are not measured, and the clause may not
+  be cited for them; its mechanism is a HYPOTHESIS.
diff --git a/GitClaudeUnrealTest/Tools/Verify/recipes/RCP-deckbuilder-slot-and-card-edit.md b/GitClaudeUnrealTest/Tools/Verify/recipes/RCP-deckbuilder-slot-and-card-edit.md
index b0d58f8..c985258 100644
--- a/GitClaudeUnrealTest/Tools/Verify/recipes/RCP-deckbuilder-slot-and-card-edit.md
+++ b/GitClaudeUnrealTest/Tools/Verify/recipes/RCP-deckbuilder-slot-and-card-edit.md
@@ -166 +166 @@ All measured at viewport **1280×725**, **DPI 0.6706**, standalone, 1 client. `[
-- **`simulate_key_press` does not reach this screen's widgets.** `Tab` → "delivered to the player controller, but NOTHING BINDS IT"; no `SlotButton` focused; `FocusedCardIndex -1`. `[M: archer50 §1(b)]`
+- **`simulate_key_press` does not reach this screen's widgets.** `Tab` → "delivered to the player controller, but NOTHING BINDS IT"; no `SlotButton` focused; `FocusedCardIndex -1`. `[M: archer50 §1(b)]` ⚠️ **Added 2026-09-27 (`TASK-1548`): the "no `SlotButton` focused" read is UNSUPPORTED, not refuted.** It (t=85.11) came after the §1(b) `ui_perform` `click` rows (t=43.63, t=57.13) with no focus-setting input in between (`.claude/pipeline/qa/TASK-1493-verify.md` *Blast radius* item 3). `click` is outside `VER-§12` cl. 7f's measured shapes (marker `VER-12-7F-UI-PERFORM-CLEARS-FOCUS`), so whether those calls cleared the focus is `NOT MEASURED`. The manager ruled that read corroboration only (`TASK-1539` status line, marker `UI-PERFORM-FOCUS-APPENDS-2026-09-27`). The reply text and `FocusedCardIndex -1` are not Slate focus readings: `FocusedCardIndex` is the widget's own index, a `UPROPERTY(Transient) int32` on `UDeckBuilderWidget` `[D]`.
@@ -167,0 +168,3 @@ All measured at viewport **1280×725**, **DPI 0.6706**, standalone, 1 client. `[
+- **A `ui_perform` clears Slate keyboard focus** (`VER-§12` cl. 7f, marker `VER-12-7F-UI-PERFORM-CLEARS-FOCUS`; measured 8 of 8 for `type`, `scroll`, `move` and a 1-frame `wait` by `.claude/pipeline/qa/TASK-1493-verify.md`, which named this recipe). Added 2026-09-27 (`TASK-1548`).
+  - **Inside this recipe: no hit.** No step takes a Slate focus reading or injects navigation after a `ui_perform`: the state reads are `EditingDeckIndex` and `WorkingDeck` (Steps 2–5), Steps 1 and 6 use `ui_snapshot` to resolve targets and screens, not focus, and the menu recipe's injected steps run before Step 2. The source record (`archer50` A1) and the Read-back rest on none either: they read those members, the save, the log and frames. This recipe has no re-verified record ("Not re-verified since", Source). The one focus read it cites is the `Tab` hazard above, marked there.
+  - **After it: fence.** A run that goes on to a Slate focus reading or an injected `Down` / `Accept` (on the main menu after Step 6, or on the builder after any step here) sends a focus-setting input first (cl. 7f names an injected `IA_MenuDown` / `IA_MenuAccept`), then reads. None of this recipe's calls is inside cl. 7f's measured scope: Steps 2, 3 and 6 are `double_click`-only calls, a shape the clause does not measure and may not be cited for, and Step 4 mixes `double_click`s with 20-frame `wait`s, where the clause measured a 1-frame `wait` on its own. Whether these calls clear focus is `NOT MEASURED`, and the re-establish after them is caution, not the clause. `RCP-deckbuilder-set-active-by-keyboard.md` excludes this path (its Precondition 5), and `RCP-menu-to-deckbuilder.md` fences the returned menu (its Known hazards).
diff --git a/GitClaudeUnrealTest/Tools/Verify/recipes/RCP-menu-to-deckbuilder.md b/GitClaudeUnrealTest/Tools/Verify/recipes/RCP-menu-to-deckbuilder.md
index 85df27a..63b79cf 100644
--- a/GitClaudeUnrealTest/Tools/Verify/recipes/RCP-menu-to-deckbuilder.md
+++ b/GitClaudeUnrealTest/Tools/Verify/recipes/RCP-menu-to-deckbuilder.md
@@ -130 +130 @@ Layout-dependent (not coordinate-dependent): the ×2 depends on the menu's butto
-- **Two `WBP_MainMenu` instances after returning from the builder.** After Exit, `ui_snapshot "WBP_MainMenu"` returned *"matched 2 widgets: WBP_MainMenu_C, WBP_MainMenu_C"* (the Back handler creates a new menu instance). Observed, not diagnosed. `[M: archer50 Not-examined]` If you re-enter the builder from a returned menu, disambiguate (`selector` with `match_index`) and re-read focus first; that path is `NOT MEASURED`.
+- **Two `WBP_MainMenu` instances after returning from the builder.** After Exit, `ui_snapshot "WBP_MainMenu"` returned *"matched 2 widgets: WBP_MainMenu_C, WBP_MainMenu_C"* (the Back handler creates a new menu instance). Observed, not diagnosed. `[M: archer50 Not-examined]` If you re-enter the builder from a returned menu, disambiguate (`selector` with `match_index`) and re-read focus first; that path is `NOT MEASURED`. ⚠️ **Added 2026-09-27 (`TASK-1548`): the return is itself a `ui_perform` `double_click`** (`RCP-deckbuilder-slot-and-card-edit.md` Step 6). `VER-§12` cl. 7f (marker `VER-12-7F-UI-PERFORM-CLEARS-FOCUS`) voids a Slate focus reading taken after a `ui_perform` with no focus-setting input in between. `double_click` is outside its measured shapes, so whether that call clears focus is `NOT MEASURED`, and the clause may not be cited for it. ⇒ Before that re-read, send a focus-setting input (cl. 7f names an injected `IA_MenuDown`), then read; after a `double_click` that is caution, not the clause. Where it lands on a returned menu is `NOT MEASURED` (Fences: any starting focus other than `Button_0`). No record in this recipe rests on such a read: every focus read it cites came before any `ui_perform` in its run (`archer50`'s first `ui_perform` row is §1(b), t=43.63; `1511`, `1512`, `1524` and `1519` record none).
diff --git a/GitClaudeUnrealTest/Tools/Verify/recipes/RCP-deckbuilder-set-active-by-keyboard.md b/GitClaudeUnrealTest/Tools/Verify/recipes/RCP-deckbuilder-set-active-by-keyboard.md
index fe516b9..1a9de68 100644
--- a/GitClaudeUnrealTest/Tools/Verify/recipes/RCP-deckbuilder-set-active-by-keyboard.md
+++ b/GitClaudeUnrealTest/Tools/Verify/recipes/RCP-deckbuilder-set-active-by-keyboard.md
@@ -44 +44 @@ Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are d
-5. **No mouse input inside the builder before the walk.** `[L: VER-§8 cl. 12, 2026-09-26 amendment]` The reason is a code read: after a mouse click puts focus on a bar slot while `FocusedCardIndex` is still armed, injected `Home`/`Left`/`Right` act on the grid while a real key acts on the bar (`1509` N4). `1511` sent no mouse input at all. `[M: 1511 Not examined]`
+5. **No mouse input inside the builder before the walk.** `[L: VER-§8 cl. 12, 2026-09-26 amendment]` The reason is a code read: after a mouse click puts focus on a bar slot while `FocusedCardIndex` is still armed, injected `Home`/`Left`/`Right` act on the grid while a real key acts on the bar (`1509` N4). `1511` sent no mouse input at all. `[M: 1511 Not examined]` ⚠️ **Added 2026-09-27 (`TASK-1548`): nor any `ui_perform` call before the walk,** including one with no pointer step. `VER-§12` cl. 7f (marker `VER-12-7F-UI-PERFORM-CLEARS-FOCUS`) measured a `ui_perform` clearing Slate keyboard focus 8 of 8 for `type`, `scroll`, `move` and a 1-frame `wait` (its other shapes are not measured), and Steps 1–3 move and read focus: after one, the walk may start from a focus no read established. No record in this recipe rests on such a read: `1511` and `1524` record no `ui_perform`, and `1519` records its `ui_perform` right-button shapes as not re-traced.
diff --git a/GitClaudeUnrealTest/Tools/Verify/recipes/README.md b/GitClaudeUnrealTest/Tools/Verify/recipes/README.md
index 0899ef0..a99b58f 100644
--- a/GitClaudeUnrealTest/Tools/Verify/recipes/README.md
+++ b/GitClaudeUnrealTest/Tools/Verify/recipes/README.md
@@ -68,0 +69,6 @@ Run short names used in tags: `archer50` = `.claude/pipeline/qa/PLAYTEST-archer5
+
+**Parked 2026-09-27 (`TASK-1548`): measured, not promoted.** `.claude/pipeline/qa/TASK-1493-verify.md` *Recipe candidates* 1–3 do not yet meet `VER-§13` cl. 3's bar. The reader and the re-establish inputs are named in the law already (`VER-§12` cl. 7f, marker `VER-12-7F-UI-PERFORM-CLEARS-FOCUS`), so a run can use them without a recipe. No row is boarded to unpark them.
+- *1, the Slate focus reader* (`HasKeyboardFocus()` plus the owning root's `HasFocusedDescendants()`, through `execute_unreal_python_readonly`; 26 reads in that run, moving in both directions, on a `UEditableTextBox` and a `UButton`). Neither that report nor the reader's origin, `qa/TASK-1489-verify.md` §2.2, quotes the script: they name the calls and the object paths. A recipe's steps are exact and cite the sentence that measured them (format item 4), so a script written here would be unmeasured. Unparks when a run quotes its reader verbatim.
+- *2, the console re-establish* (`IA_AssistantConsole` ×2, 0.5 s apart, close then open; `True` 4 of 4). Its read-back is reader 1, and the report quotes neither the call envelope nor how the console was first opened.
+- *3, the detail-page re-establish* (`IA_MenuDown` on the one-stop `BackButton` ring; `True` 3 of 3). The report does not give the route to the Controls help detail page (no preconditions), and its read-back is reader 1.
+- Candidate 4 is law, not a recipe (`VER-§12` cl. 7f).
```

## QA loop 1 — 2026-09-27, on `qa/TASK-1549.md` (FAIL: 1 BLOCKER · 0 WARN · 2 NIT)

- **Start state = QA's §6 anchors, re-measured `rb` + `hashlib` before any edit:** agent `9b4976d6…347723` · menu `aef9083b…fc436c` · set-active `6167149c…0965fd` · slot `f72c9530…b2e2d7` · README `89347e54…d56d7bd7` · this handoff `f9ecdf2c…4e8c3`. All matched. Copies of the four files this loop could touch were saved to my scratchpad first, so the loop-1 delta below is a byte diff against QA's reviewed bytes, not against `HEAD`.
- **No `playtest-verifier` live** (dispatch). **Read-only git only:** `git --no-optional-locks diff --no-index` between the scratch copies and the working tree (no index read or written). No `CONVENTIONS.md`, `CLAUDE.md`, `Source/` or `Content/` byte. Slot, README, vsbot and play were not touched in this loop (hashes below = QA's §6).

### B1 — fixed: `Tools/Verify/recipes/RCP-menu-to-deckbuilder.md` `:128` and `:130`, plus the census

**(a) `:128`, appended dated marking (same shape as slot `:166`).** The pre-existing bullet is unchanged and is the byte prefix of the line. It still ends "`[M: archer50 §1(b)]`; law `VER-§5` cl. 5. Use `inject_input_action`." The appended text marks the "focus did not move" leg **UNSUPPORTED, not refuted**. Each sentence traces:
- The leg rests on one reading: archer50's `Tab` row, "no `SlotButton` `focused`" (t=85.11), `qa/PLAYTEST-archer50-verify.md:35`. A grep of archer50 for `focus` / "did not move" hits only `:27` (the §1(a) menu reads) and `:35`.
- It came after the `ui_perform` `click` rows at t=43.63 and t=57.13 (`:33`–`:34`) with no focus-setting input between: `qa/TASK-1493-verify.md:55` (*Blast radius* item 3).
- `click` is outside cl. 7f's measured shapes, so whether those calls cleared focus is `NOT MEASURED`. The clause is not cited for `click`.
- The manager's "corroboration only" ruling: the `TASK-1539` status line, marker `UI-PERFORM-FOCUS-APPENDS-2026-09-27`.
- `FocusedCardIndex -1` is the widget's own index, not Slate focus: `DeckBuilderWidget.h:907`–`:908`, `UPROPERTY(Transient) int32 FocusedCardIndex = INDEX_NONE;` `[D]`. That is QA ruling 3's scoping, carried over.
- The hazard itself still rests on the tool's reply (quoted in the bullet) and on `VER-§5` cl. 5 (`CONVENTIONS.md:12270`: `simulate_key_press` "is delivered to the player controller and not to Slate").
- A cross-pointer to the slot recipe's `Tab` bullet, so the two recipes now agree on this one archer50 read.

**(b) `:130`, my own `TASK-1548` sentence corrected.** It was false; no pre-existing sentence is involved, and the commit-F text of `:130` is still the line's byte prefix.
- Before: "No record in this recipe rests on such a read: every focus read it cites came before any `ui_perform` in its run (`archer50`'s first `ui_perform` row is §1(b), t=43.63; `1511`, `1512`, `1524` and `1519` record none)."
- After: "One record in this recipe rests on such a read: the `simulate_key_press` hazard above, whose "focus did not move" leg (`archer50` §1(b), t=85.11, after that run's `ui_perform` `click`s) is marked there. Every other focus read this recipe cites came before any `ui_perform` in its run: `archer50`'s are §1(a)'s (t=4.84, t=17.38), before its first `ui_perform` row (§1(b), t=43.63), and `1511`, `1512`, `1524` and `1519` record none."
- "Every other" was re-checked against every focus site in the recipe (a `focus` grep): Precondition 4 and Steps 0–2 cite archer50 §1(a) and the four runs; Read-back rows 0 and 1–2 likewise; `:123`, `:129`, `:131`, `:132` are advice or cite `1524`'s `Button_1`; `:128` is the exception, now marked. QA's grep, `qa/TASK-1549.md` §0.5, found `ui_perform` in `1511`/`1512`/`1519`/`1524` at 0/0/1/0, and `1519`'s one hit is not a call.

**(c) The census.** §5's menu rows missed this site. **Cause:** loop 0 read the menu recipe's Steps, Read-back and run records for (2), but read *Known hazards* only for chained exposure, not for cited records. It found the slot twin only because `TASK-1493` item 3 pointed at archer50 §1(b), and I did not grep the other recipes for that same read. Loop 1 did that grep over all six recipes and the README: `85\.11|did not move|§1(b)|Tab`, then `focus` crossed with the runs known to contain `ui_perform` calls (archer50, `1493`, `1489`, `1491`, `1402`, `1436`). The read is cited as a record at exactly two sites: slot `:166` (marked in loop 0) and menu `:128` (marked now). README `:91` cites archer50's `Tab` for `binding_found` only. There is no third site. The rows below **add to and supersede** §5's menu rows and its count:

| recipe | step | hit y/n | change |
|---|---|---|---|
| `RCP-menu-to-deckbuilder.md` | Known hazards, the `simulate_key_press` bullet (`:128`): "focus did not move" `[M: archer50 §1(b)]` = the `Tab` row's "no `SlotButton` `focused`" (t=85.11), after `ui_perform` `click`s at t=43.63 / 57.13, no focus-setting input between | **y (a cited record rests on such a read)** | appended: that leg marked UNSUPPORTED, not refuted; `FocusedCardIndex -1` named as not Slate focus; the hazard's remaining support named |
| same | the "no record rests on such a read" sentence (`:130`, mine) | was false | corrected to name the one exception (B1 (b)) |

**Hit count: 5** (slot 2: the chained fence and the `Tab` record · **menu 2: the chained fence and the `Tab` record** · set-active 1, chained). **Recipes with a hit: 3 of 5.** Zero: vsbot, play, README.

**§4 addendum: pre-existing sentences changed.** The count is now **four** bullets extended by appended sentences, none reworded. The fourth is `RCP-menu-to-deckbuilder.md`, Known hazards, the bullet that begins "**`simulate_key_press` is not a menu route on this map.**" Before (unchanged, still the line's prefix): `… law \`VER-§5\` cl. 5. Use \`inject_input_action\`.` After: the same, then "⚠️ **Added 2026-09-27 (`TASK-1548`, QA loop 1): the "focus did not move" leg is UNSUPPORTED, not refuted.** …". Three more edits touched only my own loop-0 text and no pre-existing sentence: menu `:130` (B1 (b)), set-active `:44` (N2) and agent `:164` (N1).

### N1 — taken: `.claude/agents/playtest-verifier.md:164`
- "Its other steps" → "`ui_perform`'s other steps". This was my own loop-0 line. One line changed, with no rewrap, so the line is now 11 characters longer.
- CRLF is kept: CR 238 / LF 238, zero bare LF, asserted in the edit script.
- **Frontmatter re-proved after the edit:** `head -7` raw `653fc50c2a29d071cd171daf0c2625e8b09ec58d0ce8591dc174e47faaa4997e` (4326 B), CR-stripped `5c2b409f16987def46a9acd142c60f1c928e37a649395c401e931f11a4660749`, line 7 = `---\r\n`. That equals the dispatch's and QA's anchors, and the script asserted `head -7` before = after.
- **New whole-file sha256:** `dc97fe9a7852f989609a166dc7db629d942373fec4cc6875fe44fc31599ea6ce`.

### N2 — taken: `Tools/Verify/recipes/RCP-deckbuilder-set-active-by-keyboard.md:44`
- My own loop-0 clause. It now states the direct fact: "`1519` records no `ui_perform` call: its one mention (*Not examined*) says its `ui_perform` right-button shapes were not re-traced, and its pointer input in the builder was `SetMouseLocation` (a cursor move, no click), "after control a1, never before the keyboard walk". `[M: 1519 Not examined]`"
- Sources: `qa/TASK-1519-verify.md:49` (the only `ui_perform` hit in the file, by grep) and `:50` (quoted). Both sit under its *Not examined / limitations this run* heading (`:46`).

### Proof that nothing else moved (`rb` + `hashlib`)
- **Line-level delta against QA's reviewed bytes:** menu lines [128, 130] · set-active [44] · agent [164]. Line counts are unchanged (132 / 204 / 238).
- **Reverse derivation to commit F (`qa/TASK-1535.md` §5 anchors):**
  - Menu: cutting `:128` and `:130` at ` ⚠️ **Added 2026-09-27 (\`TASK-1548\`` gives `d814ea2c2624f38ed53d9a28536c8621f350cc3ab605bb8f9af95c2e08bebfa8`.
  - Set-active: cutting `:44` gives `201409670a0da22730a64edb8e551bb7e52785dc91fe4943e6a07d6ca57d3a47`.
  - Agent: dropping lines 156–166 gives `e6dcf6dc646e1240db6a53a5ee2ed0be20a057f68527023d0d72cd98337536fd`.
  - All three equal the anchors, so every byte outside `TASK-1548`'s own appends and inserts is commit F's.

### Loop-1 diff (`git --no-optional-locks diff --no-index -U0`, loop-0 bytes → now; CR stripped for display)

```diff
--- a/Tools/Verify/recipes/RCP-menu-to-deckbuilder.md (loop-0 bytes, `qa/TASK-1549.md` §6 anchor)
+++ b/Tools/Verify/recipes/RCP-menu-to-deckbuilder.md (now)
@@ -128 +128 @@ Layout-dependent (not coordinate-dependent): the ×2 depends on the menu's butto
-- **`simulate_key_press` is not a menu route on this map.** It is delivered to the player controller, not to Slate. Source: `simulate_key_press "Tab"` returned *"Key Tab was delivered to the player controller, but NOTHING BINDS IT: applied mapping context(s) IMC_MainMenu…"* and focus did not move. `[M: archer50 §1(b)]`; law `VER-§5` cl. 5. Use `inject_input_action`.
+- **`simulate_key_press` is not a menu route on this map.** It is delivered to the player controller, not to Slate. Source: `simulate_key_press "Tab"` returned *"Key Tab was delivered to the player controller, but NOTHING BINDS IT: applied mapping context(s) IMC_MainMenu…"* and focus did not move. `[M: archer50 §1(b)]`; law `VER-§5` cl. 5. Use `inject_input_action`. ⚠️ **Added 2026-09-27 (`TASK-1548`, QA loop 1): the "focus did not move" leg is UNSUPPORTED, not refuted.** It rests on one reading, `archer50` §1(b)'s `Tab` row: "no `SlotButton` `focused`" (t=85.11), taken in the builder after the §1(b) `ui_perform` `click` rows (t=43.63, t=57.13) with no focus-setting input in between (`.claude/pipeline/qa/TASK-1493-verify.md` *Blast radius* item 3). `click` is outside `VER-§12` cl. 7f's measured shapes (marker `VER-12-7F-UI-PERFORM-CLEARS-FOCUS`), so whether those calls cleared the focus is `NOT MEASURED`. The manager ruled that read corroboration only (`TASK-1539` status line, marker `UI-PERFORM-FOCUS-APPENDS-2026-09-27`). The same row's `FocusedCardIndex -1` is the widget's own index (a `UPROPERTY(Transient) int32` on `UDeckBuilderWidget`), not a Slate focus reading `[D]`. The same read is marked in `RCP-deckbuilder-slot-and-card-edit.md` (Known hazards, the `Tab` bullet). The hazard itself still rests on the tool's reply, quoted above, and on `VER-§5` cl. 5.
@@ -130 +130 @@ Layout-dependent (not coordinate-dependent): the ×2 depends on the menu's butto
-- **Two `WBP_MainMenu` instances after returning from the builder.** After Exit, `ui_snapshot "WBP_MainMenu"` returned *"matched 2 widgets: WBP_MainMenu_C, WBP_MainMenu_C"* (the Back handler creates a new menu instance). Observed, not diagnosed. `[M: archer50 Not-examined]` If you re-enter the builder from a returned menu, disambiguate (`selector` with `match_index`) and re-read focus first; that path is `NOT MEASURED`. ⚠️ **Added 2026-09-27 (`TASK-1548`): the return is itself a `ui_perform` `double_click`** (`RCP-deckbuilder-slot-and-card-edit.md` Step 6). `VER-§12` cl. 7f (marker `VER-12-7F-UI-PERFORM-CLEARS-FOCUS`) voids a Slate focus reading taken after a `ui_perform` with no focus-setting input in between. `double_click` is outside its measured shapes, so whether that call clears focus is `NOT MEASURED`, and the clause may not be cited for it. ⇒ Before that re-read, send a focus-setting input (cl. 7f names an injected `IA_MenuDown`), then read; after a `double_click` that is caution, not the clause. Where it lands on a returned menu is `NOT MEASURED` (Fences: any starting focus other than `Button_0`). No record in this recipe rests on such a read: every focus read it cites came before any `ui_perform` in its run (`archer50`'s first `ui_perform` row is §1(b), t=43.63; `1511`, `1512`, `1524` and `1519` record none).
+- **Two `WBP_MainMenu` instances after returning from the builder.** After Exit, `ui_snapshot "WBP_MainMenu"` returned *"matched 2 widgets: WBP_MainMenu_C, WBP_MainMenu_C"* (the Back handler creates a new menu instance). Observed, not diagnosed. `[M: archer50 Not-examined]` If you re-enter the builder from a returned menu, disambiguate (`selector` with `match_index`) and re-read focus first; that path is `NOT MEASURED`. ⚠️ **Added 2026-09-27 (`TASK-1548`): the return is itself a `ui_perform` `double_click`** (`RCP-deckbuilder-slot-and-card-edit.md` Step 6). `VER-§12` cl. 7f (marker `VER-12-7F-UI-PERFORM-CLEARS-FOCUS`) voids a Slate focus reading taken after a `ui_perform` with no focus-setting input in between. `double_click` is outside its measured shapes, so whether that call clears focus is `NOT MEASURED`, and the clause may not be cited for it. ⇒ Before that re-read, send a focus-setting input (cl. 7f names an injected `IA_MenuDown`), then read; after a `double_click` that is caution, not the clause. Where it lands on a returned menu is `NOT MEASURED` (Fences: any starting focus other than `Button_0`). One record in this recipe rests on such a read: the `simulate_key_press` hazard above, whose "focus did not move" leg (`archer50` §1(b), t=85.11, after that run's `ui_perform` `click`s) is marked there. Every other focus read this recipe cites came before any `ui_perform` in its run: `archer50`'s are §1(a)'s (t=4.84, t=17.38), before its first `ui_perform` row (§1(b), t=43.63), and `1511`, `1512`, `1524` and `1519` record none.
--- a/Tools/Verify/recipes/RCP-deckbuilder-set-active-by-keyboard.md (loop-0 bytes, `qa/TASK-1549.md` §6 anchor)
+++ b/Tools/Verify/recipes/RCP-deckbuilder-set-active-by-keyboard.md (now)
@@ -44 +44 @@ Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are d
-5. **No mouse input inside the builder before the walk.** `[L: VER-§8 cl. 12, 2026-09-26 amendment]` The reason is a code read: after a mouse click puts focus on a bar slot while `FocusedCardIndex` is still armed, injected `Home`/`Left`/`Right` act on the grid while a real key acts on the bar (`1509` N4). `1511` sent no mouse input at all. `[M: 1511 Not examined]` ⚠️ **Added 2026-09-27 (`TASK-1548`): nor any `ui_perform` call before the walk,** including one with no pointer step. `VER-§12` cl. 7f (marker `VER-12-7F-UI-PERFORM-CLEARS-FOCUS`) measured a `ui_perform` clearing Slate keyboard focus 8 of 8 for `type`, `scroll`, `move` and a 1-frame `wait` (its other shapes are not measured), and Steps 1–3 move and read focus: after one, the walk may start from a focus no read established. No record in this recipe rests on such a read: `1511` and `1524` record no `ui_perform`, and `1519` records its `ui_perform` right-button shapes as not re-traced.
+5. **No mouse input inside the builder before the walk.** `[L: VER-§8 cl. 12, 2026-09-26 amendment]` The reason is a code read: after a mouse click puts focus on a bar slot while `FocusedCardIndex` is still armed, injected `Home`/`Left`/`Right` act on the grid while a real key acts on the bar (`1509` N4). `1511` sent no mouse input at all. `[M: 1511 Not examined]` ⚠️ **Added 2026-09-27 (`TASK-1548`): nor any `ui_perform` call before the walk,** including one with no pointer step. `VER-§12` cl. 7f (marker `VER-12-7F-UI-PERFORM-CLEARS-FOCUS`) measured a `ui_perform` clearing Slate keyboard focus 8 of 8 for `type`, `scroll`, `move` and a 1-frame `wait` (its other shapes are not measured), and Steps 1–3 move and read focus: after one, the walk may start from a focus no read established. No record in this recipe rests on such a read: `1511` and `1524` record no `ui_perform`, and `1519` records no `ui_perform` call: its one mention (*Not examined*) says its `ui_perform` right-button shapes were not re-traced, and its pointer input in the builder was `SetMouseLocation` (a cursor move, no click), "after control a1, never before the keyboard walk". `[M: 1519 Not examined]`
--- a/.claude/agents/playtest-verifier.md (loop-0 bytes, `qa/TASK-1549.md` §6 anchor)
+++ b/.claude/agents/playtest-verifier.md (now)
@@ -164 +164 @@ S3 before you plan (step 3); apply S1 and S2 to every input you send.
-  clause). Its other steps (`click`, `double_click`, `press`/`release`, `drag`,
+  clause). `ui_perform`'s other steps (`click`, `double_click`, `press`/`release`, `drag`,
```

### Final bytes, loop 1 (supersedes §7; read `rb` + `hashlib` after my last recipe/agent edit; blob = `sha1("blob <n>\0" + LF bytes)`, computed)

| file | sha256 | bytes | CR / LF | git blob (computed) |
|---|---|---|---|---|
| `.claude/agents/playtest-verifier.md` | `dc97fe9a7852f989609a166dc7db629d942373fec4cc6875fe44fc31599ea6ce` (LF-normalised `a1446ab1244aa526058038d635085b29bd946cac77ff084e77daf9ecc3319d1f`, 21548 B) | 21786 | 238 / 238 | `38df8687bc46dad4070094f71109945824d1c767` |
| ↳ `head -7` raw / CR-stripped | `653fc50c…a4997e` / `5c2b409f…4660749` (unchanged) | 4326 | line 7 `---\r` | — |
| `Tools/Verify/recipes/RCP-menu-to-deckbuilder.md` | `54c7e067b17659727d9cc0550e6a41587d3449178ef8acef6fa01ede5837882d` | 21868 | 0 / 132 | `f3f5178bc8c628bd50dee6a5e365790779597d0a` |
| `Tools/Verify/recipes/RCP-deckbuilder-set-active-by-keyboard.md` | `c1965e7bcd4eaaa38abdd76fd209c3e4d56c90a0400d394dd3d9b23ee75567dc` | 37412 | 0 / 204 | `ac42a79a09ae174c6f0c8be423ee108e728929dc` |
| `Tools/Verify/recipes/RCP-deckbuilder-slot-and-card-edit.md` (not touched in loop 1) | `f72c9530c0633fb002ec7231cdda8e76cde20831addaedeee88cadde60b2e2d7` | 22547 | 0 / 170 | `c985258390…` |
| `Tools/Verify/recipes/README.md` (not touched in loop 1) | `89347e54082573dd4b5ea84ed18027d4ecee0cf1d275a008e2ded3e2a56d7bd7` | 17367 | 0 / 105 | `a99b58f972…` |
| `Tools/Verify/recipes/RCP-vsbot-capture-center-and-summon.md` (untouched) | `e1ae86a6acb7cf69843ed60165f47cbb571a86eb919cbb8071274f5f638d8f13` | 32433 | 0 / 248 | `a1f647de06…` |
| `Tools/Verify/recipes/RCP-play-unit-card-from-hand.md` (untouched) | `3eb2d019c372f1d786967e7ba8ad03b15929bae1d5982bf77d56aa8fe44f425f` | 49130 | 0 / 298 | `9b31c76a2b…` |

This handoff cannot carry its own final hash. It is reported in the reply to the orchestrator. Also touched: `TASKBOARD.md`, this row's `status:` line only (→ `ready-for-qa`).

### For QA to scrutinise (loop 1)
1. **The `:128` marking's last sentence** ("The hazard itself still rests on the tool's reply … and on `VER-§5` cl. 5"). I kept it to the cited reply and the clause, and did not characterise cl. 5's own sources. The next bullet (`:129`) says `binding_found` is not the observable, so I did not name `binding_found:false` as support.
2. **"Every other focus read this recipe cites"** (`:130`) is a universal claim. The site list I checked it against is under B1 (b).
3. **N1 changes the agent file.** It now differs from the `qa/TASK-1549.md` §6 anchor, and the re-gate needs the frontmatter re-proof above, not a hash-equality check.
