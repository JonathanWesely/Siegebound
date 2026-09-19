Verdict: VERIFIED
verdict-clause: the FULL token `POST-FLUSH read-back: IDENTITY=MATCH` appeared **exactly once**, on a deck builder opened by the `TASK-1274` keyboard route, and the focused `SWidget` address **equals** the builder's own (`0x0000022E10D70390`).
subject: TASK-1307
leg: `TASK-1298` A2-5 / acceptance (11) — the SECOND 5b verify leg, carried intact from `TASK-1309` cl. (4)
report-name: ⛔ `TASK-1309`-verify by order of A2-5 (the absorbed row keeps the leg's name); the SUBJECT is `TASK-1307`
verifier: playtest-verifier · 2026-09-19 · attempt 1 of 3
⛔ line 1 is deliberately BARE — same reason as `qa/TASK-1296-verify.md`: `TASK-1298` cl. (5) orders a bare
verdict line (`VER-§6` cl. 5 (iv)), so the dispatch's one clause sits on line 2 instead of as a suffix that
would break a `head -1` equality test.

# Verification — TASK-1307 (leg filed as TASK-1309)

**Editor/Aura state:** connected **yes** · editor **PID 14432**, GUI editor, `unreal.is_editor() == True`,
no `-game` instance, no second editor log live · map **`/Game/Maps/L_MainMenu`** · PIE **standalone, 1 client,
1280×720** · **Aura loaded and serving** (every read below came through Aura's read-only Python lane) ·
**attempts used 1 of 3** · wall time ≈ 01:29→01:34 local (2026-09-19). Binaries: game DLL mtime
**2026-09-18 23:41:40**; session log opened **23:47:02**, i.e. after the build. ⛔ No compile, no relaunch, no
editor close, no Git. ⚠️ The `SC-§118` command-line string was **unreadable** — full declaration of what the
classification actually rests on is in `qa/TASK-1296-verify.md`, and it applies unchanged here.

⚠️ **Row status on disk is `qa-passed`, not `built`** — `handoffs/TASK-1298-buildmaster.md` §4 records the
`built` flip as deliberately deferred to the commit leg. Declared, not waived (same note as the sibling report).

**Both legs ran inside ONE PIE session** — the builder open that leg 1 needed as its regression half *is* the
open this leg measures. One boot, one open, both readings.

---

## 🚨 PIN 1 — THE FULL TOKEN, AND WHY I MEASURED FIVE STRINGS INSTEAD OF ONE

`TASK-1308` WARN-1 is not hypothetical: `SetUserFocus` returns `false` when focus has **not** changed
(`SlateApplication.cpp:3029`), so the **PRE-FLUSH** line can legitimately print the bare substring
`IDENTITY=MATCH`. A grep for the short form can therefore be satisfied by the non-answering line. I counted the
whole family, in the post-cursor PIE slice **and** across the whole session log:

| string measured | PIE slice | whole session log |
|---|---|---|
| ⭐ **`POST-FLUSH read-back: IDENTITY=MATCH`** (the pinned FULL token) | ⭐ **1** | ⭐ **1** |
| `POST-FLUSH read-back: IDENTITY=NO-MATCH` | **0** | **0** |
| `POST-FLUSH read-back: IDENTITY=UNREADABLE` | **0** | **0** |
| `AcquireBuilderFocus POST-FLUSH read-back` (the pin without a verdict) | **1** | **1** |
| `AcquireBuilderFocus PRE-FLUSH read-back` | **1** | **1** |
| `AcquireBuilderFocus` (both moments — `TASK-1308` NIT-4's ×2) | **2** | **2** |
| ⚠️ the bare `IDENTITY=MATCH` — **the trap** | **1** | **1** |
| `PRE-FLUSH read-back: IDENTITY=MATCH` — the shape that would have poisoned the short grep | **0** | **0** |
| ⛔ deliberately-WRONG control `POST-FLUSH read-back: IDENTITY=MATCH**ED**` | **0** | **0** |

⭐ **THE TOKEN, EXACTLY AS FOUND, OCCURRENCE COUNT 1:** `POST-FLUSH read-back: IDENTITY=MATCH`

⚠️ **On this run the bare form happened to be safe** (its single occurrence *is* the full token — the PRE-FLUSH
line printed `NO-MATCH`). ⛔ **That is luck, not a licence.** It held because this open was not already-focused;
the re-entrant open WARN-1 describes would have produced `PRE-FLUSH … IDENTITY=MATCH` and a short grep would
have returned a hit with **zero** post-flush lines in the file. The counts above are what distinguish those two
worlds, and I took them rather than inferring them.

**Firing baseline (`SC-§39`, and `TASK-1306`'s discipline reproduced):** every one of those strings counted
**0** in the whole log immediately before `start_pie` (cursor byte **373,193** / line **2,745**, 01:32:04
local), and the non-zeros above are all inside the **20,690-byte / 133-line** slice this PIE run wrote. ⇒ the
instrument went **0 → 1** across the event. A count that was never zero would have proved nothing.

## 🚨 PIN 2 — THE ADDRESS CLASSIFICATION, ANSWERED EVEN THOUGH NO `NO-MATCH` OCCURRED

⛔ **No `NO-MATCH` was produced on the POST-FLUSH line**, so PIN 2's `VERIFY-FAILED` gate was never reached.
The classification is reported anyway because the addresses are the whole point:

- focused `SWidget` = **`0x0000022E10D70390`**
- this builder's `SWidget` = **`0x0000022E10D70390`**
- ⇒ **the same pointer — the builder ITSELF.** Not an ancestor (the `TASK-1286` defect), not a descendant (the
  healthy card-tile/deck-bar state `DeckBuilderWidget.cpp:1217` deliberately produces).

⭐ And the corroboration runs the right way round: `ui_snapshot` at PIE t=29.103 reports
`WBP_DeckBuilder_C_0` `"focused": true` — **the log and the snapshot agree**. The `VERIFY-FAILED` shape this
row exists to catch is a POST-FLUSH `NO-MATCH` *while* `ui_snapshot` says the builder is focused; here both
instruments say the same thing, which is the outcome that makes the new read-back trustworthy rather than
merely new.

---

## Acceptance lines → observations

| # | acceptance line | observable chosen | observed (quoted values, evidence path) | pass/fail/unobs |
|---|---|---|---|---|
| A2-5 (a) | *"open the deck builder via the EXISTING `TASK-1274` keyboard route"* | `inject_input_action` ×3, then `ui_snapshot` | `IA_MenuDown` at PIE t=**25.760** and **26.177**, `IA_MenuAccept` at **26.594**, all `status: action_injected`. At t=**29.103** the root widget is **`WBP_DeckBuilder_C_0`** with `DeckBar` holding **10** `DeckSlotEntryWidget` children and `TextBlock_6` reading **`"Deck: 50/50"`**. ⛔ No key was pressed at the grid; the grid was never armed; `Escape` was never injected — evidence `…/VER-TASK-1307-deckbuilder-opened-keyboard-route.png` | **pass** |
| A2-5 (b) | *"the NEW post-flush line must appear EXACTLY ONCE"* | occurrence count of `AcquireBuilderFocus POST-FLUSH read-back` in this instance's log slice | **1** in the slice and **1** in the whole session log. One builder open ⇒ one line, which is `TASK-1308` NIT-5's correct reading of "exactly once" (per open). The once-per-open guard (`cpp:1835` + the clear at `:1841`) held | **pass** |
| A2-5 (c) | *"…and must report a MATCH"*, **FULL token pinned** (PIN 1) | the five-string family above | ⭐ **`POST-FLUSH read-back: IDENTITY=MATCH` × 1**, with `NO-MATCH` = 0 and `UNREADABLE` = 0 | **pass** |
| A2-5 (d) / acceptance (12) | *"NET ZERO on 🧑 his real deck — both `.sav` byte-identical, hashes quoted"* | sha256 of every file in `Saved/SaveGames/`, before and after the whole run | ⭐ **all five byte-identical; `ALL_SAV_BYTE_IDENTICAL=True`; file count 5 → 5; mtimes not even touched** (table below) | **pass** |
| PIN 2 | *"on any `NO-MATCH`, quote the focused `SWidget` address and classify it"* | the log line's two addresses | **no `NO-MATCH` occurred**; classification supplied regardless — the two addresses are **equal** ⇒ the builder itself | **pass (n/a as a failure gate)** |

### THE TWO LOG LINES, VERBATIM — the primary evidence for this leg

```
[2026.09.19-08.32.34:171][ 40]LogGitClaudeUnrealTest: UDeckBuilderWidget::AcquireBuilderFocus PRE-FLUSH read-back: IDENTITY=NO-MATCH (NOT the verdict - a NO-MATCH here is EXPECTED whenever the request was deferred); SetUserFocus returned DEFERRED to the next frame (diagnostic colour only, never a pass signal); focused SWidget 0x0000000000000000 ('<none>') vs this builder's SWidget 0x0000022E10D70390 ('SObjectWidget'); IsFocusable()=true. A second read-back line follows and carries the verdict; if none follows, this builder was torn down or stopped ticking before the flush.
[2026.09.19-08.32.34:198][ 41]LogGitClaudeUnrealTest: UDeckBuilderWidget::AcquireBuilderFocus POST-FLUSH read-back: IDENTITY=MATCH (THIS is the verdict - taken 1 frame(s) after the request, past FEngineLoop::ProcessLocalPlayerSlateOperations); focused SWidget 0x0000022E10D70390 ('SObjectWidget') vs this builder's SWidget 0x0000022E10D70390 ('SObjectWidget'); IsFocusable()=true. The two type names are context, never the test - 'SObjectWidget' is every UUserWidget's Slate type.
```

⚠️ **Timestamps are UTC** (`08.32.34`) while the wall clock was **01:32:34 local** — the same offset that makes
`Saved/Logs`' backup filenames look like the future. Both lines are inside this PIE run's slice; they are not
imported from another session.

⭐⭐ **WHAT THE PAIR ACTUALLY DEMONSTRATES, and it is the row's whole reason for existing.** The two lines are
**27 ms and one frame counter apart** (`[ 40]` → `[ 41]`, `:171` → `:198`), and the focused-widget address
moves `0x0000000000000000 ('<none>')` → `0x0000022E10D70390`. ⇒ the old instrument's `'<none>'` — the reading
that started `TASK-1307` — **was not a focus failure at all**; focus landed one frame later, exactly where the
programmer's `LaunchEngineLoop.cpp` phase argument said it would (`:5918` flush before `:5991` widget tick,
`GFrameCounter++` at `:6131`, hence the `>` guard and not `>=`). The old line was structurally incapable of
witnessing it. **This run is the first time the read-back has printed a positive, and the positive is the
builder's own pointer.**

---

## NET ZERO ON 🧑 HIS REAL DECK — hashed before and after the whole run

| file | sha256 **before** | sha256 **after** | identical | mtime after |
|---|---|---|---|---|
| ⭐ **`SiegeDecks.sav`** | `646d442fc11c27704ef1e30962470c1a4db475667679c3268af7a5ee4039a071` | `646d442fc11c27704ef1e30962470c1a4db475667679c3268af7a5ee4039a071` | ✅ **yes** (3,814 B) | 2026-08-02 10:09:34 |
| ⭐ **`SiegeDecks_4E46A9EE49D3A7C91C583B8457E1EE34.sav`** (the profile save) | `7ad6029879fc8a2535a1de3bd64b7692507423efb1a09a3d24e9787dd9bd7610` | `7ad6029879fc8a2535a1de3bd64b7692507423efb1a09a3d24e9787dd9bd7610` | ✅ **yes** (6,521 B) | 2026-09-18 17:31:50 |
| `SiegeAccounts.sav` | `2fd96fe18af9a4cfc4b1174497f992a5d841a21011787f3efe88624454d442b1` | same | ✅ yes (3,083 B) | 2026-08-28 22:43:52 |
| `SiegeSettings.sav` | `c8555088e2918c517fdc88e6e877a1bd2ab860c38e5fecbd9ff5b27acc06f2e7` | same | ✅ yes (2,004 B) | 2026-08-04 22:24:38 |
| `SiegeSettings_4E46A9EE49D3A7C91C583B8457E1EE34.sav` | `684ae64f9378bdf34d5aa45a4b14c8048f46787c41952ad35202262feeb41793` | same | ✅ yes (2,056 B) | 2026-09-09 13:48:19 |

⭐ **I hashed all five, not the two required** — cheap, and it closes the "the deck is fine but the profile
moved" gap by measurement instead of by argument. **`ALL_SAV_BYTE_IDENTICAL=True`**, directory count 5 → 5, and
**not one mtime advanced** (the profile deck still reads 2026-09-18 17:31:50, from before this wave). ⛔ His
hand-trimmed 50-card deck1 is untouched — and the builder itself agrees: `TextBlock_6` read **`Deck: 50/50`**
and `TextBlock_7` **`Avg cost: 25.02`** while it was open.

---

## Evidence (promoted)

Written **directly** to the promotion path by `capture_pie_frame` and read back from it afterwards (no `Saved/`
copy owed):

- `.claude/pipeline/playtest-evidence/2026-09-19/VER-TASK-1307-deckbuilder-opened-keyboard-route.png` — PIE t=**29.119**, frame 378189, 900×510, mean_luma 152. The deck builder filling the screen: ten slot tabs `deck1 … deck10` across the top with `deck1` highlighted, the `Deck Builder` title top-left, a full row of illustrated card tiles with cost pips, a dark right-hand panel reading *"Click a card to see how it works"* over a `Close` button, `Reset to Default` bottom-left, `Exit` bottom-right, `Play` along the bottom. ⛔ **Shared with `qa/TASK-1296-verify.md`** (it is that row's regression half) — **stage it once.**
- `.claude/pipeline/playtest-evidence/2026-09-19/VER-TASK-1296-mainmenu-cold-boot-focus-state.png` — PIE t=**25.699**, frame 377990, 900×510, mean_luma 195. The `L_MainMenu` cold-boot state this open was reached from: sky/ground backdrop, seven legible menu labels, `Play (vs Bot)` topmost. Cited here as the **provenance of the route** (the builder was reached from a real menu, not spawned).

⚠️ **The PNGs are not this leg's proof and I will not present them as such.** The claim is a log token and a
pointer comparison; the pixels prove only that the builder genuinely opened. Said plainly so the host does not
over-read the image list it is about to copy into a commit.

## Hypotheses (not verdicts)

- The `1 frame(s)` delta printed in the POST-FLUSH line is **consistent with** the builder having been
  constructed from the world tick (flush same-frame, read one frame later) rather than from a Slate click.
  ⛔ I did not instrument the construct path; the delta is a printed diagnostic, not a branch, and the verdict
  does not rest on it.
- `PRE-FLUSH` reading `NO-MATCH` with `SetUserFocus returned DEFERRED` matches the deferred-lane behaviour the
  handoff describes. ⛔ Observed shape, not a measured mechanism.

## Not examined / limitations this run

- ⛔ **One open, one session.** "Exactly once" is proven **per open**, on a single open. Two opens in one
  session should legitimately produce two lines (`TASK-1308` NIT-5) — **not tested**, and a second open is the
  obvious next probe if anyone doubts the guard's re-arm.
- ⛔ **The already-focused / re-entrant open — the exact case PIN 1 warns about — was NOT exercised.** This run
  produced `PRE-FLUSH … NO-MATCH`, so the trap line never appeared. **My measurement shows the trap is real and
  unsprung here; it does not show what the instrument does when it springs.**
- ⛔ **`NO-MATCH` was never produced**, so PIN 2's descendant-vs-ancestor discrimination is **untested in
  anger**. `TASK-1308` WARN-2's suggested `DESCENDANT=` field would be what makes that case readable from the
  line alone; it remains unboarded and is not mine to board.
- ⛔ **`IDENTITY=UNREADABLE` never occurred** — the third answer is present in code (`cpp:1876-1882`) but
  unobserved.
- ⛔ **X / B remain `UNOBSERVABLE` by construction** and were **not** in this criterion (A2-5 says so
  explicitly). I did not reach for them. The card grid was never armed and `Escape` was never injected.
- ⛔ **No source was re-read for this leg** — `TASK-1308` did that at engine source and I did not duplicate it.
  My claim is confined to what the running process printed and what the live widget tree reported.
