# TASK-1480 — [CITATION-ROT-REANCHOR] programmer handoff

**Row:** `TASK-1480` · marker `TASK-1480-CITATION-ROT-REANCHOR` (refresh `TASK-1480-REFRESH-2026-09-27`) · gate `TASK-1481` · 5a `TASK-1538` · host `TASK-1540`
**Status flipped to:** `ready-for-qa`
**Date:** 2026-09-27
**Approval:** Jonathan, verbatim, relayed by the orchestrator: *"yes go ahead and run all those tasks"*.

## ⛔ ZERO EXECUTABLE CHANGE

**Every hunk in this row is comment bytes. ZERO EXECUTABLE CHANGE.** No token, no string literal, no
`TEXT(…)` literal (so no `TASK-1394` instrument string), no preprocessor line, no declaration moved.
How that was established is in §3 (a lexer comparison against `HEAD`, with three negative controls that
each FAIL it). ⛔ No compile, no PIE, no MCP, no `.uasset`, no `CONVENTIONS.md`, no mutating git.

## §0 — Read-only git and hashing, declared up front (`SC-§71a`, marker `SC-71A-READ-ONLY-GIT-START-STATE`)

Every git call ran as `git --no-optional-locks …`, from the git root one level up
(`C:\GitProjects\GitHub\GitClaudeUnrealTesting`). No commit host was live. Nothing was staged, stashed,
checked out or restored.

- `rev-parse HEAD` → `9a67a27984458056dcd0019da587d6b2d97f5769`
- `status --porcelain` → only `.claude/pipeline/TASKBOARD.md` (M), `handoffs/TASK-1536-buildmaster.md` (??),
  `qa/TASK-1499-verify.md` (??). ⇒ **`Source/` was clean at my start**, so every before-sha256 below is `HEAD`'s content.
- `log --oneline -3`, and `log --oneline -3 -- .claude/pipeline/handoffs/TASK-568-artist.md` (one commit, `93c5ec8`).
- `diff --stat`, `diff --numstat`, `diff -U0` over `GitClaudeUnrealTest/Source` (the excerpts in §5).
- `show HEAD:<path>` for each of the eight files, **inside** the lexer script of §3.
- `sha256sum` on each target file, before and after.

## §1 — Start state: before-sha256 (= `HEAD` `9a67a27`)

| File (`Source/GitClaudeUnrealTest/Siegebound/…`) | before-sha256 | EOL |
|---|---|---|
| `SiegeControlsHelpWidget.cpp` | `44e3c46f9eb8fc57322cbe2ab6aa100946357b3306c0a2c9652c8383b9b69f5f` | LF |
| `SiegeControlsHelpWidget.h` | `4578363b43166d496494dcb538082dbb087189d3580a12ad6bc8c3f15c6e32f9` | LF |
| `SiegeMenuInputSubsystem.cpp` | `9cb571d1bb79c25b69608b790c399e58b81ef3ae74bdb5417932845b0104942d` | LF |
| `SiegeMenuInputSubsystem.h` | `e4ad7ecf0a2134ccb8f06f137d30a9730d2608a964856711f82dc414699f4c77` | LF |
| `Tests/SiegeMenuInputTest.cpp` | `6cbae282d431a14b18591df780e729317cd7ad261d70ba44abb8e3accb0d9405` | LF |
| `DeckBuilderWidget.h` | `66cdd9f114d67e7cdae83c4935096cbfa6e7938013b63728660e19e32cc1394b` | CRLF |
| `DeckBuilderWidget.cpp` | `27a478bdd71396f9175fd0c3fc8d9f1c943b8b6410675d4a832bc2804585a9a6` | CRLF |
| `SiegePlayerController.cpp` | `773d15605f05976ae0c8edc0e34dc66fdd61295bc842e57967d22dc2b2058d11` | LF |

## §2 — The per-item table (a)–(o)

"Found" means verified at my own instant (`SC-§91`) before touching it. No item was already fixed by
another row, so there are **no useful zeros** except (k), which is a notice by design.

| Item | Source | Found at my instant? | Result | Where (by text) |
|---|---|---|---|---|
| **(a)** | `qa/TASK-1433.md` WARN-L3 | **Yes.** R-02 still cited `SiegePlayerController.cpp:768-770 and :783-790` and `:4361-4370`, no flag. Both ranges land on unrelated code today. ⭐ Also found: its `handoffs/TASK-568-artist.md:28` cite was wrong (that line is the IA_Move "3 S" row; IA_Look is 3 lines lower). | **Re-anchored by text** (the whole R-02 block), old numbers quoted as "WAS". ⭐ Plus QA's suggested **file-level declaration** at the TASK-707 transfer rules, and R-08's "nobody has boarded it" **struck** (it now has an owner). | `SiegeControlsHelpWidget.cpp`: `FSiegeControlsHelpRegistry::GetActions()`, the "704 §4 R-02 detail" block; the "TASK-707: THE `Detail` COLUMN" banner; the R-08 "SCOPE, DECLARED" lines |
| **(b)** | `handoffs/TASK-1474-programmer.md` *Not examined* | **Yes**, verbatim. | **Struck and replaced with TASK-1474's text, verbatim** (`SC-§120` form). | `DeckBuilderWidget.h`, `RegisterAsMenuNavTarget`'s doc |
| **(c)** | same handoff | **Yes, all four.** `.h`/`.cpp` `:177`, test `:176-178` ×2. ⭐ And the **claim** under the numbers had also expired (no live opt-out). | **Re-anchored by text to `ApplyButtonNotFocusable`**; the present-tense claim struck at the `.h`. The `.cpp` one is (m); the two test ones are (f). | `SiegeMenuInputSubsystem.h`, class doc, "FENCE (c) — THE VOCABULARY" |
| **(d)** | `qa/TASK-1426.md` NIT-1 | **Yes**, `.cpp` twins now at `:234` / `:350`. ⭐ **A third instance** was found at `SiegeMenuInputSubsystem.h` (`FocusReentryPollSeconds`' doc). | **Re-anchored by text, all three** (`USessionMenuWidget::BackPressed`'s `LoadClass<UUserWidget>` of `/Game/UI/WBP_MainMenu.WBP_MainMenu_C` → `CreateWidget` → `AddToViewport`). | subsystem `.cpp` `OnWorldBeginPlay` TASK-1400 block and `FindMainMenuWidget`; `.h` `FocusReentryPollSeconds` doc |
| **(e)** | `qa/TASK-1479.md` WARN-1 | **Yes**, both clauses unstruck. | **Struck the same way as the twin** (`~~`CloseButton`~~` inline, then a dated `⭐ TASK-1480 (e)` sentence stating what is true). The not-reorder decision is kept on its surviving reason. | `SiegeControlsHelpWidget.cpp`, `ReturnToList`'s "ONE TRANSIENT PAIR" lines |
| **(f)** | `qa/TASK-1479.md` NIT-2 | **Yes**, both `:176-178`. | **Re-anchored by text** (`ApplyButtonNotFocusable`'s `Button->IsFocusable = false;` write). "shipped" struck (no live caller). ⛔ **No assertion touched.** | `Tests/SiegeMenuInputTest.cpp`, FIXTURE C doc + the comment above `OptedOutButton`'s field write |
| **(g)** | `qa/TASK-1483.md` NIT-5 | **Yes, absent.** The (b)/"ON OPEN" text argued the arming re-read, never the ring. Verified at source: `FocusFirstNavStop`'s silent `HasUserFocus \|\| HasUserFocusedDescendants` guard; `RegisterMenuNavTarget` discards the return. | **Added a trip-wire comment** stating the ordering and what breaks if it moves. | `SiegePlayerController.cpp`, `HandleMatchEnd`, the TASK-1482 "REGISTER ON OPEN" block, right above `if (UWorld* World = GetWorld())` |
| **(h)** | `qa/TASK-1509.md` N6 | **Yes, both.** Seven bindings confirmed in `BindMenuNavActions`' table. | **Six → seven, nothing else.** | `DeckBuilderWidget.h`, `UnbindMenuNavActions` doc and `RouteMenuNavKey` doc |
| **(i)** | `qa/TASK-1509.md` N1 | **Yes**, DOWN-only. | **Extended to the key-UP** with QA's traced reasoning. Verified: every C++ binding of IA_MenuLeft/Right is `Started` (subsystem ×2 sites, `BindMenuNavActions`). | `DeckBuilderWidget.h`, `RelayDeckBarNavigationKeyToSlate` doc, after "THE FEEDBACK GUARD" |
| **(j)** | `TASK-1513` item 6 | **Yes, and more than named.** The row named 3 sites. My census of "Play with this deck" / "Play gate" / "play gate" in both deck files found **6**. ⭐ The row's `IsWorkingDeckLegal` is really **`IsCurrentDeckLegal`**; it has **no C++ caller** (only its definition), so its doc was reworded, not left. | **All 6 reworded/struck**, citing `DECK-§4(c)` and written as "per `DECK-§4(c)`'s ruling" (the WBP was not opened). | `.cpp` `SetActiveDeck` TASK-671 comment; `.h` `IsCurrentDeckLegal` doc, "Card details" section comment, `SaveDeckAs` doc, `OnDeckModelChanged` doc, `RefreshDeckBarStates` doc |
| **(k)** | `qa/TASK-1522.md` N1 | Read. | **Notice recorded: adapter bytes untouched.** The only change in its comment block is (l)'s phrase; the TASK-1521 "NO LONGER TRUE" wording is kept, not reverted. | `DeckBuilderWidget.cpp`, `HandleCardGridKey(const FKeyEvent&)` |
| **(l)** | `qa/TASK-1522.md` N3 | **Yes.** | **Reworded to "the OS / controller repeat rate"**, with both rates. Pad figures verified at engine source: the XInputDevice plugin's `InitialButtonRepeatDelay = 0.2f;` / `ButtonRepeatDelay = 0.1f;`, read from `/Script/Engine.InputSettings`; `Config/` has no override (grep: 0 hits). | same adapter comment |
| **(m)** | `qa/TASK-1497.md` WARN | **Yes.** `:177` / `:2240` / `:2787`, and the CloseButton claim false. | **False claim struck** (`SC-§120`: quoted, struck, what is true stated); the two scroll-box opt-outs **re-anchored by text** (`DetailScrollBox->SetIsFocusable(false);` in `ConstructDetailTree`, `RowScrollBox->SetIsFocusable(false);` in `ConstructHelpTree`). ⭐ See §4: the flip dates to **TASK-1432**, not TASK-1478. | `SiegeMenuInputSubsystem.cpp`, `IsNavFocusStop`, "FENCE (c) — THE FOUR ADMITTED CLASSES" |
| **(n)** | `qa/TASK-1497.md` NIT | **Yes**, arithmetic true (N + 2). | **Arithmetic unchanged**; one dated round-trip line added. | `SiegeControlsHelpWidget.h`, `NativeOnPreviewKeyDown` doc |
| **(o)** | `qa/TASK-1497.md` NIT (RULED retain) | **Yes**, 0 live callers (the census: 0 calls; the three `UButton`s all go through `ApplyButtonFocusable`). | **Marked deliberately retained**, both trip-wires named by their text. Not deleted, not fenced off. ⭐ Plus the mirror `ApplyButtonFocusable`'s "THREE buttons … NOT focus stops … the fourth … ONLY focus stop" **struck**, because it now contradicts the new note on the same page. | `SiegeControlsHelpWidget.cpp`, the docs of `ApplyButtonNotFocusable` and `ApplyButtonFocusable` |

Every replacement is quoted in full in §5's `-U0` excerpts. They are the exact bytes.

## §3 — How "comment-only" was established (`SC-§93` cl. 4)

**Instrument:** `lexcmp.py` in my scratchpad (not in the repo). For each file it takes `git show HEAD:<path>`
and the working tree, applies translation phase 2 (backslash-newline splice) **before** removing comments,
removes `//` and `/* … */` comments while keeping every string, char and raw-string literal verbatim,
collapses whitespace per line, drops empty lines, and compares the two code-line lists.

**Result, all eight files:**

| File | code lines HEAD → tree | identical |
|---|---|---|
| `SiegeControlsHelpWidget.cpp` | 2002 → 2002 | True |
| `SiegeControlsHelpWidget.h` | 258 → 258 | True |
| `SiegeMenuInputSubsystem.cpp` | 1073 → 1073 | True |
| `SiegeMenuInputSubsystem.h` | 123 → 123 | True |
| `Tests/SiegeMenuInputTest.cpp` | 464 → 464 | True |
| `DeckBuilderWidget.h` | 159 → 159 | True |
| `DeckBuilderWidget.cpp` | 1710 → 1710 | True |
| `SiegePlayerController.cpp` | 3963 → 3963 | True |

**Controls. The instrument can fail** (each applied to a copy of `SiegePlayerController.cpp`, not the file):
C1, a `\` appended to one of my new `//` lines: **identical=False**. C2, one character changed inside a
`TEXT(…)` literal: **identical=False**. C3, a stray `*/` after a statement: **identical=False**.
C4, the unmodified tree: **identical=True**.

**Cross-checks:** no line in any of the eight files ends in `\` (grep: 0). The added lines contain no `/*`
or `*/` except the opener and closer of the one `/** … */` block I reshaped (`IsCurrentDeckLegal`'s
one-line doc became multi-line). No added line contains `TEXT(`. Line endings are preserved: LF files CR=0,
CRLF files CR=LF (`DeckBuilderWidget.h` 1417/1417, `.cpp` 3364/3364).

**The comment block each hunk sits in:**

| File @ new line | Block (opener and closer read) | Item |
|---|---|---|
| `DeckBuilderWidget.cpp` @1846 | the file-scope `//` run above `FReply UDeckBuilderWidget::HandleCardGridKey(const FKeyEvent&)` | (l) |
| `DeckBuilderWidget.cpp` @2948, @2950 | the `//` run inside `SetActiveDeck`, above `RefreshDeckBarStates();` | (j) |
| `DeckBuilderWidget.h` @508 | `/** … */` above `UFUNCTION(BlueprintPure) bool IsCurrentDeckLegal() const;` | (j) |
| `DeckBuilderWidget.h` @565 | the `// --- Card details` section `//` run | (j) |
| `DeckBuilderWidget.h` @634 | `/** … */` above `UFUNCTION(BlueprintCallable) void SaveDeckAs` | (j) |
| `DeckBuilderWidget.h` @681 | `/** … */` above `UFUNCTION(BlueprintImplementableEvent) void OnDeckModelChanged()` | (j) |
| `DeckBuilderWidget.h` @872 | `/** … */` above `void RefreshDeckBarStates();` | (j) |
| `DeckBuilderWidget.h` @1193, @1231 | `/** … */` above `UnbindMenuNavActions` / `RouteMenuNavKey` | (h) |
| `DeckBuilderWidget.h` @1293 | `/** … */` above `RelayDeckBarNavigationKeyToSlate` | (i) |
| `DeckBuilderWidget.h` @1324 | `/** … */` above `RegisterAsMenuNavTarget` | (b) |
| `SiegeControlsHelpWidget.cpp` @178 | `/** … */` above `void ApplyButtonNotFocusable(UButton*)` | (o) |
| `SiegeControlsHelpWidget.cpp` @209, @211, @214 | `/** … */` above `void ApplyButtonFocusable(UButton*)` | (o) |
| `SiegeControlsHelpWidget.cpp` @337 | the file-scope TASK-707 transfer-rules `//` banner, before its closing `// ════` line | (a) |
| `SiegeControlsHelpWidget.cpp` @410 | the `//` run of R-02 in `GetActions()`, above `Row.Detail =` | (a) |
| `SiegeControlsHelpWidget.cpp` @580 | the `//` run of R-08 in `GetActions()`, above `Row.Detail =` | (a) |
| `SiegeControlsHelpWidget.cpp` @4124, @4128 | the `// ═══ 🚨 TASK-1432 QA LOOP 1` block in `ReturnToList`, above `if (bWasOnDetail && bHelpOpen)` | (e) |
| `SiegeControlsHelpWidget.h` @1062 | `/** … */` above `virtual FReply NativeOnPreviewKeyDown(…) override;` | (n) |
| `SiegeMenuInputSubsystem.cpp` @234 | the TASK-1400 `//` run in `OnWorldBeginPlay` | (d) |
| `SiegeMenuInputSubsystem.cpp` @352 | the `//` run in `FindMainMenuWidget` | (d) |
| `SiegeMenuInputSubsystem.cpp` @882, @886 | the FENCE (c) `//` run in `IsNavFocusStop`, above `if (const UButton* Button = …)` | (m) |
| `SiegeMenuInputSubsystem.h` @233 | the `USiegeMenuInputSubsystem` class doc `/** … */` (opener :92, closer :501, directly above `UCLASS(BlueprintType)`) | (c) |
| `SiegeMenuInputSubsystem.h` @581 | `/** … */` above `static constexpr float FocusReentryPollSeconds` | (d) |
| `SiegePlayerController.cpp` @2522 | the TASK-1482 "REGISTER ON OPEN" `//` run in `HandleMatchEnd`, above `if (UWorld* World = GetWorld())` | (g) |
| `Tests/SiegeMenuInputTest.cpp` @448 | the FIXTURE C `/** … */` above `static FSiegeNavStopFixture BuildVocabularyFixture()` | (f) |
| `Tests/SiegeMenuInputTest.cpp` @488 | the `//` run above `PRAGMA_DISABLE_DEPRECATION_WARNINGS` in `BuildVocabularyFixture` | (f) |

**What a comment edit still changes (why 5a `TASK-1538` is real):** four edited `/** */` blocks sit on
**reflected** declarations, so UHT's ToolTip metadata in the generated code changes: `IsCurrentDeckLegal`,
`SaveDeckAs`, `OnDeckModelChanged` (all `UDeckBuilderWidget`), and the `USiegeMenuInputSubsystem` class
doc. **The exec-symbol set cannot change:** no `UFUNCTION`/`UPROPERTY`/`UCLASS` line or signature moved
(the code streams are identical). That is a prediction until 5a regenerates the headers. Every other edited
block is above a non-reflected declaration or is in a `.cpp`.

## §4 — Spec claims that did not survive the source (`SC-§101`), and scope I added

- **(j) names `IsWorkingDeckLegal`.** The function is `IsCurrentDeckLegal` (no symbol `IsWorkingDeckLegal`
  exists in `Source/`). The test "a Play-gate caller in C++?" was run on the real name: **0 callers** besides
  its definition ⇒ the doc was reworded.
- **(j) says "three".** The census found six sites carrying the same pre-amendment claim; all six addressed.
  The extra three are `RefreshDeckBarStates`' doc (the direct header twin of the `.cpp` `SetActiveDeck`
  comment), `OnDeckModelChanged`'s "(Play gate)", and the "Card details" section's "exactly-50 play gate".
- **(m) says `CloseButton` has been focusable "since `TASK-1478`".** The `CloseButton` site's own comment
  dates the flip to **`TASK-1432`** ("⭐⭐ TASK-1432 — FOCUSABLE, AND IT IS THE ONLY ONE OF THIS FILE'S FIVE
  DE-FOCUS SITES THAT WAS FLIPPED"), and `ApplyButtonFocusable` itself was introduced by TASK-1432. I wrote
  TASK-1432 for the CloseButton flip and TASK-1478 for "no `UButton` opted out at all". Both rows committed
  in `4620f71`, so nothing downstream moves on this.
- **(d)** QA named the two `.cpp` twins. The `.h` had a third identical pointer; fixed with them.
- **(a)** R-02 also carried a wrong handoff cite (`TASK-568-artist.md:28`). `handoffs/` files do not move
  (one commit ever), so that cite was wrong when written. Re-anchored by text with the rest of the block.
  `TASK-399-artist.md:137` was **correct** and is now quoted by its row text anyway.
- **(a)** QA's suggested file-level declaration was added beside the R-02 re-anchor, and R-08's
  *"nobody has boarded it"* was struck, because this row is that boarding. Without the declaration, the
  other unflagged `Citations (T1)` blocks would still read as trustworthy, which is WARN-L3's hazard.
- **(o)** The mirror `ApplyButtonFocusable`'s "THREE buttons … NOT focus stops" count was struck, because
  it contradicted the (o) note on the same page. Census of `ConstructWidget<UButton>` in the file: 3
  (`RowButton`, `BackButton`, `CloseButton`), all `ApplyButtonFocusable`.
- **(c)/(f)/(m)** fix the **claim** as well as the number: "deliberate in this project" / "shipped opt-out"
  were present-tense claims that expired at TASK-1478. Census of `Source/`: the only live `IsFocusable ==
  false` writes are the two `UScrollBox` `SetIsFocusable(false)` calls and the test fixture's own field write.

## §5 — After-sha256 and `git diff -U0` excerpts

| File | after-sha256 | numstat (informational only) |
|---|---|---|
| `SiegeControlsHelpWidget.cpp` | `0bc5907e69f1a0ec2db988c4c336d991c45d7573751fd2e1a8d75ba984a681e9` | +66 −10 |
| `SiegeControlsHelpWidget.h` | `cc16d2caade4dab2a4676724ba9a2596b1f455d115560afe35f259941012d174` | +3 −0 |
| `SiegeMenuInputSubsystem.cpp` | `7e7f1eb75de338a4476cde1ac0da56579d2a20a75c4dd0c39b1e49636e742f22` | +16 −3 |
| `SiegeMenuInputSubsystem.h` | `b1a5d87582aeb1db858aae62f4846d37612301aee851a60c737253fbb998e21b` | +10 −2 |
| `Tests/SiegeMenuInputTest.cpp` | `5470c88605442b63bdbe60126d37dc33fd30b2a1e6fada7cb1661a451d85797d` | +9 −2 |
| `DeckBuilderWidget.h` | `82ae9c562f1fac936f58211a06918bba08aac30a7f902fd7f2b3955f38eecdd4` | +38 −9 |
| `DeckBuilderWidget.cpp` | `9f39e0e10435fc78f05e1db57e0cf040dce585439b56a7278185082df3161691` | +13 −3 |
| `SiegePlayerController.cpp` | `811da097d8145129546bda3153ceca3fb14dee638b0c5df9bfe27b605b27bed0` | +14 −0 |

⛔ `TASK-1538` compiles these exact bytes and `TASK-1540` commits them: **both must match this table.**
`SiegePlayerController.h` was not touched (`840416b6…6d10`, unchanged). The numstat is informational;
the proof is §3.

Excerpts (`git --no-optional-locks diff -U0`, `\r` removed for display from the two CRLF files):

### `DeckBuilderWidget.cpp`
```
@@ -1846 +1846,5 @@ FReply UDeckBuilderWidget::HandleCardGridKey(const FKey& Key)
-// deck, one refusal Warning on an illegal one, ~30 a second.
+// deck, one refusal Warning on an illegal one, at the OS / controller repeat rate
+// (TASK-1480 (l), 2026-09-27 — this said "~30 a second", which is the keyboard's
+// OS rate only; a pad's XInput repeat starts after 0.2 s and then fires every
+// 0.1 s, ≈10 a second: the XInputDevice plugin's InitialButtonRepeatDelay /
+// ButtonRepeatDelay defaults, which this project's Config/ does not override).
@@ -2944 +2948 @@ void UDeckBuilderWidget::SetActiveDeck(const FString& Name)
-	// nothing); this one site covers SetActiveDeckBySlot's right-click lane AND
+	// nothing); this one site covers SetActiveDeckBySlot's right-click lane ~~AND
@@ -2946 +2950,7 @@ void UDeckBuilderWidget::SetActiveDeck(const FString& Name)
-	// both. Existing behavior above is untouched (DECK-§4 byte-compatibility).
+	// both~~. ⛔ TASK-1480 (j), 2026-09-27: per DECK-§4(c)'s ruling (2026-08-27)
+	// the Play button's SaveDeckAs("Active") / SetActiveDeck("Active") nodes were
+	// CUT and Play starts the match with the ACTIVE deck, so it activates nothing
+	// and there is no D8 lane here. (Not measured: this row did not open the WBP,
+	// so that is the ruling's word.) SetActiveDeckBySlot also carries TASK-1507's
+	// Home / pad Y key route, so the orange follows that too. Existing behavior
+	// above is untouched (DECK-§4 byte-compatibility).
```

### `DeckBuilderWidget.h`
```
@@ -508 +508,7 @@ public:
-	/** True iff the working deck is a legal exactly-50-card deck via UDeckLibrary::IsDeckLegal (gates "Play with this deck"; per-card caps abolished, CARD-UNCAP 2026-08-28). */
+	/**
+	 *  True iff the working deck is a legal exactly-50-card deck via UDeckLibrary::IsDeckLegal
+	 *  (per-card caps abolished, CARD-UNCAP 2026-08-28). Not a Play gate: per DECK-§4(c)'s
+	 *  ruling (2026-08-27) Play starts the match with the ACTIVE deck and its legality
+	 *  enable-gate was removed. No C++ caller reads this (TASK-1480 (j), 2026-09-27; until then
+	 *  this line said it gates "Play with this deck").
+	 */
@@ -559 +565,2 @@ public:
-	// are untouched by everything in this block.
+	// (⛔ TASK-1480 (j): no longer a UI gate — per DECK-§4(c)'s ruling the Play
+	// button's enable-gate was removed) are untouched by everything in this block.
@@ -627 +634,4 @@ public:
-	 *  never blocks saving; "Play with this deck" is the only 50-card gate. Fires
+	 *  never blocks saving. (TASK-1480 (j), 2026-09-27: this said "Play with this deck"
+	 *  was the only 50-card gate; per DECK-§4(c)'s ruling that button's enable-gate was
+	 *  removed, so no UI surface gates on 50 cards — in the ruling's words the "n/50"
+	 *  text is the feedback and the match-side legality check is the enforcement.) Fires
@@ -671 +681,3 @@ public:
-	 *  (x/50), GetAverageCost (§8), IsCurrentDeckLegal (Play gate), and each
+	 *  (x/50), GetAverageCost (§8), IsCurrentDeckLegal (once the Play gate; DECK-§4(c)'s
+	 *  ruling removed that gate wiring, and whether the graph still reads it is not
+	 *  measured here — TASK-1480 (j)), and each
@@ -860,2 +872,3 @@ private:
-	 *  path (which also covers SetActiveDeckBySlot and the D8 "Play with this
-	 *  deck" activation — the orange follows it).
+	 *  path (which also covers SetActiveDeckBySlot ~~and the D8 "Play with this
+	 *  deck" activation — the orange follows it~~; ⛔ TASK-1480 (j): per DECK-§4(c)'s
+	 *  ruling the Play button no longer activates a deck, so there is no D8 lane).
@@ -1180 +1193 @@ private:
-	 *  leave six bindings pointing at a dead UObject on the controller for the rest
+	 *  leave seven bindings pointing at a dead UObject on the controller for the rest
@@ -1218 +1231 @@ private:
-	 *  implementation. Everything above is six names for this call.
+	 *  implementation. Everything above is seven names for this call.
@@ -1279,0 +1293,12 @@ private:
+	 *  ⭐ TASK-1480 (i), 2026-09-27 (qa/TASK-1509.md N1) — AND THE KEY-UP, which the
+	 *  paragraph above covered for the DOWN only. SButton::OnKeyUp claims only the
+	 *  Accept action, so the synthesized ProcessKeyUpEvent(Left/Right) DOES bubble
+	 *  unhandled to SViewport → UGameViewportClient::InputKey. On L_MainMenu that
+	 *  Released edge is closed by UGameViewportClient::InputKey's IgnoreInput()
+	 *  early return (the `if (IgnoreInput())` return). Anywhere else it arrives as
+	 *  IE_Released, which cannot produce a Started edge — and every C++ binding of
+	 *  IA_MenuLeft / IA_MenuRight (USiegeMenuInputSubsystem's and this class's
+	 *  BindMenuNavActions table) is Started-only — while UPlayerInput::InputKey
+	 *  only records key state (action delegates evaluate on the next
+	 *  ProcessInputStack, so nothing re-enters RouteMenuNavKey synchronously).
+	 *  ⇒ no loop and no re-fire on the up event either.
@@ -1299 +1324,5 @@ private:
-	 *  (UWidgetTree::ForEachWidget descends through UPanelWidget only). Registration
+	 *  ~~(UWidgetTree::ForEachWidget descends through UPanelWidget only)~~ (they are
+	 *  Blueprint sub-widgets, which `USiegeMenuInputSubsystem::IsCodeAuthoredSubWidget`
+	 *  refuses to enter — the walker's descent boundary, TASK-1474). ⛔ TASK-1480 (b),
+	 *  2026-09-27: the struck parenthetical named the wrong mechanism while its
+	 *  conclusion held; the replacement is TASK-1474's handoff text, verbatim. Registration
```

### `SiegeControlsHelpWidget.cpp`
```
@@ -177,0 +178,13 @@ namespace
+	 *
+	 *  ⭐ TASK-1480 (o) (2026-09-27, `qa/TASK-1497.md` NIT, ⛔ RULED: RETAIN) — ⛔ DELIBERATELY
+	 *  KEPT WITH ⛔ ZERO LIVE CALLERS. `CloseButton`'s call was flipped at ⭐ `TASK-1432` and the
+	 *  last two (`RowButton`, `BackButton`) at ⭐ `TASK-1478`; since then every `UButton` this file
+	 *  builds goes through `ApplyButtonFocusable` below. ⛔ IT STAYS BECAUSE THE TWO TRIP-WIRES
+	 *  NAME IT AS THE REVERT: the one at `RowButton`'s flip in
+	 *  `USiegeControlsHelpRowWidget::ConstructRowTree` (it opens "TRIP-WIRE — ⛔ READ
+	 *  `USiegeControlsHelpWidget::ApplyActiveView` BEFORE TOUCHING THIS LINE" and ends "THIS WRITE
+	 *  MUST GO BACK TO `ApplyButtonNotFocusable` ⛔ IN THE SAME DIFF"), and the one at
+	 *  `BackButton`'s flip in `USiegeControlsDetailWidget::ConstructDetailTree` (it opens "THE
+	 *  TRIP-WIRE (`qa/TASK-1433.md` WARN-L1)"). ⛔ Do NOT delete it or fence it off as dead code:
+	 *  either is an executable change, and the revert those wires prescribe would have nothing
+	 *  left to call.
@@ -196 +209 @@ namespace
-	 *  changes ⛔ no engine default — what it changes is the ⛔ READING of the call site. THREE
+	 *  changes ⛔ no engine default — what it changes is the ⛔ READING of the call site. ~~THREE
@@ -198 +211 @@ namespace
-	 *  the fourth is deliberately the screen's ⛔ ONLY focus stop, and a ⛔ BLANK LINE there
+	 *  the fourth is deliberately the screen's ⛔ ONLY focus stop, and~~ a ⛔ BLANK LINE there
@@ -200,0 +214,6 @@ namespace
+	 *  ⭐ TASK-1480 (o) (2026-09-27) — THE STRUCK COUNT EXPIRED AT ⭐ `TASK-1478`: every `UButton`
+	 *  this file builds (`RowButton`, `BackButton`, `CloseButton` — the whole census of its
+	 *  `ConstructWidget<UButton>` calls) now goes through THIS function and ⛔ none through its
+	 *  opposite number, which is retained with no live caller (see its own comment above). The
+	 *  reason for writing the call survives the count: a blank line at any of the three sites
+	 *  would still read as a forgotten call.
@@ -317,0 +337,9 @@ namespace
+//  ⚠️ TASK-1480 (a) — A FILE-LEVEL DECLARATION, ADDED 2026-09-27 ON `qa/TASK-1433.md` WARN-L3,
+//     BECAUSE THE PER-BLOCK ONES TAUGHT THE WRONG LESSON: three blocks below (R-08, R-19, R-24)
+//     flag their remaining numbers ⛔ UNVERIFIED and the others say nothing, which reads as
+//     "an unflagged block is trustworthy". ⛔ IT IS NOT. Every `file:line` NUMBER in this file's
+//     `Citations (T1)` blocks is as of the row that wrote it and is ⛔ UNVERIFIED today, UNLESS
+//     its own block says it was anchored BY TEXT or BY SYMBOL (e.g. R-02's whole block since
+//     TASK-1480, and the one `bWantCursor` anchor each in R-08 / R-19 / R-24 since TASK-1432
+//     QA loop 1). Treat a bare number as a lead: find the target by its symbol or its quoted
+//     text, never by the digit (`CITE-BY-TEXT-RULED-2026-09-24`).
@@ -382,4 +410,21 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			// 704 §4 R-02 detail. Citations (T1): the Negate_2 binding = handoffs/TASK-568-artist.md:28
-			// and handoffs/TASK-399-artist.md:137 (X✗ Y✓ Z✗); the ignore-look pairing =
-			// SiegePlayerController.cpp:768-770 and :783-790; the GameAndUI/DoNotLock modes =
-			// SiegePlayerController.cpp:4361-4370.
+			// 704 §4 R-02 detail. Citations (T1) — ⛔ RE-ANCHORED BY TEXT, ⛔ NOT RENUMBERED, at
+			// TASK-1480 (a) (2026-09-27, `qa/TASK-1433.md` WARN-L3, `CITE-BY-TEXT-RULED-2026-09-24`);
+			// every anchor below was opened and read at source for that row:
+			//   the Negate_2 binding = handoffs/TASK-568-artist.md, its readback row
+			//   "6 Mouse2D  IA_Look  modifiers: [InputModifierNegate_2]", and
+			//   handoffs/TASK-399-artist.md, its "`InputModifierNegate_2` | `IA_Look` / Mouse2D" row
+			//   (X✗ Y✓ Z✗);
+			//   the ignore-look pairing = `ASiegePlayerController::SetupInputComponent`'s IA_UICursor
+			//   BindAction triple (Started → OnUICursorPressed; Completed AND Canceled →
+			//   OnUICursorReleased), `ASiegePlayerController::OnUICursorPressed`'s
+			//   `SetIgnoreLookInput(true);` ("paired 1:1 with ClearUICursorHold"), and
+			//   `ASiegePlayerController::ClearUICursorHold`'s guarded `SetIgnoreLookInput(false);`;
+			//   the GameAndUI/DoNotLock modes = `ASiegePlayerController::ApplyCursorInputState`'s
+			//   `if (bWantCursor)` branch (`FInputModeGameAndUI InputMode;` then
+			//   `SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock)`), where `bWantCursor`
+			//   composes bInPlacementMode, bInTargetingMode and GroupPickStage among its terms.
+			//   ⛔ WAS: `handoffs/TASK-568-artist.md:28` (that line is the IA_Move "3 S" row; the
+			//   IA_Look row sits three lines lower), `SiegePlayerController.cpp:768-770 and :783-790`
+			//   and `SiegePlayerController.cpp:4361-4370` — both controller ranges measured rotted by
+			//   `qa/TASK-1433.md` WARN-L3 and landing on unrelated code again today. Quoted, not
+			//   deleted, so a reader holding an older copy can still map them.
@@ -535 +580,4 @@ const TArray<FSiegeControlsHelpAction>& FSiegeControlsHelpRegistry::GetActions()
-			// blocks is a real follow-up and ⛔ nobody has boarded it.
+			// blocks is a real follow-up ~~and ⛔ nobody has boarded it~~. ⭐ TASK-1480 (a)
+			// (2026-09-27): boarded, and answered with a FILE-LEVEL declaration at the TASK-707
+			// transfer rules near the top of this file plus R-02's re-anchor by text, ⛔ not a sweep
+			// of every block — so the numbers above are STILL unverified, now by a stated rule.
@@ -4076 +4124 @@ void USiegeControlsHelpWidget::ReturnToList()
-	//  `CloseButton`) → `ApplyOpenState(false)` (unregisters) in one call stack, so that ⛔ ONE
+	//  ~~`CloseButton`~~) → `ApplyOpenState(false)` (unregisters) in one call stack, so that ⛔ ONE
@@ -4080,2 +4128,10 @@ void USiegeControlsHelpWidget::ReturnToList()
-	//  statements to make a log tidier trades a real risk for a cosmetic gain, and the transient
-	//  focus placement is onto the same button that already holds focus on the ordinary close.
+	//  statements to make a log tidier trades a real risk for a cosmetic gain~~, and the transient
+	//  focus placement is onto the same button that already holds focus on the ordinary close~~.
+	//  ⭐ TASK-1480 (e) (2026-09-27, `qa/TASK-1479.md` WARN-1) — ⛔ BOTH STRUCK HALVES EXPIRED AT
+	//  ⭐ `TASK-1478`, for the reason the twin at the top of this block already gives: stop 0 of the
+	//  list is now the ⛔ FIRST `RowButton`, so the transient registration rings ⛔ THAT button,
+	//  ⛔ not `CloseButton`, and it is ⛔ not in general the button that held focus before the close
+	//  (a list-view close leaves the ring wherever the player put it; this route lands it on row 1).
+	//  ⛔ THE DECISION NOT TO REORDER `CloseHelp` STANDS on the reason that survives: the pair is two
+	//  true log lines, the end state (overlay closed, screen unregistered) is still correct, and a
+	//  shipped close route is still not worth reordering for a tidier log.
```

### `SiegeControlsHelpWidget.h`
```
@@ -1061,0 +1062,3 @@ protected:
+	 *  ⚠️ TASK-1480 (n), 2026-09-27 (`qa/TASK-1497.md` NIT): the `+ 2` is true today by ROUND TRIP,
+	 *  ⛔ not by upkeep — true at ⭐ `TASK-1478` (rows + `CloseButton` + `BackButton`), ⛔ FALSE (N + 3,
+	 *  `DetailScrollButton`) through ⭐ `TASK-1484`'s whole life, true again at ⭐ `TASK-1496`.
```

### `SiegeMenuInputSubsystem.cpp`
```
@@ -234 +234,3 @@ void USiegeMenuInputSubsystem::OnWorldBeginPlay(UWorld& InWorld)
-	//   • `USessionMenuWidget::BackPressed` (`SessionMenuWidget.cpp:151-165`) -- the same shape.
+	//   • `USessionMenuWidget::BackPressed` (its standalone branch: the `LoadClass<UUserWidget>` of
+	//     `/Game/UI/WBP_MainMenu.WBP_MainMenu_C` → `CreateWidget` → `AddToViewport` swap; cited as
+	//     `SessionMenuWidget.cpp:151-165` until TASK-1480 (d)) -- the same shape.
@@ -350 +352,3 @@ UUserWidget* USiegeMenuInputSubsystem::FindMainMenuWidget() const
-	// WBP_MainMenu (SessionMenuWidget.cpp:151-164), so the instance is resolved LIVE on
+	// WBP_MainMenu (`USessionMenuWidget::BackPressed`'s `LoadClass<UUserWidget>` of
+	// `/Game/UI/WBP_MainMenu.WBP_MainMenu_C` → `CreateWidget`; cited as
+	// `SessionMenuWidget.cpp:151-164` until TASK-1480 (d)), so the instance is resolved LIVE on
@@ -878 +882 @@ bool USiegeMenuInputSubsystem::IsNavFocusStop(const UWidget* Widget)
-	// project — `SiegeControlsHelpWidget.cpp:177` sets it on the help overlay's CloseButton, and
+	// project — ~~`SiegeControlsHelpWidget.cpp:177` sets it on the help overlay's CloseButton, and~~
@@ -881,0 +886,9 @@ bool USiegeMenuInputSubsystem::IsNavFocusStop(const UWidget* Widget)
+	// ⭐ TASK-1480 (m) (2026-09-27, `qa/TASK-1497.md` WARN, "the dangerous half") — ⛔ THE STRUCK
+	// CLAUSE IS FALSE, NOT MERELY MIS-NUMBERED: `CloseButton` is FOCUSABLE — the
+	// `ApplyButtonFocusable(CloseButton);` in `USiegeControlsHelpWidget::ConstructHelpTree`, whose
+	// own comment dates the flip to TASK-1432 — and since TASK-1478 no `UButton` in that file is
+	// opted out at all (its `ApplyButtonNotFocusable` helper has no live caller). ⛔ THE TWO
+	// SCROLL-BOX OPT-OUTS STAND, re-anchored BY TEXT: `DetailScrollBox->SetIsFocusable(false);` in
+	// `USiegeControlsDetailWidget::ConstructDetailTree` and `RowScrollBox->SetIsFocusable(false);`
+	// in `USiegeControlsHelpWidget::ConstructHelpTree` — the `:2240` / `:2787` above, in that
+	// order, kept only so an older copy can map them.
```

### `SiegeMenuInputSubsystem.h`
```
@@ -233 +233,6 @@ public:
- *  than stomping it (deliberate in this project at `SiegeControlsHelpWidget.cpp:177`).
+ *  than stomping it (~~deliberate in this project at `SiegeControlsHelpWidget.cpp:177`~~
+ *  ⛔ TASK-1480 (c), 2026-09-27 — struck on BOTH counts: the number rotted twice in one wave, and
+ *  the present tense expired at TASK-1478. The authored shape is `SiegeControlsHelpWidget.cpp`'s
+ *  `ApplyButtonNotFocusable` helper, found by that name; since TASK-1478 it has NO live caller
+ *  and no `UButton` in `Source/` is opted out. The rule is kept because an author's opt-out, in
+ *  C++ or in an asset, must still win when one appears).
@@ -576 +581,4 @@ public:
-	 *  `USessionMenuWidget::BackPressed` (`SessionMenuWidget.cpp:151-165`) is the SAME shape in
+	 *  `USessionMenuWidget::BackPressed` (its standalone branch: the `LoadClass<UUserWidget>` of
+	 *  `/Game/UI/WBP_MainMenu.WBP_MainMenu_C` → `CreateWidget` → `AddToViewport` swap; ⛔ cited
+	 *  as `SessionMenuWidget.cpp:151-165` until TASK-1480 (d), a range TASK-1425's insert had
+	 *  moved off that code) is the SAME shape in
```

### `SiegePlayerController.cpp`
```
@@ -2521,0 +2522,14 @@ void ASiegePlayerController::HandleMatchEnd(ETeamId Winner)
+	//
+	// 🚨🚨 ⛔ TRIP-WIRE — TASK-1480 (g), from qa/TASK-1483.md NIT-5 (2026-09-27): ⛔ THE SAME FRAME,
+	// AFTER SetInputMode, IS LOAD-BEARING FOR THE RING, ⛔ NOT ONLY FOR THE ARMING RE-READ ABOVE.
+	// (b) holds only because SetInputMode's SetWidgetToFocus request is STILL DEFERRED in the local
+	// player's FReply when this registration runs: Btn_Jump does not hold focus yet, so the
+	// idempotence guard in USiegeMenuInputSubsystem::FocusFirstNavStop (the "is ANYTHING among the
+	// stops already focused?" loop just above its `return FocusWidget(Stops[0]);`) falls through,
+	// and FocusWidget applies EFocusCause::Navigation — the only cause that paints the ring.
+	// ⛔ MOVE THIS CALL OUT OF THIS FRAME — a timer, a next-tick, a delegate, anything that lets the
+	// FReply flush first — AND Btn_Jump IS ALREADY FOCUSED (cause SetDirectly) WHEN THE GUARD ASKS:
+	// FocusFirstNavStop returns false, RegisterMenuNavTarget discards that return, the guard logs
+	// nothing, and ⛔ THE SCREEN OPENS RINGLESS WITH NO LOG LINE SAYING WHY. The press still works
+	// (the by-name focus above stands); only the outline is lost. ⇒ keep this call in this
+	// function, after SetInputMode, with nothing deferred between them.
```

### `Tests/SiegeMenuInputTest.cpp`
```
@@ -448 +448,6 @@ namespace SiegeMenuInputTestUtils
-	 *  shipped opt-out does at `SiegeControlsHelpWidget.cpp:176-178`, pragmas and all. ⭐ Mirroring
+	 *  ~~shipped opt-out does at `SiegeControlsHelpWidget.cpp:176-178`~~ opt-out helper does —
+	 *  `ApplyButtonNotFocusable` in `SiegeControlsHelpWidget.cpp`, its
+	 *  `Button->IsFocusable = false;` field write, found BY TEXT (TASK-1480 (f), 2026-09-27: the
+	 *  `:176-178` number rotted twice in one wave, and "shipped" expired at TASK-1478, since when
+	 *  the helper has no live caller; it is retained on purpose, see its own comment) — pragmas
+	 *  and all. ⭐ Mirroring
@@ -483 +488,3 @@ namespace SiegeMenuInputTestUtils
-		// (`SiegeControlsHelpWidget.cpp:176-178`). Same field, same pragmas, same order.
+		// (`ApplyButtonNotFocusable` in `SiegeControlsHelpWidget.cpp`, found BY TEXT; cited as
+		// `:176-178` until TASK-1480 (f) — no live caller since TASK-1478, retained on purpose).
+		// Same field, same pragmas, same order.
```

## §6 — What QA (`TASK-1481`) should scrutinize

1. **Re-run the comment-only proof yourself** (`SC-§92`: two instruments agreeing, not one trusted). The
   fastest check: for each hunk in §5, confirm that every `+` line sits inside the block named in §3's table.
   Only one hunk touches a block delimiter: `IsCurrentDeckLegal`'s `/** … */` went from one line to seven.
2. **The UHT ToolTip exposure** (§3, last paragraph): four reflected doc comments changed. The ToolTip
   metadata in the generated `.gen.cpp` changes. The exec-symbol set does not. 5a confirms it.
3. **The (j) wording** is the ruling's word, not a measurement of `WBP_DeckBuilder`, and says so twice.
   If you want a stronger claim, that needs an asset read, which this row may not do.
4. **The (a) file-level declaration** is a judgement call beyond the row's literal (a) text (QA WARN-L3's
   suggested fix). If you rule it scope creep, it is one self-contained `//` paragraph, and removing it is
   also comment-only.

## Not examined / limitations

- ⛔ **No compile, no suite, no PIE.** "Comment-only" is established by a lexer comparison against `HEAD`
  (§3), not by a compiler. `TASK-1538` owns the compile and the suite.
- ⛔ **`WBP_DeckBuilder` was not opened.** Every (j) sentence about the Play button cites `DECK-§4(c)`'s
  ruling and says it was not measured. Whether the WBP graph still reads `IsCurrentDeckLegal` is unknown
  and the `OnDeckModelChanged` doc says so.
- ⚠️ **The keyboard's "~30 a second" is not measured by me.** It is `qa/TASK-1509.md`'s figure for Windows'
  default repeat. The pad's 0.2 s / 0.1 s were read at engine source (XInputDevice plugin), with no
  `Config/` override (grep 0).
- ⚠️ **The (g) trip-wire's mechanism** is read at source (the subsystem's guard, the discarded return) plus
  `qa/TASK-1483.md` NIT-5's engine reading of `SetInputMode` deferring into the `FReply`. I did not re-read
  the engine's `FInputModeDataBase::SetFocusAndLocking`. The "cause SetDirectly" wording comes from the
  controller's own existing comment in the same block.
- ⚠️ **`numstat` is informational only.** Its deletion counts are not stable evidence, and nothing here rests on it.
- ⚠️ **Interaction with `TASK-1468` (backlog, not dispatched), declared so it is not discovered:** its spec
  (1) re-finds "the `IsFocusable == false` OPT-OUT CENSUS COMMENT" that "censuses THREE C++ sites
  (`:177`, `:2240`, `:2787`)". **That is the block (m) edited.** It now reads: one struck site plus two
  sites re-anchored by text. `TASK-1468` must re-find it by content, which it already says to do, and should
  expect the new shape. Its **other** site, the rotted claim in the `TASK-1474` `HasVisibleSlateAncestry`
  block ("every `UButton` in both switcher branches is authored `IsFocusable = false` … UNMOVED", with the
  trip-wire that has fired), is **`TASK-1468`'s**, per the manager's widening. ⛔ **I did not touch it**,
  although it is in a file this row may write comment bytes into.
- ⚠️ **Noticed, NOT taken (outside every item's text; offered to the manager):**
  - `SiegeControlsHelpWidget.cpp`, the `CloseButton` site's per-site table cites
    `SiegeMenuInputSubsystem.cpp:773-870` for `IsNavFocusStop` ("re-read live"). It has rotted: the
    function opens at `:859` today.
  - `Tests/SiegeCardHandKeyLabelTest.cpp` (`SiegeControlsHelpWidget.cpp:1211`, `:420-427`) and
    `Tests/SiegeControlsHelpTest.cpp` (`:1150-1152`, `:1205-1208`) cite the overlay `.cpp` by number.
    Both files are outside this row's file set, and the overlay grew by 56 lines today.
  - `DeckBuilderWidget.cpp`, `RegisterAsMenuNavTarget`'s body comment: *"The six IA_Menu\* bindings
    BindMenuNavActions() made"*. That is seven since `TASK-1507`. (h) was bounded to its two header sites
    ("six → seven, nothing else"), so I left it.
  - Every other numeric cite in the overlay's `Citations (T1)` blocks: **covered by the (a) file-level
    declaration, not re-anchored.** A full sweep would be its own row.
- (k): the adapter's executable bytes are untouched. The lexer comparison proves it for the whole file.

---

## QA loop 1

**Date:** 2026-09-27 · **Report answered:** `qa/TASK-1481.md` (FAIL: 1 BLOCKER, 1 WARN, 5 NIT) · **QA loop:** 1 of 3 · **Status flipped to:** `ready-for-qa` (re-gate)

### ⛔ ZERO EXECUTABLE CHANGE, again

Every loop-1 hunk is comment bytes. No token, no string literal, no `TEXT(…)` literal (so no `TASK-1394`
instrument string), no preprocessor line and no declaration moved. The proof is below. ⛔ No compile, no PIE,
no MCP, no `.uasset`, no `CONVENTIONS.md`, no mutating git. `TASK-1468`'s site is untouched (the
`HasVisibleSlateAncestry` TASK-1474 block: subsystem `.cpp` lines 780–880 are byte-identical to the loop-0
bytes). `SiegeControlsHelpWidget.cpp`'s non-comment bytes are unchanged, as `TASK-1541` needs.

### L1.0 — Start bytes = QA's anchors (8/8)

Before any edit I hashed all eight files. Every one equals `qa/TASK-1481.md` §4, including the two anchors the
dispatch named: `DeckBuilderWidget.h` `82ae9c56…eecdd4` and `SiegeControlsHelpWidget.cpp` `0bc5907e…a681e9`.
`SiegePlayerController.h` is still `840416b6…6d10`. I copied those bytes to my scratchpad (`loop0/`) as the
loop-1 baseline.

**Read-only git this loop (`SC-§71a`), declared:** exactly eight `git --no-optional-locks show HEAD:<path>`
calls, all inside the proof script (the HEAD baseline). Nothing else. No status, no diff and no index access:
the loop-1 `-U0` excerpt below is a plain `diff -U0` of the scratchpad snapshot against the tree.

### L1.1 — Resolution, per finding

Line numbers are **as of the loop-1 bytes and informational only**. Every site is found by the quoted text or
symbol given.

| Finding | Resolution | Where (by text; loop-1 line, informational) |
|---|---|---|
| **B1** BLOCKER | **Fixed.** The clause is now scoped to Play, and the activation gate is stated. New text: *"…that button's enable-gate was removed, so Play no longer gates on 50 cards, and per the same ruling the "n/50" text is the builder's feedback and the match-side legality check the enforcement. Deck ACTIVATION is gated, though: since TASK-1270 SetActiveDeck refuses to activate an illegal deck, and right-click on a deck-bar slot, or Home / pad Y while one holds focus, reaches it through SetActiveDeckBySlot; that gate postdates the ruling's "no UI surface hard-enforces the 50-card rule". Scoped to Play at TASK-1480 QA loop 1, qa/TASK-1481.md B1."* ⛔ "in the ruling's words" is **gone**, because it introduced a paraphrase. The one fragment still in quotation marks is a verbatim substring of `DECK-§4(c)`'s "Named consequence" (`CONVENTIONS.md`: *"no UI surface hard-enforces the 50-card rule anymore — …"*), and it sits on one line so a literal grep finds it. **Verified at source before I wrote it:** `Entry->OnRightClicked.BindUObject(this, &UDeckBuilderWidget::SetActiveDeckBySlot)`; the TASK-1507 key branch `if (IsDeckBarActivationKey(Key))` → `SetActiveDeckBySlot(BarSlot);`, where `IsDeckBarActivationKey` is `Key == EKeys::Home \|\| Key == EKeys::Gamepad_FaceButton_Top` and the branch is claimed only while `FindFocusedDeckBarSlot()` resolves a slot; `SetActiveDeckBySlot` → `SetActiveDeck(USiegeDeckSaveGame::MakeFixedDeckName(SlotIndex))`; `SetActiveDeck` → `if (!TryActivateSavedDeck(…))` → the `"… — refused; the active deck stays '%s'."` Warning + `OnDeckActivationRefused`; `TryActivateSavedDeck` → `if (!UDeckLibrary::IsDeckLegal(CardTable, *Found, OutRefusalReason))`. **Reflected:** this is the `SaveDeckAs` ToolTip, already one of the four reflected blocks. | `DeckBuilderWidget.h`, `SaveDeckAs`'s `/** … */` (the `(TASK-1480 (j), 2026-09-27: …)` parenthesis, ~:635–643) |
| **B1, optional in the same edit** | **Taken.** The "Card details" `//` run's *"(⛔ TASK-1480 (j): no longer a UI gate — …)"* now reads *"(⛔ TASK-1480 (j): it no longer gates Play — per DECK-§4(c)'s ruling the Play button's enable-gate was removed; deck activation is still gated, see SetActiveDeck's doc)"*. That closes the same broad misreading. **Not reflected** (QA measured: `GetCardDescription`'s own `/** */` wins). | `DeckBuilderWidget.h`, the `// --- Card details ("how it works", TASK-268)` run (~:565–567) |
| **W1** WARN | **Fixed.** The count is corrected and the retention rationale kept. Now: *"IT STAYS BECAUSE A TRIP-WIRE NAMES IT AS THE REVERT"*, with the `RowButton` wire quoted as before (and a note that both of its quotes span a line break in the source), then *"IT IS THE ONLY ONE THAT DOES"*. The `BackButton` wire carries a trip-wire for the SAME collapse but names no revert and never mentions the helper. The TASK-1478 switcher block cites the helper's comment only as a warning, in its rejected (γ). The close now reads *"the revert the `RowButton` wire prescribes would have nothing left to call"*. **Census at my instant, not taken from QA:** `ApplyButtonNotFocusable` has **0** mentions in `USiegeControlsDetailWidget::ConstructDetailTree` (its whole body) and **1** in `USiegeControlsHelpRowWidget::ConstructRowTree` ("THIS WRITE MUST GO BACK TO `ApplyButtonNotFocusable`"). The switcher block's only mention is (γ)'s *"This file's own `ApplyButtonNotFocusable` comment is a 20-line warning against exactly that misreading"*. A grep for "TWO TRIP-WIRES" / "those wires" in the file now returns 0. | `SiegeControlsHelpWidget.cpp`, `ApplyButtonNotFocusable`'s `/** … */`, the `⭐ TASK-1480 (o)` paragraph (~:179–193) |
| **N1** NIT | **Fixed.** The collapsed-padding row quote is gone. It now cites two whitespace-free fragments, `IA_Look` and `modifiers: [InputModifierNegate_2]`, and says the row pads its columns with runs of spaces, so a grep should target a fragment. Checked: both fragments are verbatim in `handoffs/TASK-568-artist.md` (the row is ` 6 Mouse2D           IA_Look            modifiers: [InputModifierNegate_2]`). | `SiegeControlsHelpWidget.cpp`, `GetActions()`, the "704 §4 R-02 detail" run (~:416–418) |
| **N2** NIT | **Fixed, quoted exactly.** Now: *"(the loop over the stops that, in its own comment's words, asks "is ANYTHING already focused?", just above its `return FocusWidget(Stops[0]);`)"*. The source is `FocusFirstNavStop`'s TASK-1469 LIMB 1(b) comment: *"this loop asks "is ANYTHING already focused?","*. | `SiegePlayerController.cpp`, `HandleMatchEnd`, the `TASK-1480 (g)` trip-wire (~:2527–2529) |
| **N3** NIT | **Fixed.** The surviving legacy pair is now marked where it first appears: *"`:2240` / `:2787` (⛔ WAS — `SiegeControlsHelpWidget.cpp` line numbers, cited so until TASK-1480; the by-text anchors are in the (m) note below) set it on the two `UScrollBox`es …"*. This is the (d)/(f) "cited as … until TASK-1480" style. The two sentences after it were only re-wrapped (same words). | `SiegeMenuInputSubsystem.cpp`, `IsNavFocusStop`, "FENCE (c) — THE FOUR ADMITTED CLASSES" (~:883–886) |
| **N4** NIT | **Fixed.** Now: *"…no `UButton` in `Source/` is opted out outside the test fixture (whose `OptedOutButton` in `Tests/SiegeMenuInputTest.cpp` is authored `IsFocusable = false` to pin this rule)"*. **Census of `Source/`:** the only `UButton` opt-out writes are the helper's `Button->IsFocusable = false;` (no live caller) and the fixture's `OptedOutButton->IsFocusable = false;` (a `UButton`, `ConstructWidget<UButton>`). The two other `false` writes are the `UScrollBox` `SetIsFocusable(false)` calls. **Reflected:** this is the `USiegeMenuInputSubsystem` class-doc ToolTip, already one of the four. | `SiegeMenuInputSubsystem.h`, class doc, "FENCE (c) — THE VOCABULARY" (~:237–240) |
| **N5** NIT (the manager's) | **Taken: "six" → "seven", one word, nothing else.** **Fence reasoning:** the row's (5) FENCES and `names: WRITES` are both **file-scoped** ("only the files carrying (a)–(o)" / "the COMMENT BLOCKS ONLY in the files carrying (a)–(o)"), and `DeckBuilderWidget.cpp` carries (j), (k) and (l). So the fence covers this comment block. (h)'s own text bounded *its* edit to two header sites; this is the same census-widening QA upheld for (d) and (j). **Verified at my instant:** `BindMenuNavActions`' table has 7 rows (`IA_MenuUp/Down/Left/Right/Accept/Back` + `IA_MenuSecondary`), and this block had no seven/TASK-1507 qualifier. If the manager rules it out, reverting it is one comment word. | `DeckBuilderWidget.cpp`, `RegisterAsMenuNavTarget`'s body, *"The seven IA_Menu\* bindings BindMenuNavActions() made"* (~:2378) |

**The other "six IA_Menu\*" mentions, found by the N5 census and ⛔ deliberately NOT changed:**
- `DeckBuilderWidget.h`, `BindMenuNavActions`' doc: *"Bind the six IA_Menu\* actions …"* and *"never takes the other five down with it"*. **Already qualified in the same block** by its own *"⭐ TASK-1507 — SEVEN, not six: IA_MenuSecondary … added as one more row of the same table"* paragraph. That follows the file's "qualify rather than delete" idiom (TASK-1423's own words at the `.cpp`'s NativeConstruct note), so the block is not false as it stands.
- `DeckBuilderWidget.h`, the `// ═══` TASK-1423 design-rationale run (*"bind the six IA_Menu\* actions … not smuggled in as a seventh binding"*): TASK-1423's own history. Its "seventh" is Remove's Delete / `Gamepad_FaceButton_Left`, not `IA_MenuSecondary`.
- `DeckBuilderWidget.cpp`, the `#include "InputAction.h"` trailer (*"TASK-1423: … the six IA_Menu\* assets"*): TASK-1423-attributed.
- `DeckBuilderWidget.cpp`, NativeConstruct's TASK-1423 note (*"the six IA_Menu\* actions TASK-1408 authored"*): true, since TASK-1408 authored six.

### L1.2 — The comment-only proof, re-run on the loop-1 bytes (`SC-§93` cl. 4, `SC-§92`)

**Instrument:** `lexcmp2.py` in my scratchpad (not in the repo). It is a second-generation instrument, not the
loop-0 `lexcmp.py`. It classifies every character after translation phase 2 as code, comment or literal, using a
state machine over `//`, `/* */`, string, char, raw-string and digit separators. From that it compares:
- the **code-line stream** (comments → one space, whitespace collapsed, blank lines dropped);
- the **ordered literal list**, verbatim;
- **per-line purity:** every line that differs, on both sides of a `difflib` line diff, must be pure comment or blank.

It also flags `STRAY_CLOSE_IN_CODE`, `NESTED_OPEN_IN_BLOCK`, unterminated literals and blocks, and `SPLICE`.
It runs against two baselines. The loop-0 snapshot isolates the loop-1 delta, and `HEAD` covers the whole row.

| File | code lines | literals | loop0 → tree: changed lines `+`/`−`, impure, issues | HEAD → tree: `+`/`−`, impure, issues |
|---|---|---|---|---|
| `SiegeControlsHelpWidget.cpp` | 2002 = 2002 | 610 = 610 | +15 / −11, 0, 0 | +70 / −10, 0, 0 |
| `SiegeControlsHelpWidget.h` | 258 = 258 | 47 = 47 | +0 / −0 | +3 / −0, 0, 0 |
| `SiegeMenuInputSubsystem.cpp` | 1073 = 1073 | 145 = 145 | +4 / −3, 0, 0 | +20 / −6, 0, 0 |
| `SiegeMenuInputSubsystem.h` | 123 = 123 | 10 = 10 | +4 / −2, 0, 0 | +12 / −2, 0, 0 |
| `Tests/SiegeMenuInputTest.cpp` | 464 = 464 | 140 = 140 | +0 / −0 | +9 / −2, 0, 0 |
| `DeckBuilderWidget.h` | 159 = 159 | 44 = 44 | +10 / −4, 0, 0 | +44 / −9, 0, 0 |
| `DeckBuilderWidget.cpp` | 1710 = 1710 | 199 = 199 | +1 / −1, 0, 0 | +14 / −4, 0, 0 |
| `SiegePlayerController.cpp` | 3963 = 3963 | 564 = 564 | +3 / −2, 0, 0 | +15 / −0, 0, 0 |

⇒ **Code-identical and literal-identical 8/8 against both baselines.** Every changed line is pure comment, and
there are 0 hazard flags. ⭐ **A cross-instrument check:** the code-line counts (2002/258/1073/123/464/159/1710/3963)
and literal counts (610/47/145/10/140/44/199/564) equal **QA's own lexer's** figures in `qa/TASK-1481.md` §0.3,
number for number. So two independent tokenisers agree. The `+`/`−` counts are `difflib` line counts and are
informational only; the proof is the equalities.

**The subsystem literal lists are identical (`.cpp` 145/145, `.h` 10/10). ⇒ no `TASK-1394` instrument string moved.**

**Negative controls.** Each one mutates an in-memory copy of the loop-1 tree and is never written. Each is caught
by at least one discriminator:

| Control | code same | literals same | impure lines | flags |
|---|---|---|---|---|
| C0 unmodified controller | True | True | 0 | none |
| C1 `\` after the (g) trip-wire's last line (the next line is code) | **False** | True | — (purity is not computed under a splice) | **SPLICE** |
| C2 one char inside the subsystem `.cpp`'s first `TEXT("…")` | **False** | **False** | 2 | none |
| C3 stray `*/` after a statement | **False** | True | 2 | **STRAY_CLOSE_IN_CODE** |
| C4 the B1 `SaveDeckAs` doc's `*/` deleted | **False** | **False** | 0 | **NESTED_OPEN_IN_BLOCK** |
| C5 `//` → `/` on N3's first new line | **False** | True | **1** | none |
| C6 a nested `/*` inside the W1 doc | True | True | 0 | **NESTED_OPEN_IN_BLOCK** (benign in C++; the flag works) |

**The comment block each loop-1 hunk sits in:**

| File @ loop-1 line | Block | Finding |
|---|---|---|
| `DeckBuilderWidget.h` @565 | the `// --- Card details` section `//` run | B1 (optional) |
| `DeckBuilderWidget.h` @637 | `/** … */` above `UFUNCTION(BlueprintCallable, …) void SaveDeckAs(const FString& Name);` | B1 |
| `DeckBuilderWidget.cpp` @2378 | the `//` run inside `RegisterAsMenuNavTarget`, above `MenuInput->RegisterSelfDrivingMenuNavTarget(this);` | N5 |
| `SiegeControlsHelpWidget.cpp` @182 | `/** … */` above `void ApplyButtonNotFocusable(UButton* Button)` | W1 |
| `SiegeControlsHelpWidget.cpp` @416 | the R-02 `//` run in `GetActions()`, above `Row.Detail =` | N1 |
| `SiegeMenuInputSubsystem.cpp` @883 | the FENCE (c) `//` run in `IsNavFocusStop`, above `if (const UButton* Button = …)` | N3 |
| `SiegeMenuInputSubsystem.h` @237 | the `USiegeMenuInputSubsystem` class doc `/** … */` (directly above `UCLASS(BlueprintType)`) | N4 |
| `SiegePlayerController.cpp` @2527 | the TASK-1482 "REGISTER ON OPEN" `//` run in `HandleMatchEnd`, above `if (UWorld* World = GetWorld())` | N2 |

**Hazard census over the loop-1 added lines:** none ends in `\` (SPLICE 0 in all eight files), none contains
`TEXT(`, and none starts with `#`. None contains `/*` or `*/`. The only `*` next to text is `IA_Menu*` followed by
a space, inside a `//` line. EOLs are preserved: the LF files have CR=0, and the CRLF files have CR=LF
(`DeckBuilderWidget.h` 1423/1423, `.cpp` 3364/3364). No BOM, and all files are valid UTF-8.

### L1.3 — What 5a (`TASK-1538`) must know, updated

- **The same four reflected blocks as loop 0, and no new one.** Loop 1 touches two of them again:
  - `SaveDeckAs` (B1) → `DeckBuilderWidget.gen.cpp`'s `SaveDeckAs` `Comment`/`ToolTip` changes;
  - the `USiegeMenuInputSubsystem` class doc (N4) → `SiegeMenuInputSubsystem.gen.cpp`'s class `Comment`/`ToolTip` changes.
- The "Card details" `//` run is **not** reflected (QA measured it). The other loop-1 hunks are in `.cpp` files.
- `SiegeControlsHelpWidget.h` is **byte-unchanged** since loop 0, so `SiegeControlsHelpWidget.gen.cpp` should still come out byte-unchanged.
- The **exec-symbol set** of both `.generated.h` files must be set-identical. Assert the set, never size or sha.
- No 5b.

### L1.4 — After-sha256, all eight files (loop-1 bytes = what `TASK-1538` compiles and `TASK-1540` commits)

| File (`Source/GitClaudeUnrealTest/Siegebound/…`) | after-sha256 (loop 1) | bytes | EOL | touched in loop 1? |
|---|---|---|---|---|
| `SiegeControlsHelpWidget.cpp` | `f407d0b4c1d1df474fe2c0303cbe2eed6cb9636fb33526ffc2afe308ade96137` | 273661 | LF | **yes** (W1, N1) |
| `SiegeControlsHelpWidget.h` | `cc16d2caade4dab2a4676724ba9a2596b1f455d115560afe35f259941012d174` | 80750 | LF | no, = `qa/TASK-1481.md` §4 |
| `SiegeMenuInputSubsystem.cpp` | `53050739542a4eb8df72223839089c66bdb907f356ab15ea0dbe1aa17eb6aa63` | 121682 | LF | **yes** (N3) |
| `SiegeMenuInputSubsystem.h` | `5bba1a035c40a39574223fe98f88130bf8fca9ded0385e27ecb4f81d1b68f2dc` | 85647 | LF | **yes** (N4) |
| `Tests/SiegeMenuInputTest.cpp` | `5470c88605442b63bdbe60126d37dc33fd30b2a1e6fada7cb1661a451d85797d` | 53148 | LF | no, = `qa/TASK-1481.md` §4 |
| `DeckBuilderWidget.h` | `950dc811bbebc4b345eaa50cebd5f814f47262da990215d1a581ca8cfb6b1385` | 85606 | CRLF 1423/1423 | **yes** (B1 + optional) |
| `DeckBuilderWidget.cpp` | `de406a0ccd1a0bc9b665372a339fd85e9a6ebf9180adf0e09a17d31cd938934e` | 165312 | CRLF 3364/3364 | **yes** (N5) |
| `SiegePlayerController.cpp` | `8f33a5de3c8e06fe39ef3a484756d93f40facd8c0cadaea81116d40078db7fb4` | 387770 | LF | **yes** (N2) |

`SiegePlayerController.h` was not touched (`840416b6f03bb436d90772caa5821c1455495917b8b8ed277c9119e543536d10`).
⛔ The §5 table above is **superseded** by this one for the six touched files.

### L1.5 — Loop-1 hunks (`diff -U0` of the loop-0 snapshot against the tree, `\r` removed for display from the two CRLF files)

```
### DeckBuilderWidget.h
@@ -565,2 +565,3 @@
-	// (⛔ TASK-1480 (j): no longer a UI gate — per DECK-§4(c)'s ruling the Play
-	// button's enable-gate was removed) are untouched by everything in this block.
+	// (⛔ TASK-1480 (j): it no longer gates Play — per DECK-§4(c)'s ruling the Play
+	// button's enable-gate was removed; deck activation is still gated, see
+	// SetActiveDeck's doc) are untouched by everything in this block.
@@ -636,2 +637,7 @@
-	 *  removed, so no UI surface gates on 50 cards — in the ruling's words the "n/50"
-	 *  text is the feedback and the match-side legality check is the enforcement.) Fires
+	 *  removed, so Play no longer gates on 50 cards, and per the same ruling the "n/50"
+	 *  text is the builder's feedback and the match-side legality check the enforcement.
+	 *  Deck ACTIVATION is gated, though: since TASK-1270 SetActiveDeck refuses to activate
+	 *  an illegal deck, and right-click on a deck-bar slot, or Home / pad Y while one holds
+	 *  focus, reaches it through SetActiveDeckBySlot; that gate postdates the ruling's
+	 *  "no UI surface hard-enforces the 50-card rule". Scoped to Play at TASK-1480 QA
+	 *  loop 1, qa/TASK-1481.md B1.) Fires
### DeckBuilderWidget.cpp
@@ -2378 +2378 @@
-	// stop 0 at registration. The six IA_Menu* bindings BindMenuNavActions() made
+	// stop 0 at registration. The seven IA_Menu* bindings BindMenuNavActions() made
### SiegeControlsHelpWidget.cpp
@@ -182,9 +182,12 @@
-	 *  builds goes through `ApplyButtonFocusable` below. ⛔ IT STAYS BECAUSE THE TWO TRIP-WIRES
-	 *  NAME IT AS THE REVERT: the one at `RowButton`'s flip in
-	 *  `USiegeControlsHelpRowWidget::ConstructRowTree` (it opens "TRIP-WIRE — ⛔ READ
-	 *  `USiegeControlsHelpWidget::ApplyActiveView` BEFORE TOUCHING THIS LINE" and ends "THIS WRITE
-	 *  MUST GO BACK TO `ApplyButtonNotFocusable` ⛔ IN THE SAME DIFF"), and the one at
-	 *  `BackButton`'s flip in `USiegeControlsDetailWidget::ConstructDetailTree` (it opens "THE
-	 *  TRIP-WIRE (`qa/TASK-1433.md` WARN-L1)"). ⛔ Do NOT delete it or fence it off as dead code:
-	 *  either is an executable change, and the revert those wires prescribe would have nothing
-	 *  left to call.
+	 *  builds goes through `ApplyButtonFocusable` below. ⛔ IT STAYS BECAUSE A TRIP-WIRE NAMES IT
+	 *  AS THE REVERT: the one at `RowButton`'s flip in `USiegeControlsHelpRowWidget::ConstructRowTree`
+	 *  (it opens "TRIP-WIRE — ⛔ READ `USiegeControlsHelpWidget::ApplyActiveView` BEFORE TOUCHING
+	 *  THIS LINE" and ends "THIS WRITE MUST GO BACK TO `ApplyButtonNotFocusable` ⛔ IN THE SAME
+	 *  DIFF"; both quotes span a line break in the source). ⛔ IT IS THE ONLY ONE THAT DOES:
+	 *  `BackButton`'s flip in `USiegeControlsDetailWidget::ConstructDetailTree` carries a
+	 *  trip-wire for the SAME collapse (it opens "THE TRIP-WIRE (`qa/TASK-1433.md` WARN-L1)") but
+	 *  names no revert and never mentions this helper, and the TASK-1478 switcher block above
+	 *  `USiegeControlsHelpWidget::ApplyActiveView` cites this comment only as a warning, in its
+	 *  rejected (γ) (TASK-1480 QA loop 1, 2026-09-27, `qa/TASK-1481.md` W1). ⛔ Do NOT delete it
+	 *  or fence it off as dead code: either is an executable change, and the revert the
+	 *  `RowButton` wire prescribes would have nothing left to call.
@@ -413,2 +416,3 @@
-			//   the Negate_2 binding = handoffs/TASK-568-artist.md, its readback row
-			//   "6 Mouse2D  IA_Look  modifiers: [InputModifierNegate_2]", and
+			//   the Negate_2 binding = handoffs/TASK-568-artist.md, its readback row naming `IA_Look`
+			//   and `modifiers: [InputModifierNegate_2]` (the row pads its columns with runs of
+			//   spaces, so grep either fragment, not the whole row), and
### SiegeMenuInputSubsystem.cpp
@@ -883,3 +883,4 @@
-	// `:2240` / `:2787` set it on the two `UScrollBox`es (which this walker never admits anyway,
-	// a `UScrollBox` not being one of the four classes). Overriding an author's opt-out would be
-	// a regression wearing a widening's clothes.
+	// `:2240` / `:2787` (⛔ WAS — `SiegeControlsHelpWidget.cpp` line numbers, cited so until
+	// TASK-1480; the by-text anchors are in the (m) note below) set it on the two `UScrollBox`es
+	// (which this walker never admits anyway, a `UScrollBox` not being one of the four classes).
+	// Overriding an author's opt-out would be a regression wearing a widening's clothes.
### SiegeMenuInputSubsystem.h
@@ -237,2 +237,4 @@
- *  and no `UButton` in `Source/` is opted out. The rule is kept because an author's opt-out, in
- *  C++ or in an asset, must still win when one appears).
+ *  and no `UButton` in `Source/` is opted out outside the test fixture (whose `OptedOutButton`
+ *  in `Tests/SiegeMenuInputTest.cpp` is authored `IsFocusable = false` to pin this rule). The
+ *  rule is kept because an author's opt-out, in C++ or in an asset, must still win when one
+ *  appears).
### SiegePlayerController.cpp
@@ -2527,2 +2527,3 @@
-	// idempotence guard in USiegeMenuInputSubsystem::FocusFirstNavStop (the "is ANYTHING among the
-	// stops already focused?" loop just above its `return FocusWidget(Stops[0]);`) falls through,
+	// idempotence guard in USiegeMenuInputSubsystem::FocusFirstNavStop (the loop over the stops
+	// that, in its own comment's words, asks "is ANYTHING already focused?", just above its
+	// `return FocusWidget(Stops[0]);`) falls through,
```

(`SiegeControlsHelpWidget.h` and `Tests/SiegeMenuInputTest.cpp`: no loop-1 hunk.)

### L1.6 — What QA should scrutinize in the re-gate

1. **B1's new sentence is the one claim of substance.** Every link in the activation chain it names was read at
   source (L1.1). It still says nothing about what `WBP_DeckBuilder`'s graph reads. "Play no longer gates on 50
   cards" rests on the ruling and on QA's own §0.5 asset read (`OnPlayClicked` → `StartMatch`). The
   match-side clause is attributed to the ruling ("per the same ruling"), not asserted.
2. **N5 was taken on a fence reading** (file-scoped fence (5) and `names: WRITES`), not on (h)'s item text. If
   that reading is ruled wrong, the revert is one comment word.
3. **The four "six" sites left unchanged** (L1.1, below the table): each is either qualified in its own block or
   attributed history. Tell me if you read any of them as a present-tense false count.

## Not examined / limitations (loop 1)

- ⛔ **No compile, no suite, no PIE.** "Comment-only" is a lexical proof (L1.2), agreeing with QA's independent
  lexer on every count. `TASK-1538` owns the compile.
- ⛔ **`WBP_DeckBuilder` was still not opened by me.** B1's Play clause leans on `DECK-§4(c)` plus
  `qa/TASK-1481.md` §0.5's strand read. It is not a whole-graph census.
- ⚠️ **Declared for the manager, not taken (`CONVENTIONS.md` is fenced):** `DECK-§4(c)`'s *"Named consequence:
  no UI surface hard-enforces the 50-card rule anymore"* (2026-08-27) has been overtaken on activation since
  `TASK-1270`. `DECK-§3`'s TASK-1507 bullet in the same file already says so (*"an illegal deck is refused exactly
  as right-click refuses it (`TASK-1270`)"*). The two sections of the law now disagree, and that is how B1's
  wording was imported. A dated note on `DECK-§4(c)` would stop the next reader importing it again.
- ⚠️ **Whether an ACTIVE deck can become illegal after activation** (for example, by editing and auto-saving the
  active slot) was not examined. That is why B1 says the activation gate "postdates" the ruling's sentence
  rather than calling the sentence false outright.
- ⚠️ Everything in loop 0's *Not examined* list still stands. That includes the three rotted cites outside every
  item and `TASK-1468`'s site, which is still untouched.
