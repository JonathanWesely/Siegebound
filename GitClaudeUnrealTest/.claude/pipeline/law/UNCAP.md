<!-- MOVED from .claude/pipeline/CONVENTIONS.md on 2026-10-04 by Tools/split_conventions.py. Sections are byte-identical to the original; this comment is the only addition. Cite clauses by tag (e.g. `VER-§3 cl. 6`); CONVENTIONS.md '## Law index' maps tags to files. -->
## ⚖️ CARD-UNCAP — every per-card copy cap ABOLISHED; the 50-card deck total stays EXACT (2026-08-28) — namespace **UNCAP-§**

Jonathan's directive, verbatim (2026-08-28): *"lets also uncap the maximum number of cards you can hold for any and every card. I still want there to be only 50 cards for a deck, but you can have as many as you would like for any specific card."* Tasks TASK-676..679 cite this section; ⛔ no task restates it.

### UNCAP-§1 THE RULING + THE LEGALITY PIN
- A deck may hold **any number of copies of any single card**. A 50-Footman deck is LEGAL.
- The deck-total invariant is untouched and **PINNED: a legal deck is EXACTLY 50 cards** (`SiegeLegalDeckSize`), never ≤50. Grounds: his words ("only 50 cards for a deck" continues the shipped exactly-50 meaning) + the shipped D5/DECK-§ posture (Play always enabled; an illegal deck falls back to the curated default — a ≤50 relax would silently change match entry). Overrule window = FOR-JONATHAN row U5 on the board.

### UNCAP-§2 THE COLUMN SEVERANCE — `MaxCopies` had two meanings; ONE survives
- `MaxCopies` (FCardRow / DT_Cards) is from this date the **hero-upgrade STACK CAP only** (`AHeroCharacter::ApplyUpgrade`/`GetStackCapForUpgrade` — untouched, still data-driven §3.0). Its deck-copies-cap meaning is ABOLISHED.
- ⛔ The column is NOT deleted, NOT renamed; **cards.csv / DT_Cards carry ZERO data diff** this wave (U6 default).
- `DeckCount` = the CURATED DEFAULT composition only; `sum(DeckCount)==50` still binds (the default deck must itself be legal). The historical "50-card invariant" bookkeeping is reinterpreted as default-composition maintenance — dated riders sit at the original clauses ("Data-driven card stats" + the M6 curated-default line).

### UNCAP-§3 THE LEGALITY LAW — `IsDeckLegal`, amended in place (see the M6 section's dated amendment)
- Signature **BYTE-IDENTICAL**. New semantics: true iff (a) every entry CardID resolves in DT_Cards, (b) every entry `Count >= 0`, (c) `TotalCount() == SiegeLegalDeckSize` exactly. The per-CardID aggregate cap check, its `RunningCounts` map, and the "the cap is %d (MaxCopies)" reason string are REMOVED.
- **Strictly WIDENING** ⇒ every previously legal deck stays legal: the curated default, both bot decks, all ten `deck1..deck10` slots, existing save files, cloud payloads. **Zero migration, zero SaveGame version bump, zero cloud/schema impact** (copy counts are data inside the jsonb deck payload; legality is re-judged client-side, and a relaxation cannot orphan a stored deck — verified at source 2026-08-28: no server-side legality exists).

### UNCAP-§4 THE WIDGET COMPAT SHIM — ⛔ no WBP/.uasset edit
- `UDeckBuilderWidget::AddCopy` loses the `Current >= Row->MaxCopies` refusal. It gains **NOTHING**: ⛔ NO new deck-total guard at add time (U4 default — over-50 WORKING decks were already reachable under the old caps, since sum-of-MaxCopies > 50, and are handled today; the live x/50 counter + the exactly-50 legality gate carry the invariant; the behavior class is unchanged). The `!Row` (unknown card / missing table) refusal STAYS.
- `GetCardMaxCopies(FName) const` — BlueprintPure **signature UNCHANGED** (`WBP_DeckCardTile` calls it for the "+" cap-grey). It now returns `SiegeLegalDeckSize` for a resolved row (0 for missing table/row — unchanged). Accepted consequence: the "+" greys only at 50-of-one-card, which coincides with deck-full for that card. This is why no WBP graph edit is needed.
- DECK-§ auto-save, broadcast-on-success-only, right-click, ten-slot, and no-clip laws — ALL UNTOUCHED.

### UNCAP-§5 DISPLAY LAW
- Card-details identity line: `"<Type> · Cost <Cost> gold"` — the `Max <MaxCopies> per deck` clause DELETED (amended in place at the 2026-07-23 composition law; truth-law grounds).
- The hero-upgrade RULES tail keeps printing the stack cap from `MaxCopies` — still true, UNCHANGED.
- The above-tile copy-count badge shows the TRUE working count, uncapped (U3 default) — no display clamp.

### UNCAP-§6 MATCH-SIDE + BOT — no behavior change
- Draw/hand/consumption is per-card-INSTANCE (`UDeckComponent` builds Count copies into the pile; controller/hand logic assumes nothing about per-card uniqueness — verified at source 2026-08-28, `SiegePlayerController.cpp:241-300`). NO code change (U2 default); stale comments citing `[0..MaxCopies]` get comment-only riders.
- Hero-upgrade stack caps still bind IN MATCH: a deck may hold more copies of an upgrade than its stack cap; plays past the cap keep today's refuse-no-spend "at max stacks" path (U1 default). ⛔ `HeroCharacter.{h,cpp}` untouched.
- Bot decks (`ASiegeBotController` constructor defaults) are legal by construction under the wider law — logic untouched; comment riders only.

### UNCAP-§7 FILE MAP + FENCES (the cross-task contract, TASK-676..679)
- **OWNED (TASK-676):** `DeckLibrary.{h,cpp}` · `DeckBuilderWidget.{h,cpp}` · `DeckTypes.h` (comments) · `CardRow.h` (`MaxCopies` comment) · `DeckComponent.cpp` (comment-only) · `SiegeBotController.cpp` (comment-only) · `Tests/SiegeDeckSlotsTest.cpp` (new cases).
- ⛔ **UNTOUCHED:** `HeroCharacter.{h,cpp}` · every `.uasset`/WBP · `cards.csv`/DT_Cards · `SiegeDeckSaveGame.{h,cpp}` · cloud/account files · **ALL menu-widget files (MainMenu/Multiplayer surfaces — the VID-002 Back-button lane owns those; the two batches' file maps are DISJOINT by this fence and only the compile slot serializes them, QUIET-MODULE).**
- **Tests law:** new `Siegebound.Deck.*` cases in `SiegeDeckSlotsTest.cpp`, built on a **transient in-test `UDataTable`** (`NewObject` + `FCardRow` rows — ⛔ never the shipped asset, commandlet-safe): (1) 50× one card ⇒ LEGAL; (2) 51 total ⇒ illegal with the exact-50 reason; (3) unknown CardID ⇒ still illegal; (4) negative Count ⇒ still illegal. TASK-676's handoff DECLARES the new expected suite total (current baseline 136) for the TASK-678 gate.

