<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
#### TASK-268 — [DB-A] `UDeckBuilderWidget`: generated card description + details-selection API (C++, file-only)
- assignee: gameplay-programmer
- status: ✅ **done — COMMITTED `24b1f0a` (2026-07-26).** 📋 **MANAGER FLIP 2026-09-07, ⛔ 43 DAYS LATE, ⛔ ON GIT EVIDENCE** — `handoffs/BOARD-STALENESS-audit.md` §2 (build-master's git-resolved sweep; ⛔ the manager holds ⛔ NO `Bash`, `SC-§71b`). ⛔ **LOGGED EXCEPTION TO WRITE-DISCIPLINE RULE 1** — `status:` is the ⛔ assignee's field; the ⛔ commit host never flipped it, so this row asserted a falsehood for ⛔ 43 days. ⛔ **BATCH HOST: `TASK-268..272: deck-builder card details — click-a-card 'how it works' side panel`** (rule 8). ⚠️⚠️ **NOTE THE ⛔ SHARPEST PART: this row's own text said *"STAYS PARKED/uncommitted"* and *"INTEGRATION (TASK-269+) STAYS HELD"* — ⛔ BOTH became FALSE on 2026-07-26 and ⛔ neither was corrected, which is how ⛔ TASK-269 sat fenced behind a shipped commit.** ⛔⛔ **A FLIP IS ⛔ NOT A GO.** Marker `BOARD-STALE-FLIP-2026-09-07`. ← was: ~~qa-passed~~ (COMPILE-FIX 2026-07-24, gameplay-programmer — resolved C7595 by promoting the 14 glossary format constants from `const TCHAR[]` to `constexpr TCHAR[]` (UE 5.8's TCheckedFormatString reads the format inside a `constexpr` initializer, so the format array must be usable in a constant expression — a `constexpr` array is, plain `const` is not; verified against Engine `.../String/FormatStringSan.h:18`). Format STRINGS and all 14 Printf call sites are byte-for-byte untouched → rendered descriptions byte-identical to the QA-verified composition; zero logic change. STAYS PARKED/uncommitted — integration held for TASK-269+ per manager ruling 4; this un-blocks the shared-module compile for the W1 build only. --- ORIGINAL BUILD-MASTER FAILURE (retained for history): 2026-07-24 — compile FAILED in TASK-277's editor-target build: DeckBuilderWidget.cpp feeds NON-LITERAL FString format strings to FString::Printf → error C7595 (TCheckedFormatString consteval) ×15 at lines 873/901/909/929/937/942/969/974/982/987/995/1003/1011/1037. UE 5.8 requires the format arg to be a compile-time literal. Deck-builder collision (NOT a Shield-Wall defect); route to gameplay-programmer; errors in qa/TASK-268.md. ⚠ These changes are UNCOMMITTED and block the shared-module compile — fix OR stash them before the W1 build (TASK-277) can go green; build-master did not touch them. --- Prior status qa-passed — QA 2026-07-23 (PASS, 0 blockers / 2 WARN / 2 NIT, report `qa/TASK-268.md`). Verified: `Notes` never surfaced (comment-only in the diff), anti-drift proven (Lightning composes from live `AoERadius` 700; stale Notes "400" structurally unreachable), all 20 mirrored magnitudes value-checked against source and correct, chain-falloff formula identical to `Tower.cpp:456`, deck-neutral (no `OnDeckModelChanged`, M6 counter/gate/MaxCopies untouched). Flagged-call rulings: (1) `SwarmCount > 1` ACCEPTED — truth-correct, the play path treats ≤1 as a single non-swarm spawn; (2) Damage-line suppression for support/suicide/spell CORRECT+COMPLETE across all 28; (3) reciprocal glossary-only comments ACCEPTED as correct scope discipline. Two non-blocking follow-ups carried to the next task that opens the owning classes: WARN-1 (register the 14 reciprocal mirror sites + the stale `ESpellDelivery` CardRow.h comment on the do-not-lose shelf), WARN-2 (balance-pass note: a future non-Siege suicide card would omit the ×2-vs-structure line — safe-side omission, no shipping card affected). **INTEGRATION (TASK-269+) STAYS HELD for Jonathan's W1 sign-off per manager ruling 4 — this PASS advances readiness only, no build triggered.** Original programmer note follows. — 2026-07-23, gameplay-programmer. `DeckBuilderWidget.{h,cpp}` ONLY, **676 insertions / 0 deletions** (machine proof that every existing signature/behavior is byte-identical). Delivered: `GetCardDescription` (generated from the row, `Notes` never read), `SelectCardForDetails` / `ClearCardDetails` / `GetSelectedDetailCardID` / `OnCardDetailsRequested`, and a 29-string per-keyword glossary with 14 mirror sites. NOT compiled, editor untouched, nothing staged (TASK-269 owns the compile). Handoff: `handoffs/TASK-268.md` (API signatures, composition order, glossary + mirror table, clause-by-clause truth-law verification with file:line, six rendered examples).
- blocked-by: none — **dispatchable NOW** (file-only: no compile, no editor, no Git; the diff carries cleanly across any later checkout because no other lane owns this file)
- parallel-safe: yes (sole owner of `DeckBuilderWidget.{h,cpp}`; disjoint from every file the M7.6 branch owns)
- spec: >
    Edit `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.{h,cpp}` ONLY. Additive — do not change ANY existing
    signature or behavior (`AddCopy`/`RemoveCopy`/`GetCountOf`/`GetTotalCount`/`GetAverageCost`/`IsCurrentDeckLegal`/
    `GetCollectionCardIDs`/`GetCardDisplayName`/`GetCardCost`/`GetCardMaxCopies`/`GetCardArtTexture`/`LoadDefaultDeck`/
    `SaveDeckAs`/`LoadDeck`/`GetSavedDeckNames`/`SetActiveDeck` and both existing BIEs stay byte-identical).
    (1) **Selection API.** `SelectCardForDetails(FName CardID)` (BlueprintCallable) — stores the private
    `UPROPERTY(Transient) FName SelectedDetailCardID` and fires `OnCardDetailsRequested(CardID.ToString())`; an unknown row
    or `NAME_None` CLEARS the selection and still fires with an empty string (so the panel can show its hint).
    `ClearCardDetails()` (BlueprintCallable). `GetSelectedDetailCardID() const` (BlueprintPure).
    `OnCardDetailsRequested(const FString& CardID)` (BlueprintImplementableEvent — FString param only, the widget rule).
    (2) **`GetCardDescription(FName CardID) const` (BlueprintPure, returns FString)** — the deliverable. Compose the
    multi-line player-facing body per CONVENTIONS "Deck-builder card details" §composition order: identity line
    (Type · Cost · Max per deck) → blank → STAT block (`HP`, `Damage` + `AoERadius` splash, `Cadence`, `Range` with
    melee-vs-homing-projectile wording from `bRanged`, `MinRange`, `Speed` — OMIT every line whose field is 0/N-A) → blank
    → RULES block, one plain-English line per applicable clause: `bCharge`, `bSlayer`, `bSuicide`, `SwarmCount`,
    `ChainTargets`/`ChainFalloff` (print the actual falloff sequence from `Damage`/`ChainFalloff`), spawner
    (`SpawnCardID`/`SpawnInterval`/`Lifetime` — resolve and print the spawned card's **DisplayName**, never the raw CardID),
    all five `SpellEffect` values, spell delivery, `Profile` (Siege / Support prose; Standard omitted), and castle-damage
    scaling. Numbers ALWAYS come from the row (§3.0 — never hardcode a stat that exists in the table); the only literals
    are the glossary strings.
    (3) **Spell delivery must reuse the existing brain:** resolve `ESpellDelivery::Auto` through
    `USpellLibrary::GetEffectiveDelivery` — do NOT re-implement the per-effect default (that would be a second source of
    truth for TASK-236's law). Null-safe; if the library cannot be consulted, omit the delivery line rather than guess.
    (4) **TRUTH LAW — verify before you write.** Every rules line must describe what the SHIPPING code actually does: read
    the owning class for each clause (`ACastle::TakeDamage` for castle scaling, the Charge/Slayer/Chain/Swarm/Suicide
    implementations, `ADeepMine`, the spawner building, `USpellLibrary`) and confirm the wording. **If a clause cannot be
    verified, OMIT it** — a description that states a rule the game does not implement is a FAIL, not a nit. List in the
    handoff, clause by clause, WHICH file/line you verified each against. No GDD section refs, no class or property names,
    no CSV column names in player-facing text.
    (5) **Glossary-mirror rule:** magnitudes that are mechanic rules rather than CSV columns (Charge 2 s / 2×, Slayer
    150 HP / 2×, castle scaling, Deep Mine +2 gold/s, Masons 300 HP over 10 s, hero-upgrade magnitudes) go in ONE
    contiguous block of named string constants at the top of the .cpp, each with a `// mirrors <Class>::<Property>` comment.
    (6) **Null-safety:** unknown/None CardID or a missing table ⇒ EMPTY FString (the WBP shows its own hint), logged once
    through the EXISTING `bWarnedMissingTable` / `WarnedMissingRowIDs` spam guards. Never a crash, never an ensure.
    (7) **`Notes` IS NOT SURFACED.** Do not read it, do not add a `Description` column, do not touch `CardRow.h` or
    `cards.csv`.
    Target ≤ ~12 lines per card. In the handoff, paste the COMPOSED OUTPUT you expect for these six cards, hand-derived
    from `Docs/Data/cards.csv`: `Footman` (plain melee), `Archer` (ranged + castle scaling), `CrystalTower` (chain),
    `Barracks` (spawner), `Fireball` (spell + HeroLine delivery), `Ogre` (Siege profile) — that table is what QA reviews.
    NOT IN SCOPE: any UMG asset, any other C++ file, compiling, Git. **FILE-ONLY: do NOT compile and do NOT touch the
    editor** (the branch is frozen for Jonathan's W1 look — TASK-269 owns the compile). QA implied (shadow scan,
    complete-type include scan, null-safety, §3.0 no-hardcoded-stats, truth-law spot-check against cards.csv).
    Post in ⚙️ Dev & QA.
- names: >
    `Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.{h,cpp}`. New identifiers, EXACT: `GetCardDescription`,
    `SelectCardForDetails`, `ClearCardDetails`, `GetSelectedDetailCardID`, `OnCardDetailsRequested`, `SelectedDetailCardID`.
    Existing names consumed as-is: `ResolveCardTable`, `ResolveCardRow`, `bWarnedMissingTable`, `WarnedMissingRowIDs`,
    `FCardRow`, `ECardType`, `ECardProfile`, `ESpellEffect`, `ESpellDelivery`, `USpellLibrary::GetEffectiveDelivery`.
    Law: CONVENTIONS "Deck-builder card details — click-a-card 'how it works' (2026-07-23)", "Deck-builder & saved decks
    (M6)", "Data-driven card stats (GDD §3.0)", "Widgets with C++ bases" (BIE param rule), "Spell delivery overhaul".

