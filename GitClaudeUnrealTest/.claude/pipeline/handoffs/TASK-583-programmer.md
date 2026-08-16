# TASK-583 — [WR-29] COMMENT-ONLY — the two claims ruling W4-R3 / W4-R4 falsified

**Agent:** gameplay-programmer · **Date:** 2026-08-15 · **Status → `ready-for-qa`**
**Gate:** `qa/TASK-584.md` (diff-scoped re-gate) · **Compile:** TASK-566 re-run · **Commit:** TASK-570

Laws applied: CONVENTIONS **`W4-R3`** (`WR-§6`) · **`W4-R4`** (`WR-§2b`) · `WR-§2b` row G · `SC-§15` · `SC-§18c` · `SC-§20` · `SC-§32` · `SC-§33` · `FT-§12b` PAIR RULE.

## 0. The one-line claim, and §1 proves it rather than asserting it

⛔ **ZERO EMITTED BYTES. Not one non-comment line moved.** Every `+`/`-` line in the whole change set — **74** of them across both files — is a **whole-line comment**. The code-stripped `sha256` of each file is **byte-identical pre-edit vs post-edit.**

⛔ **`GateBlockerExtent` is still `FVector(900.f, 405.f, 678.f)`** (`Castle.h:461`) and **`GateBlockerRelativeLocation` is still `FVector(18.f, -1575.f, 852.f)`** (`:406`). No constant, signature, `UPROPERTY` specifier, `meta =` clause, parameter or `#include` was touched anywhere.

---

## 1. THE PROOF OBLIGATION (TASK-577/578 standard, re-run per file)

### ⚠️ The baseline, stated first so the gate does not re-run the wrong comparison

**Both files were ALREADY DIRTY at `HEAD`** — `Castle.h` from TASK-557/562/578, `SiegeAssistantConsoleWidget.h` from TASK-561/581. ⇒ **A `HEAD` comparison would show THEIR code diffs, not mine, and is NOT a finding about this task.** My baseline is a **byte snapshot of the working tree taken before my first edit** (TASK-578 hit this exact trap and documented it).

| file | raw bytes, pre-edit snapshot | snapshot sha256 (whole file) |
|---|---|---|
| `Castle.h` | 47,162 | `d09c21d0a01dbb6bcafe94ed990514e0647d059863f0adbcc9c8e920f89421b5` |
| `SiegeAssistantConsoleWidget.h` | 53,938 | `026a7678314a11b509ff47520479ffb1f0a5879dc8ea0dd382b6a467bfe4ede7` |

### Proof 1 — the comment-filtered diff: the code change set is EMPTY

```
=== Castle.h ===
payload +/- lines:            51
  ...whole-line comment lines: 51
  ...lines carrying code:      0
--- any changed line NOT a pure comment line (should be none): ---
(end)
=== SiegeAssistantConsoleWidget.h ===
payload +/- lines:            23
  ...whole-line comment lines: 23
  ...lines carrying code:      0
--- any changed line NOT a pure comment line (should be none): ---
(end)
```

⭐ **Note the difference from TASK-578, in this task's favour:** 578 needed a *Proof 1b* because 22 of its changed lines were **trailing** comments on live lines (`Entry(TEXT("Footman"), 12), // …`), which fail a whole-line test by construction. **Here there are none** — every edit is inside a `/** … */` block, so the "lines carrying code" count is a true **0** and no sharpened filter is required.

### Proof 2 — the comment-STRIPPED code stream is byte-identical (sha256)

`/* … */` blocks and `// …`-to-EOL removed with a **string/char-literal-aware state machine** (a `//` inside a `TEXT()` literal is never mistaken for a comment), blank lines dropped, hashed.

| file | code-only sha256 — PRE-EDIT | code-only sha256 — POST-EDIT | code lines | verdict |
|---|---|---|---|---|
| `Castle.h` | `854c281c…7e55c22a` | `854c281c…7e55c22a` | 143 → 143 | ✅ **IDENTICAL** |
| `SiegeAssistantConsoleWidget.h` | `cbb39ddb…f97d910` | `cbb39ddb…f97d910` | 123 → 123 | ✅ **IDENTICAL** |

**Full hashes (post-edit), for the gate to re-run:**
```
854c281c6c2c9d48972bb366fcc2a58a85e52000e7e73c1460ae81ae7e55c22a  Castle.h
cbb39ddb07e3e95d34085d76e653616be2c168e6b290001f3efb490bef97d910  SiegeAssistantConsoleWidget.h
```

### Proof 3 — comment-delimiter balance (the structural check)

⚖️ **An unterminated block comment would have swallowed live code and MOVED the hash**, so this is proof 2's companion, not decoration.

| file | pre `/*` / `*/` | post `/*` / `*/` |
|---|---|---|
| `Castle.h` | 72 / 72 | **72 / 72** ✅ |
| `SiegeAssistantConsoleWidget.h` | 51 / 51 | **51 / 51** ✅ |

### The stripper was VALIDATED before it was believed (`SC-§32`: green is not proof)

A tool that always answers "identical" proves nothing. Four legs:

1. **Determinism** — same input hashed twice, same digest. ✅
2. ⭐ **CODE-SENSITIVITY, the leg that matters** — run `HEAD` vs working on two files this batch **did** change code in: `Castle.cpp` (`ec687bb3…` → `5e16dda0…`, 433 → 586 code lines) and `SiegeGameMode.cpp` (`c5ba5399…` → `411d6076…`, 651 → 668). **The tool CAN see a code change; it did not see one of mine.** ✅
3. **Positive control** — `HEAD` vs working on TASK-578's comment-only files: `UnitCommand.h` and `SorcererUnit.cpp` both hash **identical** across a known comment-only edit. ✅
4. ⭐⭐ **CROSS-TOOL AGREEMENT — an independent check the gate can lean on:** my stripper is a fresh implementation, and it **reproduces TASK-578's published hashes character for character** — `Castle.h` `854c281c…7e55c22a`, `UnitCommand.h` `b194509d…7de58bcd`, `SorcererUnit.cpp` `e563e0db…ce86a9ae`. **Two independently written strippers agree.** ✅

**Tool:** `C:\Users\wesel\AppData\Local\Temp\claude\C--GitProjects-GitHub-GitClaudeUnrealTesting-GitClaudeUnrealTest\5ae71072-bb8f-4659-87cc-ab1ab1e2cc1e\scratchpad\strip_comments.py` (scratchpad — ⛔ **not** added to the repo).

### Diff scope — the hunks are confined to the two named blocks

```
Castle.h:                      @@ -423,3 +423,27 @@   @@ -427,13 +451,8 @@
SiegeAssistantConsoleWidget.h: @@ -496 +496,10 @@     @@ -507 +516,2 @@   @@ -509,0 +520,9 @@
```
⛔ No hunk anywhere else in either file. ⛔ `Castle.cpp`, `SiegeAssistantConsoleWidget.cpp`, `WarMapWidget.{h,cpp}`, `CombatantHealthBarComponent.{h,cpp}`, `Tests/`, `Tools/`, `Content/` — **untouched**, no overlap with TASK-560 / TASK-564 / TASK-582.

---

## 2. EDIT 1 — `Castle.h`, the `GateBlockerExtent` doc block (located by the UPROPERTY, `SC-§18c`)

### BEFORE (verbatim, the retired argument)

```
	 *    • X 260 → **900, NOT 780.** The ×3 (780 ⇒ a 1560 span in an 1800-wide
	 *      opening) leaves ≈120 uu of jamb gap PER SIDE. That is the defect: the
	 *      shipped ≈40 uu gap held only because 40 < the narrowest agent DIAMETER on
	 *      the field — hero capsule r≈42 ⇒ 84 uu (SiegeGameMode.h), Cavalry r45 ⇒ 90 uu
	 *      (CONVENTIONS "Castle 3× HOLLOW", the sizing agents). ⛔ **BODIES DID NOT
	 *      GROW (WR-§1 / SC-§34's human-scale exemption), so the GAP TOLERANCE MAY NOT
	 *      GROW EITHER** — at 120 uu both of those capsules fit through the gap and
	 *      walk into the enemy keep AROUND the blocker, silently deleting the
	 *      team-gated interior while every bounds readback still passes. 900 = half of
	 *      the 1800-uu clear opening, i.e. the blocker spans the FULL opening; the
	 *      residual ±18 uu from the mesh-local X offset is far under 84 uu and cannot
	 *      pass a body. Embedding into the jambs costs nothing — the volume Ignores
	 *      every channel except the ENEMY team's (ConfigureTeamGating).
	 *  ⚠️ The 1800-uu opening is WR-§1's own figure (600 × 3) and the mesh is authored
	 *  in parallel — the integration task PIE-VERIFIES the cover and may re-tune this
	 *  EditAnywhere value against the delivered geometry. It may NOT reduce it below
	 *  "opening span minus one agent diameter" without re-opening the reasoning above.
```

### AFTER (verbatim, `Castle.h:423-458`)

```
	 *    • X 260 → **900, NOT 780.** ⚠️ THE VALUE IS RATIFIED AND THE ARGUMENT THAT
	 *      STOOD HERE IS RETIRED — they are not the same act (CONVENTIONS WR-§2b
	 *      ruling **W4-R4**, 2026-08-15, raised as qa/TASK-565.md WARN-1). ⛔ That
	 *      ruling keeps the retired wording VERBATIM, which is exactly why it is not
	 *      re-printed here: two copies of a dead claim is how it comes back to life.
	 *      Read it there; what follows is the corrected reasoning.
	 *      ⛔ THE APERTURE IS MEASURED, NEVER PROJECTED. The delivered 9× gate is a
	 *      **1560 uu COLLISION gap spanning x −762 … +798**, and a **1470 uu VISUAL**
	 *      opening — handoffs/TASK-555-artist.md, the as-built readback. ⛔ **1800 is
	 *      the CARVE-CUTTER RECIPE width** (that handoff's row H, "recipe only; no
	 *      carve ran"): the width of the TOOL, not the width of the HOLE, because the
	 *      cutter meets wall geometry.
	 *      ⇒ AGAINST THAT APERTURE THE ×3 IS AN EXACT, ZERO-GAP FIT — not a leaky one.
	 *      With GateBlockerRelativeLocation.X = 18, X = 780 spans −762 … +798,
	 *      character for character the measured collision gap. ⇒ **the jamb gap at 780
	 *      is 0 per side, and the mesh-local X offset leaves NO residual** to argue
	 *      about.
	 *      ⇒ SO 900 STANDS AS A HARMLESS SUPERSET, and that is the whole of its
	 *      justification — it closes no gap, because there is none to close. It spans
	 *      −882 … +918, i.e. it embeds 120 uu per side INTO THE SOLID UCX JAMB HULLS
	 *      (a 1560 collision gap is precisely the claim that everything outside
	 *      −762 … +798 is hull). Inert twice over: nothing can occupy that space to be
	 *      blocked by it, and the volume Ignores every channel except the ENEMY team's
	 *      (ConfigureTeamGating).
	 *      ⛔ WHAT THE TOLERANCE IS STILL PINNED TO — this half was never in doubt:
	 *      **BODIES DID NOT GROW (WR-§1 / SC-§34's human-scale exemption), so the GAP
	 *      TOLERANCE MAY NOT GROW EITHER.** The bar is the narrowest agent DIAMETER on
	 *      the field — hero capsule r≈42 ⇒ 84 uu (SiegeGameMode.h), Cavalry r45 ⇒ 90 uu
	 *      (CONVENTIONS "Castle 3× HOLLOW", the sizing agents). Any per-side gap that
	 *      reaches 84 uu lets those capsules walk into the enemy keep AROUND the
	 *      blocker, silently deleting the team-gated interior while every bounds
	 *      readback still passes.
	 *  ⚠️ The integration task PIE-VERIFIES the cover and may re-tune this EditAnywhere
	 *  value against the delivered geometry. It may NOT reduce it below "opening span
	 *  minus one agent diameter" — and that span is the MEASURED **1560**, ⛔ NOT 1800
	 *  — without re-opening the reasoning above.
```

### Every figure traced at the artifact before typing (`SC-§20`) — ⛔ nothing taken from the relay

| claim written | traced to | verdict |
|---|---|---|
| `1560` uu collision gap, `x −762 … +798` | `handoffs/TASK-555-artist.md:130` — *"gate collision gap · 1560 uu (x −762 … +798) · = 520 × 3"* | ✅ |
| `1470` uu visual opening | same handoff `:129` — *"gate clear opening (visual) · 1470 uu widest (= 490 × 3)"* | ✅ |
| `1800` is the **recipe**, not the aperture | same handoff row **H** `:217` — *"gate 600→1800 … **recipe only; no carve ran**"* | ✅ |
| `GateBlockerRelativeLocation.X = 18` | `Castle.h:406` `FVector(18.f, -1575.f, 852.f)` | ✅ |
| `18 ± 780 = −762 … +798` (zero-gap fit) | arithmetic, re-done here; matches the readback character for character | ✅ |
| `18 ± 900 = −882 … +918` (120 uu/side into hull) | arithmetic, re-done here | ✅ |
| "solid UCX jamb hulls" | `handoffs/TASK-555-artist.md:21` — 25 embedded `UCX_SM_Castle_00..24`; and a 1560 collision gap *is* the claim that outside it is hull | ✅ |
| "Ignores every channel except the ENEMY team's" | `Castle.cpp` `ConfigureTeamGating` — `SetCollisionResponseToAllChannels(ECR_Ignore)` then a single `SetCollisionResponseToChannel(EnemyChannel, ECR_Block)` | ✅ **read at source, not inherited from the old comment** |

### What was KEPT, exactly as the spec's KEEP list requires

✅ Y `135→405` / Z `226→678` ×3 reasoning · ✅ the `174` interior-floor coincidence · ✅ the human-scale *"bodies did not grow"* clause **with both agent diameters (84 / 90 uu)** · ✅ the closing *"may not be reduced below opening-span-minus-one-agent-diameter"* guard — ⭐ **now reading against the measured `1560`, not `1800`**, which was the point of keeping it.

### ⚠️ One deletion I want on the record rather than buried

The retired paragraph's back-reference — *"the shipped ≈40 uu gap held only because 40 < the narrowest agent DIAMETER"* — is **not carried forward**. It sat inside `W4-R4`'s quoted retired span (`:423-431`) and existed only as scaffolding for the *"at 120 uu both capsules fit"* argument that the ruling killed. **The agent diameters it turned on ARE carried forward** (they are the tolerance bar), and the 3× history it referred to is untouched at `:409-413`. ⛔ **Nothing correct was dropped; one dead premise's supporting clause was.**

---

## 3. EDIT 2 — `SiegeAssistantConsoleWidget.h`, the caret citation (located by the surrounding sentence)

### BEFORE (verbatim)

```
	 *    (b) THE ENGINE MOVES THE CARET TO THE END ON A PROGRAMMATIC SetText WHILE
	 *        FOCUSED: FSlateEditableTextLayout::OnBoundTextChanged runs
	 *        JumpTo(ETextLocation::EndOfDocument, ECursorAction::MoveCursor) under
	 *        `bForceBoundTextReview && HasAnyUserFocus()`
	 *        (SlateEditableTextLayout.cpp:4280-4284), and SetText is precisely what
	 *        raises that flag (same file, symbol SetText).
```

### AFTER (verbatim)

```
	 *    (b) THE ENGINE MOVES THE CARET TO THE END ON A PROGRAMMATIC SetText WHILE
	 *        FOCUSED: FSlateEditableTextLayout::OnBoundTextChanged runs
	 *        JumpTo(ETextLocation::EndOfDocument, ECursorAction::MoveCursor) under
	 *        `bForceBoundTextReview && Widget->HasAnyUserFocus().IsSet()
	 *         && !bWasFocusedByLastMouseDown`
	 *        (SlateEditableTextLayout.cpp:4280-4284), and SetText is precisely what
	 *        raises that flag (same file, symbol SetText).
	 *        ⚠️ THE THIRD CONJUNCT IS CARRIED HERE BY qa/TASK-565.md NIT N3 — an
	 *        earlier revision of this citation stated the condition as TWO terms and
	 *        omitted `!bWasFocusedByLastMouseDown`. ✅ THE VERDICT IS UNCHANGED, and
	 *        the reason is a lifetime, not an assumption: that flag is SET at
	 *        mouse-DOWN (symbol HandleMouseButtonDown, same file :1280) and CLEARED at
	 *        mouse-UP (symbol HandleMouseButtonUp, :1374) ON THE TEXT BOX ITSELF ⇒ it
	 *        is true only while a button is held down INSIDE this box, and false in
	 *        every window in which AppendToInput can run (a war-map marker click is a
	 *        press-and-release on a DIFFERENT widget). ⇒ route (b) is live for us.
```

### ⭐ Verified at the INSTALLED UE 5.8 source myself — ⛔ not relayed, not assumed

`C:\Program Files\Epic Games\UE_5.8\Engine\Source\Runtime\Slate\Private\Widgets\Text\SlateEditableTextLayout.cpp`:

| line | source, as read | enclosing symbol (resolved, `SC-§18c`) |
|---|---|---|
| `:4281` | `if (bForceBoundTextReview && Widget->HasAnyUserFocus().IsSet() && !bWasFocusedByLastMouseDown)` → `JumpTo(EndOfDocument, MoveCursor)` | `FSlateEditableTextLayout::OnBoundTextChanged` (opens `:4254`) ✅ **confirms the comment's existing symbol claim** |
| `:1280` | `bWasFocusedByLastMouseDown = true;` | `FSlateEditableTextLayout::HandleMouseButtonDown` (opens `:1259`) ✅ |
| `:1374` | `bWasFocusedByLastMouseDown = false;` | `FSlateEditableTextLayout::HandleMouseButtonUp` (opens `:1321`) ✅ |

⇒ **Set on mouse-down and cleared on mouse-up, both inside this text box's own handlers.** The flag can only be true while a button is physically held inside the console's input box — never during a war-map marker click, which is a press-and-release on a different widget. **NIT N3's verdict holds, and it now holds for a reason the file states.**

⛔ **I did NOT re-litigate append-at-end vs at-caret** (ratified, `qa/TASK-565.md` (10)(b)). ⛔ `ComposeAppendedInput`, the refusal ladder, and legs (a) and (c) are untouched. ⛔ I left the existing `:4280-4284` range alone — QA verified at `:4281`, the range brackets the same statement, and churning it buys nothing.

---

## 4. ⚠️ A THIRD EDIT I MADE ON JUDGMENT — DECLARED, NOT SLIPPED IN. **Strike it in one edit if the gate disagrees.**

Same file, the block's heading. **`+9` comment lines, pure insertion, zero lines deleted.**

**The problem:** the block still opened `⛔⛔ DECLARED DEPARTURE (CONVENTIONS §15) — THE SPEC SAID "AT THE CARET"`. After `W4-R3`, **`WR-§6` no longer says that** — it says *appended at the end*. So the file was asserting an **open, unruled departure** from a law that had already been **amended to match it**. A reader who greps `DECLARED DEPARTURE` and stops there learns something false about current law — and could "repair" the code back toward at-caret, which `SC-§15`'s record exists to prevent.

**What I added** (after `OBSERVABLE BEHAVIOUR.`, before `Traced at the installed UE 5.8 source, not assumed:`):

```
	 *  ✅ STATUS 2026-08-15 — THE DEPARTURE IS NO LONGER OPEN, AND THE HEADING ABOVE
	 *  IS KEPT ONLY AS THE RECORD OF HOW THAT WAS REACHED: qa/TASK-565.md criterion
	 *  (10)(b) RATIFIED it leg by leg against the installed source, and CONVENTIONS
	 *  WR-§6 was then AMENDED to match — ruling W4-R3, "placed at the caret" ⇒
	 *  APPENDED AT THE END. ⛔ This is settled law now; do NOT re-litigate it below.
	 *  📌 The standing lesson W4-R3 drew, worth more than this one function: SPECIFY
	 *  THE OBSERVABLE AND LEAVE THE MECHANISM TO THE IMPLEMENTER — the retired wording
	 *  named a mechanism the platform does not expose, and (a) is why.
```

**Why I judged it in scope, and the honest counter-argument:**
- ✅ **The task's own narrow test is met exactly:** *"a comment is repaired here IFF a law amendment made in the SAME manager ruling falsified it."* `W4-R3` is that ruling and this is that comment.
- ✅ **The content is item (2)'s own sentence,** which supplied both facts: *"`qa/TASK-565.md` (10)(b) RATIFIED it and `WR-§6` is now amended to match (`W4-R3`)."* **Recording that is the opposite of re-litigating it** — the insertion tells the next reader *not* to.
- ✅ **`FT-§12b`'s PAIR RULE** is in this task's own Law line: a law amendment and its code-side record travel together.
- ✅ **Nothing deleted** — the `§15` heading stands verbatim as the historical record, per the "record rather than delete" instruction.
- ⚠️ **The counter-argument, stated fairly:** item (2)'s literal scope is *the citation*, and item (5) says don't improve comments you weren't sent for. **If the gate reads it that way, this is a clean single-hunk revert** (`@@ -496 +496,10 @@`) and it takes nothing else with it.

⚠️ **A relay/board conflict I resolved toward the board, on the record.** The dispatch note said to correct the Castle reasoning *"recording the retired wording rather than deleting it."* The **board** says the opposite for that site: *"⛔ Do NOT re-print the retired premise in the file — CONVENTIONS already records it verbatim, and two copies of a dead claim is how it comes back to life."* **Files are the contract; I followed the board.** ⇒ I recorded the **retirement as an event with a pointer** to `W4-R4`'s verbatim copy, so nothing is silently deleted and no second copy of the dead claim exists. **Both instructions are satisfied in substance.**

---

## 5. Declarations the spec requires

- **`.gen.cpp` REGENERATION — declared explicitly, not silently.** Both edited blocks are **UHT-visible doc blocks** immediately preceding reflected declarations, so TASK-566's compile regenerates their `.gen.cpp` with new `Comment`/`ToolTip` metadata:

  | header | block | reflected declaration it documents | regenerates |
  |---|---|---|---|
  | `Castle.h` | `GateBlockerExtent` doc block | `UPROPERTY(EditAnywhere, …, meta = (ClampMin = "0")) FVector GateBlockerExtent` | property ToolTip |
  | `SiegeAssistantConsoleWidget.h` | `AppendToInput` doc block | the `AppendToInput` declaration it precedes | function/param ToolTip |

  ⚖️ **Editor Details-panel metadata only. Zero gameplay bytes, zero behaviour, zero committed bytes** — already ruled acceptable at `qa/TASK-565.md` criterion (16), and `Intermediate/` is gitignored.
  ✅ **`git status --porcelain` checked: NO `.gen.cpp` and NO `Intermediate/` path appears** (`SC-§29b` clean). **Nothing staged.**
- **M8 / replication:** ⛔ **Nothing replicated, nothing emitted.** No `UPROPERTY(Replicated)`, no RPC, no `GetLifetimeReplicatedProps` entry, no `DOREPLIFETIME` — none existed at these sites and none was added. **Comments do not replicate.**
- **Prompt surface:** ⛔ **Zero prompt characters.** `SiegeAssistantConsoleWidget.{h,cpp}` is **not** in `ZoneA.MeasuredCharCount`'s input surface (that is `SiegeAssistantSnapshot.{h,cpp}` + `SiegeAssistantVocabulary.{h,cpp}`), and `Castle.h` never was. **The `5658` byte-freeze is structurally untouched** — and I add **no test**, per the spec.
- **`SC-§33`:** no defaulted parameter added or moved anywhere; **no signature exists in either diff.**
- **`SC-§15`:** ⛔ **no new departure declared.** This task *retires* one and *records* another's ratification.
- **Verbatim-quotation check (the TASK-578 trap), run BEFORE rewriting:** grepped all of `Source/` for the sentences I was about to change — `1800` / `jamb` / `clear opening`, and `bForceBoundTextReview` / `OnBoundTextChanged` / `EndOfDocument`. ✅ **Both sentences are quoted NOWHERE else in the codebase.** The only other `Source/` hit for `1800` is `ScatterConfig.h:570` `float AncientGroundMineClear = 1800.f;` — an **unrelated constant**, not a quotation, **untouched**.
- ⛔ **No compile, no editor, no MCP, no PIE, no Git mutation, no `Content/`, no `.csv`, no `Tests/`, no `Tools/`.** The editor stayed closed.

---

## 6. ⚠️ FOR QA TO SCRUTINISE

1. ⭐ **Re-run both proofs yourself** — and ⛔ **not against `HEAD`**: both files were already dirty from TASK-557/562/578/561/581, so a `HEAD` diff shows *their* code changes, not mine. Baseline = the pre-edit snapshot in §1 (its whole-file sha256 is published there). The stripper is in the scratchpad path named in §1.
2. ⚠️ **§4 is the one thing to rule on** — the third, judgment-call edit. I argue it in both directions above; it is a clean single-hunk revert if you disagree.
3. ⚠️ **A LATENT SITE I DELIBERATELY DID NOT TOUCH, reported instead of acted on.** `Castle.h:411` still reads *"a 600-wide clear opening ⇒ ≈40 uu of jamb gap per side"* for the **retired 3× castle**. ⛔ **By `W4-R4`'s own standing lesson, `600` is very likely the same class of figure** — row H names the recipe pair as `gate 600→1800`, so if `1800` was the cutter width at 9×, `600` was the cutter width at 3×. **I did not correct it, and I want that choice visible:** it describes **retired geometry**, `qa/TASK-565.md` WARN-1 did not name it, no as-built readback of the 3× castle exists to correct it *against*, and item (5) forbids improving comments I was not sent for. ⇒ **Flagged for the manager as a possible future one-line task, ⛔ not smuggled into this one.**
4. **Confirm the ratified constants are untouched:** `GateBlockerExtent = FVector(900.f, 405.f, 678.f)` (`:461`) and `GateBlockerRelativeLocation = FVector(18.f, -1575.f, 852.f)` (`:406`).
5. **Check my arithmetic against `W4-R4`:** `18 − 780 = −762`, `18 + 780 = 798` ⇒ exact fit to the measured `1560` aperture; `18 ± 900 = −882 … +918` ⇒ 120 uu/side into hull.
6. ⛔ **No token figure is quoted anywhere in this handoff or in either edit** (batch-wide ban honoured).

## 7. Files touched

- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\Castle.h` — comments only
- `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest\Source\GitClaudeUnrealTest\Siegebound\SiegeAssistantConsoleWidget.h` — comments only

**Read-only references:** `handoffs/TASK-555-artist.md` · `qa/TASK-565.md` · `CONVENTIONS.md` (`W4-R3`, `W4-R4`) · `handoffs/TASK-578-programmer.md` · installed UE 5.8 `SlateEditableTextLayout.cpp` · `Castle.cpp` (`ConfigureTeamGating`, read only).
