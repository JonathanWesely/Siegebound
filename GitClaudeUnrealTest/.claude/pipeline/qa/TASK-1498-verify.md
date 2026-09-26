Verdict: VERIFIED
# Verification — TASK-1498 [DETAIL-SCROLL-REMOVAL-REGRESSION]

## THE READING THE ROW EXISTS FOR, FIRST

**The detail page reads ONE focus stop, and that stop is `BackButton`. `IA_ControlsHelp` still
closes the overlay from the detail page.** Both quoted from the shipped log, in a live match:

```
[ControlsHelp] Detail page open for 'Cards.PlacementResize' (3 related control(s)).   16.30.04:974
[USiegeMenuInputSubsystem] menu nav target registered -> 'SiegeControlsHelpWidget_0'
                           (registered screen), 1 focus stop(s), 1 screen(s) registered.
[USiegeMenuInputSubsystem] MoveFocus(+1): focus moved 0 -> 0 of 1 ('BackButton').      16.30.05:886
```

**Nothing regressed. The milestone stays at TEN of ten against `4620f71`.** This row does not close
a screen — screen 9 was already closed — it confirms the cleanup did not break one, and it did not.

| # | criterion | reading | verdict |
|---|---|---:|---|
| 1 | detail page **2 → 1**, `BackButton` **stop 0** | `1 focus stop(s)`; `focus moved 0 -> 0 of 1 ('BackButton')` ×3 | **PASS** |
| 2 | list **still 28** | `28 focus stop(s)` on **4** opens; `of 28` on **11** MoveFocus lines | **PASS** |
| 3 | `Tab` closes from **BOTH** views | list `16.31.04:343`; detail `16.31.07:180`, replicated `16.35.33:041` | **PASS** |
| 4 | `IA_ControlsHelp` closes **FROM THE DETAIL PAGE** (BLOCKER-class) | `16.31.01:589 [ControlsHelp] Overlay closed.` | **PASS** |

---

## PRESS SPACING, STATED BECAUSE THE STOP COUNTS DEPEND ON IT

**0.30 s** between every injected navigation action (the dispatch's floor is 0.25 s; 0.15 s is
known to swallow one in twelve). Waits of **0.8–0.9 s** around every state change, **1.0 s** before
every capture.

**Hazard 1 did not fire, and that is measured rather than hoped:** the eleven consecutive
`IA_MenuDown` presses produced eleven consecutive `MoveFocus` lines with **no gap in the index** —
`0->1, 1->2, 2->3, 3->4, 4->5, 5->6, 6->7, 7->8, 8->9, 9->10, 10->11 of 28` — so **11 of 11 landed,
zero coalesced**. Every frame and every reading below is labelled by what the **log** said, never by
what I aimed at: the page I aimed for (`Cards.PlacementResize`, 11 down) is the page the log named.

---

## Acceptance lines → observations

| # | acceptance line | observable chosen | observed (quoted values, evidence path) | pass/fail/unobs |
|---|---|---|---|---|
| 1 | line 1 `Verdict:` | — | `Verdict: VERIFIED`, byte-literal, no suffix | pass |
| 2 | detail page **2 → 1** | `menu nav target registered -> … N focus stop(s)` (the instrument `TASK-1496` planted for 5b at `cpp:3998-4000`) | **`1 focus stop(s)`** on **three** page opens across **two** PIE sessions: `Cards.PlacementResize` (3 related) `16.30.04:974`; `Hero.Move` (0 related) `16.31.06:190`; `Hero.Move` again in a fresh session `16.35.31:954`. Every one names `'SiegeControlsHelpWidget_0'` | pass |
| 3 | `BackButton` **is stop 0** | `MoveFocus(+1): focus moved X -> Y of N ('<name>')` | **`focus moved 0 -> 0 of 1 ('BackButton')`** on **three consecutive** presses (`16.30.05:886`, `16.30.06:221`, `16.30.06:537`) — index **0**, total **1**, named `BackButton`, and the ring **does not advance**, which is what a one-stop ring must do | pass |
| 4 | list **still 28** | same registration line + the `of N` field of `MoveFocus` | **`28 focus stop(s)`** on **four** independent overlay opens: `16.30.00:575`, `16.31.02:511`, `16.31.05:268`, `16.35.30:862`. Independently, **11** `MoveFocus` lines all read **`of 28`** | pass |
| 5 | `Tab` closes from the **LIST** | `[ControlsHelp] Overlay closed.` + `VOCABULARY DISARMED` | opened `16.31.02:513` (28 stops) → `Tab` at PIE t=103.64 → `DISARMED` + `unregistered -> 'None' … 0 focus stop(s)` + **`Overlay closed.`** `16.31.04:343` | pass |
| 6 | `Tab` closes from the **DETAIL** page | same, plus the declared detail-close signature | on `Hero.Move` (1 stop, `16.31.06:190`) → `Tab` at PIE t=106.47 → transient `registered … 28 focus stop(s)` `16.31.07:178` → `DISARMED` → **`Overlay closed.`** `16.31.07:180`. **Replicated in a fresh session** `16.35.33:039-041` | pass |
| 7 | `IA_ControlsHelp` closes **from the detail page** | same | from `Cards.PlacementResize` (1 stop, ring on `BackButton`), `inject_input_action /Game/Input/Actions/IA_ControlsHelp.IA_ControlsHelp` at PIE t=100.88 → transient `registered … 28 focus stop(s)` `16.31.01:587` → `DISARMED` + `unregistered … 0 focus stop(s)` → **`[ControlsHelp] Overlay closed.`** `16.31.01:589` | pass |
| 8 | press spacing stated | — | 0.30 s; 11/11 presses landed, index unbroken | pass |
| 9 | `## Not examined / limitations` | — | present, 9 entries | pass |

**The detail close is not a mislabelled list close, and that is read off the log rather than
assumed.** `cpp:4074-4081` declares that closing *from the detail page* runs
`CloseHelp() → ReturnToList()` (registers, 28) `→ ApplyOpenState(false)` (unregisters), so that one
route emits a `registered … 28` line **immediately followed by** an `unregistered` line. Both detail
closes (rows 6 and 7) carry that transient pair; the **list** close (row 5) does **not**. The two
routes have distinguishable log signatures, and mine match the route I claim.

---

## Evidence (promoted)

- `.claude/pipeline/playtest-evidence/2026-09-26/VER-TASK-1498-a2-detail-page-one-button-block.png`
  — the `Hero.Move` detail page (log `16.35.31:954`, `1 focus stop(s)`), 1086 × 615 composited,
  mean luma 42. Byte-decoded: in the whole lower panel (y ≥ 500) there is **exactly ONE** painted
  button chip — **y 593-606, x 494-591, width 98 px, 1372 bright px**. 366,936 B,
  sha256 `89c0b24331e684b0…`.
- `.claude/pipeline/playtest-evidence/2026-09-26/VER-TASK-1498-a2-list-view-twenty-eight-focus-stops.png`
  — the list view of the same overlay one second earlier (log `16.35.30:862`,
  `28 focus stop(s)`), mean luma 67. A completely different pixel layout — banded list rows filling
  x 20-1065 from y 500 to the frame bottom. 689,643 B, sha256 `fc129f2b341eec04…`.

Both promoted files were re-opened from disk after PIE stopped and decoded again to confirm the
bytes are really there; neither path is claimed on the strength of a tool's success return.

**Working frames retained in `Saved/` (gitignored, not promoted), cited as corroboration only:**
`Saved/AuraVerify/T1498/A_detail_after_open_t45.24s_f45808.png` (`Cards.PlacementResize`, log
`16.30.04:974`) and `Saved/AuraVerify/T1498/B_detail_row0_t106.40s_f49472.png` (`Hero.Move`, log
`16.31.06:190`). My own promoted frames omit the `-t<time>` token per the dispatch (`name_by_frame:
false`); the two working frames kept the tool's default stamp and were never promoted.

**No landed evidence frame was renamed, moved or deleted.** Two of `TASK-1494`'s promoted frames are
cited below **by their existing paths, unmodified**, as the pre-removal arm.

---

## THE PIXEL ARM — THE ONE SOURCE THAT SHARES NOTHING WITH THE NAV WALKER

Byte-level PNG decode (zlib inflate + per-row unfilter, all five filter types, BT.601 luma,
threshold ≥ 150, x-window 20-1065). **The chip is IDENTIFIED BY ITS WIDTH, not assumed.**

| frame | painted button chips below y = 500 |
|---|---|
| **PRE** `Cards.PlacementResize` — `TASK-1494`'s promoted frame | **TWO**: y 574-588 x **494-591 (w 98)**, 1468 px · y 596-606 x **479-606 (w 128)**, 1406 px |
| **PRE** `Hero.Sprint` — `TASK-1494`'s promoted frame | **TWO**: identical, w **98** and w **128** |
| **POST** `Cards.PlacementResize` — mine, a1 | **ONE**: y 593-606 x **494-591 (w 98)**, 1372 px |
| **POST** `Hero.Move` — mine, a1 | **ONE**: identical |
| **POST** `Hero.Move` — mine, a2, **promoted** | **ONE**: identical |

The survivor's x-extent — **494-591, width 98** — is byte-identical to the pre-removal
`Back to the controls list` chip's. The **128 px**-wide chip (`Scroll down — back to the top at the
end`, a longer label) is **absent from all three post-removal frames**. Per-row fill density
confirms it: pre-Back 1468/15 = **97.9 px/row**, post-survivor 1372/14 = **98.0 px/row**, pre-Scroll
1406/11 = **127.8 px/row**.

**An internal negative control on the one page that exists in BOTH builds.** On
`Cards.PlacementResize`, the two painted prose bands above the buttons are **byte-identical across
the two builds** — y 501-504, x 165-570, **554** bright px, and y 513-516, x 164-613, **604** bright
px, in the pre-removal frame and in mine. The page did not move; the **only** delta in the lower
panel is the missing chip. (This also replicates `TASK-1494`'s *"lowest painted text row 516"*
cross-build, cross-session, with a different decoder run.)

---

## EVERY READING CARRIES BOTH ARMS, AND EACH INSTRUMENT'S SOURCE IS NAMED

| instrument | what it reads | positive arm | negative arm |
|---|---|---|---|
| `menu nav target registered -> … N focus stop(s)` | `USiegeMenuInputSubsystem::GetMenuFocusStops` — a UMG tree walk + the visibility/focusable predicate | reads **28** | reads **1**, seconds later, same session, same line |
| `MoveFocus(+1): … of N ('name')` | **SAME source** — declared, not independent | `of 28` ('RowButton') ×11 | `0 -> 0 of 1` ('BackButton') ×3 |
| live UMG `WidgetTree` walk (`ui_snapshot`) | the UMG object graph via the editor plugin | 29 Buttons in the whole overlay | **1** Button in the DetailView subtree |
| framebuffer (byte decode) | the composited render target — **shares no source with any of the above** | **2** chips in both pre-removal frames | **1** chip in all three post-removal frames |
| CDO reflection off the freshly linked DLL | the new binary's reflection data (design-time) | `BackButton` **True**, `DetailScrollBox` **True** | `DetailScrollButton` **False**, `bogus_symbol_xyz` **False** |
| Enhanced Input `applied_mapping_contexts` | the Enhanced Input subsystem — independent of the widget's own log | overlay open ⇒ `IMC_MainMenu` present | overlay closed ⇒ `IMC_MainMenu` absent |

**The honest statement about shared sources.** Criteria 1 and 2's headline numbers come from the nav
walker — but the nav walker **is the subject**, not a proxy for it: "how many stops does the ring
have" is a question about `GetMenuFocusStops` by definition. The arms that share **nothing** with it
are the **framebuffer** and the **CDO reflection**, and both agree. The `MoveFocus` line is called
out above as **not** independent of the registration line, because it is the same walker.

**The live UMG tree, in full — 14 nodes, the whole detail subtree, nothing elided:**
`DetailView` → `DetailBackdrop` → `DetailColumn` → { `DetailHeaderBox` → (`DetailKeyChipBorder` →
`DetailKeyChipText` `', / O / A / E'`, `DetailTitleText` `'Move'`), `DetailSummaryText`,
`DetailScrollBox` → (`DetailBodyText`, `RelatedHeaderText` *Collapsed*, `RelatedBox`),
**`BackButton`** → `BackLabelText` `'Back to the controls list'` }. **Exactly one `Button`. No
`DetailScrollButton`. No scroll label.** `DetailScrollBox` is present and `Visible`
(h 545.8) — preservation (iii) holds **at runtime**, not merely in the source.

**The list's 28 reconciled from a third direction:** the UMG walk finds **29** `Button` nodes —
27 × `RowButton` + `CloseButton` + `BackButton` — and `BackButton` sits inside the collapsed
`DetailView` branch, so the walker's visibility predicate drops it ⇒ **27 + 1 = 28**. That agrees
with `[ControlsHelp] Rebuilt 27 of 27 registry rows`, a fourth, unrelated log line.

**The `Tab` lane's own controls.** POSITIVE: `Tab` also **opened** the overlay twice
(`16.31.02:513`, `16.31.05:270`), `binding_found: true`, `bound_actions: ["IA_ControlsHelp"]` —
the key reaches the lane. NEGATIVE: `NumPadFive` (`binding_found: false`, *"NOTHING BINDS IT"*)
pressed through the **same** key path while the overlay was open produced **zero log lines** between
`Overlay opened.` `16.31.02:513` and `Overlay closed.` `16.31.04:343` — the instrument does not
print "closed" for just any key.

---

## ⚠️ THE PRE-EXISTING STATE THE DISPATCH NAMED — NOT FILED, AND NOT OBSERVED EITHER

A `BackButton` construction failure would now leave the detail page with **zero** nav stops
(pre-existing `TASK-1478` state, masked during `TASK-1484`'s life, logged as an `Error` at
`SiegeControlsHelpWidget.cpp:2567`). **It did not occur.** The string
`could not construct BackButton` matches **0** lines in this run — a zero **controlled** against the
same probe returning **4** for `Detail page open for` and **44** for `ControlsHelp` — and
`BackButton` was present, focusable and stop 0 in every reading, on three page opens across two
sessions. I am not filing it, and I am not reading it as a regression.

---

## Hypotheses (not verdicts)

- **H-A — the survivor moved DOWN into the slot the scroll chip vacated.** Pre-removal the `Back`
  chip painted at y 574-588 and the scroll chip at y 596-606; post-removal the single `Back` chip
  paints at y 593-606. That is consistent with an end-aligned vertical column in which removing the
  last child lets the remaining one sit at the bottom. It is a reading of where pixels are, not of
  layout code, and I name no remedy (`SC-§101`). **It is cosmetic and nothing in this row's four
  criteria depends on it.**
- **H-B — `ui_snapshot`'s absolute geometry and the capture's row index do not share an origin.**
  `BackButton` reports `abs_y 1529.27, h 40.42` with the viewport top at `abs_y 867.5`, which maps
  to capture rows ≈ 561-596, while the painted chip is at 593-606. I did **not** resolve the offset
  (a window-chrome origin is the obvious suspect) and **no claim above rests on that conversion** —
  the chip is identified purely by its **x-extent**, which needs no vertical mapping at all.
- **H-C — the `Tab` cover argued from the tree's SHAPE survived a change to the tree, as predicted.**
  `cpp:4315-4318` argues the overlay is on the focus path *"by construction"* because every
  focusable stop is a descendant of it. Removing a leaf stop cannot break that, and re-measuring
  agreed. Recorded as corroboration of the argument, not as a substitute for it — the dispatch was
  right to demand the measurement.

---

## Hazards, as encountered

1. 🚨 **Hazard 2 FIRED — and EARLIER than the dispatch warns. This is a correction the next row
   should carry.** `menu nav target registered -> 'WBP_VictoryScreen_C_0' (registered screen),
   1 focus stop(s)` at **`16.32.44:037`**, i.e. **≈ 3.4 minutes** into session 1's match, not the
   *"~7-8 min, 4 sightings"* on record. **Nothing was contaminated:** my last session-1 reading is
   `16.32.11:683`, **33 s earlier**, and session 2 is a fresh match (`16.35.30`-`16.35.33`) with
   **zero** victory-screen lines. ⚠️ Note also that the victory screen registers **`1 focus stop(s)`**
   — the same integer as the detail page — so a `1` read without its screen name would be
   ambiguous. **Every `1 focus stop(s)` I cite names `'SiegeControlsHelpWidget_0'` in the same
   line.** ⚠️ A further correction on my own method: I probed `VictoryScreen` at ~`16.31.4x` and got
   a true **0**, which was correct then and **stale within a minute**. The 0 is reported here as
   superseded rather than left standing.
2. **Hazard 1 (coalescing) controlled and did not fire** at 0.30 s — 11/11 presses landed with an
   unbroken index. See the spacing section.
3. **Hazard 3 (`ui_perform` moves the ring) NOT settled, deliberately.** I made **zero** `ui_perform`
   calls; `ui_snapshot` is a read, and the detail page it snapshotted still reported
   `1 focus stop(s)` (`16.32.10:750`) after it. This run contributes **no evidence either way** and
   `TASK-1493` stands.
4. **Hazard 4 (`unreal_inspector.grep` silent zeros) not hit** — I used the project `Grep` for
   source and `get_unreal_output_logs` for the log. **Every zero I took was controlled** with a
   positive probe on the same instrument: the empty Siegebound-category baseline against 2,674
   unfiltered lines; `VictoryScreen`=0 against `nav target registered`=7; `could not construct
   BackButton`=0 against `Detail page open for`=4.
5. 🚨 **Hazard 5 CONFIRMED AGAIN — no image reached my context.** `capture_pie_frame` returned
   `add_to_context: false` on all four calls and I never called `attach_pie_frames`. **I did not
   look at a single frame.** Every pixel statement above is a byte-level decode: it can see **where**
   painted regions are and **how wide**, never **what** they say. The chip's identity rests on its
   **x-extent** plus the UMG snapshot's `BackLabelText` string — **not** on reading glyphs.
6. **Two tool results overflowed the size limit.** `start_pie` (799,681 chars) — **ignored
   entirely**, PIE state read from `is_pie_active`, and **no finding here rests on it**
   (`TASK-1494` recorded the same). `run_verification_sequence` carrying the UMG snapshots (117,879
   chars) — recovered by parsing the saved result file; the button census above comes from that
   parse, and the parse is shown to have read all 245 / 14 nodes the snapshots reported.
7. **`get_command_line()` returned an empty string** (length 0) on this build ⇒ `SC-§118` identity
   by the in-process PID probe, as at `TASK-1436` / `TASK-1491` / `TASK-1494`.

---

Editor/Aura state: Aura connected (`editor_connected`). Editor **PID 15188** — identified per
`SC-§118` by the in-process probe `os.getpid() = 15188` returned by the MCP server answering on
`:8000`, so the answering server **is** the owning process; the command-line route was unavailable
(empty string). **The binary under test was proven from inside that process, not from the dispatch:**
`Binaries/Win64/UnrealEditor-GitClaudeUnrealTest.dll`, **10,018,816 B**, mtime
**2026-09-26 09:14:54.182** — byte-exact to what 5a's link produced. The three-armed reflected read
was **re-run by me** rather than inherited: on `USiegeControlsDetailWidget`'s CDO,
`DetailScrollButton` **False**, `BackButton` **True**, `DetailScrollBox` **True**,
`bogus_symbol_xyz` **False**. Map `L_Arena` — open at dispatch, never changed, confirmed
`/Game/Maps/L_Arena.L_Arena` with `DirtyMaps=[] DirtyContent=[]` at the end. PIE standalone, window
1280 × 720, viewport 1280 × 725, DPI 0.670639. **Two PIE sessions, 2 attempts of 3.** Wall time
≈ 12 min. **PID 15188 left UP**, PIE stopped (both sessions were mine; no session I did not start
was touched). No code, no asset, no compile, no git, no commit, no editor lifecycle action, no
evidence frame renamed or deleted. Milestone host `4620f71` untouched and local; origin still
`40c824b`. The dispatch's explicit PIE operating directives (*"Keep PIE short"*, the press-spacing
and match-duration fences) were taken as the `VER-§3` go.

**The automation suite is cited for exactly what it buys, per the dispatch's correction.** 18
`Siegebound.ControlsHelp.*` tests ran green in this build, including `DetailBackSeamReturnsToTheList`
(constructs a real `USiegeControlsDetailWidget`, drives `RequestBack` / `GetDetailActionId` /
`SetDetailContent`, asserts bind **and** unbind) — so the class still constructs and its Back seam
still fires. It buys **nothing** on my four criteria: those tests are `EditorContext` unit tests with
**no Slate tree**, and `DetailScrollButton` / `HandleScrollButtonClicked` / `AdvanceBodyScroll` /
`DetailScrollBox` are **0 hits across the whole Tests tree**. **All four runtime observations above
are mine.** I did not re-run the suite (outside this row's verbs) and cite it only as `TASK-1496`'s
5a reported it.

## Routing

All four criteria observed passing ⇒ **`VERIFIED`**. The cleanup is confirmed, the row closes, and
the change is commit-ready on the then-current host — **I took no commit, no stage and no push**
(`4620f71` is local; 🧑 he commits). The count stays **TEN of ten** (marker
`SCREEN-9-COUNTS-TEN-OF-TEN-2026-09-26`); this row does **not** re-open the scroll question, and it
does **not** close a screen.

## Not examined / limitations this run

1. **Three detail pages of 27 were opened** — `Cards.PlacementResize` (3 related) and `Hero.Move`
   (0 related), the latter twice. The "1 stop" claim is a property of `ConstructDetailTree`, which
   builds one tree for all pages, and it held on a 3-related page and a 0-related page; it is **not**
   27 measurements.
2. **`Tab` was tested from the list and from the detail page, in a match, on one keyboard layout**
   (`IMC_Hero_Positional_0/1`). No Dvorak/positional-remap sweep; `KBD-§` behaviour is untested here.
3. **The mouse route was not exercised at all** — no click on a `RowButton`, no wheel. `cpp:4319`'s
   claim (4) that clicking a row gives its `SButton` focus **inside** the overlay is **not** measured
   by me. All navigation was injected input actions and raw key presses.
4. **The `CloseButton` route was not exercised.** Only the two close routes the row names (`Tab` and
   `IA_ControlsHelp`) were driven.
5. **I never read the glyphs** — see hazard 5. I cannot certify from pixels that the surviving chip
   *says* "Back to the controls list"; that comes from the UMG snapshot's `BackLabelText`, a
   different source.
6. **One window size only**, 1280 × 725 at DPI 0.670639. No size sweep.
7. **The `abs_y` → capture-row conversion is unresolved** (H-B) and nothing rests on it.
8. **`ui_snapshot` was taken on `Hero.Move` only**, not on `Cards.PlacementResize`; the 14-node
   subtree census is one page's.
9. **No `MEASURED`-class number is offered.** This row is a gate, not a measurement: the four
   criteria are pass/fail readings, and the pixel work exists to give criterion 1 a second, unshared
   source — not to re-measure overflow (`TASK-1494` owns that and is untouched here).
