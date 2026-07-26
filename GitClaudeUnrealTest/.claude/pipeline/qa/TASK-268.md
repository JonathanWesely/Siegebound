# QA Report — TASK-268
Verdict: **PASS**  (0 BLOCKER / 2 WARN / 2 NIT)

Reviewed pre-compile (file-only; TASK-269 owns the compile). Files:
`Source/GitClaudeUnrealTest/Siegebound/DeckBuilderWidget.{h,cpp}` (+676 / −0, purely additive).
Cross-checked against: board `## M7.7 tasks` / `#### TASK-268`; CONVENTIONS "Deck-builder card
details — click-a-card 'how it works' (2026-07-23)"; `CardRow.h`; `Docs/Data/cards.csv`; and the
owning gameplay classes for every mirrored magnitude.

Integration is HELD until Jonathan's W1 playtest (manager ruling 4) — this PASS advances readiness
only; it does NOT trigger a build.

---

## Acceptance — verified

- **Public API** — all present with EXACT signatures/specifiers: `GetCardDescription` (BlueprintPure,
  const, returns FString), `SelectCardForDetails`/`ClearCardDetails` (BlueprintCallable),
  `GetSelectedDetailCardID` (BlueprintPure, const), `OnCardDetailsRequested(const FString&)`
  (BlueprintImplementableEvent), private `UPROPERTY(Transient) FName SelectedDetailCardID`. Matches
  the board `names:` block and CONVENTIONS character-for-character.
- **Truth law / Notes never surfaced** — `Notes` appears exactly ONCE in the diff, and it is inside a
  comment ("the designer-only Notes column is never surfaced"); it is never READ into any output
  (grep-confirmed). No `Description` column, no `FCardRow` change, no DT_Cards reimport,
  `CardRow.h`/`cards.csv` untouched. Every player-facing magnitude is either interpolated from the row
  or a mirrored constant.
- **Anti-drift proof** — Lightning composes `SpellTopTargetsFmt` from `Row.AoERadius` (live **700**),
  MaxTargets (3) and Damage (200); the stale `Notes` "in 400" is structurally unreachable. Verified
  the composition for a unit-with-keywords (Pikeman/Slayer, Cavalry/Charge, MilitiaMob/Swarm), a
  spell (Fireball line-delivery, Lightning ground), and a spawner (Barracks → resolves `Footman`
  DisplayName, 8 s, 60 s) against `CardRow.h` semantics — all correct.
- **Glossary-mirror rule (divergence trap) — checked against source, all 20 magnitudes match:**
  MinerGoldPerTick 1 / MaxActiveMiners 6 (`SiegePlayerState.h:302,294`), DeepMineIncome 2
  (`DeepMine.h:66`), Masons 300/10 (`SiegePlayerController.h:398,402`), MeleeDamageBonus 10 /
  MaxHPBonus 100 / MoveSpeedBonus 0.25 / WarBannerAuraRadius 600 / WarBannerDamageBonus 0.20
  (`HeroCharacter.h:468,472,476,480,484`), ChargeMoveSeconds 2 / ChargeMultiplier 2 /
  SlayerHPThreshold 150 / SlayerMultiplier 2 / BattleCryAttackSpeedBonus 0.5 / BattleCryMoveSpeedBonus
  0.25 (`SummonedUnit.h:395,399,415,411,424,428`), SwarmSpawnRadius 300 (`SiegePlayerController.h:617`),
  ChainBounceRadius 350 (`Tower.h:114`), LineRange 900 / LineHalfWidth 100
  (`SpellLineSweep.h:103,112`). Castle scaling ×2 Siege / ×0.5 projectile / ×0.5 spell
  (`Castle.cpp:189,193,197`); Building ×2 Siege only, projectile/spell full (`Building.cpp:322`). The
  computed chain-falloff sequence uses the SAME formula the tower applies — `Max(Damage − n×Falloff,
  0)` (`DeckBuilderWidget.cpp:920` vs `Tower.cpp:456`) → 15/10/5 for Crystal Tower. Every `// mirrors`
  comment is present at the glossary site and its baked value is correct.
- **Scope / M6 safety** — additive by inspection: all pre-existing functions
  (AddCopy…SetActiveDeck, IndexOfCard, ResolveCard*, Load*SaveGame) present and unchanged. The details
  path is deck-neutral: `GetCardDescription`, `SelectCardForDetails`, `ClearCardDetails` fire ONLY
  `OnCardDetailsRequested` and never `OnDeckModelChanged`, so the x/50 counter, §8 average-cost guide,
  MaxCopies enforcement and the exactly-50 `PlayBtn` gate cannot churn on a details click.
- **Null-safety** — `NAME_None` → empty FString, silent (matches `GetCardArtTexture`);
  unknown row / missing table → empty FString, logged once through the EXISTING
  `bWarnedMissingTable` / `WarnedMissingRowIDs` guards (no new channel, no ensure, no crash).
  `SelectCardForDetails` on None/unknown clears the selection and still fires with an empty string.
- **Delivery brain reused** — resolves `ESpellDelivery::Auto` via
  `USpellLibrary::GetEffectiveDelivery(Row)` (static, `const FCardRow&`, include present); no
  re-implementation of TASK-236's per-effect default. Line wording only taken for AoEDamage/Freeze on
  the HeroLine path, matching the resolver's actual branch set.
- **UE 5.8 / compile correctness** — glossary strings are `const TCHAR[]` arrays (UE `FString::Printf`
  static-asserts on a TCHAR array — a pointer would not compile; kept correctly); every `%s` receives
  a dereferenced FString (`*Format…` / `*GetCardDisplayName` / `*Separator`), every `%d` an int32
  (Cost/MaxCopies/SwarmCount/ChainTargets/MaxTargets/GoldSteal); literal `%` escaped as `%%` in
  `SpellAllyBuffFmt`; middle dot composed from code point `0x00B7` (no non-ASCII string-literal bytes).
  Header forward-declares `FCardRow` and passes it by const-ref to the composers; the .cpp includes
  the full type. No shadowing, no removed/deprecated APIs, const-correct throughout, GC-safe (only new
  member is an FName). No blockers.

---

## Rulings on the 3 flagged calls

**1. `SwarmCount > 1` (not `> 0`) gates the swarm line — ACCEPT.**
Not merely cosmetic — it is truth-law *correct*. The shipping play path itself treats `SwarmCount <= 1`
as a single, non-swarm unit: `SiegeBotController.cpp:1437` ("SwarmCount<=1 spawns a single unit",
`FMath::Max(1, SwarmCount)`) and `SiegePlayerController.cpp:1139` ("everything else spawns a single
unit"). So `> 1` matches gameplay's own threshold; `> 0` would print a swarm clause for a value the
game does not treat as a swarm. Output for the shipping 28 is identical (only MilitiaMob authors a
swarm, at 4), and the reading is forward-safe.

**2. Support / suicide / spell suppress the plain `Damage: n per attack` line — CORRECT and COMPLETE.**
`bHasAttackStats = !bIsSpell && !bIsSupport && !bSuicide && Damage > 0`. The suppression set is exactly
{spell, support, suicide}, and each surfaces its number in a dedicated rules line instead: suicide via
`SuicideFmt` (Sapper 80 in 250 — detonation is real, `SummonedUnit.cpp:1983`), support via
`ProfileSupportFmt` (Cleric 8 HP/s in 400), spells via their effect lines. Siege units are NOT in the
set — Ogre correctly shows "Damage: 35 per attack" (its melee is genuine, Siege-typed at
`SummonedUnit.cpp:1606`). Checked all 28 rows: no card that should show a plain attack-damage line has
it hidden.

**3. Reciprocal `// mirrors` comments written only at the glossary site, not in the owning classes —
ACCEPT as correct scope discipline (WARN-level follow-up, not a gap).**
CONVENTIONS (line 362) and board spec item (5) require the `// mirrors <Class>::<Property>` comment
*only at the glossary site* — satisfied, and all 20 magnitudes + 7 CardID constants are commented and
value-verified against source. Writing reciprocal comments into `SummonedUnit.h` / `HeroCharacter.h` /
`Castle.cpp` / `Building.cpp` / `DeepMine.h` / `Tower.h` / `SpellLineSweep.h` /
`SiegePlayerController.h` / `SiegePlayerState.h` would blow TASK-268's two-file /
+676-−0 confinement — itself an explicit acceptance item — so NOT doing so is the right call. The
handoff tabulation (14 sites + 7 CardIDs, with values) is complete and durable. The reverse-direction
guard (a comment in the owning class reminding an editor to update the glossary) is a genuine
improvement but out of scope here; see WARN-1 to keep it from being lost.

---

## Findings

- [WARN] handoffs/TASK-268.md — the 14 reciprocal mirror sites + 7 CardID constants are correct and
  tabulated, but the reverse-direction divergence guard (`// mirrors …DeckBuilderWidget glossary`
  comments in the owning classes) is deferred. Not a TASK-268 defect (cross-file edits would break the
  diff confinement). **Orchestrator action:** register this on the same "do not lose" shelf as the
  stale `ESpellDelivery` comment in `CardRow.h` (manager note in TASKBOARD) so the next task that opens
  those files lands the reciprocal comments. Nothing diverges today — all 20 values verified equal.
- [WARN] DeckBuilderWidget.cpp:1044 — forward-safety only: the `ScalingSiege` ("DOUBLE vs
  castles/structures") line is gated on `Profile == Siege`, but a suicide unit's detonation is
  Siege-typed unconditionally (`SummonedUnit.cpp:1983`). A *future* non-Siege suicide card would OMIT
  the ×2 line even though its blast is doubled vs structures — omission is the safe side (never states
  a false rule). No shipping card is affected (Sapper is Siege). Flag for the pending balance pass.
- [NIT] DeckBuilderWidget.h:165 — `GetCardDescription` returns `FString`, not `FText`. Board-mandated
  and consistent with every sibling getter + the BIE param rule; this project has no localization goal
  and the glossary is authored inline English. Acceptable as-is; noted only for the record.
- [NIT] DeckBuilderWidget.cpp:145-151 — file-scope `const FName GlossaryCardID_*` constructed at
  static init. Safe in UE (the FName pool is lazily created) and a common engine pattern; no action
  needed. Informational.

---

## Notes for build-master (when the lane gate lifts, TASK-269)

- Compile only — no code defect blocks the build. Watch for the two expected-benign items the wave
  already flagged elsewhere: a DT_Cards reimport may log a `SpellDelivery` missing-column notice on
  older data (rows keep `Auto`; the CSV header IS present at `cards.csv:1`), and the stale
  `ESpellDelivery` doc comment in `CardRow.h` is deliberately out of this diff.
- Nothing in TASK-268 touches the editor, assets, or Git; it is pure C++ additive to a single class.
- After the compile, the new nodes to expect in the WBP graph: `GetCardDescription`,
  `SelectCardForDetails`, `ClearCardDetails`, `GetSelectedDetailCardID`, `OnCardDetailsRequested`
  (TASK-270/271 bind these).

---

## BUILD-MASTER COMPILE FAILURE — 2026-07-24 (surfaced during TASK-277 editor-target build)

Verdict flips to **qa-failed**. The uncommitted working-tree `DeckBuilderWidget.cpp` fails to compile
in the editor target (`Build.bat GitClaudeUnrealTestEditor Win64 Development`,
`Result: Failed (OtherCompilationError)`). Flagged here as a **deck-builder collision** independent of
the Shield Wall feature (per the build routing rule), NOT a TASK-274/277 defect.

**Root cause — non-literal format string (UE 5.8 consteval check):** `error C7595
'...TCheckedFormatStringPrivate...': call to immediate function is not a constant expression` at 15
sites. In UE 5.8 the FORMAT argument of `FString::Printf` (and `UE_LOG`/`checkf`) must be a
compile-time string LITERAL — the `TCheckedFormatString` consteval wrapper validates the format at
compile time. Passing a runtime-composed `FString` as the format (e.g. the `Spell*Fmt` values this
code builds from row data and then feeds back in as the format) is rejected. This contradicts the
prior QA "UE 5.8 / compile correctness" note, which assumed every format was a `const TCHAR[]` literal.

Error sites — `DeckBuilderWidget.cpp` lines: 873, 901, 909, 929, 937, 942, 969, 974, 982, 987, 995,
1003, 1011, 1037.

**Fix (gameplay-programmer):** compose the strings with a LITERAL format and USE them as plain `%s`
arguments — `FString::Printf(TEXT("... %s ..."), *ComposedText)` — never re-pass a composed FString as
the format (`FString::Printf(*ComposedFmt, ...)` / `FString::Printf(ComposedFmt, ...)`). Audit all 15
sites; re-QA must actually reason about the consteval literal requirement, not just `%s`/`%d` arg
types.

**Orchestrator/manager note:** these TASK-268 changes are UNCOMMITTED and live in the shared module, so
they BLOCK the TASK-277 W1 build even after TASK-274 is fixed. Either TASK-268 is fixed too, or its
working-tree changes are stashed/removed before the W1 build can go green. Build-master did NOT touch
these files (left uncommitted per direction).
