Verdict: VERIFIED
# Verification — TASK-1499 [PIXEL-LIMB-REREAD-BATCH]

> **What this `VERIFIED` covers:** only TASK-1499's own acceptance lines (triage table, positive control,
> per-limb re-reads with capture date and read date kept apart, contradictions stated first, limitations).
> **It upgrades no other row.** Under `REREAD-APPENDS-NEVER-UPGRADES-2026-09-26` no landed report was
> edited and no other row's `status:` was touched. The append lines for the manager are in §4.
> **Method for every observation below: `RE-READ OF COMMITTED FRAME`.** Nothing here was `observed live`.
> Read date for every frame: **2026-09-27**. Capture date = the frame's evidence-folder date, which is the
> original report's run date.

Editor/Aura state: no editor state used. **No PIE, no capture, no editor call, no inspector call.** The
lane was `Read`/`Glob`/`Grep` over the repo. Editor PID 3108 was reported up by the dispatch and was not
touched; no `SC-§118` census was needed because nothing was driven. **PIE announcement: the dispatch says
NO PIE will be driven, so no announcement is owed (`VER-3-6-THE-REPORT-RECORDS-THE-ANNOUNCEMENT`).** None
was driven. Attempts: 1 of 3 (one read pass). Wall time about 25 min. Credit: not visible.
**How "committed" was established:** the session-start `git status` snapshot showed only
`TASKBOARD.md` modified and one untracked handoff (`handoffs/TASK-1536-buildmaster.md`). No evidence PNG
was listed as untracked or modified, so every frame cited here was tracked and clean at session start. I
hold no Git, so this rests on that snapshot and was not re-run.
model (self-reported): "Opus 5.5 (1M context)", model ID `claude-opus-5-5[1m]` (as seen in my own system context).

---

## 0. CONTRADICTIONS OF A LANDED `VERIFIED`: NONE FOUND

No re-read disagrees with its original report. The two landed readings that rested on byte-level decodes
only (`TASK-1494` `MEASURED`, `TASK-1498` `VERIFIED`) were checked against the glyphs, and both agree.
The four ring limbs (`TASK-1402`/`1413`/`1421`/`1427`) are unchanged: I could not resolve a ring either,
and my eye is not a discriminating instrument for this ring (§2), so they remain `UNOBSERVABLE`.

---

## 1. TRIAGE TABLE (done before any re-read; the fence)

Scope: every `UNOBSERVABLE` limb in `qa/*-verify.md` whose stated ground was **perception**, meaning no
image reached the verifier, or only a byte-level decode was available. That is the row's own list
(`1413`, `1421`, `1427`, the byte-only readings in `1494`/`1498`) plus the rows `TASK-1450` named as
carrying a blind pixel limb (`1402`, `1436`, `1449`). Every other `unobs` cell in `qa/` was read and is
listed in §1b, because its ground was reach, input or instrument, not pixels.

| # | report · limb | original verdict on the limb | committed frame that would answer it | answerable by `Read`? |
|---|---|---|---|---|
| T1 | `TASK-1402` (4) PIXEL: is a focus ring drawn on `Button_0`? | `unobs` (row `MEASURED`) | `playtest-evidence/2026-09-24/VER-TASK-1402-a2-top-option-focused-before-viewport-click.png`, `…-a2-top-option-focus-restored-by-poll.png` (1086×615, downscaled) | **frame exists, question NOT answerable by eye** (see §2 eye control) |
| T2 | `TASK-1413` §7 PIXEL limb | `UNOBSERVABLE` (row `VERIFIED` on STATE) | `2026-09-25/VER-TASK-1413-t04m33s-graphics-shadingqualityslider-stepped-by-ia-menuright.png` (1280×725 native) | **ring: not answerable by eye**; screen content: answerable |
| T3 | `TASK-1421` §7 PIXEL limb, "the three frames were not seen by me" | `UNOBSERVABLE` (row `VERIFIED` on STATE) | `2026-09-25/VER-TASK-1421-t03m33s-graphics-backbutton-reached-by-down-x18.png`, `…-graphics-ring-wrapped-to-showframeratecountercheckbox.png`, `…-login-loggedin-mode-ring-on-backbutton.png` (1280×725) | **ring: not answerable by eye**; "what is drawn": **answerable** |
| T4 | `TASK-1427` §7 PIXEL limb, "no image content reached my context" | `UNOBSERVABLE` (row `VERIFIED` on STATE) | `2026-09-25/VER-TASK-1427-session-ring-wrapped-to-hostbutton.png`, `…-mainmenu-reentry-top-option-after-session-back.png`, `…-deckbuilder-focusedcardindex-after-first-injected-down.png`, `…-deckbuilder-grid-to-deckbar-crossing-slotbutton-deck1-focused.png` (1280×725) | **ring: not answerable by eye**; "what is drawn": **answerable** |
| T5 | `TASK-1494` limitation 3: "I never read the glyphs … is the last sentence of the third related block on screen?" | byte-level only (row `MEASURED`) | `2026-09-26/VER-TASK-1494-a3-longest-page-lowest-painted-text-row.png`, `…-negative-control-same-screen.png`, `…-short-page-positive-control.png` (1086×615) | **answerable** |
| T6 | `TASK-1498` hazard 5: "chip identity rests on its x-extent … not on reading glyphs" | byte-level only (row `VERIFIED`) | `2026-09-26/VER-TASK-1498-a2-detail-page-one-button-block.png`, `…-a2-list-view-twenty-eight-focus-stops.png` (1086×615) | **answerable** |
| T7 | `TASK-1436` 7b / `TASK-1484` pixel limb (prose scrolls on an overflowing page) | `not witnessed` (row `MEASURED`) | frames exist (`2026-09-25/VER-TASK-1436-a1/a2-detail-scroll-before/after.png`), but the page did not overflow, so **no frame of the named event was ever captured** | **no frame captured** (premise failure, not perception) |
| T8 | `TASK-1449` pixel limb (named by `TASK-1450`) | no `qa/TASK-1449-verify.md` exists | none | **no frame captured** |
| T9 | `TASK-1395` pixel limb (fenced out by its dispatch) | fenced; report promotes no PNG | none (an auto-film exists in `Saved/`, uncommitted and never read) | **no frame captured** |
| T10 | `TASK-1274` limitation: "no ring is discernible to this reader" on the S1/S2/S3 frames | limitation note | the S1/S2/S3 frames were **not promoted**; the three promoted `VER-TASK-1274-*` frames show other states | **no frame captured** (committed) |

**Counts: 10 limbs triaged.**
- **5 answerable** from a committed frame, whole or in part: T2, T3 and T4 (the "what is drawn" half),
  T5 and T6 (whole).
- **1 has a frame but is not answerable at all:** T1, whose only question is the ring.
- **4 have no frame captured:** T7, T8, T9 and T10.

The **ring half** of T2, T3 and T4 is also not answerable by eye, so **every ring question (T1-T4)
stays `UNOBSERVABLE`**.

### 1b. `unobs` cells read and excluded as not perception-grounded
Their ground is reach, input, a diff, a suite, a log verbosity or a missing hand check, and `Read` cannot
retake a reading nobody took: `TASK-671` rows 1-2 (builder never opened) · `TASK-787` rows 2-3 (input
release / ghost pawn) · `TASK-1230` row 2 · `TASK-1270`/`-loop0` rows 1,4,5,6 · `TASK-1271` rows 1-3 ·
`TASK-1286` (11) steps · `TASK-1306` 5b-3/5b-4/Escape/face buttons · `TASK-1314` (7b)-i/-ii · `TASK-1348`
P8b · `TASK-1357` B4/B5b · `TASK-1395` rows 5-7 · `TASK-1399` per-screen (e) and screens 4/7/8 ·
`TASK-1402` (1)/(1b)/(2b)/(3) · `TASK-1421` (d) and the Login form row · `TASK-1427` 1423-e · `TASK-1436`
2e/2f · `TASK-1459` Q0-Q3 · `TASK-1489` row 5 · `TASK-1511` A4/A5 · `TASK-1524` A3 ·
`PLAYTEST-archer50` A2-A4.

---

## 2. POSITIVE CONTROL (mandatory), and a second control on my own eye

**(a) Delivery control: FIRED.** `Read` of
`2026-09-26/VER-TASK-1498-a2-detail-page-one-button-block.png` (366,936 B per its report) rendered a dark
detail page titled **"Move"** with key chip **", / O / A / E"**, three short paragraphs at top-left, and
**one** button, **"Back to the controls list"**, at bottom-centre. `Read` of
`2026-09-26/VER-TASK-1498-a2-list-view-twenty-eight-focus-stops.png` (689,643 B) rendered a **different**
screen: a "Controls" list panel with category headers ("Your hero", "Cards and the HUD", "Army orders",
"Drawing the circles", "Interface"), about 30 key-chip rows, a "Close" button, and "Gold: 28" top-left.
Different layout, different text, different button. **`Read` delivered two distinct images, each
matching its own file's description.** A second pair also discriminates: the `TASK-1494` subject ("Resize
what you are placing", dense prose to row ~516) versus its short-page control ("Sprint", four lines).

**(b) Eye control for the RING question: MARGINAL, so not relied on.** `TASK-1450` measured a ring
(`max(B−R)` 128–134 LSB on the focused row) on
`2026-09-24/VER-TASK-1450-mainmenu-button0-focused-native1920.png` (Play focused) and
`…-button6-focused-keypresslane-native1920.png` (Quit focused). With `Read` I **may** see a slightly
darker edge on Play in the first and on Quit in the second, but I would not bet a verdict on it. So a
ring that an instrument can detect is **at or below** what my eye resolves in a rendered frame. My failing
to see a ring in T1-T4 is therefore **not evidence of no ring**, and those ring halves stay `UNOBSERVABLE`.
This is also `VER-§11` cl. 9: the four-part bar was never specced for these rows, and a re-read cannot
spec it after the fact.

---

## 3. PER-LIMB RE-READS (method: `RE-READ OF COMMITTED FRAME`; read 2026-09-27)

| # | limb | frame · capture date | what I see | vs original |
|---|---|---|---|---|
| T1 | `1402` (4) ring on `Button_0` | both `VER-TASK-1402-a2-*` · **2026-09-24** | The seven-button main menu (Play (vs Bot), Sandbox (No Bot), Deck Builder, Multiplayer, Settings, Login, Quit) over sky and a white plain. All seven buttons are the same grey. **No ring discernible** on Play (vs Bot) in either frame. The two frames are near-identical by eye. | **agrees** ("no ring resolved"). Ring question **still `UNOBSERVABLE`** (§2b; 1.18× downscale; `VER-§12` cl. 7e). |
| T2 | `1413` §7 | `VER-TASK-1413-t04m33s-…` · **2026-09-25** | Graphics screen, "60 FPS · 16.7 ms". Overall Quality "Custom"; ten rows "High"; **Shading "Epic"**, with its slider handle at about **3/4** of the track while the other tracked sliders sit at about 1/2. Resolution Scale 87%, Screen Resolution 1024 x 768, Window Mode Borderless Window, Frame Rate Limit Unlimited, Back. **No Keep/Revert bar.** The dimmed Settings screen shows through. **No ring discernible** on the Shading row or elsewhere. | **agrees** with every element the original's Evidence line described. **New pixel corroboration:** the Shading handle's position matches the logged `0.5000 -> 0.7500`. Ring still `UNOBSERVABLE`. |
| T3a | `1421` frame 1 (Back after 18 Downs) | `VER-TASK-1421-t03m33s-…` · **2026-09-25** | Same Graphics layout and values as T2, "52 FPS · 19.1 ms". The Back button looks the same as the Auto-Detect Quality bar. No Keep/Revert bar. **No ring discernible.** | original made **no** pixel claim. Now attested: **the frame shows the Graphics screen its slug names.** Ring still `UNOBSERVABLE`. |
| T3b | `1421` frame 2 (wrapped to `ShowFrameRateCounterCheckBox`) | `VER-TASK-1421-graphics-ring-wrapped-…` · **2026-09-25** | Same Graphics screen, "**1 FPS · 1645.8 ms**" (a hitch frame). The Settings screen ghosted behind has shifted relative to frame 1. **No ring discernible** on the checkbox row. The Back button looks the same as in frame 1, where it was focused. | now attested: the Graphics screen. The frame 1/frame 2 comparison is a focus change (Back → checkbox) with **no visible change on Back by eye**. Ring still `UNOBSERVABLE`. |
| T3c | `1421` frame 3 (Login, `LoggedIn`) | `VER-TASK-1421-login-loggedin-mode-ring-on-backbutton.png` · **2026-09-25** | Dimmed main menu behind a small "Account" panel: "Logged in as JonBonWes", **Log Out**, "cloud session expired - sign in again to re-link", **Sync Now**, **Back**. **No ring discernible** on Back. To my eye Log Out is a slightly lighter fill than Sync Now and Back (low confidence). | now attested: **the Login screen in LoggedIn mode**, as the slug says. Ring still `UNOBSERVABLE`. |
| T4a | `1427` session menu, ring wrapped to `HostButton` | `VER-TASK-1427-session-ring-wrapped-to-hostbutton.png` · **2026-09-25** | "Multiplayer" panel: address box "127.0.0.1:7777", **Host**, Join, Back, "Ready - host a match or join an address." **Host has a visibly lighter fill than Join and Back.** No outline ring. | now attested: the session menu. The lighter Host fill is an **observation, not ring evidence**: T3c shows a lighter fill on an **unfocused** button (Log Out), and `TASK-1399`/`TASK-1446` §3 record a fill artefact of this kind. |
| T4b | `1427` main-menu re-entry | `VER-TASK-1427-mainmenu-reentry-top-option-after-session-back.png` · **2026-09-25** | The seven-button main menu, all the same grey. **No ring discernible** on Play (vs Bot). | now attested: a main menu is on screen after Back. Ring still `UNOBSERVABLE`. |
| T4c | `1427` deck builder, `FocusedCardIndex = 0` | `VER-TASK-1427-deckbuilder-focusedcardindex-…` · **2026-09-25** | Deck Builder: deck1…deck10 tabs, with deck1 carrying an orange underline; a 34-card tile strip; "Deck: 51/50" bottom-left; right info panel "Click a card to see how it works." / Close; Reset to Default, Play, Exit. **No highlight discernible** on the first tile (Footman). | now attested: the builder, with deck1's orange underline visible at native 1280. Focus highlight still `UNOBSERVABLE`. |
| T4d | `1427` grid → deck bar, `deck1` `SlotButton` focused | `VER-TASK-1427-deckbuilder-grid-to-deckbar-…` · **2026-09-25** | Same builder layout, sky differs. The bar and grid are **indistinguishable by eye from T4c**. | now attested: the builder. A focus change produced no visible change by eye. `UNOBSERVABLE`. |
| T5 | `1494` limitation 3: glyphs | `VER-TASK-1494-a3-longest-page-lowest-painted-text-row.png` (+ negative) · **2026-09-26** | Header chip "Mouse Wheel Up / Mouse Wheel Down", title **"Resize what you are placing"**. Six prose paragraphs, then **"All the controls that go with it"** with three related blocks: **"Stack a tower taller"**, **"Resize the circle"**, **"Draw circles on the map"**. The third block's paragraphs run from "Your own numbered circles, drawn on the war map…" through the four bullets, "THE MAP'S OWN PLACE MARKERS WIN", "NUMBERS ARE PERMANENT NAMES…", "You can hold MaxMapMarks circles at once…", to the last painted line, **"…and they are cleared when the match resets. They are never saved."** That is the **final string** of `Interface.MapMarks`'s `Detail` (`SiegeControlsHelpWidget.cpp:1531-1533`). Below it: clear panel, then **"Back to the controls list"** and **"Scroll down — back to the top at the end"**. The negative-control frame is the same by eye. Short-page control: "Left Shift" / **"Sprint"**, four lines. | **agrees** with the `MEASURED` answer. **Discharges the named open question:** on this page, **the last sentence of the third related block is on screen**, and all ten paragraphs of that block are painted. It does not speak for the other 26 pages. |
| T6 | `1498` hazard 5: chip identity | `VER-TASK-1498-a2-detail-page-one-button-block.png` · **2026-09-26** | The "Move" detail page. **Exactly one button, reading "Back to the controls list"**; no "Scroll down…" chip anywhere. | **agrees** with the `VERIFIED` pixel arm. The width-based identification is now confirmed **by glyph**. |

---

## 4. APPEND LINES FOR THE MANAGER (I edit no landed report and no other row's `status:`; `SC-§120`, `SC-§101`)

Each line is proposed for the named row, dated, citing both reports and the frame path:
- **TASK-1400/1402:** `2026-09-27 RE-READ OF COMMITTED FRAME (qa/TASK-1499-verify.md T1): VER-TASK-1402-a2-top-option-focused-before-viewport-click.png + …-focus-restored-by-poll.png (captured 2026-09-24): no ring discernible by eye; eye not a discriminating instrument at this scale; pixel limb stays UNOBSERVABLE (qa/TASK-1402-verify.md unchanged).`
- **TASK-1413:** `2026-09-27 RE-READ (qa/TASK-1499-verify.md T2): VER-TASK-1413-t04m33s-…png (captured 2026-09-25): content agrees with the report; Shading handle at ~3/4 matches the logged 0.75; no ring discernible; pixel limb stays UNOBSERVABLE.`
- **TASK-1421:** `2026-09-27 RE-READ (qa/TASK-1499-verify.md T3): the three VER-TASK-1421-*.png (captured 2026-09-25) show the screens their slugs name (Graphics ×2, Login LoggedIn); no ring discernible on any; pixel limb stays UNOBSERVABLE.`
- **TASK-1427:** `2026-09-27 RE-READ (qa/TASK-1499-verify.md T4): the four VER-TASK-1427-*.png (captured 2026-09-25) show session menu / main menu / deck builder ×2 as slugged; Host has a lighter fill (not ring evidence); no ring discernible; pixel limb stays UNOBSERVABLE.`
- **TASK-1494:** `2026-09-27 RE-READ (qa/TASK-1499-verify.md T5): glyphs read on VER-TASK-1494-a3-longest-page-lowest-painted-text-row.png (captured 2026-09-26): page is "Resize what you are placing"; the last painted line is the final authored sentence of the third related block (SiegeControlsHelpWidget.cpp:1531-1533). Limitation 3 discharged for this page only; MEASURED unchanged.`
- **TASK-1498:** `2026-09-27 RE-READ (qa/TASK-1499-verify.md T6): the single chip on VER-TASK-1498-a2-detail-page-one-button-block.png (captured 2026-09-26) reads "Back to the controls list"; agrees with VERIFIED.`

---

## Acceptance lines → observations

| # | acceptance line | observable chosen | observed (quoted values, evidence path) | pass/fail/unobs |
|---|---|---|---|---|
| 1 | the TRIAGE TABLE first | §1, written before any re-read, scoped by the stated ground of each `unobs` | 10 perception limbs triaged: 5 answerable in whole or part (T2-T4 content halves, T5, T6), 1 frame-present-but-unanswerable (T1), 4 with no frame captured (T7-T10); every ring half (T1-T4) stays `UNOBSERVABLE`; plus §1b excluding every non-perception `unobs` in `qa/` | pass |
| 2 | the positive control declared | two `Read`s of frames known to differ | 1498 detail ("Move", one button) vs 1498 list ("Controls", ~30 rows, "Close"): visibly different, each matching its report; second pair 1494 subject vs "Sprint". Eye-for-ring control on 1450 native frames: marginal, declared, not relied on (§2) | pass |
| 3 | per-limb observation, capture date and read date separate, method labelled | §3 table | 11 re-read rows, each with frame path, capture date (2026-09-24/25/26) and read date 2026-09-27, method `RE-READ OF COMMITTED FRAME` | pass |
| 4 | any CONTRADICTION of a landed `VERIFIED` led with | §0 | none found; stated first | pass |
| 5 | `## Not examined / limitations` | below | present | pass |

## Evidence (promoted)
None. This row captures nothing and promotes nothing; every frame cited is an already-committed file
under `.claude/pipeline/playtest-evidence/` and was **not renamed, moved or modified**.

## Hypotheses (not verdicts)
- **H1:** in the stock Slate style at 1280×725 and DPI 0.67, the default focus ring is too faint to see by
  eye. `TASK-1450` detected it at 128–134 LSB of `B−R`, and I could not see it with confidence. If that
  holds, the ring limbs can only close by `TASK-1450`'s instrument under a specced four-part bar, or by
  Jonathan's eye on a live screen. **HYPOTHESIS.**
- **H2 (observations in passing, not findings, not measured; no remedy proposed per `SC-§101`):**
  (a) on the Graphics frames (T2/T3), the Overall Quality, View Distance, Anti-Aliasing, Global
  Illumination, Foliage and Resolution Scale sliders show a handle tick with **no visible track line**,
  while Shadows, Reflections, Post Processing, Textures, Effects and Shading show one. (b) Player-facing
  help prose shows raw code identifiers, for example "You can hold **MaxMapMarks** circles at once" and
  "AHeroCharacter::WalkSpeed". The `MaxMapMarks` text is authored literally at
  `SiegeControlsHelpWidget.cpp:1529`, so this is authored text, not a missed substitution; whether it is
  wanted is not mine to judge. (c) The `1498` list frame's bottom row is cut off at the panel edge, which
  is `TASK-1500`'s already-boarded observation, corroborated.

## Not examined / limitations this run
- **No Git.** "Committed" rests on the session-start `git status` snapshot (§ header). Blob identity was
  not re-checked.
- **Eye, not instrument.** Every ring answer is a human-style look at a rendered image. §2(b) shows the
  eye is marginal at this ring strength, so every "no ring discernible" above is **not** a negative.
- **Downscaled frames:** T1, T5 and T6 are 1086×615 (1.18× down). T5's glyphs were legible; T1's ring
  question is doubly bounded.
- **T5 speaks for one page.** It does not certify the other 26 detail pages' last sentences.
- **Colour judgements** ("lighter fill" on Host / Log Out) are low confidence and are not ring evidence.
- **Not re-read:** `TASK-1399` §6 and `TASK-1274`'s pixel notes. Their pixel readings were negatives
  recorded as `MEASURED`/notes, not `UNOBSERVABLE` limbs, so they are outside this row's scope; §2(b)
  bears on them in the same way.
- **Tools:** no `run_verification_sequence` step, no `pie_scene_edit`, no Aura call of any kind
  (`VER-§7` cl. 2 declaration: nothing reached through the runner).

## Recipes used
none. `Tools/Verify/recipes/README.md` was read (S3). No recipe covers a committed-frame re-read, and no
PIE step was taken.

## Recipe candidates
- *Committed-frame re-read:* (1) triage every limb by the stated ground of its `unobs`; (2) run a
  two-frame delivery control with `Read`; (3) run an eye-for-signal control on a frame where an
  instrument measured the signal, before scoring any "not seen"; (4) record the capture date and read
  date separately; (5) append, never upgrade. Proposed for the manager to board; I write nothing under
  `Tools/`.
