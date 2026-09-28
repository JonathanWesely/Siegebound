PASS
# QA Report: TASK-1481 re-gate, QA loop 1 (gate for TASK-1480, citation re-anchor, comment bytes only)

Verdict: **PASS**: 0 BLOCKER, 0 WARN, 0 NIT. Board: `TASK-1480` → `qa-passed` (loop 1 of 3 closes here).

Marker `TASK-1481-LOOP-1-PASSED`. Date 2026-09-27. Prior gate: `qa/TASK-1481.md` (FAIL: B1, W1, N1–N5). Programmer's answer: `handoffs/TASK-1480-programmer.md` `## QA loop 1` (L1.0–L1.6). `TASK-1480` was at `ready-for-qa` when I started.

**Why a separate file, not an amendment.** No `CONVENTIONS.md` clause says where a re-gate is written (grep: 0 hits), and the dispatch named this path, following the `qa/TASK-1502-loop1.md` precedent. `qa/TASK-1481.md` §4 carries the loop-0 byte anchors that the programmer's L1.0 cites as its start state, so a separate file keeps that report byte-stable.

**The two questions, answered again.**
1. **Is it really comment-only? YES, for the loop-1 delta and for the whole row.** I checked this independently and did not accept the handoff's word. My method: reverse application in two stages (the tree back to my loop-0 anchors, then back to `HEAD`), a third tokenizer, and eight negative controls (§3). All 8 files have identical code and identical literals against both baselines. Every changed line is pure comment. The `TASK-1394` literal lists are identical (145/145, 10/10). `TASK-1468`'s site is byte-untouched.
2. **Is the new text rot-proof and true? YES.** No loop-1 line adds a `file:line` number. The only colon-numbers in the delta are the pre-existing `:2240` / `:2787`, which are now marked "WAS" inline. Every loop-1 claim holds at source (§1). B1's Play clause is now confirmed at the asset by a whole-package name census, which is stronger than loop 0's strand read.

---

## §0: What I inspected, beyond reading text

`unreal_inspector` status was `editor_connected`. Everything below was read-only. No lifecycle call, no git, no shell, no compile, no PIE, and nothing written except this file and one board `status:` line.

- **2 read-only Python queries.** Files were opened `rb`, and only `hashlib` / `re` / `difflib` / `bisect` / `os` were used. Nothing was written, and no `subprocess` or `.git/` read was made.
  - Query 1: the sha256 of all 8 files; both stages of reverse application; the lexer and its controls; the `TASK-1468` site check; the `WBP_DeckBuilder.uasset` name census.
  - Query 2: the byte context of the package's single `SetIsEnabled` name.
- **1 `get_asset_meta` and 4 `get_asset_graph` reads of `/Game/UI/WBP_DeckBuilder`**: Functions/Events; `OnPlayClicked`, `PlayBtnClicked`, `RefreshAll`, `RefreshDetailsPanel`, `BuildSandboxButton`, `BuildDetailsPanel`, `SplitGrid`, `OnDeckModelChanged`, `OnDeckSlotCountChanged`, `OnClicked_Event`, `OnClicked_Event_17`.
- **`Grep`/`Read`** of every loop-1 site, plus the source chains they cite.

---

## §1: Prior findings, all resolved (each checked at the site, not in the handoff)

| Prior | Ruling | How checked |
|---|---|---|
| **B1** (`DeckBuilderWidget.h`, `SaveDeckAs` doc, reflected ToolTip) | **RESOLVED** | See the B1 detail below the table. |
| **B1, optional** ("Card details" `//` run) | **RESOLVED (taken)** | *"it no longer gates Play — … deck activation is still gated, see SetActiveDeck's doc"*. The referenced doc (`SetActiveDeck`'s `TASK-1270` paragraph, the same header) says exactly that. Not reflected (measured at loop 0, and the structure is unchanged). |
| **W1** (`SiegeControlsHelpWidget.cpp`, (o) note) | **RESOLVED** | See the W1 detail below the table. |
| **N1** (R-02 padded-row quote) | **RESOLVED** | Both fragments are verbatim in `handoffs/TASK-568-artist.md` (the row ` 6 Mouse2D           IA_Look            modifiers: [InputModifierNegate_2]`). `IA_Look` has exactly **1** hit in that file, so the fragment finds the row uniquely. The padding is declared. |
| **N2** (the (g) trip-wire's quote) | **RESOLVED** | *"is ANYTHING already focused?"* is verbatim in `FocusFirstNavStop`'s `TASK-1469 LIMB 1(b)` comment (*"this loop asks "is ANYTHING already focused?","*), directly above the `HasUserFocus \|\| HasUserFocusedDescendants` loop and `return FocusWidget(Stops[0]);`. |
| **N3** (`IsNavFocusStop` FENCE (c)) | **RESOLVED** | `:2240` / `:2787` now carry an inline *"(⛔ WAS — `SiegeControlsHelpWidget.cpp` line numbers, cited so until TASK-1480; the by-text anchors are in the (m) note below)"*. They are indeed stale today: the writes are `DetailScrollBox->SetIsFocusable(false);` in `ConstructDetailTree` and `RowScrollBox->SetIsFocusable(false);` in `ConstructHelpTree`. The following sentences are re-wrapped only, with the same words (diffed). |
| **N4** (subsystem class doc, reflected ToolTip) | **RESOLVED** | *"no `UButton` in `Source/` is opted out outside the test fixture"*. A census of `Source/` for `IsFocusable = false`, `SetIsFocusable(false)`, `InitIsFocusable(false)` and `.IsFocusable(false)` finds two `UButton` writes: the helper's `Button->IsFocusable = false;` (0 callers) and the fixture's `OptedOutButton->IsFocusable = false;` (a `ConstructWidget<UButton>`). The two other writes are the `UScrollBox` calls. The sentence is exact. |
| **N5** (`DeckBuilderWidget.cpp`, `RegisterAsMenuNavTarget` body) | **TAKEN. Ruled in-fence (§2)** | "six" → "seven", one word. `BindMenuNavActions`' table has 7 rows (`IA_MenuUp/Down/Left/Right/Accept/Back` + `IA_MenuSecondary`). |

**B1 detail.** The new text scopes the removed gate to Play and states the activation gate. I read every link of the chain at source:
- **Right-click:** `Entry->OnRightClicked.BindUObject(this, &UDeckBuilderWidget::SetActiveDeckBySlot)`.
- **Home / pad Y** (`HandleCardGridKey` row (3), `TASK-1507`): `if (IsDeckBarActivationKey(Key))` → `ResolveDeckBarActivationSlot(Key, FindFocusedDeckBarSlot())` → `SetActiveDeckBySlot(BarSlot)`. Here `IsDeckBarActivationKey` is `Key == EKeys::Home || Key == EKeys::Gamepad_FaceButton_Top`, and a key is claimed only while a bar slot holds focus. That is "while one holds focus" ✓.
- **The rest of the chain:** `SetActiveDeckBySlot` → `SetActiveDeck(USiegeDeckSaveGame::MakeFixedDeckName(SlotIndex))` → `if (!TryActivateSavedDeck(…))` → `"… — refused; the active deck stays '%s'."` + `OnDeckActivationRefused`. `TryActivateSavedDeck` → `if (!UDeckLibrary::IsDeckLegal(CardTable, *Found, OutRefusalReason))`.
- **The quoted ruling fragment** *"no UI surface hard-enforces the 50-card rule"* is a verbatim substring of `DECK-§4(c)`'s "Named consequence" (`CONVENTIONS.md` L7202), on one line.
- **"Postdates":** the ruling is dated 2026-08-27, and `TASK-1270` was boarded 2026-09-14.
- **"Match-side legality check":** it exists (`SiegePlayerController.cpp`, `UDeckLibrary::IsDeckLegal(CardTable, *ActiveDeck, LegalityReason)`), and the text attributes it to the ruling.
- **"Play no longer gates on 50 cards" is measured at the asset, and now more strongly than in loop 0:**
  - `OnPlayClicked` wires directly to `StartMatch`.
  - A byte census of `WBP_DeckBuilder.uasset` (758041 bytes, real, not an LFS pointer) finds `IsCurrentDeckLegal` **0**, `IsDeckLegal` 0, `SaveDeckAs` 0, `SetActiveDeck` 0 and `bIsEnabled` 0. A `CallFunction` node stores its member name in the package name map, so 0 means no node references it.
  - The package's single `SetIsEnabled` node is `In Is Enabled=false` on a **"Deck Builder (Coming Soon)"** button. It sits inside a main-menu construction chain (`OnClicked_Event` strand) whose head `SetVisibility` is **exec-unwired**, so it is dead code, and it is not the Play button either way.
- `Grep` for "no UI surface" in `Source/` returns only the quoted fragment.
- The header no longer contradicts itself: `SaveDeckAs`'s doc and `SetActiveDeck`'s `TASK-1270` doc now agree.

**W1 detail.** The note now says ONE trip-wire names the helper as the revert. Census of `ApplyButtonNotFocusable` in the file:
- :185 (the note itself), :195 (the definition), :208 (the mirror's doc).
- :2033, the `RowButton` wire inside `ConstructRowTree` (opens :1962). That is **1** mention.
- **0** mentions inside `ConstructDetailTree` (:2247 up to `ConstructHelpTree` at :2928).
- :3169, the `CloseButton` site's historical *"⛔ was: `ApplyButtonNotFocusable(CloseButton);`"*. That is a record, not a revert instruction, so "IT IS THE ONLY ONE THAT DOES" holds.
- :3754, the switcher block's (γ), under *"⛔ (γ) REJECTED"*, above `ApplyActiveView` (:3801). This is exactly the note's "only as a warning, in its rejected (γ)".

The quotes check out:
- Both `RowButton` quotes are verbatim, and each spans a line break, as the note itself declares.
- The `BackButton` quote *"THE TRIP-WIRE (`qa/TASK-1433.md` WARN-L1)"* is verbatim on one line (:2560).
- "TWO TRIP-WIRES" / "those wires" now return **0** in `Source/`.

**Other loop-1 lines re-read for truth:** none is false. Line numbers above are as of these bytes and informational only.

**The four "six IA_Menu\*" sites left unchanged (the programmer's L1.6 q3).** I agree with the programmer: none is a present-tense false count on its face.
- `BindMenuNavActions`' doc is qualified inside the same block (*"⭐ TASK-1507 — SEVEN, not six"*).
- The `TASK-1423` design run is history. Its "seventh" is Remove's Delete, not `IA_MenuSecondary`.
- The `#include "InputAction.h"` trailer is `TASK-1423:`-attributed.
- NativeConstruct's note (*"the six IA_Menu\* actions TASK-1408 authored"*) is true, since `TASK-1408` authored six.

---

## §2: Rulings

| Call | Ruling | Why |
|---|---|---|
| **N5 taken in loop 1** | **UPHELD: in-fence, comment-only, true** | The row's fence (5) reads *"only the files carrying (a)–(o)"*, and `names: WRITES` reads *"the COMMENT BLOCKS ONLY in the files carrying (a)–(o)"*. Both are file-scoped, and `DeckBuilderWidget.cpp` carries (j), (k) and (l). (h)'s *"Six → seven, nothing else"* bounds what (h) changes at its two sites. It does not forbid correcting the same count elsewhere, and it is the same census-widening I upheld for (d) and (j) at loop 0. N5 was a QA finding, and answering it inside the loop is the normal rule-4 path. Measured: `DeckBuilderWidget.cpp` is code- and literal-identical, its one `+`/`−` line pair is pure comment, it is a `.cpp` (not reflected), and CR=LF=3364 (the EOL is preserved). This agrees with the orchestrator's view. |
| B1 wording "per the same ruling" instead of "in the ruling's words" | **Correct** | The paraphrase is now attributed, not quoted. The one quoted fragment is verbatim and on one line. |
| W1 "both quotes span a line break" instead of re-quoting single-line fragments | **Acceptable** | It is declared at the quote, in the style of N1's "(the row pads its columns …)". The `BackButton` quote, which is the one a reader would grep to find the second wire, is single-line. |
| Read-only git in loop 1 (8 × `show HEAD:<path>`, `SC-§71a`) | **Allowed, not a finding** | It was declared in L1.0. |

---

## §3: The comment-only proof, re-applied to the loop-1 bytes

**Stage 1: reverse application of the L1.5 `-U0` hunks** (parsed straight from the handoff file, not re-typed; `\r\n` restored for the two CRLF files):
- 8 hunks, **+37 / −23** lines.
- Every `+` block equals the tree at its stated new-side lines, and every `−` block lands at its stated old-side lines after reversal (**0 mismatches** on each side, and 0 count mismatches).
- Result: **the reconstruction equals my `qa/TASK-1481.md` §4 anchors, 8/8.** So L1.5 is the complete loop-1 delta, and I reviewed it line by line.

**Stage 2: reverse application of the loop-0 §5 hunks on top.** 31 hunks, +169 / −29, 0 mismatches. The result reproduces the handoff §1 before-sha256 (`HEAD` `9a67a27`), **8/8**.

**Stage 3: a third tokenizer.** This is a regex tokenizer. It is not my loop-0 character state machine and not the programmer's `lexcmp2.py`.
- **How it works:**
  - It treats `//` comments with phase-2 splice continuation, `/* */`, raw strings with delimiters, strings and chars with `u8`/`u`/`U`/`L` prefixes, and pp-numbers with digit separators.
  - It flags `STRAY_CLOSE_IN_CODE`, `NESTED_OPEN_IN_BLOCK`, unterminated literals and blocks, and `SPLICE`.
  - It compares the code-line lists (comments → spaces, whitespace collapsed, empty lines dropped) and the ordered literal lists.
  - It checks per-line purity on both sides of an independent `difflib` line diff.
- **Result:**

| File | code lines L0 = L1 = HEAD | literals | L0→L1 `+`/`−`, impure | HEAD→L1 `+`/`−`, impure | flags |
|---|---|---|---|---|---|
| `SiegeControlsHelpWidget.cpp` | 2002 | 610 | +15/−11, 0 | +70/−10, 0 | none |
| `SiegeControlsHelpWidget.h` | 258 | 47 | +0/−0 | +3/−0, 0 | none |
| `SiegeMenuInputSubsystem.cpp` | 1073 | 145 | +4/−3, 0 | +20/−6, 0 | none |
| `SiegeMenuInputSubsystem.h` | 123 | 10 | +4/−2, 0 | +12/−2, 0 | none |
| `Tests/SiegeMenuInputTest.cpp` | 464 | 140 | +0/−0 | +9/−2, 0 | none |
| `DeckBuilderWidget.h` | 159 | 44 | +10/−4, 0 | +44/−9, 0 | none |
| `DeckBuilderWidget.cpp` | 1710 | 199 | +1/−1, 0 | +14/−4, 0 | none |
| `SiegePlayerController.cpp` | 3963 | 564 | +3/−2, 0 | +15/−0, 0 | none |

⇒ **The code streams and the ordered literal lists are identical, 8/8, against both baselines.** Every changed line is pure comment, and there are 0 hazard flags. The `difflib` L0→L1 totals (+37/−23) equal the parsed hunk totals. Both `difflib` columns equal the programmer's L1.2 table number for number. **Three independent tokenizers now agree on every code-line and literal count** (my loop-0 state machine, `lexcmp2.py` and this one). Hazard census over the 37 added lines: 0 end in `\`, 0 contain `??`, `TEXT(`, `/*` or `*/`, and 0 start with `#`.

**The specific guarantees the dispatch asked for:**
- **`TASK-1394` instrument strings:** the subsystem literal lists are identical L1 = L0 = HEAD (`.cpp` **145/145**, `.h` **10/10**).
- **`TASK-1468`'s site** (subsystem `.cpp` lines 780–880, which hold `HasVisibleSlateAncestry` and the *"UNMOVED"* claim) is identical between L1 and L0, and appears verbatim in HEAD. **Byte-untouched by the whole row.**
- **`SiegeControlsHelpWidget.cpp`: no non-comment change**, against L0 and against HEAD (2002 = 2002 code lines, 610 = 610 literals, 0 impure). It is safe for `TASK-1541` to edit next.

**Negative controls.** Each is an in-memory copy of the loop-1 tree, never written.

| Control | code same | literals same | impure | flags |
|---|---|---|---|---|
| C0 unmodified | True | True | — | none |
| C1 `\` appended to the (g) trip-wire's last line (the next line is code) | **False** | True | — | **SPLICE** (in comment) |
| C2 one char inside the subsystem `.cpp`'s first `TEXT("…")` | **False** | **False** | 2 | none |
| C3 ` */` after `MenuInput->RegisterMenuNavTarget(VictoryWidget);` | **False** | True | 2 | **STRAY_CLOSE_IN_CODE** |
| C4 B1 `SaveDeckAs` doc's closer deleted | **False** | **False** | 0 | **NESTED_OPEN_IN_BLOCK** |
| C5 `//` → `/` on N3's first new line | **False** | True | **1** | none |
| C6 nested `/*` inside the W1 doc | True | True | 0 | **NESTED_OPEN_IN_BLOCK** (benign in C++; the flag works) |
| C7 `Button)` → `Buttonx)` on `ApplyButtonNotFocusable`'s signature, directly under W1's doc | **False** | True | 2 | none |
| C8 CRLF → LF on the N5 line | True | True | 0 | none |

C1–C7 discriminate. **C8 is the honest limit:** the lexer is whitespace- and EOL-insensitive by design, so EOL integrity is carried by the sha256 and the CR/LF counts in §4, not by the lexer.

---

## §4: Measured after-sha256 (the loop-1 bytes = what `TASK-1538` compiles and `TASK-1540` commits)

`Source/GitClaudeUnrealTest/Siegebound/…`. All 8 equal the handoff L1.4 claim. No BOM, and all are valid UTF-8.

| File | sha256 (measured) | bytes | EOL | vs `qa/TASK-1481.md` §4 |
|---|---|---|---|---|
| `SiegeControlsHelpWidget.cpp` | `f407d0b4c1d1df474fe2c0303cbe2eed6cb9636fb33526ffc2afe308ade96137` | 273661 | LF (CR 0) | changed (W1, N1) |
| `SiegeControlsHelpWidget.h` | `cc16d2caade4dab2a4676724ba9a2596b1f455d115560afe35f259941012d174` | 80750 | LF | unchanged |
| `SiegeMenuInputSubsystem.cpp` | `53050739542a4eb8df72223839089c66bdb907f356ab15ea0dbe1aa17eb6aa63` | 121682 | LF | changed (N3) |
| `SiegeMenuInputSubsystem.h` | `5bba1a035c40a39574223fe98f88130bf8fca9ded0385e27ecb4f81d1b68f2dc` | 85647 | LF | changed (N4) |
| `Tests/SiegeMenuInputTest.cpp` | `5470c88605442b63bdbe60126d37dc33fd30b2a1e6fada7cb1661a451d85797d` | 53148 | LF | unchanged |
| `DeckBuilderWidget.h` | `950dc811bbebc4b345eaa50cebd5f814f47262da990215d1a581ca8cfb6b1385` | 85606 | CRLF 1423/1423 | changed (B1 + optional) |
| `DeckBuilderWidget.cpp` | `de406a0ccd1a0bc9b665372a339fd85e9a6ebf9180adf0e09a17d31cd938934e` | 165312 | CRLF 3364/3364 | changed (N5) |
| `SiegePlayerController.cpp` | `8f33a5de3c8e06fe39ef3a484756d93f40facd8c0cadaea81116d40078db7fb4` | 387770 | LF | changed (N2) |

`SiegePlayerController.h` is untouched: `840416b6f03bb436d90772caa5821c1455495917b8b8ed277c9119e543536d10`.

⛔ **This table supersedes `qa/TASK-1481.md` §4 for 5a and the host.**

## Notes for build-master (`TASK-1538` 5a, then host `TASK-1540`)

- **The same four reflected blocks as loop 0, and no new one.** Loop 1 re-touched two of them, `SaveDeckAs` and the `USiegeMenuInputSubsystem` class doc. The generated code must reflect all four:
  - in `DeckBuilderWidget.gen.cpp`, the `Comment`/`ToolTip` metadata of `IsCurrentDeckLegal`, `SaveDeckAs` and `OnDeckModelChanged` changes;
  - in `SiegeMenuInputSubsystem.gen.cpp`, the class `Comment`/`ToolTip` changes.
- **Assert the exec-symbol SET of both `.generated.h`, set-identical.** Never assert size or sha.
- `SiegeControlsHelpWidget.gen.cpp` should come out **byte-unchanged**: the header's only change, (n), is not reflected (measured at loop 0).
- Three widely included headers changed, so a broad recompile is expected and is not a signal.
- **No 5b.** Comments have no runtime criterion, and that is not an `UNOBSERVABLE`.
- Host: commit exactly these 8 files at these sha256, and verify the **commit** (`git show --stat HEAD`), not the index.

---

## §5: For the manager

1. **`DECK-§4(c)` disagrees with `DECK-§3` (carried from the programmer; `CONVENTIONS.md` is fenced from this row).**
   - `DECK-§4(c)`'s *"⚠️ Named consequence: no UI surface hard-enforces the 50-card rule anymore — `TotalText` "n/50" remains the feedback and the match-side legality check remains the enforcement"* (L7202, 2026-08-27) has been overtaken on **activation** since `TASK-1270`. `DECK-§3`'s `TASK-1507` bullet already says an illegal deck is refused.
   - This is exactly how B1's wording was imported.
   - Suggested: a dated `SC-§120`-style note on `DECK-§4(c)` scoping the consequence to Play.
2. **`OnDeckModelChanged`'s hedge is now measurable, and needs no action.** Its doc says *"whether the graph still reads it is not measured here"*. It is now measured: `IsCurrentDeckLegal` has **0** occurrences in `WBP_DeckBuilder.uasset`'s bytes. The hedge is honest as written, so nothing is owed. This is for any future row that wants the stronger claim.
3. **Inert dead code in `WBP_DeckBuilder`, not this row's.**
   - What is there: main-menu construction chains (strings *"WBP_MainMenu: root Overlay not found"*, *"Play (vs Bot)"*, *"Deck Builder (Coming Soon)"*, a `SetIsEnabled(false)`, a `QuitGame`), headed by an exec-unwired `SetVisibility`, plus an orphan `PlayBtnClicked` custom event.
   - The risk: a future census asking "who disables a button?" will hit that `SetIsEnabled` and could misread it as a Play gate.
   - Cleanup is optional. It is an asset edit, and belongs to an asset row.
4. **Carried from loop 0.** `TASK-1468`'s site still carries the false *"every `UButton` … UNMOVED"* claim, now contradicted about 80 lines lower by the (m) note. This row left it byte-untouched (verified in §3). `TASK-1468`'s `parallel-safe: NO vs any writer of SiegeMenuInputSubsystem.{cpp,h}` covers `TASK-1480` until `TASK-1540` commits.
5. **Board bookkeeping.** Per the dispatch, I flipped only `TASK-1480`'s `status:`. `TASK-1481`'s own row still shows the loop-0 verdict text. Append this loop-1 PASS there if you want the row to carry it.

---

## Not examined / limitations

- **The proof is lexical, not a compile.** Three tokenizers agree, but none is a compiler. `TASK-1538` is the independent instrument. The tokenizer does not model trigraph replacement (0 `??` in the delta) or macro expansion.
- **The lexer is blind to whitespace and EOL (C8).** EOL integrity rests on the sha256 and on CR = LF counts (`DeckBuilderWidget.h` 1423/1423, `.cpp` 3364/3364, all LF files CR 0).
- **"HEAD = the handoff §1 values."** Stage 2 reproduces those values, 8/8. The values themselves rest on the programmer's `git show HEAD:` (independently anchored for the subsystem pair only, via `TASK-1507`/`TASK-1521` handoffs). I read no git object. The host's diff closes this.
- **The `WBP_DeckBuilder` reads are a package name-table byte census plus strand reads, not an editor-side node iteration.** A name-map absence rules out a `CallFunction` or property-binding reference by that name. It does not rule out behaviour that reaches legality by another route (none is known).
- **`TASK-1270`'s date** comes from its board header ("boarded 2026-09-14"), not from git.
- **Carried unchanged from loop 0:**
  - the keyboard's "~30 a second" is unmeasured;
  - (i)'s `UPlayerInput` timing and (g)'s `SetFocusAndLocking` deferral were not re-read at engine source;
  - (n)'s history was checked against `qa/TASK-1497.md`, not git.
- This is a **text-level plus source- and asset-measured** verdict. No compile, suite or PIE was run, and none is owed by a QA gate.
