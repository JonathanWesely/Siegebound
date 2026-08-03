# TASK-471 — handoff (gameplay-programmer)

**Status: `ready-for-qa`. Gate: TASK-472.**

**M8 DECLARATION, verbatim:** *adds no replicated property, no new replicated class, no new relevancy tier.*

⛔ **NOT COMPILED, NOT RUN, NO EDITOR, NO PIE, NO MCP, NO GIT, NO `Content/`.** Nothing in this file has been built since `cf8ef8e`; TASK-468 owns that. Every claim below is about SOURCE.

---

## 1. THE FIX — the frames stop RELATING the place and start PRESENTING it

**Two frames carried the barred preposition, not one.** §30 and WARN-5 both quote only the first.

| key | before | after |
|---|---|---|
| `Assistant_Order_SelectionPlace` | `"{Intent} {Selection} to {Place}"` | **`"{Intent} {Selection} ({Place})"`** |
| `Assistant_Order_Place` | `"{Intent} to {Place}"` | **`"{Intent} ({Place})"`** |
| `Assistant_Order_Selection` | `"{Intent} {Selection}"` | **unchanged** — asserts no relation, already verb-neutral |

**That is the whole behavioural change: two string literals.** No branch forked, no row duplicated, no call site moved, no control flow touched.

### Why a parenthesis rather than a preposition
`to` is a **destination** for send / charge / fall back / rally and a **location** for guard / ambush. **No English preposition is true of both**, so the frame must stop asserting a relation it cannot verify. A parenthetical apposition attaches to the whole clause and claims nothing about the verb ⇒ **one string is correct for all seven intents**, which is exactly what §30 demands. ⛔ `{Preposition}` stays barred: it is bound to the verb before it *and* the noun after it, and it would reach a translator as a fragment with no case, gender or word order.

### ⚠️ §20 — I TRACED THE OBVIOUS SEPARATOR AND REFUSED IT, AND THE TRACE CHANGED THE ANSWER
`handoffs/TASK-465-programmer.md` recommended **`"{Intent} {Selection} - {Place}"`**. ⛔ **It is wrong, and only reading the FULL line shows why:** `{Order}` is **never read alone** — `Assistant_ConfirmPrompt` is `"{Order} - accept?"`. The dash frame renders

> `Guard 8 footman - mine_near - accept?`

— **two dashes at one level**, and no reader can tell which one separates the order from the question. ✅ **Parentheses are self-delimiting, so the frame composes into every wrapper unambiguously.** Recorded at the line as an anti-revert note per §20.

### §3 — the verb's provenance, traced
`{Intent}` is `IntentDisplayText(Command.Intent)`, whose only input is `FSiegeAssistantCommand` — pinned to `uint8`/`int32`/`FName`. `{Selection}` is built from `Command.Kinds`/`Counts` (`FName`/`int32`); `{Place}` is `FText::FromName(Command.Where)`. ⇒ **Every word comes from the parsed command's own symbols; no model text can reach the sentence.** `FormatArgs` (`:1144-1146`) is **unmodified** — I added no argument and removed none.

---

## 2. ✅ THE READ-ALOUD ENUMERATION — the mechanical acceptance test

### Reachability first, established BY SYMBOL (this is what makes the table a proof rather than a list)

- `Command.Intent == None` ⇒ early `FText::GetEmpty()`. **None never reaches a frame.**
- `SiegeAssistantIntentTakesSelection` (`SiegeAssistantCommand.cpp:384`) = **Send, Guard, Ambush, Follow**.
- The parser's **ONE cross-field check** (`:709`) rejects `who:"none"` for those four. ⚠️ **`who:"all"` parses to an EMPTY selection** (`:702-708` — *"the struct has no field that distinguishes them"*), so **all four can still reach a frame with no selection.**
- `where` (`:623-644`) has **NO per-intent constraint** — any intent may carry any place, or `"none"` ⇒ `NAME_None`.
- ⚠️ **The grammar cannot narrow any of this:** the `command` root rule (`SiegeAssistantGrammar.cpp:437-450`) is **flat** — `intent`/`who`/`where`/`when` are independent rules with no cross-correlation. ⇒ **every combination below is MODEL-reachable, not merely parser-reachable.**
- `IntentTakesZoneOrder` (`SiegeAssistantComponent.cpp:185-193`) = **Send, Guard, Ambush only** (narrower than `TakesSelection` — excludes Follow). `RouteParsedCommandInternal` step (2) diverts those three to `AskWhichPlace` unless the place **resolves**, and `Assistant_AskWhichPlace` (*"Where should they go?"*) **does not embed `{Order}`**. ⇒ **Send/Guard/Ambush reaching a player-visible `{Order}` ALWAYS have a place.**
- Follow / Charge / Fallback / Rally are **not** place-gated ⇒ they reach the frames with or without one.

**Player-visible `{Order}` consumers:** `Assistant_ConfirmPrompt` · `Assistant_Executed` · `Assistant_DeferredArmed` · the raw summary broadcast at `EnterAwaitConfirm` · `GetPendingConfirmSummary`.

### FRAME A — `"{Intent} {Selection} ({Place})"` (selection **and** place)

| intent | BEFORE | AFTER |
|---|---|---|
| Send | Send 10 footman to ancient_ground_near ✓ | Send 10 footman (ancient_ground_near) ✓ |
| Guard | **Guard 8 footman TO mine_near ✗** | Guard 8 footman (mine_near) ✅ |
| Ambush | **Ambush 8 footman TO ancient_ground_near ✗** | Ambush 8 footman (ancient_ground_near) ✅ |
| Follow | **Follow 8 footman TO mine_near ✗** | Follow 8 footman (mine_near) ✅ |
| Charge | Charge 5 footman to mid ✗ | Charge 5 footman (mid) — **finding F1** |
| Fallback | Fall back 5 footman to own_castle ✗ | Fall back 5 footman (own_castle) — **F1** |
| Rally | Rally 5 footman to own_castle ✗ | Rally 5 footman (own_castle) — **F1** |

**Value variants also read aloud:** `Counts[i] == 0` ⇒ *"Guard **all** footman (mine_near)"* ✓ · multi-kind up to `SiegeAssistantMaxSelectionKinds` ⇒ *"Send 10 footman, 1 sorcerer (ancient_ground_near)"* ✓ (the flagship sentence).

### FRAME B — `"{Intent} {Selection}"` (selection, no place) — **unchanged**

Send/Guard/Ambush are **unreachable** here (place-gated). Reachable: **Follow** *"Follow 8 footman"* ✅ · Charge / Fall back / Rally *"Fall back 5 footman"* — **F1**.

### FRAME C — `"{Intent} ({Place})"` (place, no selection) — reachable via `who:"all"`

| intent | BEFORE | AFTER |
|---|---|---|
| Send | Send to mid ✗ (objectless) | Send (mid) — **finding F2** |
| Guard | **Guard TO mid ✗ — THE UNCITED INSTANCE** | Guard (mid) ✅ |
| Ambush | **Ambush TO ancient_ground_near ✗** | Ambush (ancient_ground_near) ✅ |
| Follow | Follow to mine_near ✗ | Follow (mine_near) ✅ |
| Charge | Charge to mid ✓ | Charge (mid) ✓ |
| Fallback | Fall back to own_castle ✓ | Fall back (own_castle) ✓ |
| Rally | Rally to own_castle ✓ | Rally (own_castle) ✓ |

⚖️ **THE HONEST COST, STATED RATHER THAN BURIED:** for **charge / fall back / rally** the old `to` was *natural* and the parenthesis is *terser*. **I traded naturalness for three verbs to make three others correct**, because §30 leaves no choice: *"one string that reads correctly for every intent"*, and a per-intent fork is barred. The parenthesis asserts **nothing false** for any of the seven; `to` asserted something false for three.

### FRAME D — neither selection nor place (`IntentDisplayText` alone) — **unchanged**
*"Follow"* ✓ *"Charge"* ✓ *"Fall back"* ✓ *"Rally"* ✓. Send/Guard/Ambush unreachable (place-gated).

### THE WRAPPERS — read aloud as the player actually meets them
- `"{Order} - accept?"` ⇒ **"Guard 8 footman (mine_near) - accept?"** ✅
- `"Ordered: {Order}"` ⇒ **"Ordered: Ambush 8 footman (ancient_ground_near)"** ✅
- `"Waiting for {Requested} {Kind}, then: {Order}"` ⇒ **"Waiting for 5 footman, then: Send 10 footman (mid)"** ✅

---

## 3. ✅ THE WARN-5 SWEEP — all ten read against §30, stated as a RESULT

**WARN-5's ten out-of-table literals = `IntentDisplayText`'s 7 rows + `DescribeCommandForPlayer`'s 3 frames.** All ten read; **nobody had done this before.**

| # | literal | verdict |
|---|---|---|
| 1 | `Assistant_Order_SelectionPlace` | ⛔ **DEFECT — FIXED** (the cited one) |
| 2 | `Assistant_Order_Place` | ⛔ **DEFECT — FIXED** (⚠️ **uncited by WARN-5 AND by §30**) |
| 3 | `Assistant_Order_Selection` | ✅ compliant — no placeholder-adjacent grammar; unchanged |
| 4–10 | `IntentDisplayText`: Send · Guard · Ambush · Follow · Charge · Fall back · Rally | ✅ **all seven compliant** — each is a **whole word**, which §30 explicitly permits (*"a verb-as-a-whole-word"*); none contains a placeholder, so none assembles grammar |

⇒ **RESULT: two of the ten carried the defect and both are fixed; the other eight are compliant. The §30 sweep of WARN-5's ten is CLOSED.**

### Extended sweep (§22 — sweep the SHAPE, not the location). ✅ **NOTHING FURTHER FOUND — and here is what I checked**
- **All 19 reason-template rows** — re-read for substituted grammar. Only the `{Order}`-embedding wrappers can compose badly, and all three are enumerated above. `Assistant_ShortfallCount`'s `{Intent} {Available}?` substitutes a whole verb + a number and asserts no relation ✅ (TASK-465's row — **unmodified by me**).
- **All 5 state labels** — fixed complete strings, no placeholders. ✅
- **`SiegeAssistantConsoleWidget`'s 4 chrome literals** (*"Type an order, then press Enter"* / *"Accept"* / *"Cancel"* / *"Ready"*) — fixed complete strings, **no placeholders, no §30 exposure**. (The *"Ready"* duplication is TASK-465's recorded **§3** finding, not a §30 one.)
- ⚠️ **Per §22's *"runtime log strings are the priority surface"*: the prompt-side pending line (`.cpp:2585-2610`)** — model-facing Zone B, and **already verb-neutral**: it emits the **wire symbol** plus `" -> "` and `" x%d"`, asserting no English relation. ✅
- **The assistant lane's `UE_LOG` literals** — diagnostic prose with `%s`/`%d` for nouns and numbers; none assembles a player sentence from substituted grammar. ✅

---

## 4. ⚠️ THREE FINDINGS REPORTED AND DELIBERATELY LEFT — I want rulings, not silent redesigns

**F1 — AN ARMY-WIDE VERB CAN CARRY A SELECTION, AND NO FRAME CAN REPAIR IT.**
`who:[…]` with **charge / fallback / rally** parses (the one cross-field check rejects only `who:"none"`, and only for selection-bearing verbs) and the **flat grammar root cannot prevent it** ⇒ *"Fall back 5 footman (own_castle)"*. ⚖️ **This is NOT a §30 frame defect and changing the frame cannot fix it:** *fall back* is **intransitive**, so the defect is the **data combination**, not an asserted grammar. Worse, the parser's own comment says **the executor IGNORES the selection** for these verbs ⇒ the sentence describes something that will not happen. **Two repairs, both behaviour changes beyond wording:** (a) suppress the selection when `!SiegeAssistantIntentTakesSelection(Intent)` — **in my function, and I still refused it**, because it would **conceal a model error at exactly the step that exists to expose model errors** (`SpawnConfirmPreview`'s own note: four of the five stable eval failures are wrong-place/wrong-count and are *"visible on the ground BEFORE anything moves"*); or (b) reject the combination at the parser — **outside my lane**. ⇒ **Wants a manager ruling.**

**F2 — `who:"all"` COLLAPSES TO AN EMPTY SELECTION**, so *"send everything to mid"* renders **"Send (mid)"** with no object. Documented at the struct (`SiegeAssistantCommand.cpp:702-708` — *"the struct has no field that distinguishes them"*). ⚠️ **NOT created and NOT widened by my change** — the old frame rendered *"Send to mid"*, equally objectless. A repair needs a **new struct field** (an `all` flag) — outside my lane.

**F3 — A §22 "TRUE CONCLUSION ON A DEAD MECHANISM", found while tracing.** `SpawnConfirmPreview`'s comment says *"charge / fallback / rally are army-wide with **no place at all**."* ⛔ **False:** `where` has no per-intent constraint, and `SiegeAssistantGrammarTest.cpp:844-847` **pins the opposite** — `fallback + who:"all" + where:"own_castle"` parses and *"fallback keeps its place"*. The **conclusion** still holds (no decal is drawn, because the function returns early on `IntentTakesZoneOrder`), but **the stated reason is gone** — §22's hardest shape. **Comment-only; left unedited** because it sits inside the confirm-step lane and my edit was ordered narrow.

---

## 5. Files touched — and the EXACT symbols, so TASK-469's collision is bounded

- **`Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.cpp`** — **`USiegeAssistantComponent::DescribeCommandForPlayer` ONLY.** Two `FText::Format` frame literals (`Assistant_Order_SelectionPlace`, `Assistant_Order_Place`) + the §30/§20 comment block above the branch chain + one comment inside each of the other two branches. ⛔ **No statement, no branch, no control flow, no signature changed.**
- **`Source/GitClaudeUnrealTest/Siegebound/SiegeAssistantComponent.h`** — the **doc comment on the `DescribeCommandForPlayer` declaration** only. ⛔ No signature, no member, no `UPROPERTY`.

**No other file. No asset. Nothing under `Content/`.**

### ⚠️ TASK-463's AND TASK-465's WORK IS UNDISTURBED — verified BY SYMBOL, never by offset (§18c)
`GetVocabulary` · `BeginPlay` · `EnsureStaticPrefixRegistered` (definition **and** all three call sites) · `SiegeAssistantReasonTemplate`'s `ShortfallCount` row · `PushMessage`'s `{Intent}` `FormatArgs.Add` — **all byte-unmodified.** My entire `.cpp` diff is inside one function that none of those tasks touched. ⚠️ **`TASKBOARD.md` and this file have both moved hundreds of lines today** — every citation here is by symbol, and **no line number should be trusted after the next edit.**

### Localization
**Both keys UNCHANGED** (`Assistant_Order_SelectionPlace`, `Assistant_Order_Place`); source strings changed. **No translations exist, so nothing is orphaned.** The `NSLOCTEXT` census in the `.cpp` is **UNCHANGED at 35** (34 sites + the comment token at the head of the file) — **I added no literal and removed none**, so WARN-5's count of ten is **not increased**.

---

## 6. FOR QA — what to scrutinise, and what I could not check

1. ⛔ **Not compiled.** The edit changes **two string literals inside existing `FText::Format` calls whose argument set is unchanged** — `{Intent}`, `{Selection}`, `{Place}` were all already registered in `FormatArgs`, and `{Place}` is still referenced by both frames. **No new symbol, no new include, no signature change.** I expect no build impact, but that is a source argument, not a build.
2. **Re-derive the reachability table independently** — especially **`who:"all"` ⇒ empty selection**, which is what makes Frame C reachable for Guard/Ambush and is the whole basis of the uncited second defect. **Use a positive control on the search (§14).**
3. **Rule on F1** — it is the one live combination the frame cannot repair, and my refusal to fix it in my own function is a **judgement call I would rather have argued than defaulted**.
4. ⚖️ **Disagreeing with the parenthesis is a legitimate outcome.** If it should read differently, the constraint to hold any alternative to is the one in §2: **it must compose unambiguously inside `"{Order} - accept?"` and `"Ordered: {Order}"`, and assert nothing false for any of the seven verbs.**
5. **Not accuracy progress.** This changes what the game says, not what the model emits. **Bar #5 stands uncleared at 20/25 vs 22. WARN-5 is now discharged for §30; it remains open for §3 (the literals still live outside the table).**
