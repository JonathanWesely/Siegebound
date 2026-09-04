# QA Report — TASK-822 (Lane A gate: `TASK-819` + `TASK-821`)

**Verdict: PASS** — per task: **TASK-819 = PASS**, **TASK-821 = PASS**.
**Blockers: 0** · WARN: 5 · NIT: 4 · Rulings issued: 4
**Reviewer:** qa-reviewer · **Date:** 2026-09-02 · ⛔ no code edited, ⛔ no engine, ⛔ no Git, ⛔ no compile.

---

## 0. ⛔ `SC-§29` COVERAGE LEDGER — **THE VALVE FIRED, AND HERE IS WHICH WAY**

| task | in this gate? | why |
|---|---|---|
| **TASK-819** (discard-all mechanic) | ✅ **COVERED — PASS** | named by the row; `ready-for-qa` |
| **TASK-821** (controls menu, card half) | ✅ **COVERED — PASS** | named by the row; `ready-for-qa` |
| **TASK-844** (bot discard parity) | ⛔ **OUT OF SCOPE — THE CONDITIONAL VALVE FIRED** | The row's pre-decided conditional: *"if `819`+`821` are ready and `844` has NOT started, `844` DROPS OUT of this gate and takes `TASK-845`/`TASK-846` instead."* **Condition met and stated explicitly here as the row demands:** `819` and `821` both reached `ready-for-qa`; `844` was **never dispatched**. ⇒ ⛔ **I did not review the bot's discard, its trigger, or the `20`-in-two-places question. `TASK-845` owns them.** |
| **TASK-807** | ⛔ not re-gated | `TASK-810`'s, per the row |
| **TASK-815** (placement wheel) | ⛔ **NOT GATED HERE** | `TASK-816`'s. Its code sits **in the same two files** — see build-master note B-1. I read around it, never through it. |

⛔ **This gate covers `TASK-819` + `TASK-821` and nothing else.** Union check against the Lane A roster: `809` (art, backlog), `811` (ship), `824` (compile) carry no code gated here.

---

## 1. TASK-819 — the discard-all mechanic. **PASS**

### 1a. ⭐ ONE ROUTE, ⛔ ZERO DEAD SURFACE — **re-measured, not accepted**

| claim | my measurement | verdict |
|---|---|---|
| `DiscardEntireHand();` at **exactly 1** call site | `SiegePlayerController.cpp:1340` — the only invocation in the tree. Every other hit is the definition (`:1187`), a `UE_LOG` format string, or a comment. No other production file names it. | ✅ |
| `OnDiscardAllPressed` carries **0** duplicated guards | `:1333-1341` — one statement. ⛔ no `HasAuthority`, ⛔ no `bInPlacementMode`, ⛔ no `SpendGold`, ⛔ no `DiscardFromHand`. | ✅ |
| `CardHandWidget.{h,cpp}`: **0** `NativeOnMouseButtonDown` · **0** `RequestDiscardAll` · **0** `DiscardEntireHand` | Grepped both files directly: **0 / 0 / 0**. Not declared, not stubbed, not commented-out, not an unused `UFUNCTION`. | ✅ |
| the retired symbols survive | `DiscardHandSlot` (`.h:544-545`, `.cpp:1075+`), `DiscardCost` (`.h:1566-1567`), `UCardHandWidget::RequestDiscardSlot` (`.h:110-111` **`UFUNCTION(BlueprintCallable)` intact**, `.cpp:108`) | ✅ |

⇒ **gate row (b) — inverted and stricter — is SATISFIED: neither symbol exists in any form.**

### 1b. ⛔⛔ THE GOLD MOVES **ONCE** — all three instruments checked, including whether the control can fail

- **The code** (`:1281-1317`): one `SiegeState->SpendGold(DiscardAllCost)`, then a loop of `DeckComponent->DiscardFromHand(Slot)` over an occupied-slot list collected **once, before** the charge (`:1252-1261`). The empty-hand refusal (`:1263-1270`) precedes the charge. The gold-leak tripwire survives (`:1311-1314`). One sound, gated on `DiscardedCount > 0` (`:1319-1325`). One summary log.
- **The source probe (test 21)**: I re-read the assertions and the scanner. `CountOccurrencesInCode` (`SiegePlacementTest.cpp:945-985`) skips whole comment lines only; the self-check at `:2134` proves `DiscardCost` **is** present in the body's prose, so the zero on code lines is the skipper working. The receiver-named tokens (`SiegeState->SpendGold(`, `DeckComponent->DiscardFromHand(`) genuinely exclude the eight `UE_LOG` format strings, which do sit on code lines. **The ladder order chain matches the shipped body character-for-character.**
- **⭐ THE NEGATIVE CONTROL ACTUALLY DISCRIMINATES — the thing I was asked to check.** Test 19(d) (`:1889-1912`) builds the loop-bug charge (`DiscardAllCost + 6 × DiscardCost`) on a second fixture and asserts `BuggySpend != DiscardAllCost`. **That assertion goes RED if the discrimination is lost** — i.e. if `DiscardCost` were ever aliased to `0`, the control announces itself rather than silently ceasing to discriminate. It is reinforced from the other side by test 18(b), which asserts `DiscardCost > 0` **and says why** (*"required for test 19's loop-bug control to discriminate at all"*). ✅ **This is a real control, not a decorative one.**
- The fixture cannot pass trivially: full-hand control asserted first (`:1821-1838`), six **distinct** cards, `HandSize + 1` spares so the §3.4 eager reshuffle cannot fire mid-loop (verified against `DeckComponent.cpp:242-260` — after the 6th pop the draw pile still holds 1, so no reshuffle), and a "not one binned card came back" measurement.

### 1c. ⚖️ RULING 1 — **THE LODGER: RATIFIED** (`SC-§15` declared departure)

The discard tests live in `Tests/SiegePlacementTest.cpp` (tests 18–23, namespace `SiegeDiscardAllFixture`), declared in the file charter at `:55-59` and again at `:1497-1501`. **I ratify it**, for the reasons the author gave and one he could not have known:

1. the dispatch **fenced** him to that file — writing elsewhere would have been the undeclared departure;
2. the instrument his sharpest claims need (`LoadProjectSource` / `CountOccurrencesInCode`) already lives there and cloning it would have created a second definition of "code line";
3. **it is namespaced cleanly** — I checked every helper in `SiegeDiscardAllFixture` against `SiegePlacementTestFixture`, `SiegePlacementUpgradeFixture` and `SiegePlacementWheelFixture`: **zero name collisions**, so the four tests that pull two namespaces in with `using namespace` (`:2106-2107`, `:2263-2264`, `:2749-2750`, `:2843-2844`) are unambiguous. That was a genuine compile risk and it is clear.

⚠️ **Ratified as PATH (`SC-§29b`):** `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegePlacementTest.cpp`.
🧑 **But the re-homing is no longer free — see NIT-7.**

### 1d. ⚖️ RULING 2 — **THE BEHAVIOURAL/PROBE SPLIT: SUFFICIENT, and I am naming its exact limit**

The tests deliberately never call `DiscardEntireHand` (its success path reaches `PlaySound2D`, which resolves a world); tests 19/20 re-enact the **same two shipped APIs in the same order** on world-free scratch objects, and test 21 proves the shipped body has that shape.

✅ **RULED SUFFICIENT.** The pair closes because the two halves cover each other's exact gap: 21 proves *shape without behaviour*, 19/20 prove *behaviour without shape*, and the tokens 21 counts are the very calls 19/20 make. Refusing it would demand a possessed-controller world fixture this suite has never had, to buy a property the pair already delivers.

⛔ **THE LIMIT, STATED SO NOBODY READS GREEN AS DONE (`SC-§32`):** the pair is **coupled to a source-text probe**. A behavioural regression inside `DiscardEntireHand` that keeps the token shape (e.g. reordering statements *within* one guard block, or passing a wrong slot to `DiscardFromHand`) would be caught by **neither** half. The order chain and the receiver-named counts make that window narrow, and `TASK-811`'s `H` pixel row is what actually closes it — ⛔ **so that pixel row is REQUIRED, not optional** (819 §8 asks for it in the right shape: full hand → `H` → **exactly `DiscardAllCost` leaves, not 26** → six new cards).

Also observed and accepted: the observer-lockout branch is a **diff read**, correctly declared (`HasAuthority()` is true by construction on a world-free actor).

### 1e. ⛔ `CARDBAR-§7` — the inverse trap, verified independently of the tests

- **`EKeys::H` on a code line of `SiegePlayerController.{h,cpp}`: 0.** Every occurrence in the tree is prose (`.cpp:643`, `.h:1875`, `.h:1878`) except `SiegeControlsHelpWidget.cpp:511`, which is the **law-mandated `QwertyReferenceKeys` fallback**, and the shipped 26-letter table (`SiegeKeyboardLayoutStatics.cpp:58`).
- **`GetPositionalKey` calls in the controller: 0** (`.cpp:608`, `:625`, `:643`, `:6393` are all comments forbidding it).
- The action is soft-resolved once (`:515`) and bound once on `ETriggerEvent::Started` (`:650-652`), inside the `if (DiscardAllAction)` null guard ⇒ **a missing asset leaves the key inert, never a crash.**
- ⛔ **Zero conditional layout logic. No `if (bIsDigit)`, no per-key case, in either file.**

### 1f. UE correctness / safety sweep (my own, not the handoff's)

- `DiscardAllCost`: `UPROPERTY(EditDefaultsOnly, meta=(ClampMin="0"))`, `int32`, not replicated ✅. `DiscardAllAction`: `UPROPERTY` + `TObjectPtr<UInputAction>` ⇒ GC-safe ✅. `DiscardAllActionAsset`: `UPROPERTY(EditDefaultsOnly)` `TSoftObjectPtr` ✅. `DiscardEntireHand`: `UFUNCTION(BlueprintCallable, Category="Siegebound|Cards")`, header/cpp signatures agree ✅. `OnDiscardAllPressed`: private, no `UFUNCTION` needed (bound by member-pointer) ✅.
- Null-safety: `DeckComponent` checked before use; `GetPlayerState<ASiegePlayerState>()` checked before the charge; `GetNameSafe(this)` throughout; `PlaySound2D` is the shipped null-safe wrapper. **No unguarded pointer on the path.**
- No deprecated/removed UE 5.8 API in the diff surface. `EAutomationTestFlags::EditorContext | EngineFilter`, `FindFProperty`, `ContainerPtrToValuePtr`, `TStrongObjectPtr`, `FChar::IsDigit`, `ParseIntoArrayLines` all current and already idiomatic in this suite.
- No per-tick work added. `PlayerTick` is untouched; the feature is entirely event-driven off one bound action.
- **No refusal string was reworded** — measured, not accepted: every `CardRefused_CantAfford` in the file is character-identical `"Not enough gold"` (11 sites), `CardRefused_EmptySlot` is identical at both sites (`:971`, `:1144`), `DiscardRefused_Placing` / `DiscardRefused_Targeting` identical at both sites each. **`DiscardAllRefused_EmptyHand` ("No cards to discard") is the ONE new key**, and the justification holds: `CardRefused_EmptySlot` says *"No card in that hand slot"*, which is about one slot and would be wrong here.
- **Interaction with `TASK-815` (asked, and answered rather than silently widened):** the wheel and the discard-all share `SiegePlayerController.{h,cpp}` and `SiegePlacementTest.cpp` but are **disjoint by symbol**. The only shared state is `bInPlacementMode` / `bInTargetingMode`, which `DiscardEntireHand` **only reads** (to refuse). The wheel lives in the `PlayerTick` placement branch, which this diff does not touch, and pressing `H` mid-placement is refused before anything moves. ⇒ **no behavioural interaction.** The *packaging* interaction is real and is build-master note **B-1**.

---

## 2. TASK-821 — the controls menu, card half. **PASS**

### 2a. ⛔⛔ THE ROW WAS REWRITTEN IN PLACE — and the automatic-fail line was NOT touched

- `AddRow(TEXT("Cards.Discard"), …)` at `SiegeControlsHelpWidget.cpp:508` — **id kept**, lane `MappedAction`, `Actions = { IA_DiscardAll }`, `QwertyReferenceKeys = { EKeys::H }` (fallback only), `bPointerOnly` line **deleted** (struct default `false`, `SiegeControlsHelpWidget.h:210-211`).
- **`RequiredIds[]` is INTACT.** It lists `TEXT("Cards.Discard")` alongside `Cards.Play` / `Cards.CursorHold` / `Cards.Cancel`, and the surrounding assertion is unchanged: a **required subset**, asserted by `TestNotNull` in a loop. ⛔ **No deletion was made to pass, and no test above test 14 was weakened.**
  ⚠️ **Line-number correction for the record (NIT-8): as shipped that array is at `Tests/SiegeControlsHelpTest.cpp:243-255`, not `:227`.** The spec's `:227` is a pre-edit cite; `821`'s own include (+6) and file-charter note pushed the array down. **Its CONTENT is untouched — the automatic-fail condition did not occur.**
- **Registry still 24 rows**, no duplicate id, ⛔ no tower/stack/wheel row leaked into Lane A (`TASK-823`'s three are absent).
- ⛔ **`Cards.Play` and `Cards.Cancel` untouched** — verified structurally: exactly one `AddRow` block in the Cards category changed, and `Cards.Cancel`'s row (`:571`) still teaches *"right-click or Escape cancels"*, which is true again.

### 2b. ⛔ ALL THREE SHIPPED FALSEHOODS ARE DEAD

| falsehood | state now |
|---|---|
| the one-liner *"Click a hand card's discard button…"* | ⛔ **gone** — replaced by *"Bin every card in your hand at once and draw a full replacement — one flat fee, however many cards you were holding."* |
| the `PointerOnly` chip (*"Mouse click"*) | ⛔ **gone** — lane flipped and `bPointerOnly` deleted; `ComposeKeyChipLabel:1274-1277` only returns the pointer chip for `bPointerOnly \|\| Lane == PointerOnly`, so the chip is now the derived key |
| the code comment *"no key binding exists"* | ⛔ **gone** — replaced; one does exist (`SiegePlayerController.cpp:650-652`) |

⚠️ All three survive **as quotations inside the replacement comment** (`:489-497`). That is prose in a `//` block, ⛔ not shipped data, and it is the correct way to leave a record. It does mean a naive `grep` of the file hits them — flagged here so the next reader does not misread it (and note it is exactly the trailing/whole-line comment trap of NIT-9).

⇒ **gate row (e) SATISFIED: the page mentions right-click NOWHERE**, and the Alt-cursor caveat went with it — asserted **as data** (test 14(f) at `:1847-1848`: `Cards.CursorHold` is gone from this row's `RelatedActionIds`), not as a substring gamble.

### 2c. ⛔ THE FEE IS NAMED, ⛔ NEVER TYPED

I read both strings character by character. **The one-liner and the detail page contain no digit character at all**, and the prose names `DiscardAllCost`. Test 14(e) asserts it with a **scanner self-check first** (`:1824-1825` — the scanner is proven able to find a digit in `"a fee of 20 gold"` before the two `TestFalse`s mean anything), plus a direct claim that the retired vocabulary (`DiscardCost` / `DiscardHandSlot` / `RequestDiscardSlot`) is absent — a real independent claim, since `"DiscardAllCost"` does **not** contain `"DiscardCost"`. ⇒ **no automatic fail.** (Readability caveat: WARN-3.)

### 2d. ⭐⭐ ITEM (5) — TEST 14 DISCRIMINATES. This is the one I pushed hardest on.

- **(a) the lane flip is asserted as the precondition it is** — and the reasoning checks out at source: on `PointerOnly`, `ComposeKeyChipLabel` (`:1274-1277`) answers the pointer chip **regardless of the keys handed to it**, so the "changes" half would be *unreachable*, not merely red. `Cards.Play` is asserted onto the **same lane**, which is the one-code-path claim.
- **(b) ⭐ THE DIGITS' IMMUNITY IS READ FROM THE SHIPPED 26-LETTER TABLE, NOT FROM THE FIXTURE'S SILENCE** — exactly as the brief demanded. It calls `FSiegeKeyboardLayoutStatics::GetQwertyLetterScanCodes()` (`:1682`) and asserts the discard reference key **is** in it while **each of** `Cards.Play`'s six reference keys is **not**. ⭐ **And that loop is not vacuous: `Cards.Play` genuinely carries six keys** (`SiegeControlsHelpWidget.cpp:428` — `EKeys::One…Six`), which I checked precisely because an empty array would have made the digit half silently empty.
- **(c) the fixture self-check has BOTH hops** — `H → D → E` is three distinct keys (`D → E` was already two lines above the new entry), so a double translation is *detectable*. The `H → D` entry is taken from the shipped table at `Tests/SiegeKeyboardLayoutTest.cpp:211`, not re-derived ✅.
- **(d) `SC-§37` is satisfied — there is no transcription anywhere.** The Dvorak chip is asserted `==` **`GetPositionalKey`'s own one-hop answer** and `!=` the two-hop answer; the QWERTY chip is asserted against the accessor's answer on an empty map. ⛔ **Nowhere does it assert `== "H"` or `== "D"`.** The pair is then asserted **as a pair in one expression** (`:1764-1765`).
- **the two declared vacuity traps are genuinely dead:** the page claim is now *"the raw prose carries the `{Cards.Discard}` token"* (`:1784-1786`) rather than `Contains("D")`, and the digit self-check runs on a synthetic string rather than on `Cards.Play`'s prose. **Both replacements can fail; the originals could not.**
- I verified the mechanism the test rests on: `ResolveRowDisplayKeys` Lane-A fallback (`:1258-1268`) applies `GetPositionalKey` exactly once, and `ResolveDetailTokens` (`:1366-1404`) splices the token with that same chip. **One resolver, two opposite outcomes, zero branches.**
- Shared-fixture blast radius checked: adding `H → D` to `MakeDvorakTranslation()` touches no other test — tests 2/3/4/8/10 use `F/T/C/M/A/Tab/Z/SpaceBar`, and test 8's `ChangedChips > 0 / HeldChips > 0` census still stands on the two surviving `PointerOnly` rows (`Interface.WarMapReveal`, `Interface.WarMapMarker`) plus the digit and mouse rows.

### 2e. ⚖️ RULING 3 — the `Cards.CursorHold` → `Cards.Cancel` swap: **CONCUR, and the edge RESOLVES**

The manager already ruled this acceptable in advance (`TASK-822` (d2), now `HELP-§7`). I record my concurrence rather than re-adjudicating: it is **outbound from the row it owns**, it was **declared**, and it is **substantively right** — the Alt-cursor caveat died with right-click. ✅ **And I verified the new edge is not dangling: `Cards.Cancel` is a real row (`SiegeControlsHelpWidget.cpp:571`).**

⛔ **And per (d1): I did NOT fail `TASK-821` for the absence of the orphan it correctly reported does not exist.** I re-measured its finding independently and confirm it: **all 13 `RelatedActionIds` assignments in the file resolve to real rows, and NOT ONE names `Cards.Discard`** — including `Cards.CursorHold`, which has no `RelatedActionIds` at all. Its conclusion was right and the row was correctly rewritten in place.

---

## 3. ⚖️ RULING 4 — ITEM (d3): **YES. THE `RelatedActionIds` TRIPWIRE EARNS A PERMANENT TEST. BOARD IT.**

**Current state, measured today (so the manager boards it with evidence, not a worry): all 13 edges resolve. ZERO are dangling.** I am still ruling YES, and the reasons are not "something is broken":

1. ⛔ **The failure is silent BY CONSTRUCTION, and I read the line that makes it so.** `ComposeDetailContent` (`SiegeControlsHelpWidget.cpp:1454-1460`) does `FindAction(RelatedId)` and, on null, **`continue`s with no `UE_LOG` at all**. A renamed row does not warn, does not render a gap, does not appear in any log — the related block simply stops existing. That is precisely `SC-§37`'s condition: *wrongness invisible in a diff, invisible at review, invisible on screen*.
2. ⭐ **The next authoring event is already boarded.** `TASK-823` adds three rows (tower/stack/wheel + the missing map-marks row) to this registry and will author edges into it. The cheapest moment to own the graph is before it grows, not after.
3. ⚖️ **This project has already paid for the un-measured graph once, and the receipt is in the law itself:** `CARDBAR-§9` built a rule on an inbound edge (`Cards.CursorHold` → `Cards.Discard`) that **never existed**, and it took a programmer hand-reading 13 assignments mid-task to find out. `HELP-§7` says it plainly — *"an unvalidated graph will also be an un-measured one, and laws will get written about edges that were never there."* **A ten-line test answers in one run what cost a hand audit.**
4. ✅ **It is cheap and total:** for every row, for every id in `RelatedActionIds`, `FindAction(id) != nullptr`. `RequiredIds[]` cannot cover it — it asserts a required subset of **rows**, never **edges**.

**Recommended shape (⛔ I did not write it — the manager boards it):** one test in `Tests/SiegeControlsHelpTest.cpp` asserting (a) every id in every row's `RelatedActionIds` resolves to a real row, naming the offending `row → id` pair in the failure message; and (b) a **fixture self-check** proving the resolver answers `nullptr` for a deliberately bogus id, so a "no dangling edges" green is the check working rather than a lookup that always succeeds. Optional (b2): no row lists itself — the composer already skips a self-edge (`:1449`), so a self-edge is silent authoring noise today.

---

## 4. Findings

- **[WARN-1]** `SiegeControlsHelpWidget.cpp:518-531` (and the same table in `handoffs/TASK-821-programmer.md` §2) — **every `SiegePlayerController.cpp` citation is now STALE by +15 lines, and the header fee-property cite by +90.** Measured pairs (cited → actual): the single `SpendGold` `:1269` → **`:1284`** · the loop `:1287-1302` → **`:1302-1317`** · the occupied-slot scan `:1237-1246` → **`:1252-1261`** · the empty-hand refusal `:1248-1255` → **`:1263-1270`** · placing/targeting `:1202-1220` → **`:1217-1235`** · the fee property `SiegePlayerController.h:1479-1496` → **`:1569-1586`**. The uniform offsets are consistent with `TASK-815` landing in that file **after** `821` read it (`SiegePlayerController.h:547-585` and `DeckComponent.cpp:206-240`/`:231-232`, which 815 did not move, are still **exact**). ⇒ **This is line-cite rot, ⛔ not a wrong claim: I re-verified every sentence of the page against the CURRENT source and all of them are ACCURATE.** *Fix:* re-anchor at integration, or prefer symbol/function cites over line cites in comments — under parallel lanes a line number is stale before the ink dries (`TL-§5b`'s lesson applied to citations).
- **[WARN-2]** `SiegeControlsHelpWidget.cpp:546-547` — the detail page states *"there is no longer any way to bin one card on its own at any price."* **That is FALSE until `TASK-809` lands, and `TASK-809` is `backlog`.** The six per-slot discard buttons still ship in `WBP_CardHand`, and `UCardHandWidget::RequestDiscardSlot` → `ASiegePlayerController::DiscardHandSlot` is **fully live** (retired ≠ removed — correctly so). ⛔ **Not `TASK-821`'s defect** — the board ordered exactly this text and `CARDBAR-§9` names `TASK-809` as the remover — but `CARDBAR-§9`'s own rule is *"each half ships in the SAME COMMIT as the behaviour it documents."* ⇒ *Fix (build-master, `TASK-811`):* land `TASK-809`'s button removal **in the same commit** as this row, or knowingly accept and record a window in which the help page carries one false sentence.
- **[WARN-3]** `SiegeControlsHelpWidget.cpp:544` — *"The fee is DiscardAllCost…"* puts a **C++ identifier in player-facing prose.** It is exactly what the spec demanded (named, never typed; zero digits) and it is `HELP-§2`-correct, so ⛔ **not a blocker and ⛔ not to be "fixed" without Jonathan's word** — but a player reads a symbol where a number belongs, and *"it reads well"* is his call under `HELP-§6`. ⭐ *The shape that would satisfy both:* a value token resolved from the controller CDO at compose time (the `{ActionId}` mechanism already proves the pattern) — the number appears on screen and is still **never typed**. Flagging for his sitting, not boarding it.
- **[WARN-4]** `SiegeControlsHelpWidget.cpp:549` — the prose quotes the shipped refusal *"Not enough gold"* as a **second copy of a shipped string**, and nothing asserts the two agree. Reword `CardRefused_CantAfford` and this page rots silently — the exact class `HELP-§2` exists to prevent. *Fix:* a 3-line tripwire asserting the page's prose contains the shipped `NSLOCTEXT`'s text (cheap; fold into the `HELP-§7` task if it is boarded).
- **[WARN-5]** `Tests/SiegePlacementTest.cpp` test 20 (`:1926+`) — unlike test 19 (`:1801-1802`) it does **not** assert `HasAuthority()` as an explicit precondition, though it leans on the same authority-gated `ASiegePlayerState::SpendGold` (`SiegePlayerState.cpp:118-136`). If the default actor role ever changes, 20 reports a phantom economy failure instead of naming its cause. *Fix:* clone 19's precondition line.
- **[NIT-6]** `CardHandWidget.h:104-111` — `RequestDiscardSlot`'s **retirement comment is still owed** (the doc block describes the old 1-gold behaviour with no retirement note). Correctly declared by `TASK-819` as outside its fence and board-tracked; test 23(e) enforces the symbol's **existence** but not the prose. Falls to whoever next edits that file — ⚠️ `TASK-809` is a WBP/art task and will **not** pay it.
- **[NIT-7]** **The lodger's re-homing is no longer free, and this changes the future decision.** `TASK-815`'s tests 27/28 now consume `SiegeDiscardAllFixture` (`Tests/SiegePlacementTest.cpp:2750`, `:2844`) for `ExtractControllerFunctionBody` / `CodeLinesOnly` / `CheckPrecedes`. A future `SiegeCardDiscardTest.cpp` must therefore either leave those helpers behind in the placement frame or move them with a second consumer in mind. **Recorded so the tidy-up is a decision, not a surprise.**
- **[NIT-8]** `TASK-822`/`CARDBAR-§9` cite `Tests/SiegeControlsHelpTest.cpp:227` for `RequiredIds[]`; as shipped it is at **`:243-255`**. **The array's content is untouched** — this is a stale cite in the law, not an edit. Worth correcting in `CARDBAR-§9` so the next reader checking the automatic-fail condition looks at the right line.
- **[NIT-9]** `Tests/SiegePlacementTest.cpp:945-985` — `CountOccurrencesInCode` skips **whole comment lines only**; a warning appended to the **end of a code line** is counted as code. `TASK-819` was bitten by this and documented it (§5.8) rather than "fixing" the shared scanner, which was the right call. ⚠️ **`TASK-823` will write probes of the same shape and will hit the same trap** — worth carrying into its spec.

---

## 5. Notes for build-master (`TASK-811` / `TASK-824`)

**B-1 ⛔⛔ THE PACKAGING FACT THAT OUTRANKS EVERYTHING ELSE HERE — `SC-§29` on the union, not the members.**
`Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.{h,cpp}` and `Tests/SiegePlacementTest.cpp` each contain **`TASK-819`'s work AND `TASK-815`'s wheel** (tests 24–28). `TASK-815` is gated by **`TASK-816`, not by me.** ⇒ ⛔ **A Lane A commit touching those files ships `TASK-815`'s code, and the hard gate (*"nothing is committed without a PASS QA report"*) means it may not land until `TASK-816` PASSES.** ⛔ **Do not read this report as clearing those files — it clears the discard-all symbols inside them.**

**B-2 SUITE CENSUS — FRESH, SCOPED, WITH ITS FILE COUNT, AND IT RECONCILES (`TL-§5b`).**
> **`376` across `29` files** — pattern `^IMPLEMENT_SIMPLE_AUTOMATION_TEST`, scope `Source/GitClaudeUnrealTest/Siegebound/Tests/*.cpp`, taken at review time.

Reconciliation, by **deltas only**: `371/29` (TASK-821's reading, which already contained 819's +6) **+ 5** (`TASK-815`'s tests 24–28, an out-of-scope lane) **= 376** ✅. Per-file check: `SiegePlacementTest.cpp` = **28** = 17 base + **6 (819)** + 5 (815); `SiegeControlsHelpTest.cpp` = **14** = 13 + **1 (821)**. **Both declared deltas are exact. No new test file; the file count is unchanged at 29, so no undeclared file landed.** ⛔ **Take your own fresh census anyway — 376 will be stale.** ⚠️ Use the full pattern: a bare `^IMPLEMENT_` also catches `IMPLEMENT_PRIMARY_GAME_MODULE` and reports `377/30`.

**B-3 Commit paths ratified by this gate** (`SC-§29b` — derived, stated as paths):
`Source/GitClaudeUnrealTest/Siegebound/SiegePlayerController.h` · `…/SiegePlayerController.cpp` · `…/Tests/SiegePlacementTest.cpp` (⚠️ **B-1**) · `…/SiegeControlsHelpWidget.cpp` · `…/Tests/SiegeControlsHelpTest.cpp`.
⛔ **No other path is ratified. `CardHandWidget.{h,cpp}` must appear in NO diff from these two tasks** — a clean file there is the expected state and part of the verdict.

**B-4 The pixel rows that actually close this batch** (nothing in the suite can):
1. **`H` row, REQUIRED** — full hand → `H` → **exactly `DiscardAllCost` (20) leaves, ⛔ NOT 26** → six new cards. The `26` is the loop bug and the eye is the only instrument that sees it in-game.
2. empty-ish hand → `H` → refusal line, **gold unchanged**.
3. `H` while a ghost is up → *"Cannot discard while placing a card"*, gold unchanged.
4. `TAB` → *"Discard your whole hand"* under Cards with a **key chip**, ⛔ never *"Mouse click"* → open it → the page names **`DiscardAllCost`**, shows **no number**, and mentions **no right-click**.
5. ⛔ **Report the right-click rows as "NOT BUILT — scrapped by Jonathan 2026-09-03"** — ⛔ never "not exercised", ⛔ never passed.
6. ⚠️ **If `IA_DiscardAll` is absent from `IMC_Hero`, the key is INERT and the chip degrades to the reference key. That is a REPORTABLE state, ⛔ not a silent pass** — the code is correct and the key simply does nothing.

**B-5** `TASK-824`'s compile must run with the editor **DOWN**, and `Build.bat` returns **exit 0 on a failed build** — parse the log for `Result: Failed`.

---

## 6. What this gate did NOT and COULD NOT verify (`SC-§32`)

⛔ No compile was run · ⛔ no test was executed · ⛔ no key was pressed · ⛔ no PIE, no pixels · ⛔ no Git diff was taken (I verified **current file state**, which is what the automatic-fail conditions are actually about) · ⛔ `IMC_Hero`'s binary contents are `TASK-820`'s claim, not mine · ⛔ the observer-lockout branch is a diff read on both sides · ⛔ **that the help page READS WELL is Jonathan's under `HELP-§6` and no agent may claim it.**
