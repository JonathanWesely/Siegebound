PASS
# QA Report: TASK-1549 re-gate, QA loop 1 (gate for TASK-1548: `VER-§12` cl. 7f pointers in the verifier body and the recipes)

Verdict: **PASS**: 0 BLOCKER · 0 WARN · 1 NIT (optional, record wording only). Board: `TASK-1548` → `qa-passed` (loop 1 of 3 closes here).

Marker `TASK-1549-LOOP-1-PASSED`. Date 2026-09-27 (editor clock 17:43 local). Prior gate: `qa/TASK-1549.md` (FAIL: B1 at `RCP-menu-to-deckbuilder.md:128`/`:130`; N1, N2 optional). Programmer's answer: `handoffs/TASK-1548-programmer.md` `## QA loop 1` (`:182`–`:273`). `TASK-1548` was at `ready-for-qa` (marker `TASK-1548-READY-FOR-QA-LOOP1-2026-09-27`) when I started. This is a separate file, following the `qa/TASK-1481-loop1.md` precedent, so `qa/TASK-1549.md` and its §6 anchors stay byte-stable.

| file | sub-verdict | BLOCKER | WARN | NIT |
|---|---|---|---|---|
| `Tools/Verify/recipes/RCP-menu-to-deckbuilder.md` | **B1 fixed** | 0 | 0 | 0 |
| `Tools/Verify/recipes/RCP-deckbuilder-set-active-by-keyboard.md` | N2 taken, quote exact | 0 | 0 | 0 |
| `.claude/agents/playtest-verifier.md` (BODY) | N1 taken, frontmatter re-proved | 0 | 0 | 0 |
| slot, README, vsbot, play | byte-identical to loop 0 | 0 | 0 | 0 |
| `handoffs/TASK-1548-programmer.md` | append-only, claims hold | 0 | 0 | 1 (L1-N1) |

## §0 — What I inspected, beyond reading text

`get_headless_status` → `editor_connected`. Five read-only `unreal_inspector` Python queries. Files were opened `rb`; only `hashlib`, `re`, `os`, `os.path.commonprefix` and `datetime` were used. No git, no `subprocess`, no editor state touched, no lifecycle call.

1. **Byte census** of the six `Tools/Verify/recipes/` files, the agent file and the 1548 handoff (sha256, bytes, CR/LF/lone counts, LF-normalised sha, computed git blob, mtime). Agent `head -7` raw and CR-stripped. Table in §4.
2. **Loop-0 reconstruction (the delta since my review).** I put the handoff's four `-` lines (`:238`, `:241`, `:246`, `:251`, leading `-` removed; `\r` restored on the agent line) back into the current files at menu `:128`/`:130`, set-active `:44` and agent `:164`. The results hash to **exactly my `qa/TASK-1549.md` §6 anchors**: menu `aef9083b…fc436c`, set-active `6167149c…0965fd`, agent `9b4976d6…347723`. The four `+` lines (`:239`, `:242`, `:247`, `:252`) equal the current lines byte for byte. **So the loop-1 delta is exactly those four lines, and nothing else in those three files moved.** Line counts are unchanged (132 / 204 / 238).
3. **Reverse derivation to commit F, by my own cut.** I cut at ` ⚠️ **Added 2026-09-27 (\`TASK-1548\``, one occurrence per line (counted). Menu `:128` + `:130` → `d814ea2c2624…08bebfa8`. Set-active `:44` → `201409670a0d…6ca57d3a47`. Agent, lines 156–166 dropped → `e6dcf6dc646e…cd98337536fd`. Slot (`:166` cut, `:168`–`:170` dropped) → `550ed8b34e2b…af36f0a6a`. README (`:69`–`:74` dropped) → `fb7900d90767…c1c3fe539`. **All five equal the `qa/TASK-1535.md` §5 / `qa/TASK-1528.md` §5 anchors.** On `:128` the loop-0 line is a byte prefix of the new line (`startswith` = true). On `:130` the old and new lines share 1131 bytes, and the old tail begins "No record in this recipe rests on such a read", which is `TASK-1548`'s own loop-0 sentence. The commit-F part of the line lies inside that shared prefix, as the derivation proves.
4. **Handoff append-only:** sha256 of the current handoff's first 29528 bytes = `f9ecdf2c8f22…a24094e8c3` = my loop-0 anchor. The whole file is `ab8610af…09f2` (48357 B), which is the dispatch's figure.
5. **mtime census** (project tree minus `Saved/`, `Intermediate/`, `.git/`, `DerivedDataCache/`, `Binaries/`, `Content/`, `Plugins/`; last 2 h). Files newer than my loop-0 report (17:30:46): `qa/TASK-1481-loop1.md` (17:31:56, another gate), **the agent file, menu and set-active (all 17:35:57), the 1548 handoff (17:38:00)**, `CONVENTIONS.md` 17:42:33 and `TASKBOARD.md` 17:42:44 (the manager's parallel text edits), and `SiegeControlsHelpWidget.cpp` 17:43:28 (`TASK-1541`, live now, declared parallel). Slot 17:18:36 and README 17:17:49 predate my loop-0 report. This corroborates "nothing else moved" but does not prove it. Mtime cannot show reads, and I did not read `.git/index`.
6. **Source reads** for every loop-1 claim (§1).

## §1 — Re-check of B1, N1, N2 against the sources

**B1 (a) — menu `:128`, the appended marking: holds, sentence by sentence.**
- "rests on one reading, `archer50` §1(b)'s `Tab` row: 'no `SlotButton` `focused`' (t=85.11)": `qa/PLAYTEST-archer50-verify.md:35` has exactly that. A `focus` grep of archer50 hits only `:27` (the §1(a) menu reads) and `:35`, so "one reading" is right.
- "taken in the builder after the §1(b) `ui_perform` `click` rows (t=43.63, t=57.13)": `:33`–`:34`. They are `ui_perform` steps (archer50 `:8`: "the same button under `click`, under `press`/`release`"). The builder was open from t=21.50 (`:27`).
- "no focus-setting input in between": the only row between t=57.13 and t=85.11 is the `Tab` probe itself (t=84.78), which is the thing under test. `TASK-1493-verify.md:55` sits under `## Blast radius` (`:48`) as item 3 and says the same ("with no refocus in between").
- "`click` is outside cl. 7f's measured shapes … `NOT MEASURED`": right. The clause is not cited for `click`, which meets BLOCKER criterion (2). Cl. 7f's scope bullet is `CONVENTIONS.md:12525` at my instant (one line lower than at loop 0, because of the manager's concurrent edit). The marker `VER-12-7F-UI-PERFORM-CLEARS-FOCUS` resolves at `:12521`.
- "The manager ruled that read corroboration only (`TASK-1539` status line, marker `UI-PERFORM-FOCUS-APPENDS-2026-09-27`)": `TASKBOARD.md:9426` carries the marker, and its sub-bullet `:9429` reads "`TASK-1501` (… §1(b), the `Tab` row): corroboration only". This is the same citation form as slot `:166`, which loop 0 accepted.
- "`FocusedCardIndex -1` is the widget's own index (a `UPROPERTY(Transient) int32` on `UDeckBuilderWidget`) … `[D]`": `DeckBuilderWidget.h:907`–`:908` still reads `UPROPERTY(Transient)` / `int32 FocusedCardIndex = INDEX_NONE;`. That file is dirty from `TASK-1480` (17:15:24), and the declaration is intact.
- "The hazard itself still rests on the tool's reply … and on `VER-§5` cl. 5" (the handoff's scrutiny item 1): **ACCEPT.** Cl. 5 (`CONVENTIONS.md:12271` at my instant) says `simulate_key_press` "is delivered to the player controller and not to Slate". The sentence names only the reply and the clause and leaves `binding_found` out, which is consistent with `:129`.
- The shape matches slot `:166`: dated, `(TASK-1548, QA loop 1)`, UNSUPPORTED not refuted, the old sentence kept as a byte prefix (§0.3), and nothing re-verdicted. That meets criterion (5). The cross-pointer means the two recipes now agree on this one archer50 read.

**B1 (b) — menu `:130`, the "every other" universal: VERIFIED at source (the dispatch's ask).** I listed every focus read the recipe cites:
- Precondition 4 (`:27`): archer50 t=4.84, `1511` t=4.89, `1512` menu t=20.42, `1519` t=6.70.
- Step 0 (`:37`–`:39`): the same four reads.
- Step 1 (`:52`, `:56`–`:58`, `:72`–`:73`): archer50 t=17.38, `1511` t=9.62, `1524` `Button_1`, `1519` t=7.35 / 6.70.
- Step 2 (`:77`–`:78`): archer50 17.38, `1511` 9.62, `1519` 7.35, `1524` t=13.76.
- Read-back (`:103`–`:104`): the same reads.
- `:128`: the exception, now marked.
- `:130`'s own "matched 2 widgets" is a `ui_snapshot` match count, not a focus read.

Against the sources:
- archer50's cited focus reads are all §1(a)'s (t=4.84, t=17.38, `:27`). A `ui_perform` grep of archer50 hits `:6`, `:8`, `:49`, `:66`, `:73`, and every one of those concerns §1(b)–(d) or the hazards. None falls before §1(b)'s first row at t=43.63.
- A `ui_perform` grep of `1511`, `1512`, `1524` and `1519` gives **0 / 0 / 0 / 1**.
- `1519`'s one hit (`:49`) is "`ui_perform` right-button shapes not re-traced". Its right-button presses were `simulate_key_press` `RMB`/`RightMouseButton` (`:21`–`:23`, `:44` H1), and its pointer input was `SetMouseLocation` (`:50`). So "record none" is exact.
- The census is now **5 hits** (slot 2 · menu 2 · set-active 1), 3 of 5 recipes, zero in vsbot, play and README. My own grep for the archer50 `Tab` read (`85\.11|did not move|§1\(b\)|Tab\``) across the recipes directory finds it cited as a record only at slot `:166` and menu `:128`. README `:91` cites it for `binding_found`, and slot `:65` "did not move" is a count read-back. There is no third site. B1 is closed.

**N1 — agent `:164`: taken, frontmatter intact.** The line now reads "`ui_perform`'s other steps (`click`, `double_click`, `press`/`release`, `drag`,". `head -7` re-measured `rb`: raw `653fc50c2a29d071cd171daf0c2625e8b09ec58d0ce8591dc174e47faaa4997e` (4326 B), CR-stripped `5c2b409f16987def46a9acd142c60f1c928e37a649395c401e931f11a4660749`, line 7 `---\r`. Both equal the `TASK-1517`/`1535`/loop-0 anchors, which meets criterion (1). The whole file is `dc97fe9a7852f989609a166dc7db629d942373fec4cc6875fe44fc31599ea6ce`, 21786 B, CRLF 238/238, lone CR 0 / lone LF 0. That is the dispatch's figure. The `Edit`-scope paragraph, the `SC-§118` census text and STEP 6 lie outside the one changed line, and the loop-0 reconstruction (§0.2) is exact, which meets criterion (4).

**N2 — set-active `:44`: taken, quote exact.** `qa/TASK-1519-verify.md:50` reads "Pointer input used in the builder: `SetMouseLocation` (a cursor move, no click) — after control a1, never before the keyboard walk". The recipe's quoted span "after control a1, never before the keyboard walk" is a verbatim substring. "(a cursor move, no click)" is the source's own parenthesis. "its one mention (*Not examined*)" is right: `:49` sits under `## Not examined / limitations this run` (`:46`), and it is the file's only `ui_perform` hit. "`1519` records no `ui_perform` call" holds (above).

## §2 — Findings

- **[NIT] L1-N1 `.claude/pipeline/handoffs/TASK-1548-programmer.md:203`**: "a grep … over all six recipes and the README". `Tools/Verify/recipes/` holds **five** recipes plus the README (6 files, by `Glob` at my instant). The same handoff's own table says "3 of 5", and the grep evidently covered every file (its results name all five), so only the count word is wrong. The dispatch's "all 7 recipe/README files" carries the same miscount. Optional: the handoff is append-only, so a correction would be an appended line. No loop needed.

Not a finding: the handoff's `CONVENTIONS.md:12270` (`:195`) and loop 0's `:12520`–`:12526` now read one line lower (`:12271`, `:12521`–`:12527`) because the manager's concurrent `CONVENTIONS.md` edit (17:42:33) landed after them. The recipes cite by clause and marker, never by line, so nothing shipped goes stale.

## §3 — BLOCKER criteria, loop 1

| # | criterion | result |
|---|---|---|
| (1) | frontmatter byte-identical, `head -7` re-measured | **holds** (§1 N1) |
| (2) | no line generalises past cl. 7f's scope or promotes the mechanism | **holds.** The three new sentences keep `click` / `double_click` `NOT MEASURED` and do not cite the clause for them. |
| (3) | census complete, every hit fenced or marked | **holds** (5 hits, all fenced or marked; §1 B1 (b)) |
| (4) | `Edit`-scope paragraph, `SC-§118` text, STEP 6 intact | **holds** (§0.2, §0.3) |
| (5) | no past record re-verdicted; unsupported ones marked | **holds.** `:128`'s old sentence is a byte prefix, and the marking is appended. |
| (6) | nothing outside WRITES moved | **holds** (§0.2–§0.5; corroboration, see limits) |

## §4 — Notes for build-master (host `TASK-1550`)

**Verdict PASS ⇒ `TASK-1548`'s cargo may ride commit H:** the agent file, the four touched recipes (menu, set-active, slot, README) and `handoffs/TASK-1548-programmer.md`, plus both gate records `qa/TASK-1549.md` and this file. `Tools/Verify/recipes/` holds **6** files (5 recipes + README), not 7. vsbot and play are untouched and carry no delta.

Reviewed bytes at my instant (sha256 `rb`; blob = `sha1("blob <n>\0" + LF bytes)`, computed, not read from git):

| file | working-tree sha256 | bytes | EOL | LF-normalised sha256 | git blob (computed) |
|---|---|---|---|---|---|
| `.claude/agents/playtest-verifier.md` | `dc97fe9a7852f989609a166dc7db629d942373fec4cc6875fe44fc31599ea6ce` | 21786 | CRLF 238/238, lone 0/0 | `a1446ab1244aa526058038d635085b29bd946cac77ff084e77daf9ecc3319d1f` (21548 B) | `38df8687bc46dad4070094f71109945824d1c767` |
| ↳ its `head -7` raw / CR-stripped | `653fc50c…a4997e` / `5c2b409f…4660749` | 4326 | line 7 `---\r` | — | — |
| `Tools/Verify/recipes/RCP-menu-to-deckbuilder.md` | `54c7e067b17659727d9cc0550e6a41587d3449178ef8acef6fa01ede5837882d` | 21868 | LF 132, CR 0 | same | `f3f5178bc8c628bd50dee6a5e365790779597d0a` |
| `Tools/Verify/recipes/RCP-deckbuilder-set-active-by-keyboard.md` | `c1965e7bcd4eaaa38abdd76fd209c3e4d56c90a0400d394dd3d9b23ee75567dc` | 37412 | LF 204, CR 0 | same | `ac42a79a09ae174c6f0c8be423ee108e728929dc` |
| `Tools/Verify/recipes/RCP-deckbuilder-slot-and-card-edit.md` | `f72c9530c0633fb002ec7231cdda8e76cde20831addaedeee88cadde60b2e2d7` | 22547 | LF 170, CR 0 | same | `c985258390f5c43b7494dc6e0a1826afdbd60391` |
| `Tools/Verify/recipes/README.md` | `89347e54082573dd4b5ea84ed18027d4ecee0cf1d275a008e2ded3e2a56d7bd7` | 17367 | LF 105, CR 0 | same | `a99b58f9723430fa2ec3fd569989ea691e3ab27c` |
| `Tools/Verify/recipes/RCP-vsbot-capture-center-and-summon.md` (untouched) | `e1ae86a6acb7cf69843ed60165f47cbb571a86eb919cbb8071274f5f638d8f13` | 32433 | LF 248, CR 0 | same | `a1f647de06cd765af2df33ee356043188d0eb871` |
| `Tools/Verify/recipes/RCP-play-unit-card-from-hand.md` (untouched) | `3eb2d019c372f1d786967e7ba8ad03b15929bae1d5982bf77d56aa8fe44f425f` | 49130 | LF 298, CR 0 | same | `9b31c76a2bbbed8bbc05939940b67f2fc43d63b1` |
| `.claude/pipeline/handoffs/TASK-1548-programmer.md` | `ab8610af1d248db930e722c608647a6c4dd8b22455adef7214022dd80d1809f2` | 48357 | LF 273, CR 0 | same | `d44a775a35dcf0f9bd3aa4f4ead189ff9ec2318c` |

- Before-state blobs (= commit F, per `qa/TASK-1535.md` §5; my reverse derivation reproduces their sha256): agent `fb5c076a…` · menu `85df27a3…` · set-active `fe516b94…` · README `0899ef0f…` · slot `b0d58f8…`.
- `TASK-1550` spec (3): the staged agent blob must equal `38df8687…` above, and its `head -7` must be unchanged against `HEAD`. If any sha above differs at your instant, the change is outside this review.
- This report cannot carry its own hash. `qa/TASK-1549.md` is unchanged since 17:30:46.

## Not examined / limitations

- **No git.** HEAD's blobs are not measured by me. The before-states rest on the reverse derivation to the `qa/TASK-1535.md` / `qa/TASK-1528.md` §5 anchors, and the blob ids are computed.
- The loop-0 reconstruction uses the handoff's `-` lines as input. It is still a check, not trust: a wrong `-` line could not reproduce my own loop-0 sha256.
- "Nothing outside WRITES" rests on hashes, the reconstruction and an mtime census. That is corroboration, not proof. I did not diff `TASKBOARD.md` or `CONVENTIONS.md` (both concurrently edited).
- No recipe was run and no PIE. archer50, `1493`, `1511`, `1512`, `1519` and `1524` were read as they stand, not against their frames or logs.
- Markdown rendering was not checked.
