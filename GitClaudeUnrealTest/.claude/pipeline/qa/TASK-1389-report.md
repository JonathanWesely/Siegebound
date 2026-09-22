# Measurement Report — TASK-1389 `[1306-INJECTION-TOOL-REMEASURE]`

⛔ **THIS IS A MEASUREMENT REPORT, NOT A GATE.** No `PASS`, no `FAIL`, no verdict over `TASK-1306`, `TASK-1304`, `TASK-1274` or any other row. Nothing here moves any status but this row's own.

---

## ⭐ THE HEADLINE, AND IT IS THE INCONVENIENT ANSWER

# The tool was **`simulate_key_press`**. The argument was the **key `"Down"`** — not an action asset path.

⇒ **BRANCH (b) IS SELECTED. THE PAIR PROVES NOTHING ABOUT ENHANCED INPUT AND THE WHOLE PARAGRAPH COLLAPSES.**

`TASK-1306`'s builder-side injection did **not** go through `inject_input_action` on `IA_MenuDown`. It went through `simulate_key_press` with the key `Down`, twice. The three-way discriminator the dispatch described — `TASK-1274` moved focus / `TASK-1306` did not / 🧑 his real key did — **does not stand**, because its two agent arms were taken with **two different injectors on two different widgets**, and the difference in outcome cannot be attributed to the layer when the instrument also changed.

⛔ **This is this row's best possible outcome reported plainly, not its failure.** The collapse was pre-written on the row before the answer was known, and it is the branch the quote selects.

⚠️ **Branch (c) also applies as the enumeration frame:** *two* tools were used across the 5b leg, at different points. `inject_input_action` was used — but **only** on `L_MainMenu` to walk the menu and open the builder, and it was **never fired while the builder held focus**. The limb that produced `FocusedCardIndex == -1` is the `simulate_key_press` limb, and that limb lands in **(b)**. The `inject_input_action` limb lands in **(a)** but produced **no builder-side read at all**, so it supplies nothing to the discriminator either.

---

## 1 · THE RE-MEASUREMENT AT SOURCE — TOOL, ARGUMENT, SECTION, ALL QUOTED VERBATIM

Source read: `.claude/pipeline/qa/TASK-1306-verify.md` (read-only; **not edited** — see §6). The fact was located **by content** (`SC-§138`), by searching the whole file for every injector token, not by jumping to an address. The two anchors the row predicted both still exist and both carry part of the answer; a third section (the acceptance table) carries a fourth independent attribution.

### 1.1 — The section, quoted by its own heading

> `## 3 · ⚠️ THE KEYBOARD NULL — control pair RE-TAKEN, and it is stronger this time than at loop 0`

### 1.2 — The decisive row of that section's table, verbatim

> `| **`Down`** (t=77.76, 111.61) | `binding_found: true`, `bound_actions: ["IA_MenuDown"]`, `enhanced_input_active: true` | ⛔ **none** — builder root still `focused: true` | `-1`, `-1` |`

The injected thing is named as a **key** (`Down`), and the row reports `bound_actions` — a **key → action resolution**, which only a key-press tool produces. An `inject_input_action` call takes the action itself as its argument and has no key to resolve.

### 1.3 — The section's own conclusion sentence, which names the tool outright, verbatim

> `⇒ **`simulate_key_press` still injects BELOW Slate, at the player-controller / Enhanced Input layer**, and `TASK-1304` is Slate-native by design. **The `Down` null has a proven alternative cause that has nothing to do with this diff.**`

⭐ **This is the attribution, made by the verifier in its own words:** the sentence that explains the `Down` null names `simulate_key_press` as the injector responsible for it.

### 1.4 — The control key in the same table, carrying `simulate_key_press`'s signature warning, verbatim

> `| **`Tab`** (t=110.76) | `binding_found: false`, `applied_mapping_contexts: ["IMC_MainMenu"]`, warning verbatim: *"Key Tab was **delivered to the player controller**, but NOTHING BINDS IT… no legacy project-settings mapping matches, and neither the player controller nor its pawn binds the key directly."* | ⛔ **none** — builder root still `focused: true` | `-1` |`

That warning string — *"Key Tab was delivered to the player controller"* — is the exact text `VER-§8` cl. 1 quotes as **`simulate_key_press`'s own words**. `Down` sits in the same table, with the same return-field schema, under the same conclusion sentence.

### 1.5 — The instruments table, quoted from `## Instruments and their scope (`SC-§119`)`, which scopes the *other* tool away from this limb

> `| `simulate_key_press` | Enhanced Input / player-controller layer **only** — measured, twice, with a firing control | ⛔ **the Slate key route this feature lives on** |`
>
> `| `inject_input_action` | Enhanced Input actions by asset path (the menu route in) | ⛔ Slate key events |`

⭐ **`inject_input_action`'s declared scope in this run is, in the verifier's own parenthesis, *"the menu route in"* — and nothing else.**

### 1.6 — The fourth attribution: the acceptance table's own notation, and the `## 1` route-in line

Acceptance line **5b-3**, verbatim:

> `| **5b-3** | inject **one** `Down` ⇒ `FocusedCardIndex` must be **≠ -1** | live widget property via `get_widget_property_in_pie` | **`-1`** before (t=43.52), **`-1`** after the first `Down` (t=78.58), **`-1`** after the second `Down` (t=112.43). ⛔ Attributable to the injector, not the feature — control re-taken at §3 | **unobs** |`

Against `## 1 · THE FOCUS-PATH READ`'s route-in line, verbatim:

> `Route in: the existing `TASK-1274` keyboard route on `L_MainMenu` — `IA_MenuDown` ×2 then `IA_MenuAccept` (`/Game/Input/Actions/…`), the same route loop 0 used.`

⭐ **The document's notation is internally consistent and splits exactly along the tool boundary:** where it means `inject_input_action` it names an **action, with an asset path** (`IA_MenuDown`, `IA_MenuAccept`, `/Game/Input/Actions/…`); where it means `simulate_key_press` it names a **bare key** (`Down`, `Tab`). Every builder-side injection is written in the key form.

### 1.7 — What the report does **not** contain, stated so the construction is visible

⚠️ The report does **not** print a literal call string such as `simulate_key_press(key="Down")`. The identification rests on **four mutually independent textual attributions inside the one document** — the §3 conclusion sentence naming the tool for the `Down` null (1.3), `simulate_key_press`'s signature warning on the paired control key in the same table (1.4), the instruments table scoping `inject_input_action` to the menu route in only (1.5), and the consistent key-vs-action-path notation split (1.6). They all point one way and none of them is required by the others. **I am naming the construction rather than asserting a call-log I do not hold** (`SC-§70`; bound stated in full at §5).

⇒ ⛔ **The row's `UNDETERMINED` stop condition — *"IF THE REPORT DOES NOT SAY"* — did NOT fire.** The report says.

---

## 2 · THE BRANCH, SELECTED BY THE QUOTE

| branch as pre-written on the row | selected? | what selects it |
|---|---|---|
| **(a)** `inject_input_action` on `IA_MenuDown` ⇒ candidate discriminator, route (i) favoured, premise stands | ⛔ **NO** for the measuring limb | `inject_input_action` is scoped in the report to *"the menu route in"* and was never fired while the builder held focus |
| **(b)** `simulate_key_press "Down"` ⇒ **the pair proves nothing about Enhanced Input and the whole paragraph collapses** | ✅ **YES** | §3's conclusion sentence names `simulate_key_press` as the cause of the `Down` null; §3's table names the key `Down`; the paired `Tab` carries that tool's signature warning |
| **(c)** both used / neither recorded ⇒ enumerate in order, and say where each limb lands | ✅ **applies as the frame** | two tools were used at different points in one 5b leg — enumerated at §2.1 |

### 2.1 — The enumeration branch (c) requires, in order, with the reads each produced

| # | when | tool | argument | the read it produced |
|---|---|---|---|---|
| 1 | on `L_MainMenu`, before the builder existed | `inject_input_action` | action asset path, `IA_MenuDown` ×2 | `Button_2` ("Deck Builder") `focused: true`, `Button_0` now `false` — **it worked** |
| 2 | on `L_MainMenu` | `inject_input_action` | action asset path, `IA_MenuAccept` | the deck builder opened; root `WBP_DeckBuilder_C_0` `"focused": true` at t=43.51 |
| 3 | **inside the builder**, t=77.76 | **`simulate_key_press`** | **key `Down`** | `FocusedCardIndex` **`-1`**; builder root still `focused: true` |
| 4 | inside the builder, t=110.76 | `simulate_key_press` | key `Tab` (the control) | `FocusedCardIndex` `-1`; builder root still `focused: true` |
| 5 | **inside the builder**, t=111.61 | **`simulate_key_press`** | **key `Down`** | `FocusedCardIndex` **`-1`**; builder root still `focused: true` |

⇒ Limbs **1–2** land in branch **(a)** — but on `WBP_MainMenu`, not in the builder, so they produce **no builder-side read** and cannot supply the discriminator. Limbs **3–5** land in branch **(b)**, and **limbs 3 and 5 are the entire content of the measurement the dispatch was asking about.**

### 2.2 — The rescue a reader will reach for, named and closed

🚨 A reader will notice that `simulate_key_press "Down"` reported `bound_actions: ["IA_MenuDown"]` and `enhanced_input_active: true`, and will say: *"then it WAS the Enhanced Input door after all, so branch (a) applies in substance."* **Three reasons that does not rescue the discriminator, and I state them because refusing the rescue is the whole point of the row:**

1. ⛔ **The row's branches are keyed to the tool name and order selection *by the quote, never by inference*.** The quote says `simulate_key_press`. Re-reading (b) into (a) on a mechanism argument is exactly the after-the-fact branch-picking the row forbids.
2. ⛔ **The claim *"the injection entered via Enhanced Input"* would itself rest on `binding_found` / `enhanced_input_active` — and `VER-§8` cl. 10 says that field *"MAY CORROBORATE AN OBSERVABLE. IT MAY NEVER *BE* THE OBSERVABLE."*** There is no observable of arrival in this run; there is a null. So even the rescue's premise is unmeasured.
3. ⛔ **The comparison is confounded on the TARGET, not just on the tool — and this is the deeper reason it collapses.** See §3.2. Under the generous reading the pair *still* discriminates nothing.

---

## 3 · THE MECHANISM NOTE — WHICH CLAUSE GOVERNS, AND WHAT THE NEGATIVE RESTED ON

### 3.1 — The governing clause

⭐ **The call actually measured was `simulate_key_press`, so `VER-§8` cl. 1 governs it**, in the clause's own words:

> `simulate_key_press` is ⛔ **delivered to the player controller / Enhanced Input and ⛔ NOT to Slate** — the tool's ⛔ own words: *"Key Tab was ⛔ delivered to the player controller."*

⇒ The injector enters the chain at the **player-controller / Enhanced Input** point. `VER-§8` cl. 11's route **(i)** — the focused `SButton` → Slate's navigation config → `SButton::OnKeyDown` — is entered **upstream of that point**, at `FSlateApplication`/the focused-widget key path. **`simulate_key_press` cannot enter there**, which is precisely what §3 of the verify report concluded and why it recorded `unobs` rather than a defect.

### 3.2 — And the target differed too, which is what makes the collapse over-determined

The two arms did not merely use different injectors; they read **different machinery**:

- **`TASK-1274`'s 7/7** moved focus on `WBP_MainMenu`'s buttons, and that report's own log quote names the mover: `[USiegeMenuInputSubsystem] IA_MenuAccept -> OnClicked.Broadcast() on 'Button_2' ("Deck Builder")`, with `IA_MenuUp / IA_MenuDown / IA_MenuAccept bound (Started)`. ⇒ **the main menu's focus movement under injection is driven by route (ii)'s machinery** — the `IA_Menu*` handlers — reached by injecting the action directly, i.e. **downstream of the `SetIgnoreInput` gate the premise is about.**
- **`TASK-1306`'s `-1`** read `FocusedCardIndex` on a grid the verify report calls, verbatim, *"Slate-native by design"*, and whose only writer it locates as *"`SetFocusedCardIndex` has exactly two non-test callers, both inside `HandleCardGridKey`"* (citing `qa/TASK-1305-report.md` §1.1).

⚠️ **HYPOTHESIS, labelled, in its own sentence, and I did not re-derive it from the C++:** *if* the builder grid is armed only from a Slate key path and has no `IA_Menu*` handler of its own, then **no Enhanced-Input-door injection of any kind — `simulate_key_press` or `inject_input_action` — could ever have armed it**, and the `-1` is the expected reading on either layer hypothesis. ⇒ **Even branch (a) would have been a weak discriminator, because the two arms were never comparable.** This sentence rests on the verify report's own prose plus its citation, **not** on any source read I took (`SC-§70`).

### 3.3 — What the report's own negative rested on: an OBSERVABLE, not `binding_found`

⭐ **`TASK-1306`'s negative rests on an OBSERVABLE, and the report is cl. 10-compliant.** Two observables carry it:

- `FocusedCardIndex` read live via `get_widget_property_in_pie` — **`-1`** at t=43.52 (before), t=78.58 (after the first `Down`), t=112.43 (after the second).
- The `ui_snapshot` `focused` field — *"builder root still `focused: true`"* after each key, i.e. **focus demonstrably did not move**, on a field the same session proved fires **both ways** (`WBP_MainMenu_C_0` root `false` while `Button_0` `true`; `Button_0`→`Button_2` on demand) and against a loop-0 negative control of `false`.

⛔ `binding_found` / `bound_actions` / `enhanced_input_active` appear in that report **only inside a column headed *"tool's own words"***, describing the **injection** side. They are never the effect being claimed. ⇒ **The null is a real negative about the injector's reach, exactly as cl. 10's worked right answer prescribes — and it is a negative about the INJECTOR, which is why it says nothing about the layer question.**

---

## 4 · WHAT WOULD STILL BE MISSING — INCLUDING ON BRANCH (a) (`SC-§39`)

### 4.1 — Even had the answer been (a), these would still be open

1. ⛔ **The target confound of §3.2 is untouched by the tool question.** A null on a Slate-native grid is not evidence about a menu driven by `IA_Menu*` handlers, whichever injector produced it.
2. ⛔ **`inject_input_action IA_MenuDown` fired *inside the builder* was never taken at all.** That cell is **UNENUMERATED, not REFUTED** — the instruments table scopes that tool to *"the menu route in"* in this run. *Unenumerated* and *refuted* are not the same word.
3. ⛔ **No agent arm on this project has ever pressed a key at the Slate layer.** Route (i) has never been entered by any instrument in the rig, only predicted in advance and labelled unmeasured.

### 4.2 — The further measurement that would move route (i) from better-supported to MEASURED

⛔ **NAMED ONLY. Not designed here, not run here, not boarded here** (`SC-§101` — the probes earn amendments, not this row; only the manager boards).

⭐ **A layer-discriminating instrument read against a REAL human keypress:** a distinguishable log line inside **`USiegeMenuInputSubsystem`'s `IA_MenuDown` handler** — route (ii)'s *only* machinery — read after 🧑 Jonathan presses a real `Down` on `L_MainMenu`.

- Handler **logs** ⇒ route (ii) carried it ⇒ **the `SetIgnoreInput` premise is wrong.**
- Handler **silent** while focus demonstrably moves ⇒ route (ii) is excluded on the real-key path ⇒ **route (i) is promoted from better-supported to measured.**
- ⭐ **It comes with a firing control already in hand:** that subsystem demonstrably logs on the *injected* arm (`TASK-1274`'s quoted `[USiegeMenuInputSubsystem] IA_MenuAccept -> …` line), so a silence would be a real silence and not a dead instrument (`SC-§137`).

**One alternative, named in a line:** a complement-control arm — `IMC_MainMenu` not applied (or `IA_MenuDown` unmapped) at the instant of his real press. If focus still moves, only Slate can have carried it.

### 4.3 — `TASK-1390`, explicitly not pre-empted

⚠️ **`TASK-1390`'s result may moot or strengthen everything above, and I take no position on its outcome.** If the `1.0.6` `ui_perform` genuinely actuates Slate, the rig gains a Slate-side arm and §4.2's discriminator becomes runnable **without** 🧑 his hands — which would change what *"still missing"* means. If it does not, §4.2 stands as written. **Nothing in this report depends on which way it lands, and nothing here should be read as predicting it.**

---

## 5 · THIS REPORT'S OWN BOUND (`SC-§70`)

- ⛔ **TEXT ONLY.** No PIE, no MCP, no engine call, no `unreal_inspector` call, no editor lifecycle action, no git, no code read. 🧑 His GUI editor (PID 26992) was not touched, censused or reached in any way. Two `playtest-verifier` probes were running concurrently; I consumed no `VER-§2` budget because I made no engine call.
- ⚠️ **The decisive fact has single-document provenance: the verifier's own prose.** I could not compare that prose against `TASK-1306`'s tool-call transcript — **I do not hold it**. ⇒ **If the verifier mis-named its own instrument in writing, I inherit the error and this report would be wrong with it.** That possibility is **mitigated** by the four mutually independent attributions at §1.7 and by the notation split at §1.6; it is **not closed**, and it is not closable from text.
- ⚠️ **I did not read the deck-builder C++.** *"Slate-native by design"*, *"`TASK-1304`"*, and the `HandleCardGridKey` caller count are **quoted from `qa/TASK-1306-verify.md` and its own citation to `qa/TASK-1305-report.md` §1.1** — not re-derived by me at source. §3.2's hypothesis is labelled accordingly.
- ⚠️ **I did not read `qa/TASK-1286-verify.md`** (loop 0). `VER-§8` cl. 1's summary of that session's control pair is **law text I read, not a measurement I took**, and I have not leaned on it for the branch selection.
- ⛔ **I measured which tool was used. I did NOT measure which layer any key reached.** Nothing in this report says a key reached Slate, and nothing says one did not. `VER-§8` cl. 11's entitled sentence stands exactly as written: ***a real key press on `L_MainMenu` does reach the menu; which layer carried it is unmeasured.***
- ⛔ **The `SetIgnoreInput` premise is untouched by this row — neither strengthened nor weakened.** What is removed is one purported *argument* for route (i), not the premise, and not route (i) itself. Route (i) remains the better-supported **hypothesis** for exactly the reasons it already had before this row ran — no more, and no fewer.

---

## 6 · FENCES — DISCHARGED EXPLICITLY

- ✅ **`.claude/pipeline/qa/TASK-1306-verify.md` was NOT edited.** Opened read-only; it is the verifier's own testimony about the rig and it stays as written (`VER-§8` cl. 3(c)).
- ✅ **`VER-§8` cl. 11 was NOT amended.** It stands as written today. `CONVENTIONS.md` was read only. No clause anywhere was touched — the probes earn amendments, and this row takes none (`SC-§101`).
- ✅ **No `PASS`/`FAIL` verdict was written**, over this row or any other. `TASK-1306`'s `UNOBSERVABLE` is untouched and was not re-verdicted.
- ✅ No PIE, no MCP/engine call, no code, no asset, no git, no commit, no other agent's report, no handoff, no `CLAUDE.md`, no other row's line. Board write: **this row's `status:` line only**, by `Edit`, never `replace_all`.
- ✅ No separate handoff written, as the row directs. This file is the whole deliverable; the commit rides `TASK-1392`.
