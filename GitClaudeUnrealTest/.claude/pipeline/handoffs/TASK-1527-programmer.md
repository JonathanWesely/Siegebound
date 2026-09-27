# Handoff — TASK-1527 (gameplay-programmer) — marker `TASK-1527-RECIPE-FOLLOWUP-WARNS`

Date 2026-09-26 (started) → 2026-09-27 (finished, local clock). Status on the board: `ready-for-qa`. Gate `TASK-1528`, host `TASK-1530`, no 5a/5b (no runtime criterion).
Text only, under `Tools/Verify/recipes/`. No source, asset, PIE, editor, `CONVENTIONS.md`, `.claude/agents/*`, `CLAUDE.md` or `qa/` file was written. No commit and no staging (see the git disclosure under *Not examined / limitations*).

## 1. Files touched: byte anchors

Measured `rb` with Python `hashlib` (my instant, after the last edit). "Before" = the `qa/TASK-1518.md` §5 anchors, which I re-measured equal at start.

| file | before sha256 (§5) | after sha256 | after bytes | after lines | line endings |
|---|---|---|---|---|---|
| `Tools/Verify/recipes/README.md` | `24c77b1c…5f8b81` | `c9ea8a9b8875ce5eeb480905d5fa8bdca8f4bd7ba258f10633e4b7669a9b7183` | 14374 | 97 | LF 97, CR 0 |
| `Tools/Verify/recipes/RCP-vsbot-capture-center-and-summon.md` | `8f76fd3d…ee96366c` | `43ebe20c8173c5d445dff2b2b01ddb9e037b9481d8ee488ce9974426edcbac41` | 31942 | 247 | LF 247, CR 0 |
| `Tools/Verify/recipes/RCP-deckbuilder-slot-and-card-edit.md` | `16eb942d…69ee740b` | `550ed8b34e2b0a0ec941496ebfd35a2b69e3b10dbf53c3fd047f736af36f0a6a` | 20084 | 167 | LF 167, CR 0 |
| `Tools/Verify/recipes/RCP-deckbuilder-set-active-by-keyboard.md` | `a85fa9db…37595fba` | `bbcff6516c5dbceb1ca067be00d173645563e075868403a3122b2106a4895c5b` | 32439 | 198 | LF 198, CR 0 |
| `Tools/Verify/recipes/RCP-play-unit-card-from-hand.md` | `4964961c…ca8bd092` | `3eb2d019c372f1d786967e7ba8ad03b15929bae1d5982bf77d56aa8fe44f425f` | 49130 | 298 | LF 298, CR 0 |
| `Tools/Verify/recipes/RCP-menu-to-deckbuilder.md` | `6f783f3d…9be9992` | `799f81edd0c5a7775181989b3c5965d9899ff0dbf0795453b9c626132f9fcde2` | 15411 | 116 | LF 116, CR 0 |

Also written: this handoff, and `TASKBOARD.md` (the `TASK-1527` `status:` line only, twice: `in-progress`, then `ready-for-qa`).

**Eight headings, per recipe file** (exact list and order checked by script): `## Source` · `## Plugin version` · `## Preconditions` · `## Steps` · `## Read-back` · `## Fences — not measured for` · `## Coordinate/resolution-dependent values` · `## Known hazards`. All five recipes pass. `README.md` is not a recipe; its seven `## ` headings are identical to the before-copy (compared by `diff`).

**Every fenced json block parses** (`json.loads` on each ```` ```json ```` block): set-active 3/3 · slot-and-card-edit 7/7 · menu 5/5 · play 7/7 · vsbot 2/2. The counts are unchanged from `qa/TASK-1518.md` §0. The one JSON edit is vsbot's Step 1 block, which gains one object (row 15a).

## 2. Each finding and how it was resolved

Line numbers are the final files'. The exact before/after text of every change is the diff in the Appendix.

### (1) WARNs, `qa/TASK-1518.md` §1
| id | resolution | where |
|---|---|---|
| **W1** | QA's fix text, verbatim: "Exactly one `played card` line in the session, the play's (t≈33.80), and none after it through the control interval (`1512` ctl: "no second `played card` line")." Tag widened to `[M: 1512 A3, ctl]`, since the t≈33.80 play is A3's. | vsbot `:165` |
| **W2** | **Option (b).** Row 15a appended after row 15: the row-5 `DeckComponent` read with `["DrawPile"]`, its position labelled `NOT MEASURED`, its read object cited as row 5's in-batch read at t=2.74 `[M: 1512 A1]`. It is added to the table (`:71`), to the JSON (`:102`, last object; the block header says "rows 1–15, then row 15a last", `:74`), and to the `NOT MEASURED` list (`:117`). The Step 3 tail now appends "after row 15a" (`:140`). Step 4 stays optional and says why (`:153`, `:158`). Read-back "the card left the hand" names row 5 → row 15a as the in-batch producer, with a2's values still attributed to Step 4's t≈55 read (`:183`). The control's `DrawPile` leg is marked as Step 4's (`:135`, `:185`). **Why (b), not (a):** `VER-§8` cl. 10(b) budgets any round trip at ≈70 s (marker `VER-8-10B-THE-LOWER-END-IS-NOT-A-FLOOR`). a2's batch ended at t=37.45, so a *required* Step 4 would be a plan the law says "DOES NOT FIT" the `t≈60 s` fence. The recipe's own hazard says to put every observable the verdict needs inside the batch, and (b) matches the play recipe's in-batch post-play `DrawPile` read (QA's own consistency point). **Residual, declared:** the control's `DrawPile` leg still has only Step 4 as a producer. QA's (b) covers the Read-back row only, and I did not add a second unmeasured-position read to the tail. | vsbot `:71`, `:74`, `:102`, `:117`, `:135`, `:140`, `:153`, `:156`, `:158`, `:183`, `:185` |
| **W3** | Fence: cites `[M: 1512 Recipe candidates Fences; Attempt 1]` for the death at ≈73 s (log), the ghost pawn at (798, −0, 98) and the quoted "died at ≈t=73 holding the zone". States "**a1 never read the zone `Blue`**" with its `Neutral`/`Red` reads. Labels "**alone, from ≈t=50**" `HYPOTHESIS` H2. Budget line: same split, and H2 labelled. | vsbot `:193`–`:195`, `:157` |
| **W4** | README play row "what it does" now teaches the current control shape: a mandatory pre-press ghost read gates the omitted-set arm on `bHidden: true`, and the no-play-interval control rides every batch. `qa/TASK-1512-verify.md` A2, A3, ctl added to the source cell. The stale "The amendment … is `TASK-1516`'s" is now past tense. | README `:54` |

### (2) CF-1, `qa/TASK-1518.md` §2, in `RCP-deckbuilder-slot-and-card-edit.md`
| site | resolution | where |
|---|---|---|
| "right-click only; no route, `VER-§8` cl. 12" | QA's text: "It does not make it the active deck; set-active is `RCP-deckbuilder-set-active-by-keyboard.md` (`VER-§8` cl. 12, 2026-09-26 amendment)." | slot `:55` |
| "Setting the active deck. Right-click only … Pending: `TASK-1511`" | Points to the set-active recipe and keeps only "`ui_perform` has no right mouse button" (`archer50` §1(d); `VER-§8` cl. 12 scope note). "Pending: `TASK-1511`" removed. | slot `:134` |
| hazard heading "No right mouse button." | "**No right mouse button through `ui_perform`.**" | slot `:165` |

### (3) Relabel follow-through (`DECK-§9` cl. 10) in `RCP-deckbuilder-set-active-by-keyboard.md`
| site | resolution | where |
|---|---|---|
| Precondition 2: "labels it working-tree only until `TASK-1514` commits; read that row's status line for the hash" | "committed in `0a5b8a7` (`DECK-§9` cl. 10, marker `DECK-9-10-RELABELLED-0A5B8A7`)". The runtime bind-line check is kept verbatim. | set-active `:32` |
| Sources line "…and `1509` W1 recorded as open" | Now points: "For the state of `1509` W1 (the held-key repeat), read that clause's W1 bullet (marker `DECK-9-10-W1-CLOSED-0500D51`); this recipe does not restate it (`SC-§126` cl. 7)." It does not write "closed" or "filtered" as a claim. The marker ID is the manager's and is cited as an address. | set-active `:17` |
| "A held press" fence "…recorded OPEN" | Same pointer. "the rig cannot see the held-key repeat" and `hold_seconds` `NOT MEASURED` are kept. | set-active `:168` |

### (4) Round-trip range (`VER-§8` cl. 10(b), marker `VER-8-10B-THE-LOWER-END-IS-NOT-A-FLOOR`)
| site | resolution | where |
|---|---|---|
| README "A batch cannot branch" bullet ("≈30–70 s of PIE clock") | "budget ≈70 s of PIE clock (same clause). Values below ≈30 s have been measured, so ≈30 s is not the cheapest a round trip can be; for the observed values, read the clause's 2026-09-26 amendment, marker `VER-8-10B-THE-LOWER-END-IS-NOT-A-FLOOR`." No range restated. | README `:84` |
| play recipe's quoted planning rule | The quote is untouched (byte-identical). Added beside it: "That is the 2026-09-22 text, quoted as written. Read it with the clause's 2026-09-26 amendment beside it (marker `VER-8-10B-THE-LOWER-END-IS-NOT-A-FLOOR`)." | play `:42` |
| **Third site, found by re-finding by text** (not in the row's list): play Step 3 "and the law's range is ≈30–70 s (`VER-§8` cl. 10(b))" | "and the law lists every value observed so far (`VER-§8` cl. 10(b), marker `VER-8-10B-THE-LOWER-END-IS-NOT-A-FLOOR`)." The 0.01 s argument still stands on `1512`'s ≈20/≈36 s. | play `:112` |

### (5) NITs N1–N11. All applied, none declined.
| id | resolution | where |
|---|---|---|
| N1 | Drops "or more", and cites a *Speed data* batch boundary: "a boundary between two of this run's *Speed data* batches spans ≈8.5 s (e.g. t=57.14, the last read of the tile-focused control batch → t=65.69, the `IA_MenuBack` of the next)". Adds "Not every stamp gap in `1511` is a call boundary". The 0.07 s reading stands. | set-active `:113` |
| N2 | `qa/TASK-1270-verify.md` row 2 (a 68-card slot, `unobs`, "carried by the state test `Siegebound.Deck.IllegalDeckCannotBecomeActive`"), labelled "a state test, not a PIE press". At the Fences and at the hazard. | set-active `:161`, `:191` |
| N3 | New fence: "**A target that is already the active deck: `NOT MEASURED`.**" The deck is not named; it says to pick a target other than Step 0's `ActiveDeckName`. | set-active `:160` |
| N4 | The handoff's fallback: "If the sequence validator rejects it, drop both `get_player_transform` objects; the walk does not depend on them." | vsbot `:116` |
| N5 | "The tail's ORDER is this recipe's composition, not a2's" (a2 read `CaptureOwner` at t=34.59, before LMB #2 at t=34.72). a2's in-batch half ran t=34.05 → 37.45 (≈3.4 s). With no waits, the tail's length is `NOT MEASURED`, and the report states the interval length next to the treatment window. | vsbot `:141`–`:142` |
| N6 | `component`/`properties`: `[L: VER-§12 cl. 7c]`. `name`: in the validator's "Valid params" list, `[L: VER-§8 cl. 10(c)]`. | vsbot `:114` |
| N7 | The Read-back walk row says the t=2.73 start value was read by a2 beside the row-5 zone reads, and that no JSON row takes a transform read before row 7. | vsbot `:177` |
| N8 | README tense: "`TASK-1516` recorded both in the recipe". Sibling on the same file, same NIT class: "`TASK-1516` amended the recipe's labels" (old: "…are `TASK-1516`'s to amend"). | README `:52`, `:74` |
| N9 | "(A2 read `RootComponent.RelativeLocation`, which equals the actor location only for an unattached root: `NOT MEASURED`)". | play `:276` |
| N10 | The no-play-interval row states `1512`'s length (t=34.05 → 54.60, ≈20.5 s; in-batch only t=34.05 → 37.45). The length inside this batch is `NOT MEASURED`, and the report states it next to the treatment window. | play `:141` |
| N11 | "It is its own recipe now: `RCP-vsbot-capture-center-and-summon.md` (`TASK-1515`)." | menu `:96` |

### (7) `qa/TASK-1524-verify.md`'s recipe findings
| item | resolution | where |
|---|---|---|
| **Menu Downs (candidate 2)** | Step 1: "measured TWICE, with different results. It is NOT reliable". `1511` 2 of 2 at 3 frames; `1524` 1 of 2 at 1-frame spacing (t=6.81, 6.82; "0.017 s apart"). Step 2 is now mandatory and carries the one-`IA_MenuDown` top-up if focus reads `Button_1` (measured: `Button_2` at t=13.76), with "anything else: `NOT MEASURED`, do not send Accept". **No wait added to the JSON.** The spacing sentence names the measured spacings (3 frames; 0.42 s), labels a wait step `NOT MEASURED`, and labels `1524`'s "`wait_pie_seconds 0.3`" a proposal. The Read-back, Fences and hazards follow. | menu `:53`–`:58`, `:61`–`:64`, `:88`, `:97`–`:98`, `:115` |
| **`EditingDeckIndex` on open (candidate 3 + both mismatches)** | Rule written at every site the manager grepped, plus one the grep missed: the builder opens with the ACTIVE deck's slot. It cites `DECK-§3` (quoted) as `[L]`, the measured pair (0 with deck1 active: `archer50`, `1511`; 2 with deck3 active: `1524` row 0), and `1524` H2 as `HYPOTHESIS` for the general claim (README rule: a source hypothesis stays labelled). Old values are kept as dated history. No active deck is named as current. The walk counts T − E from the open read E, never from 0 (set-active `:37`–`:41`, `:74`, `:93`, `:95`). Sites: menu Step 4 `:77`–`:81`, Read-back row 3–4 `:89`, the "0 = deck1 on open" line `:91`, Fences `:99`; set-active Precondition 3 `:36`, Precondition 4 `:37`–`:41`, the walk `:74`/`:93`/`:95`, Read-back `:144`/`:149`, Fences `:171`, hazard `:192`; slot-and-card-edit Precondition 1 `:27`–`:30`. **Site the grep missed:** README set-active row "`IA_MenuRight` × T", which assumed a start at 0 (`:55`). | as listed |
| **`IA_MenuLeft` on the bar (candidate 1)** | The "`IA_MenuLeft` on the bar … not driven" fence is replaced by "measured by `1524`, one slot per press, 2 of 2 (4→3, 3→2), one relay line per step, a focus read after each". The bar's ends stay `NOT MEASURED` (their own bullet). Added as a **variation of the walk** after Step 5 (return to the previous active deck), not as a new file, with Read-back rows. The snapshot params object 1524 used after a Left is not quoted, so Step 2's `DeckBar` object is labelled this recipe's substitution (`NOT MEASURED` after a Left). The set-then-restore save hygiene is a pointer only, to marker `VER-3-6C-A-RESTORED-WRITE-PROVES-THE-PAYLOAD`. | set-active `:122`–`:127`, `:150`–`:151`, `:162`–`:163` |
| Save-hygiene finding | Not in scope; not restated. The only line added is the pointer above, in the new variation (a set-then-restore run). | set-active `:127` |

### Consequential edits, each tied to a finding
- **Recording `1524` as a use** (needed to cite `[M: 1524 …]` without the files contradicting themselves). Short name defined in each file that cites it. Set-active "Read this first" "Measured once: one run" → "Measured in `1511` … Used again by `1524`" (`:8`–`:9`). Source bullets "Used by `1524`" with a verbatim *Recipes used* quote (set-active `:22`, menu `:13`). Plugin-version lines add `1524` (set-active `:27`, menu `:18`, README `:85`). The set-active first Fence adds `1524`'s second set-active and its slot 2 set (`:159`). README "last verified" cells for menu and set-active, and the paragraph under the table, which said "the other three have not been re-run" (now false) → "the other two" (`:52`, `:55`, `:58`–`:62`). README short-name line adds `1524` (`:46`).
- **"Amended by `TASK-1527`" provenance line** in each recipe's Source section (vsbot `:27`, slot `:14`, set-active `:23`, menu `:14`, play `:30`), and vsbot's law list adds the (4) marker (`:20`).
- **vsbot Precondition 2** (`:36`, `:38`): "The active deck is … deck4" → "must be …", plus "A later run's pre-registration read a different deck active `[M: 1524 Pre-registration]`, so Step 0's read decides, never this line." This follows (7)'s ⛔ "Do not name which deck is active today; the recipe reads it" and `TASK-1528` (5)'s "anywhere". The old line was a requirement, but it read like a present-tense fact that `1524` shows is no longer true. I did not name the deck `1524` read.

## 3. For the manager (flags, not edits: `SC-§82`)
1. **Row (7) says archer50's two Downs were "sent separately" (t=16.55 → 16.97). `qa/PLAYTEST-archer50-verify.md` §1(a) does not say that.** It says only "`IA_MenuDown` ×2 (t=16.55, 16.97)". The QA-passed recipe line reads "`archer50` does not say whether its two injections were one call or two; they landed 0.42 s apart". I kept the recipe's wording and did not write "separately" (`SC-§101`). If another source says so, it can be added with its citation.
2. **`VER-§8` cl. 12's 2026-09-26 amendment still lists `IA_MenuLeft` among the route's NOT-measured fences** ("The route, for boarding (cl. 2): … `IA_MenuLeft`, the bar ends, gamepad Y … were NOT measured"). `DECK-§9` cl. 10 struck `IA_MenuLeft` on `1524`'s measurement. The recipe now agrees with `DECK-§9` cl. 10. The cl. 12 sentence is law and outside my WRITES.
3. **Row (7)'s "0.3 s" is correctly labelled a proposal.** The recipe writes no wait between the Downs. If you want a spaced form, it needs a run that measures it.
4. README's "Plugin version was not read in any source run" now names `1524`, which is a use rather than a seed. The sentence was already loose about `1511`/`1512`; I kept its shape.

## 4. Where QA should look hardest
- **W2's choice of (b)** and the declared residual (the control's `DrawPile` leg is Step 4 only). This is the one place I argued an option rather than applying fixed text.
- **Row 15a is a new JSON object at a position no run measured.** It is QA's own option (b) text. Check the label is where QA wanted it (table, JSON header, `NOT MEASURED` list, Read-back).
- **The (7) rule and the HYPOTHESIS label.** `DECK-§3` is cited as `[L]`, the two runs as `[M]`, and `1524` H2 stays `HYPOTHESIS` for the general claim. Check that no line states a fixed 0 as the rule or names a current active deck. Dated history ("0 when deck1 was active", `1511` "left deck4 active") is kept as history.
- **The Left variation's quoted stamps:** press t=62.31/62.64 (candidate 1), focus reads t=62.63 and t=62.96/62.98 (A1), 2/2 (*Not examined*), restore reads t=68.43 → 69.01 (A1).
- **The `1524` *Recipes used* quotes** in set-active `:22` and menu `:13` use ellipses. Compare them with the report's text.

## Not examined / limitations
- **Git disclosure.** Row (6) forbids git. Early on I ran read-only `git status --short`, `git log --oneline -3` and `git diff --numstat` against `Tools/Verify/recipes/` to confirm the start state. Nothing was staged, committed or checked out. `git status` may refresh the index's stat cache, which is not a content change. Every later comparison used plain `diff` against my scratchpad copies of the §5-anchored bytes.
- No recipe was executed. No PIE, no editor, no MCP call. No tool schema was read this session, so every `[S]` spelling is carried over, not re-checked.
- `DECK-§3`'s open-slot rule is measured at two points only: 0 with deck1 active (two runs) and 2 with deck3 active (one run). The mechanism is not read (`1524` H2), and I did not read `DeckBuilderWidget.cpp` for it.
- `NOT MEASURED` and left so: row 15a's position; the vsbot tail's interval length; a wait step between the menu Downs (and any drop rate); the menu top-up's envelope; a top-up from `Button_0`; the `DeckBar` snapshot after an `IA_MenuLeft`; Left from slots other than 4 and 3; the bar's two ends; a target that is already active; starting `EditingDeckIndex` values other than 0 and 2.
- I did not read the state test `Siegebound.Deck.IllegalDeckCannotBecomeActive`. N2 cites what `qa/TASK-1270-verify.md` row 2 says about it, nothing more.
- Markdown rendering was not checked. The tables were checked only for their pipe structure by reading.
- The line numbers in `qa/TASK-1518.md` and on the row were QA's and the manager's at their instant. Every site was re-found by text.

## Appendix: exact before/after (`diff -U0`, before = the §5-anchored bytes, after = my final bytes)

```diff
--- a/Tools/Verify/recipes/README.md
+++ b/Tools/Verify/recipes/README.md
@@ -46 +46 @@
-Run short names used in tags: `archer50` = `.claude/pipeline/qa/PLAYTEST-archer50-verify.md`; `1391` = `.claude/pipeline/qa/TASK-1391-verify.md`. For the play recipe's entry step only: `1230` = `qa/TASK-1230-verify.md`; `1270` = `qa/TASK-1270-verify.md`; `1314A` = `qa/TASK-1314-verify.md` LIMB A; `1348` = `qa/TASK-1348-verify.md`. For the two recipes added by `TASK-1515`: `1511` = `qa/TASK-1511-verify.md`; `1512` = `qa/TASK-1512-verify.md`; `1509` = `qa/TASK-1509.md`. `1509` is a pre-compile code review, cited for code reads only and never as a runtime measurement.
+Run short names used in tags: `archer50` = `.claude/pipeline/qa/PLAYTEST-archer50-verify.md`; `1391` = `.claude/pipeline/qa/TASK-1391-verify.md`. For the play recipe's entry step only: `1230` = `qa/TASK-1230-verify.md`; `1270` = `qa/TASK-1270-verify.md`; `1314A` = `qa/TASK-1314-verify.md` LIMB A; `1348` = `qa/TASK-1348-verify.md`. For the two recipes added by `TASK-1515`: `1511` = `qa/TASK-1511-verify.md`; `1512` = `qa/TASK-1512-verify.md`; `1509` = `qa/TASK-1509.md`. `1509` is a pre-compile code review, cited for code reads only and never as a runtime measurement. For the menu and set-active recipes' 2026-09-26 amendments (`TASK-1527`): `1524` = `qa/TASK-1524-verify.md`.
@@ -52 +52 @@
-| [`RCP-menu-to-deckbuilder.md`](RCP-menu-to-deckbuilder.md) | `L_MainMenu` → Deck Builder with injected menu actions (`IA_MenuDown` ×2, `IA_MenuAccept` on `Button_2`), with the focus and builder reads that confirm each step | `qa/PLAYTEST-archer50-verify.md` §1(a) | 2026-09-26 (source run). **Re-verified in-run 2026-09-26 by `TASK-1511`**: Steps 0–4, and the batched `IA_MenuDown` ×2 landed 2/2 (`qa/TASK-1511-verify.md` *Recipes used*, *Speed data*). `TASK-1512` used Step 0 only. Recording that in the recipe itself is `TASK-1516`'s (its row, (3)). |
+| [`RCP-menu-to-deckbuilder.md`](RCP-menu-to-deckbuilder.md) | `L_MainMenu` → Deck Builder with injected menu actions (`IA_MenuDown` ×2, a focus read with a one-`IA_MenuDown` top-up if it shows `Button_1`, `IA_MenuAccept` on `Button_2`), with the focus and builder reads that confirm each step. The builder opens on the ACTIVE deck's slot (`DECK-§3`); the recipe reads it | `qa/PLAYTEST-archer50-verify.md` §1(a); the top-up and the open slot: `qa/TASK-1524-verify.md` *Recipes used*, *Not examined*, *Recipe candidates* 2–3 | 2026-09-26 (source run). **Re-verified in-run 2026-09-26 by `TASK-1511`**: Steps 0–4, and the batched `IA_MenuDown` ×2 landed 2/2 (`qa/TASK-1511-verify.md` *Recipes used*, *Speed data*). `TASK-1512` used Step 0 only. `TASK-1516` recorded both in the recipe (its row, (3)). **Used again 2026-09-26 by `TASK-1524`**, "re-verified in-run **y, with a mismatch**": the batched `IA_MenuDown` ×2 landed 1 of 2, and Step 4 read `EditingDeckIndex = 2`, not `0` (`qa/TASK-1524-verify.md` *Recipes used*). `TASK-1527` recorded both in the recipe. |
@@ -54,2 +54,2 @@
-| [`RCP-play-unit-card-from-hand.md`](RCP-play-unit-card-from-hand.md) | Play an `ECardType::Unit` card from hand on `L_Arena` in ONE batch that fits the `t≈60 s` fence: an in-batch hand read (documents the slot, does not select it), `IA_Card<N>` entry, an omitted-set control, then aim set + confirm + dependent reads. The two-call shape is kept as an alternative that does not fit the fence. ⚠️ **What `TASK-1512` measured with it:** the entry, the confirm and the post-reads ran together in one batch once, with **no aim set** (`qa/TASK-1512-verify.md` A3). The planned omitted-set control **placed the unit**, because the ghost was already visible at a legal point. The aim has still never run in one batch with the entry. The amendment to the recipe itself is `TASK-1516`'s (its row, (1)(a)–(b)). | `qa/TASK-1391-verify.md` C2–C6, §3, §4; entry step: `qa/TASK-1230-verify.md` row 1, `qa/TASK-1270-verify.md` row 3, `qa/TASK-1314-verify.md` LIMB A ceiling 2, `qa/TASK-1348-verify.md` P4/P10 | 2026-09-22 (play source run; entry sources 2026-09-14…2026-09-20). **Used in part 2026-09-26 by `TASK-1512`**, outside its aim fence; it re-verified the recipe "partly" (`qa/TASK-1512-verify.md` *Recipes used*). |
-| [`RCP-deckbuilder-set-active-by-keyboard.md`](RCP-deckbuilder-set-active-by-keyboard.md) | Make a deck the ACTIVE deck from a freshly opened Deck Builder, with injected actions only. The sequence: `IA_MenuDown` to arm the grid; then `IA_MenuBack` + `IA_MenuRight` × T in one batch, with a `DeckBar` snapshot per step; then `IA_MenuSecondary` + the `OutlineBorder.BrushColor` reads in one batch; then a disk read. It drives door 3 only: not the real `Home` key and not gamepad Y. **It changes the player's real save.** | `qa/TASK-1511-verify.md` rows 0, A1–A3; *Recipe candidates*; *Speed data*; *Pixel note*; *Save hygiene* | 2026-09-26 (source run; not re-verified since) |
+| [`RCP-play-unit-card-from-hand.md`](RCP-play-unit-card-from-hand.md) | Play an `ECardType::Unit` card from hand on `L_Arena` in ONE batch that fits the `t≈60 s` fence: an in-batch hand read (documents the slot, does not select it), `IA_Card<N>` entry, then a mandatory ghost read immediately before every press. That pre-press read is the gate: the omitted-set press counts as a control only if it shows `bHidden: true`. Then aim set + pre-press ghost read + confirm + dependent reads. The no-play-interval control rides every batch. The two-call shape is kept as an alternative that does not fit the fence. ⚠️ **What `TASK-1512` measured with it:** the entry, the confirm and the post-reads ran together in one batch once, with **no aim set** (`qa/TASK-1512-verify.md` A3). The planned omitted-set control **placed the unit**, because the ghost was already visible at a legal point; that is why the gate exists. The aim has still never run in one batch with the entry. `TASK-1516` amended the recipe for this (its row, (1)(a)–(b)). | `qa/TASK-1391-verify.md` C2–C6, §3, §4; entry step: `qa/TASK-1230-verify.md` row 1, `qa/TASK-1270-verify.md` row 3, `qa/TASK-1314-verify.md` LIMB A ceiling 2, `qa/TASK-1348-verify.md` P4/P10; the gate, the no-play-interval control and the one composed batch: `qa/TASK-1512-verify.md` A2, A3, ctl | 2026-09-22 (play source run; entry sources 2026-09-14…2026-09-20). **Used in part 2026-09-26 by `TASK-1512`**, outside its aim fence; it re-verified the recipe "partly" (`qa/TASK-1512-verify.md` *Recipes used*). |
+| [`RCP-deckbuilder-set-active-by-keyboard.md`](RCP-deckbuilder-set-active-by-keyboard.md) | Make a deck the ACTIVE deck from a freshly opened Deck Builder, with injected actions only. The sequence: `IA_MenuDown` to arm the grid; then `IA_MenuBack` + one `IA_MenuRight` per slot in one batch, counted from the slot the builder opened on (`EditingDeckIndex`, the active deck's slot, read, never assumed), with a `DeckBar` snapshot per step; then `IA_MenuSecondary` + the `OutlineBorder.BrushColor` reads in one batch; then a disk read. A variation walks back with `IA_MenuLeft` to restore the previous active deck. It drives door 3 only: not the real `Home` key and not gamepad Y. **It changes the player's real save.** | `qa/TASK-1511-verify.md` rows 0, A1–A3; *Recipe candidates*; *Speed data*; *Pixel note*; *Save hygiene*; the open slot and the `IA_MenuLeft` variation: `qa/TASK-1524-verify.md` row 0, A1, *Recipes used*, *Recipe candidates* 1, 3 | 2026-09-26 (source run). **Used 2026-09-26 by `TASK-1524`**, "re-verified in-run **y, with a mismatch**": Precondition 4 read `EditingDeckIndex` 2, not 0 (`qa/TASK-1524-verify.md` *Recipes used*). `TASK-1527` recorded it and added the `IA_MenuLeft` variation from the same run. |
@@ -59 +59 @@
-- `RCP-menu-to-deckbuilder.md` was re-verified in-run by `TASK-1511`;
+- `RCP-menu-to-deckbuilder.md` was re-verified in-run by `TASK-1511`, and used again by `TASK-1524` with a mismatch (see the table);
@@ -61 +61,2 @@
-- the other three have not been re-run since they were seeded.
+- `RCP-deckbuilder-set-active-by-keyboard.md` was used by `TASK-1524` with a mismatch (see the table);
+- the other two have not been re-run since they were seeded.
@@ -73 +74 @@
-- *The whole play batch as composed in `RCP-play-unit-card-from-hand.md`* → `TASK-1512` a2 ran the entry, the confirm and the post-reads in one batch once, with no aim and outside that recipe's aim fence. The recipe's labels are `TASK-1516`'s to amend.
+- *The whole play batch as composed in `RCP-play-unit-card-from-hand.md`* → `TASK-1512` a2 ran the entry, the confirm and the post-reads in one batch once, with no aim and outside that recipe's aim fence. `TASK-1516` amended the recipe's labels.
@@ -83,2 +84,2 @@
-- **A batch cannot branch on its own result** (`1391` §4). Anything a later step depends on choosing (which card, which slot) must be known before the batch is sent. Otherwise the batch must tolerate whatever it meets and document it by read, which is the play recipe's primary shape (`VER-§8` cl. 10(b), 2026-09-22). A second call to learn it costs a round trip: ≈30–70 s of PIE clock, budget at the upper end (same clause).
-- **Plugin version was not read in any source run**: not in `archer50` or `1391` (the first two seeds), and not in `1511` or `1512` (each report's *Not examined*: "Aura plugin version not read"). ⇒ the first use of every recipe after this date re-verifies in-run.
+- **A batch cannot branch on its own result** (`1391` §4). Anything a later step depends on choosing (which card, which slot) must be known before the batch is sent. Otherwise the batch must tolerate whatever it meets and document it by read, which is the play recipe's primary shape (`VER-§8` cl. 10(b), 2026-09-22). A second call to learn it costs a round trip: budget ≈70 s of PIE clock (same clause). Values below ≈30 s have been measured, so ≈30 s is not the cheapest a round trip can be; for the observed values, read the clause's 2026-09-26 amendment, marker `VER-8-10B-THE-LOWER-END-IS-NOT-A-FLOOR`.
+- **Plugin version was not read in any source run**: not in `archer50` or `1391` (the first two seeds), and not in `1511`, `1512` or `1524` (each report's *Not examined*: "Aura plugin version not read"). ⇒ the first use of every recipe after this date re-verifies in-run.
--- a/Tools/Verify/recipes/RCP-vsbot-capture-center-and-summon.md
+++ b/Tools/Verify/recipes/RCP-vsbot-capture-center-and-summon.md
@@ -3 +3 @@
-Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are defined in `README.md`. Run short names: `1512` = `.claude/pipeline/qa/TASK-1512-verify.md` (the run; **a2** = its attempt 2, the measured route; **a1** = its attempt 1, the measured failure).
+Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are defined in `README.md`. Run short names: `1512` = `.claude/pipeline/qa/TASK-1512-verify.md` (the run; **a2** = its attempt 2, the measured route; **a1** = its attempt 1, the measured failure) · `1524` = `.claude/pipeline/qa/TASK-1524-verify.md` (Precondition 2 only).
@@ -20 +20 @@
-  - `VER-§8` cl. 10(b): the `t≈60 s` fence.
+  - `VER-§8` cl. 10(b): the `t≈60 s` fence, and the round-trip budget (marker `VER-8-10B-THE-LOWER-END-IS-NOT-A-FLOOR`; Step 4).
@@ -26,0 +27 @@
+- Amended by `TASK-1527`, 2026-09-26, on `qa/TASK-1518.md` W1–W3 and N4–N7 (W2: option (b), row 15a), and Precondition 2's wording (the active deck is read, never assumed).
@@ -35 +36 @@
-2. **The active deck is an all-Unit, single-card, legal deck: deck4 = 50× `Archer`.** `1512` read it on disk before PIE: `ActiveDeckName = deck4`. `[M: 1512 Pre-registration]`
+2. **The active deck must be an all-Unit, single-card, legal deck: deck4 = 50× `Archer`.** `1512` read it on disk before PIE: `ActiveDeckName = deck4`. `[M: 1512 Pre-registration]`
@@ -37 +38 @@
-   - To make deck4 active, use `RCP-deckbuilder-set-active-by-keyboard.md`: `1511` set it and left it active, and `1512` read it there.
+   - To make deck4 active, use `RCP-deckbuilder-set-active-by-keyboard.md`: `1511` set it and left it active, and `1512` read it there. A later run's pre-registration read a different deck active `[M: 1524 Pre-registration]`, so Step 0's read decides, never this line.
@@ -69,0 +71 @@
+| 15a | post-play `DeckComponent` read, `["DrawPile"]`: the in-batch producer of "the card left the hand" (`qa/TASK-1518.md` W2, option (b)) | the read object is row 5's, measured in-batch at t=2.74 (`DrawPile` = 44× `"Archer"`) `[M: 1512 A1]`. **Its position here, after the play: `NOT MEASURED`.** a2 read `DrawPile` 43 after the play only at t≈55, outside the batch (Step 4) `[M: 1512 A3]` |
@@ -72 +74 @@
-The action objects, copy-paste ready (rows 1–15):
+The action objects, copy-paste ready (rows 1–15, then row 15a last):
@@ -99 +101,2 @@
-  {"type": "survey_pie_scene", "params": {"include": ["census"], "class_filter": "Archer"}}
+  {"type": "survey_pie_scene", "params": {"include": ["census"], "class_filter": "Archer"}},
+  {"type": "get_actor_property_in_pie", "params": {"name": "SiegePlayerController0", "component": "DeckComponent", "properties": ["DrawPile"]}}
@@ -111 +114 @@
-  - The `name`/`component`/`properties` keys are not in the sequence's published parameter union; measured use says they are accepted inside a sequence. `[L: VER-§12 cl. 7c]`
+  - The `component`/`properties` keys are not in the sequence's published parameter union; measured use says they are accepted inside a sequence. `[L: VER-§12 cl. 7c]` `name` is in the validator's own "Valid params" list, `['client_index', 'component', 'name', 'properties']`. `[L: VER-§8 cl. 10(c)]`
@@ -113 +116,2 @@
-  - **The transform reader.** `1512` reports position and velocity but names neither the reader nor the field. `get_player_transform` is on the sequence whitelist and returns location and velocity by its schema `[S]`. It is not quoted as a2's reader.
+  - **The transform reader.** `1512` reports position and velocity but names neither the reader nor the field. `get_player_transform` is on the sequence whitelist and returns location and velocity by its schema `[S]`. It is not quoted as a2's reader. If the sequence validator rejects it, drop both `get_player_transform` objects; the walk does not depend on them (`handoffs/TASK-1515-programmer.md` flag 1; `qa/TASK-1518.md` N4).
+  - **Row 15a's position.** The `DrawPile` read object is row 5's, measured in-batch at t=2.74 `[M: 1512 A1]`. No run has read it after the play inside a batch.
@@ -131 +135 @@
-  - `DrawPile` stayed **43** (read at t≈55);
+  - `DrawPile` stayed **43** (read at t≈55, by Step 4's call; the tail below has no `DrawPile` read);
@@ -136 +140,3 @@
-- **The in-batch tail:** append these objects to the Step 1 `actions` array, after row 15. The waits between these reads are not quoted; a2's reads landed at t=34.59, 35.28, 35.30 and 37.43.
+- **The in-batch tail:** append these objects to the Step 1 `actions` array, after row 15a. The waits between these reads are not quoted; a2's reads landed at t=34.59, 35.28, 35.30 and 37.43.
+  - **The tail's ORDER is this recipe's composition, not a2's.** a2 read `CaptureOwner` at t=34.59, **before** its second `LeftMouseButton` at t=34.72 `[M: 1512 A2, ctl]`; the tail below sends the press first. (`qa/TASK-1518.md` N5)
+  - **The interval's length.** a2's in-batch half of the interval ran t=34.05 → 37.45 (≈3.4 s, arithmetic from the stamps) `[M: 1512 ctl, Speed data]`. This tail writes no waits, so its own length is `NOT MEASURED`. The report states the interval's length, from its own stamps, next to the treatment window (the ctl row asks for "an equal interval"). `[M: 1512 ctl]`
@@ -147 +153 @@
-**Step 4 — the later call (optional, same session).** Everything here is a read; nothing depends on a set (`VER-§13` cl. 2).
+**Step 4 — the later call (optional, same session).** Everything here is a read; nothing depends on a set (`VER-§13` cl. 2). It is optional because row 15a produces "the card left the hand" inside the batch (`qa/TASK-1518.md` W2, option (b)). Step 4 is where a2 measured the values below, and it is the only producer of the control's `DrawPile` leg (Step 3).
@@ -150,2 +156,3 @@
-- `DrawPile` / `DiscardPile`: 43 / 1, read once through read-only editor Python on the PIE world ("to count `Hand`/`DrawPile`/`DiscardPile` = 6/43/1"). `[M: 1512 A3, Not examined]` An in-batch post-play `DrawPile` read is `NOT MEASURED`. The pre-play in-batch `DrawPile` read is measured (row 5).
-- Budget: a1's hero died at ≈t=73 holding the zone alone (Fences). Land this call well inside that.
+- `DrawPile` / `DiscardPile`: 43 / 1, read once through read-only editor Python on the PIE world ("to count `Hand`/`DrawPile`/`DiscardPile` = 6/43/1"). `[M: 1512 A3, Not examined]` The same `DrawPile` read inside the batch after the play is row 15a, at a position `NOT MEASURED`. The pre-play in-batch `DrawPile` read is measured (row 5). `DiscardPile` has no in-batch reader in this recipe.
+- Budget: a1's hero died ≈73 s after the arena opened, by log time `[M: 1512 Attempt 1]`. That it held the zone alone from ≈t=50 is `HYPOTHESIS` H2 (Fences). Land this call well inside that.
+  - By the law's planning rule this call does not fit inside the `t≈60 s` fence: budget a round trip at ≈70 s (`VER-§8` cl. 10(b), marker `VER-8-10B-THE-LOWER-END-IS-NOT-A-FLOOR`). a2's landed ≈17 s after its batch ended (Step 3). This is why "the card left the hand" has an in-batch producer (row 15a) and Step 4 stays optional.
@@ -158 +165 @@
-- Exactly one `played card` line over the control interval. `[M: 1512 ctl]`
+- Exactly one `played card` line in the session, the play's (t≈33.80), and none after it through the control interval (`1512` ctl: "no second `played card` line"). `[M: 1512 A3, ctl]`
@@ -170 +177 @@
-| the walk | transform reads (rows 7–8) | X −21007.8 (t=2.73) → −10710.2 at velocity 750 (t=16.79) → 250.89, stopped (t≤32.31); `CurrentHP 200` (t=32.33) `[M: 1512 A2]` |
+| the walk | transform reads (rows 7–8). The start value, X −21007.8 at t=2.73, was read by a2 beside the row-5 zone reads; no row of this recipe's JSON takes a transform read before row 7 (`qa/TASK-1518.md` N7) | X −21007.8 (t=2.73) → −10710.2 at velocity 750 (t=16.79) → 250.89, stopped (t≤32.31); `CurrentHP 200` (t=32.33) `[M: 1512 A2]` |
@@ -176 +183 @@
-| the card left the hand | `DrawPile` / `DiscardPile` (NOT the `Hand` array) | `DrawPile` 44 → 43, `DiscardPile` 1 (t≈55) `[M: 1512 A3]` |
+| the card left the hand | `DrawPile` (NOT the `Hand` array): row 5 before, row 15a after (in-batch; row 15a's position `NOT MEASURED`). Step 4 re-reads `DrawPile` and adds `DiscardPile` | `DrawPile` 44 (t=2.74, row 5) → 43 and `DiscardPile` 1, both read at t≈55 by Step 4's call `[M: 1512 A1, A3]` |
@@ -178 +185 @@
-| control | Step 3 | census stayed 1, `DrawPile` stayed 43, gold only rising, no second `played card` line `[M: 1512 ctl]` |
+| control | Step 3 (its `DrawPile` leg: Step 4) | census stayed 1, `DrawPile` stayed 43, gold only rising, no second `played card` line `[M: 1512 ctl]` |
@@ -186,2 +193,3 @@
-  - In a1 the hero reached the zone and held it alone from ≈t=50 to ≈t=73, then died. The ghost pawn spawned at (798, −0, 98), the zone read `Red` at t=83.41–85.28, and Red Cavalry, Knight, Footman and Archer were in the census. `[M: 1512 Attempt 1]`
-  - The killer was not read: `HYPOTHESIS` H2. `[M: 1512 H2]`
+  - In a1 the hero died ≈73 s after the arena opened, by log time, and the ghost pawn spawned at (798, −0, 98), inside the a2 box (arithmetic). The report's own fences sentence says it "died at ≈t=73 holding the zone". `[M: 1512 Recipe candidates Fences; Attempt 1]`
+  - **a1 never read the zone `Blue`.** `CaptureOwner` read `Neutral` at t=35.47 and 45.48, and `Red` at t=83.41, 84.57 and 85.28, with Red Cavalry, Knight, Footman and Archer in the census. `[M: 1512 Attempt 1]`
+  - That it held the zone **alone, from ≈t=50**, is `HYPOTHESIS` H2 ("it was there from ≈t=50 to ≈t=73"). The killer was not read. `[M: 1512 H2]`
--- a/Tools/Verify/recipes/RCP-deckbuilder-slot-and-card-edit.md
+++ b/Tools/Verify/recipes/RCP-deckbuilder-slot-and-card-edit.md
@@ -3 +3 @@
-Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are defined in `README.md`. `archer50` = `.claude/pipeline/qa/PLAYTEST-archer50-verify.md`.
+Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are defined in `README.md`. `archer50` = `.claude/pipeline/qa/PLAYTEST-archer50-verify.md` · `1524` = `.claude/pipeline/qa/TASK-1524-verify.md` (Precondition 1 only).
@@ -14 +14 @@
-- Seeded by `TASK-1502`, 2026-09-26. Revised the same day on QA loop 1 (`qa/TASK-1505.md` W1, N4). Not re-verified since.
+- Seeded by `TASK-1502`, 2026-09-26. Revised the same day on QA loop 1 (`qa/TASK-1505.md` W1, N4). Not re-verified since. Amended by `TASK-1527`, 2026-09-26: the set-active pointers (`qa/TASK-1518.md` CF-1) and Precondition 1 (`1524`, `DECK-§3`).
@@ -27 +27,4 @@
-1. The Deck Builder is open, reached by `RCP-menu-to-deckbuilder.md`; on open it read `EditingDeckIndex = 0`, `WorkingDeck = deck1`. `[M: archer50 §1(a)]`
+1. The Deck Builder is open, reached by `RCP-menu-to-deckbuilder.md`. **On open, `EditingDeckIndex` is the ACTIVE deck's slot; read it (that recipe's Step 4), never assume `0`.** `DECK-§3`: *"Opening the builder starts with the ACTIVE deck selected for editing"* `[L: DECK-§3]`.
+   - The source read `EditingDeckIndex = 0`, `WorkingDeck = deck1`, with deck1 active (`ActiveDeckName` read `deck1`, "unchanged", after the run). `[M: archer50 §1(a), Not-examined]`
+   - `1524` read `EditingDeckIndex=2` at open (t=19.83) when deck3 was active. `[M: 1524 row 0, Pre-registration, Recipes used]`
+   - That the builder does this in general is `HYPOTHESIS` H2 in that report ("One observation; mechanism not read"). `[M: 1524 H2]`
@@ -52 +55 @@
-- This **selects the deck for editing**. It does **not** make it the active deck (right-click only; no route, `VER-§8` cl. 12).
+- This **selects the deck for editing**. It does not make it the active deck; set-active is `RCP-deckbuilder-set-active-by-keyboard.md` (`VER-§8` cl. 12, 2026-09-26 amendment).
@@ -131 +134 @@
-- **Setting the active deck.** Right-click only, and `ui_perform` has no right button (`archer50` §1(d); `VER-§8` cl. 12). Pending: `TASK-1511`.
+- **Setting the active deck.** Not this recipe: see `RCP-deckbuilder-set-active-by-keyboard.md`. `ui_perform` has no right mouse button (`archer50` §1(d); `VER-§8` cl. 12 scope note).
@@ -162 +165 @@
-- **No right mouse button.** `click` + `"button":"right"` and `press`/`release` + `"button":"RightMouseButton"`/`"mouse_button":"right"` were each delivered as a left press; the extra keys were silently ignored. `[M: archer50 §1(d)]`; law `VER-§8` cl. 12.
+- **No right mouse button through `ui_perform`.** `click` + `"button":"right"` and `press`/`release` + `"button":"RightMouseButton"`/`"mouse_button":"right"` were each delivered as a left press; the extra keys were silently ignored. `[M: archer50 §1(d)]`; law `VER-§8` cl. 12.
--- a/Tools/Verify/recipes/RCP-deckbuilder-set-active-by-keyboard.md
+++ b/Tools/Verify/recipes/RCP-deckbuilder-set-active-by-keyboard.md
@@ -3 +3 @@
-Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are defined in `README.md`. Run short names: `1511` = `.claude/pipeline/qa/TASK-1511-verify.md` (the run) · `1512` = `.claude/pipeline/qa/TASK-1512-verify.md` (downstream corroboration only) · `1509` = `.claude/pipeline/qa/TASK-1509.md` (the pre-compile code review of the route; its sentences are code reads, **not** runtime measurements, and are cited by finding number).
+Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are defined in `README.md`. Run short names: `1511` = `.claude/pipeline/qa/TASK-1511-verify.md` (the run) · `1512` = `.claude/pipeline/qa/TASK-1512-verify.md` (downstream corroboration only) · `1524` = `.claude/pipeline/qa/TASK-1524-verify.md` (a later use of this recipe: the open slot and the `IA_MenuLeft` variation) · `1509` = `.claude/pipeline/qa/TASK-1509.md` (the pre-compile code review of the route; its sentences are code reads, **not** runtime measurements, and are cited by finding number).
@@ -8 +8,2 @@
-> - **Measured once:** one run, one set-active (target slot 3), one refusal (slot 4, empty deck5). `[M: 1511 A1, A3]`
+> - **Measured in `1511`:** one set-active (target slot 3), one refusal (slot 4, empty deck5). `[M: 1511 A1, A3]` **Used again by `1524`**, which opened on slot 2, set slot 3 with one `IA_MenuRight`, and later walked back to slot 2 with `IA_MenuLeft` and set it (the variation after Step 5). `[M: 1524 A1, Recipes used]`
+> - **The walk counts from the slot the builder opened on** (`EditingDeckIndex`, the active deck's slot, `DECK-§3`), read at open and never assumed to be 0 (Precondition 4).
@@ -16 +17,2 @@
-  - `DECK-§9` cl. 10 (marker `DECK-9-10-SET-ACTIVE-ROW`): the route's state label, what is not measured, and `1509` W1 recorded as open.
+  - `DECK-§9` cl. 10 (marker `DECK-9-10-SET-ACTIVE-ROW`): the route's state label and what is not measured. For the state of `1509` W1 (the held-key repeat), read that clause's W1 bullet (marker `DECK-9-10-W1-CLOSED-0500D51`); this recipe does not restate it (`SC-§126` cl. 7).
+  - `DECK-§3`: *"Opening the builder starts with the ACTIVE deck selected for editing"* (Precondition 4).
@@ -19 +21,3 @@
-- Boarded as `TASK-1515` by the manager on `TASK-1513` (3). Written by `TASK-1515`, 2026-09-26. Not re-verified since; `1511` is the only run.
+- Boarded as `TASK-1515` by the manager on `TASK-1513` (3). Written by `TASK-1515`, 2026-09-26.
+- **Used by `1524`**, 2026-09-26, `Verdict: VERIFIED`, 1 PIE session: "Steps 1–5 … + Control C3 … re-verified in-run **y, with a mismatch**: Precondition 4 (`EditingDeckIndex = 0`) did not hold — it read 2 — and `IA_MenuBack` put focus on `DeckSlotEntryWidget_2`, consistent with the recipe's own rule … so T counted from slot 2 (one Right to slot 3). Everything else matched". `[M: 1524 Recipes used]` Cited here for: row 0, A1 (both legs), *Pre-registration*, *Hypotheses* H2, *Not examined*, *Recipe candidates* 1 and 3.
+- Amended by `TASK-1527`, 2026-09-26: Precondition 4 and the walk count (`1524`, `DECK-§3`), the `IA_MenuLeft` variation (`1524`), and `qa/TASK-1518.md` N1–N3 plus the `DECK-§9` cl. 10 pointers.
@@ -23 +27 @@
-**Not read.** `1511` *Not examined*: "Aura plugin version not read." ⇒ the first use of this recipe re-verifies it in-run (`VER-§13` cl. 3, `VER-§8` cl. 5).
+**Not read.** `1511` *Not examined*: "Aura plugin version not read." `1524` did not read it either (same words, its *Not examined*). ⇒ the first use of this recipe re-verifies it in-run (`VER-§13` cl. 3, `VER-§8` cl. 5).
@@ -28 +32 @@
-2. **The binaries carry `TASK-1507`'s route.** `DECK-§9` cl. 10 labels it working-tree only until `TASK-1514` commits; read that row's status line for the hash. `[L: DECK-§9 cl. 10]` Check it at runtime: read the builder-open bind line **after** the builder opens. `[M: 1511 row 0]`
+2. **The binaries carry `TASK-1507`'s route**, committed in `0a5b8a7` (`DECK-§9` cl. 10, marker `DECK-9-10-RELABELLED-0A5B8A7`). `[L: DECK-§9 cl. 10]` Check it at runtime: read the builder-open bind line **after** the builder opens. `[M: 1511 row 0]`
@@ -32,2 +36,6 @@
-3. **The builder is freshly open, reached by `RCP-menu-to-deckbuilder.md` Steps 0–4.** `1511` re-verified that recipe in the same run: `Button_0` focused at t=4.89; `IA_MenuDown` ×2 in ONE `run_verification_sequence` → `Button_2` `focused:true` at t=9.62; `IA_MenuAccept` at t=14.60 → `WBP_DeckBuilder_C_0`, `EditingDeckIndex=0`, `WorkingDeck` deck1 at t=15.14. `[M: 1511 Recipes used]`
-4. **`EditingDeckIndex = 0`.** After Step 2, focus lands on `DeckSlotEntryWidget_<EditingDeckIndex>` `[M: 1511 Recipe candidates step 2]`, so the walk counts from that slot. Only 0 was measured. Any other starting value: `NOT MEASURED`.
+3. **The builder is freshly open, reached by `RCP-menu-to-deckbuilder.md` Steps 0–4.** `1511` re-verified that recipe in the same run: `Button_0` focused at t=4.89; `IA_MenuDown` ×2 in ONE `run_verification_sequence` → `Button_2` `focused:true` at t=9.62; `IA_MenuAccept` at t=14.60 → `WBP_DeckBuilder_C_0`, `EditingDeckIndex=0`, `WorkingDeck` deck1 at t=15.14 (deck1 was active in that run, `1511` *Pre-registration*). `[M: 1511 Recipes used]` That recipe's Step 1 can land 1 of 2 Downs; its Step 2 reads focus and tops up (`1524`). `[M: 1524 Recipes used]`
+4. **Read `EditingDeckIndex` at open (`RCP-menu-to-deckbuilder.md` Step 4) and call it E. It is the ACTIVE deck's slot, not a fixed 0.** `DECK-§3`: *"Opening the builder starts with the ACTIVE deck selected for editing"*. `[L: DECK-§3]`
+   - Measured: E = `0` when deck1 was active (`1511`: `EditingDeckIndex=0` at t=15.14, `ActiveDeckName = deck1` in its Pre-registration) `[M: 1511 Recipes used, Pre-registration]`; E = `2` when deck3 was active (`1524`: "Builder open t=19.83: `WBP_DeckBuilder_C_0`, `EditingDeckIndex=2`", `ActiveDeckName = deck3` in its Pre-registration) `[M: 1524 row 0, Pre-registration]`. That the builder does this in general is `HYPOTHESIS` H2 in `1524` ("One observation; mechanism not read"). `[M: 1524 H2]`
+   - After Step 2, focus lands on `DeckSlotEntryWidget_<EditingDeckIndex>` `[M: 1511 Recipe candidates step 2]`. `1524`: "`IA_MenuBack` put focus on `DeckSlotEntryWidget_2`, consistent with the recipe's own rule". `[M: 1524 Recipes used]`
+   - ⇒ **The walk counts from E, never from an assumed 0:** `IA_MenuRight` × (T − E) when T > E. `1524` sent one Right from slot 2 to reach slot 3 ("T counted from slot 2 (one Right to slot 3)"). `[M: 1524 Recipes used]` T < E needs `IA_MenuLeft` (measured only as the variation after Step 5). At a fresh open, T = E is the active deck itself (`DECK-§3`; Fences: target already active).
+   - Starting values other than 0 and 2: `NOT MEASURED`.
@@ -66 +74 @@
-**Step 2 + Step 3 — ONE sequence: `IA_MenuBack`, then `IA_MenuRight` × T, with a `DeckBar` snapshot after each step.** Shown for T = 3, the measured case:
+**Step 2 + Step 3 — ONE sequence: `IA_MenuBack`, then `IA_MenuRight` × (T − E), with a `DeckBar` snapshot after each step.** E is the `EditingDeckIndex` read at open (Precondition 4). Shown for T = 3 from E = 0, `1511`'s case (three Rights). With E = 2, `1524` sent one Right `[M: 1524 Recipes used]`: send one Right/wait/snapshot triple per slot of T − E.
@@ -85 +93 @@
-- "`inject_input_action /Game/Input/Actions/IA_MenuRight.IA_MenuRight` × T, `wait_pie_seconds 0.3` + the same `DeckBar` snapshot after each … Steps 2–3 ran as ONE `run_verification_sequence` (4/4 landed)." `[M: 1511 Recipe candidates step 3]`
+- "`inject_input_action /Game/Input/Actions/IA_MenuRight.IA_MenuRight` × T, `wait_pie_seconds 0.3` + the same `DeckBar` snapshot after each … Steps 2–3 ran as ONE `run_verification_sequence` (4/4 landed)." `[M: 1511 Recipe candidates step 3]` In `1511` E was 0, so its "× T" is × (T − E) (Precondition 4).
@@ -86,0 +95 @@
+- **From E = 2 (`1524`):** `IA_MenuBack` focused `DeckSlotEntryWidget_2` `[M: 1524 Recipes used]`; one Right then gave "focus on `DeckSlotEntryWidget_3/OutlineBorder/SlotButton` (snapshot t=26.60, relay line `…on deck-bar slot 2 ('deck3'); key-down handled=true; the focused bar slot is now 3 ('deck4')`)". `[M: 1524 A1]`
@@ -90 +99 @@
-**Step 4 — ONE sequence: before-reads, `IA_MenuSecondary`, wait 0.5 s, after-reads.** Shown for T = 3, with the active deck before the press on slot 0 (deck1):
+**Step 4 — ONE sequence: before-reads, `IA_MenuSecondary`, wait 0.5 s, after-reads.** Shown for T = 3, with the active deck before the press on slot 0 (deck1), as in `1511`:
@@ -104 +113 @@
-  - That the before-reads shared this batch is a reading from the stamps, not stated in words. 0.07 s separates the before-read from the press, while this run's separate calls were about 8.5 s or more apart (e.g. t=57.14 → 65.69).
+  - That the before-reads shared this batch is a reading from the stamps, not stated in words. 0.07 s separates the before-read from the press, while a boundary between two of this run's *Speed data* batches spans ≈8.5 s (e.g. t=57.14, the last read of the tile-focused control batch → t=65.69, the `IA_MenuBack` of the next). `[M: 1511 A1, A2, Speed data]` Not every stamp gap in `1511` is a call boundary (`qa/TASK-1518.md` N1).
@@ -112,0 +122,7 @@
+**Variation — return to the previous active deck with `IA_MenuLeft` (after Step 5, same session).** Measured once, by `1524` (A1 leg 2). `[M: 1524 A1, Not examined, Recipe candidates 1]` Use it to restore the before-value, P = the slot of Step 0's `ActiveDeckName` (slot map in Precondition 6).
+- **The walk back:** from the slot that holds focus, F (the last snapshot's single `focused: true`), send `IA_MenuLeft` × (F − P) as ONE sequence with Step 3's shape: the action path `/Game/Input/Actions/IA_MenuLeft.IA_MenuLeft` in place of `IA_MenuRight`, a wait and a snapshot after each press.
+- **What `1524` measured:** from slot 4 (focused after the deck5 refusals), `IA_MenuLeft` ×2 → "slot 3 focused (t=62.63) then slot 2 focused, slot 3 not (t=62.96/62.98)", with relay lines "`…slot 4 ('deck5') … now 3 ('deck4')` and `…slot 3 ('deck4') … now 2 ('deck3')`". `[M: 1524 A1]` The two presses were at t=62.31 and 62.64, "relay line per step". `[M: 1524 Recipe candidates 1]` Sent vs landed: "`IA_MenuLeft` 2/2". `[M: 1524 Not examined]` So `IA_MenuLeft` moves one slot per press, as `IA_MenuRight` does.
+- **Then Step 4's batch with target P**, reading the slot this run set as the "previous active" slot. `1524`: before (t=68.43/68.44) slot 3 `(1,0.5,0,1)`, slot 2 `(0,0,0,0)`; `IA_MenuSecondary` t=68.46; after (t=68.98–69.01) slot 2 `(1,0.5,0,1)`, slot 3 `(0,0,0,0)`, slot 4 `(0,0,0,0)`; log `active deck set to 'deck3' — the next match will use it.`; `ActiveDeckName=deck3` on disk in PIE and after PIE, every deck's card list identical by field to the pre-registration read: "Restored: YES". `[M: 1524 A1]`
+- **Spellings and waits:** the action path is `[D]` (`Content/Input/Actions/IA_MenuLeft.uasset`; the bind line in Precondition 2 names `IA_MenuLeft`). `1524`'s candidate names "`wait_pie_seconds 0.3` + a `ui_snapshot` of the expected `DeckSlotEntryWidget_<k>` after each" `[M: 1524 Recipe candidates 1]`; the stamps fit that spacing (press t=62.31 → focus read t=62.63). The snapshot's params object is not quoted: Step 2's `DeckBar` snapshot, used here, is this recipe's substitution (`NOT MEASURED` after a Left).
+- **Save hygiene for a run that sets and then restores:** `VER-§3` cl. 6(c), marker `VER-3-6C-A-RESTORED-WRITE-PROVES-THE-PAYLOAD`. This recipe does not restate it.
+
@@ -128 +144 @@
-| 2 | `DeckBar` `ui_snapshot`: exactly one `focused: true` | `DeckSlotEntryWidget_0/OutlineBorder/SlotButton` (t=65.69) `[M: 1511 A1]` |
+| 2 | `DeckBar` `ui_snapshot`: exactly one `focused: true`, on `DeckSlotEntryWidget_<E>` | `DeckSlotEntryWidget_0/OutlineBorder/SlotButton` (t=65.69; E = 0) `[M: 1511 A1]` · `DeckSlotEntryWidget_2` (E = 2) `[M: 1524 Recipes used]` |
@@ -133 +149,3 @@
-| side effect, checked | `EditingDeckIndex`, `WorkingDeck` | unchanged: `EditingDeckIndex` stayed 0 and `WorkingDeck` stayed deck1's 51 cards ("set-active did not re-select for editing") `[M: 1511 A1]` |
+| side effect, checked | `EditingDeckIndex`, `WorkingDeck` | unchanged from the open value: `EditingDeckIndex` stayed 0 and `WorkingDeck` stayed deck1's 51 cards ("set-active did not re-select for editing") `[M: 1511 A1]` · `EditingDeckIndex` stayed 2 through both of `1524`'s set-actives `[M: 1524 A1]` |
+| variation (each Left) | `DeckBar` snapshot + one relay log line | slot 3 focused (t=62.63), then slot 2 focused and slot 3 not (t=62.96/62.98); relay `…slot 4 ('deck5') … now 3 ('deck4')`, `…slot 3 ('deck4') … now 2 ('deck3')` `[M: 1524 A1]` |
+| variation (restore) | Step 4's reads + Step 5's disk reads on target P | slot 2 `(0,0,0,0)` → `(1,0.5,0,1)`, slot 3 `(1,0.5,0,1)` → `(0,0,0,0)` (t=68.43 → 69.01); `active deck set to 'deck3'`; disk `ActiveDeckName=deck3` in PIE and after PIE `[M: 1524 A1]` |
@@ -141,3 +159,5 @@
-- **One set-active target: slot 3 (deck4, a legal 50-card deck).** One refusal: slot 4 (deck5, 0 cards), reached by ONE more `IA_MenuRight` from slot 3 after the set-active, in a later batch, not by a four-step walk from slot 0. `[M: 1511 A3, Speed data]` Other `T`: `NOT MEASURED` as a set-active. The relay steps 0→1→2→3 and 3→4 are measured. `[L: VER-§8 cl. 12, 2026-09-26 amendment]`
-- **Illegal targets other than an empty deck.** deck1 (51 cards) and deck2 (80 cards) were not driven as targets. That the same gate refuses them rests on the refusal line's own text ("a legal deck is exactly 50"), not on a runtime press. `NOT MEASURED` here.
-- **`IA_MenuLeft` on the bar, and the bar's ends** (slot 0 Left, slot 9 Right): not driven. `[M: 1511 Not examined]`
+- **Set-active targets: slot 3 (deck4) and slot 2 (deck3), each a legal 50-card deck.** Slot 3 was set from E = 0 by `1511` and from E = 2 by `1524`; slot 2 was set by `1524` through the `IA_MenuLeft` variation. `[M: 1511 A1; 1524 A1]` Refusals: slot 4 (deck5, 0 cards), reached by ONE more `IA_MenuRight` from slot 3 after a set-active, in a later batch, not by a walk from the open slot. `1511` pressed it once `[M: 1511 A3, Speed data]`; `1524` pressed it twice `[M: 1524 A2]`. Other `T`: `NOT MEASURED` as a set-active. The relay steps 0→1→2→3 and 3→4 (`1511`) and 2→3, 3→4, 4→3, 3→2 (`1524`) are measured. `[L: VER-§8 cl. 12, 2026-09-26 amendment]` `[M: 1524 A1, A2, Not examined]`
+- **A target that is already the active deck: `NOT MEASURED`.** No run pressed `IA_MenuSecondary` on the slot that already held the orange outline. The Step 4 before/after reads could not tell a landed press from a dropped one there, because no `BrushColor` would move either way (this recipe's reading; `qa/TASK-1518.md` N3). Pick a target other than Step 0's `ActiveDeckName`.
+- **Illegal targets other than an empty deck.** deck1 (51 cards) and deck2 (80 cards) were not driven as targets. That the same gate refuses them rests on the refusal line's own text ("a legal deck is exactly 50"), not on a runtime press. `NOT MEASURED` here. The nearest record is `qa/TASK-1270-verify.md` row 2 (a right-click on a 68-card slot): `unobs` in that report and "carried by the state test `Siegebound.Deck.IllegalDeckCannotBecomeActive`". That is a state test, not a PIE press (`qa/TASK-1518.md` N2).
+- **`IA_MenuLeft` on the bar: measured by `1524`, one slot per press, 2 of 2** (4→3, 3→2), one relay line per step, a focus read after each (the variation after Step 5). `[M: 1524 A1, Not examined, Recipe candidates 1]` Left from other slots: `NOT MEASURED`.
+- **The bar's two ends** (slot 0 Left, slot 9 Right): not driven. `[M: 1511 Not examined]` Still `NOT MEASURED`: `1524` did not drive them either.
@@ -148 +168 @@
-- **A held press.** Every injection here is a tap. `1509` W1 (code review) says door 3 is `Started`-only, so the rig cannot see the held-key repeat; the held real `Home`/Y repeat is recorded OPEN. `[L: DECK-§9 cl. 10]` `hold_seconds` on `IA_MenuSecondary`: `NOT MEASURED`.
+- **A held press.** Every injection here is a tap. `1509` W1 (code review) says door 3 is `Started`-only, so the rig cannot see the held-key repeat. For the held real `Home`/Y repeat, read `DECK-§9` cl. 10's W1 bullet (marker `DECK-9-10-W1-CLOSED-0500D51`); this recipe does not restate its state (`SC-§126` cl. 7). `[L: DECK-§9 cl. 10]` `hold_seconds` on `IA_MenuSecondary`: `NOT MEASURED`.
@@ -151 +171 @@
-- **Scope of the run:** `L_MainMenu`'s Deck Builder; standalone single client; one PIE session; starting `EditingDeckIndex` 0; profile slot `SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34`.
+- **Scope of the runs:** `L_MainMenu`'s Deck Builder; standalone single client; one PIE session each; starting `EditingDeckIndex` 0 (`1511`) and 2 (`1524`), other starting values `NOT MEASURED`; profile slot `SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34`. `[M: 1511 Editor/Aura state; 1524 Editor/Aura state, row 0, Pre-registration]`
@@ -171,2 +191,2 @@
-  - Restoring a previous active deck means running this route again on that deck's slot, and the gate only accepts a legal (exactly 50) deck. `[M: 1511 A3]` deck1 read **51** cards before the run `[M: 1511 Pre-registration]`, so it cannot be made active again by this route until it reads 50. That the gate refuses a 51-card deck is the refusal text's reading here, not a press on deck1 (Fences).
-- **Set-active is not select-for-editing.** `EditingDeckIndex` stayed 0 and `WorkingDeck` stayed deck1's 51 cards after the press. `[M: 1511 A1]` Never read `WorkingDeck` as proof of set-active. The converse: a `double_click` on a slot selects it for editing and does not make it active (`RCP-deckbuilder-slot-and-card-edit.md` Step 2).
+  - Restoring a previous active deck means running this route again on that deck's slot (the `IA_MenuLeft` variation after Step 5, measured once by `1524`), and the gate only accepts a legal (exactly 50) deck. `[M: 1511 A3; 1524 A1]` deck1 read **51** cards before the run `[M: 1511 Pre-registration]`, so it cannot be made active again by this route until it reads 50. That the gate refuses a 51-card deck is the refusal text's reading here, not a press on deck1 (Fences; `qa/TASK-1270-verify.md` row 2's state test is a state test, not a PIE press).
+- **Set-active is not select-for-editing.** `EditingDeckIndex` stayed 0 and `WorkingDeck` stayed deck1's 51 cards after the press. `[M: 1511 A1]` In `1524`, `EditingDeckIndex` stayed 2 through both set-actives. `[M: 1524 A1]` Never read `WorkingDeck` as proof of set-active. The converse: a `double_click` on a slot selects it for editing and does not make it active (`RCP-deckbuilder-slot-and-card-edit.md` Step 2).
--- a/Tools/Verify/recipes/RCP-play-unit-card-from-hand.md
+++ b/Tools/Verify/recipes/RCP-play-unit-card-from-hand.md
@@ -29,0 +30 @@
+- Amended by `TASK-1527`, 2026-09-26, on `qa/TASK-1518.md` N9 and N10, and on `VER-§8` cl. 10(b)'s 2026-09-26 amendment (marker `VER-8-10B-THE-LOWER-END-IS-NOT-A-FLOOR`).
@@ -41 +42 @@
-   - The planning rule: "a round trip costs BETWEEN `≈30 s` AND `≈70 s` OF PIE CLOCK, EACH END OBSERVED ONCE ⇒ BUDGET AT THE UPPER END. ⇒ A PLAN THAT SPENDS EVEN ONE ROUND TRIP INSIDE THE `t≈60 s` FENCE DOES NOT FIT, AND *"it fit last time"* IS LUCK, NOT A BUDGET." `[L: VER-§8 cl. 10(b), 2026-09-22 correction]`
+   - The planning rule: "a round trip costs BETWEEN `≈30 s` AND `≈70 s` OF PIE CLOCK, EACH END OBSERVED ONCE ⇒ BUDGET AT THE UPPER END. ⇒ A PLAN THAT SPENDS EVEN ONE ROUND TRIP INSIDE THE `t≈60 s` FENCE DOES NOT FIT, AND *"it fit last time"* IS LUCK, NOT A BUDGET." `[L: VER-§8 cl. 10(b), 2026-09-22 correction]` That is the 2026-09-22 text, quoted as written. Read it with the clause's 2026-09-26 amendment beside it (marker `VER-8-10B-THE-LOWER-END-IS-NOT-A-FLOOR`).
@@ -111 +112 @@
-  - **Primary: `1270` row 3's own stamps.** A read of the HUD notice slot at t=01m07.22s, `IA_Card1` at t=01m07.23s, then `IA_DiscardAll` at t=01m07.57s. `[M: 1270 row 3]` A 0.01 s gap cannot contain an MCP round trip: `1512` measured round trips of ≈20 s and ≈36 s of PIE clock (`1512` *Speed data*), and the law's range is ≈30–70 s (`VER-§8` cl. 10(b)).
+  - **Primary: `1270` row 3's own stamps.** A read of the HUD notice slot at t=01m07.22s, `IA_Card1` at t=01m07.23s, then `IA_DiscardAll` at t=01m07.57s. `[M: 1270 row 3]` A 0.01 s gap cannot contain an MCP round trip: `1512` measured round trips of ≈20 s and ≈36 s of PIE clock (`1512` *Speed data*), and the law lists every value observed so far (`VER-§8` cl. 10(b), marker `VER-8-10B-THE-LOWER-END-IS-NOT-A-FLOOR`).
@@ -140 +141 @@
-| **no-play interval** | the same reads (gold, census) again after the last press, with no card key sent; then the log (Step 5) | `[M: 1512 ctl]` |
+| **no-play interval** | the same reads (gold, census) again after the last press, with no card key sent; then the log (Step 5). **Length:** `1512`'s ran t=34.05 → 54.60 (≈20.5 s), of which only t=34.05 → 37.45 was inside the batch (Read-back). The length inside this batch is `NOT MEASURED`; the report states it, from its own stamps, next to the treatment window (the ctl row asks for "an equal interval") (`qa/TASK-1518.md` N10) | `[M: 1512 ctl, Speed data]` |
@@ -275 +276 @@
-- **`(0,0,0)` is not the gate; `bHidden` is.** `1391` C4 read the hidden ghost at `(0,0,0)`, and `(0,0,0)` is also the centre of `CaptureZone_Center` `[M: 1512 A2]`, a legal point whenever that zone is Blue-owned (`1391` C6's refusal text). So a visible ghost at `(0,0,0)` does not meet the C4 condition. The rest of this bullet is `[D]`, from `UpdatePlacementGhost` and `ResolvePlacementUpgradeState` in `SiegePlayerController.cpp`:
+- **`(0,0,0)` is not the gate; `bHidden` is.** `1391` C4 read the hidden ghost at `(0,0,0)`, and `(0,0,0)` is also the centre of `CaptureZone_Center` `[M: 1512 A2]` (A2 read `RootComponent.RelativeLocation`, which equals the actor location only for an unattached root: `NOT MEASURED`), a legal point whenever that zone is Blue-owned (`1391` C6's refusal text). So a visible ghost at `(0,0,0)` does not meet the C4 condition. The rest of this bullet is `[D]`, from `UpdatePlacementGhost` and `ResolvePlacementUpgradeState` in `SiegePlayerController.cpp`:
--- a/Tools/Verify/recipes/RCP-menu-to-deckbuilder.md
+++ b/Tools/Verify/recipes/RCP-menu-to-deckbuilder.md
@@ -3 +3 @@
-Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are defined in `README.md`. `archer50` = `.claude/pipeline/qa/PLAYTEST-archer50-verify.md` · `1511` = `.claude/pipeline/qa/TASK-1511-verify.md` · `1512` = `.claude/pipeline/qa/TASK-1512-verify.md`.
+Provenance tags (`[M]`, `[S]`, `[D]`, `[L]`, `NOT MEASURED`, `HYPOTHESIS`) are defined in `README.md`. `archer50` = `.claude/pipeline/qa/PLAYTEST-archer50-verify.md` · `1511` = `.claude/pipeline/qa/TASK-1511-verify.md` · `1512` = `.claude/pipeline/qa/TASK-1512-verify.md` · `1524` = `.claude/pipeline/qa/TASK-1524-verify.md`.
@@ -12,0 +13,2 @@
+- **Used by `TASK-1524`**, 2026-09-26, `Verdict: VERIFIED`, 1 PIE session: "Steps 0–4 · re-verified in-run **y, with a mismatch**: Step 1's batched `IA_MenuDown` ×2 (the recipe's JSON, no wait step) landed **1 of 2** at 1-frame spacing (t=6.81, 6.82); `1511`'s 2/2 was at 3 frames. A top-up of 1 fixed it. Step 4 found `EditingDeckIndex = 2`, not the recipe's `0`/deck1 (the active deck was deck3; see H2). Reported, not patched." `[M: 1524 Recipes used]` Also used: its *Not examined* sent-vs-landed line, *Pre-registration*, row 0, *Hypotheses* H2 and *Recipe candidates* 2–3.
+- Amended by `TASK-1527`, 2026-09-26, to record `1524`: Step 1 (the second measurement of the batched form), Step 2 (the top-up), Step 4 and the Read-back (the open slot).
@@ -16 +18 @@
-**Not read.** Source, *Not examined*: "The Aura plugin version was not read this run." `1511` and `1512` did not read it either ("Aura plugin version not read", each *Not examined*). ⇒ the first use re-verified the recipe in-run (`1511`). The version is still unread, so a plugin update since then cannot be ruled out (`VER-§13` cl. 3, `VER-§8` cl. 5).
+**Not read.** Source, *Not examined*: "The Aura plugin version was not read this run." `1511`, `1512` and `1524` did not read it either ("Aura plugin version not read", each *Not examined*). ⇒ the first use re-verified the recipe in-run (`1511`). The version is still unread, so a plugin update since then cannot be ruled out (`VER-§13` cl. 3, `VER-§8` cl. 5).
@@ -51,3 +53,6 @@
-- **The batched form: measured ONCE, by `1511`.** "`IA_MenuDown`×2 in ONE `run_verification_sequence` → `Button_2 focused:true` t=9.62", and "the batched ×2 form the recipe marked NOT MEASURED landed cleanly with 3 frames between injections". `[M: 1511 Recipes used]` Sent vs landed: "menu `IA_MenuDown`×2 → 2 landed (focus `Button_2`, 2 subsystem log lines)". `[M: 1511 Speed data]`
-  - **Fenced to that run:** `L_MainMenu`, standalone, 1 client, viewport 1280×725, DPI 0.6706, starting focus `Button_0`. `[M: 1511 Editor/Aura state, Recipes used]`
-  - `1511` does not quote its action list. Whether the 3 frames came from a wait step or from the runner's own spacing is `NOT MEASURED`, and the JSON above carries no wait step.
+- ⚠️ **The batched form: measured TWICE, with different results. It is NOT reliable: Step 2's focus read and top-up are mandatory after it.**
+  - **`1511`: 2 of 2.** "`IA_MenuDown`×2 in ONE `run_verification_sequence` → `Button_2 focused:true` t=9.62", and "the batched ×2 form the recipe marked NOT MEASURED landed cleanly with 3 frames between injections". `[M: 1511 Recipes used]` Sent vs landed: "menu `IA_MenuDown`×2 → 2 landed (focus `Button_2`, 2 subsystem log lines)". `[M: 1511 Speed data]`
+  - **`1524`: 1 of 2**, with this JSON and no wait step, at 1-frame spacing (t=6.81, 6.82). `[M: 1524 Recipes used]` Sent vs landed: "menu `IA_MenuDown` ×2 (one batch, no wait, 0.017 s apart) → **1 landed** (focus `Button_1`)". `[M: 1524 Not examined]`
+  - **Both runs:** `L_MainMenu`, standalone, 1 client, viewport 1280×725, DPI 0.6706. `[M: 1511 Editor/Aura state; 1524 Editor/Aura state]` Starting focus `Button_0` is quoted by `1511`. `[M: 1511 Recipes used]`
+  - `1511` does not quote its action list. Whether its 3 frames came from a wait step or from the runner's own spacing is `NOT MEASURED`, and the JSON above carries no wait step.
+  - **Spacing between the two Downs.** The spacings measured where both landed are `1511`'s 3 frames (mechanism above: `NOT MEASURED`) and `archer50`'s 0.42 s (t=16.55 → 16.97; whether one call or two is not stated, see above). `1524`'s 1 frame lost one. A wait step between the two Downs has never run: `NOT MEASURED`. `1524`'s "`wait_pie_seconds 0.3` between the two `IA_MenuDown`" is a proposal, not a measurement. `[M: 1524 Recipe candidates 2]` This recipe writes no wait; a run that adds one names its spacing and labels it.
@@ -56 +61,4 @@
-**Step 2 — read the focus before accepting.** Same call as Step 0; expect `Button_2` `focused: true`. `[M: archer50 §1(a)]` The source read focus (t=17.38) before it sent Accept (t=21.50). `1511` read `Button_2` focused at t=9.62 and sent Accept at t=14.60 `[M: 1511 Recipes used]`; whether that read shared the Downs' batch is not stated. Sending Accept in the same batch as the Downs is `NOT MEASURED`, and a batch cannot branch on its own result (`README.md`, common hazards).
+**Step 2 — read the focus before accepting (mandatory: Step 1 can land 1 of 2).** Same call as Step 0; expect `Button_2` `focused: true`. `[M: archer50 §1(a)]` The source read focus (t=17.38) before it sent Accept (t=21.50). `1511` read `Button_2` focused at t=9.62 and sent Accept at t=14.60 `[M: 1511 Recipes used]`; whether that read shared the Downs' batch is not stated. Sending Accept in the same batch as the Downs is `NOT MEASURED`, and a batch cannot branch on its own result (`README.md`, common hazards).
+- **If it reads `Button_1`: top up with ONE `IA_MenuDown`** (Step 1's action object, sent once), then read again and expect `Button_2`. Measured once: after Step 1 landed 1 of 2 (focus `Button_1`), "top-up 1 → landed (`Button_2`, t=13.76)". `[M: 1524 Not examined, Recipes used]` The top-up's envelope (a standalone call or a one-action sequence) is not quoted: `NOT MEASURED`.
+- **If it reads `Button_2`:** go to Step 3.
+- **Anything else** (e.g. `Button_0`, neither Down landed): `NOT MEASURED`. Do not send Accept (Known hazards).
@@ -68,0 +77,5 @@
+- ⚠️ **Expect the ACTIVE deck's slot, not `0`.** `DECK-§3`: *"Opening the builder starts with the ACTIVE deck selected for editing"*. `[L: DECK-§3]` Measured at two points:
+  - `0` when deck1 was active: `archer50` (its `ActiveDeckName` read `deck1`, "unchanged", after the run) and `1511` (its Pre-registration read `ActiveDeckName = deck1`). `[M: archer50 §1(a), Not-examined; 1511 Recipes used, Pre-registration]`
+  - `2` when deck3 was active: "Builder open t=19.83: `WBP_DeckBuilder_C_0`, `EditingDeckIndex=2`", with `ActiveDeckName = deck3` in the pre-registration. `[M: 1524 row 0, Pre-registration, Recipes used]`
+  - That the builder does this in general is `HYPOTHESIS` H2 in `1524` ("One observation; mechanism not read"). `[M: 1524 H2]`
+  - ⇒ Read the value and report it; never assume it. A recipe that walks the deck bar from here counts from this read (`RCP-deckbuilder-set-active-by-keyboard.md` Precondition 4).
@@ -75,2 +88,2 @@
-| 1–2 | `focused: true` on `Button_2` "Deck Builder" | t=17.38 s `[M: archer50 §1(a)]` · t=9.62, after the batched Downs `[M: 1511 Recipes used]` |
-| 3–4 | `WBP_DeckBuilder_C_0` exists; `EditingDeckIndex = 0`; `WorkingDeck` = `deck1` | t=22.53 s `[M: archer50 §1(a)]` · t=15.14 `[M: 1511 Recipes used]` |
+| 1–2 | `focused: true` on `Button_2` "Deck Builder" | t=17.38 s `[M: archer50 §1(a)]` · t=9.62, after the batched Downs `[M: 1511 Recipes used]` · t=13.76, after the batched Downs landed 1 of 2 (focus `Button_1`) and a one-Down top-up `[M: 1524 Not examined]` |
+| 3–4 | `WBP_DeckBuilder_C_0` exists; `EditingDeckIndex` = the ACTIVE deck's slot (`DECK-§3`); `WorkingDeck` = that deck | `0` / `deck1`, deck1 active: t=22.53 s `[M: archer50 §1(a)]` · t=15.14 `[M: 1511 Recipes used]` · `2`, deck3 active: t=19.83 (`WorkingDeck` not quoted) `[M: 1524 row 0]` |
@@ -78 +91 @@
-`EditingDeckIndex` 0 = deck1 on open. The builder opened on deck1 (Jonathan's deck, untouched throughout the source run). Nothing here edits it. `[M: archer50 §1(a)]`
+`EditingDeckIndex` on open is the active deck's slot (`DECK-§3`; Step 4). In the source run it was 0 = deck1, which was active (Jonathan's deck, untouched throughout the source run) `[M: archer50 §1(a), Not-examined]`; in `1524` it was 2 = deck3, which was active `[M: 1524 row 0, Pre-registration]`. Nothing here edits the deck.
@@ -83,2 +96,4 @@
-- Any other menu target. "Play (vs Bot)" by `IA_MenuAccept` from `Button_0` was measured by `1512` (A1: `IA_MenuAccept` at menu t=20.44, then world `L_Arena`; 1/1 in both attempts, *Speed data*). Promoting it to a recipe is `TASK-1515`'s row, not this one. Sandbox, Multiplayer, Settings, Login and Quit were not driven.
-- **The batched Downs beyond one run.** `1511`'s single landing (2/2) is the only measurement. Its gap between the two injections is quoted as "3 frames" and nothing more (Step 1).
+- Any other menu target. "Play (vs Bot)" by `IA_MenuAccept` from `Button_0` was measured by `1512` (A1: `IA_MenuAccept` at menu t=20.44, then world `L_Arena`; 1/1 in both attempts, *Speed data*). It is its own recipe now: `RCP-vsbot-capture-center-and-summon.md` (`TASK-1515`). Sandbox, Multiplayer, Settings, Login and Quit were not driven.
+- **The batched Downs.** Two runs, two results: 2 of 2 at 3 frames (`1511`) and 1 of 2 at 1 frame (`1524`). A drop rate at any spacing is `NOT MEASURED`; so is a wait step between the Downs (Step 1).
+- **The top-up.** One `IA_MenuDown` from `Button_1`, measured once (`1524`). A top-up from `Button_0`, or of more than one Down: `NOT MEASURED`.
+- **The open slot.** `EditingDeckIndex` at open was read with deck1 active (0) and deck3 active (2) only. Other active decks: `NOT MEASURED` (Step 4).
@@ -99,0 +115 @@
+- **The batched Downs can drop one.** `1524` landed 1 of 2 with this recipe's JSON; a one-Down top-up after the focus read fixed it (Step 2). `[M: 1524 Recipes used, Not examined]`
```
