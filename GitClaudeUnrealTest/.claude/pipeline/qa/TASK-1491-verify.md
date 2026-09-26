Verdict: VERIFY-FAILED
# Verification — TASK-1491 [DETAIL-SCROLL-REVERIFY]

## THE TWO ANSWERS THE ROW ASKED FOR, FIRST

**(3) PAINT OR CAMERA — the answer is PAINT.** On `Cards.StackUpgrade`, pressing
`DetailScrollButton` does **not** move the prose on screen. Across a single press whose own log
line reads `MOVED`, **0.222 %** of the page's pixels changed (mean |Δ| 0.075) — against a
same-state negative control of **0.000 %** and a positive control of **12.592 %** measured with the
**same instrument on the same band**. The first ten lines of `DetailBodyText` changed **0.088 %**.
The mouse wheel is the same answer: two notches, prose band **0.986 %**, and that residual is the
mouse cursor `ui_perform` parked inside the box.

**(4) CAMERA POSITIVE CONTROL: FIRED.** Twice, on the exact bands the verdict rests on.
- POSITIVE: the same decoder, same band, same session registered **24.518 %** changed pixels
  (mean |Δ| 20.549) in the related-block band and **12.592 %** page-wide between two frames whose
  layout demonstrably differed. The camera can say **YES**.
- NEGATIVE: two captures of an unchanged screen 1.1 s apart returned **0.000 %** changed in every
  band (mean |Δ| 0.021). The camera can say **NO**, and it is not returning the same frame twice —
  the three frames carry distinct frame numbers (1557232 / 1557299) and distinct game times.
- ⇒ **This is not an uncontrolled bit-identical pair.** The instrument discriminated in both
  directions before I read a negative from it.

**And the plain-language reason, visible in both promoted frames: the page FITS.** The lowest row
containing text is **y = 424** of 615 in *both* the before and after frames, with `Back to the
controls list` at ~y 573 and the scroll button at ~y 600. There is nothing below the fold on this
page as rendered, so there is nothing for the button to page to.

---

## (2) METHOD CHOSEN, AND WHAT IT CANNOT SEE

I ran **two** instruments and they **disagreed**. Reporting both, and which one I believe, is the
substance of this row.

**Instrument P — arranged-widget geometry (`ui_snapshot` absolute coordinates). CHOSEN FIRST, AND
IT IS REFUTED.** Rationale at the time: candidate (i), reading `SScrollPanel::PhysicalOffset`
directly, is **unavailable** — `SScrollPanel` is a non-`UObject` Slate widget, the member is not
reflected, and no verb I hold reaches it (`unreal.WidgetBlueprintLibrary` does not exist in this
build's Python, and `UScrollBox::GetViewOffsetFraction()` is computed from `DesiredScrollOffset`
at `SScrollBox.cpp:593`, so it is *also* not the painted value). The arranged child position looked
like a faithful proxy, because `SScrollPanel::OnArrangeChildren` offsets children by exactly
`-PhysicalOffset` (`SScrollBox.cpp:177`).
**What it cannot see — and this is what bit:** it reads the geometry *cache*, not the framebuffer.
It is the **same cache the widget's own arithmetic reads**, so it agrees with the widget by
construction and cannot referee a disagreement between the widget and the screen. It reported the
content moving by *exactly* the logged offsets while the pixels did not move at all.

**Instrument X — decoded pixels in named y-bands (candidates (ii) + (iii)). THIS IS THE ONE THE
VERDICT RESTS ON.** The captured PNGs are decoded byte-for-byte in the read-only Python lane (zlib
inflate + per-row unfilter, all five filter types), converted to luma, and compared per band as
"% of pixels differing by more than 8". It compares **two distinct scroll positions**, not only a
before/after of one press, and it carries both controls above.
**What it cannot see:** anything outside the captured composited frame; sub-8-level changes; and it
cannot tell me *why* the layout it photographs differs from the layout the geometry cache reports.
It also cannot speak for any page or window size other than the one measured.

---

Editor/Aura state: Aura connected (`editor_connected`). Editor **PID 24652** — identified per
`SC-§118` by the in-process probe `os.getpid() = 24652` returned by the MCP server answering on
`:8000`, so the answering server **is** the owning process; note `SystemLibrary.get_command_line()`
returned an **empty string** on this build, so the command-line route was unavailable and the PID
probe was used, exactly as at `TASK-1436`. Map `L_Arena` (open at dispatch, unchanged, restored).
PIE standalone, window 1280×720, viewport reports 1280×725, DPI scale **0.670639**. **One** PIE
session, **2 attempts of 3** (attempt 1 = instrument P + the first frame series; attempt 2 = the
clean isolated single-press pair, re-run after a `ui_perform` disturbed the focus ring). Wall time
≈ 25 min. **PID 24652 left UP**, `GameWorld=None` after stop, `DirtyMaps=[] DirtyContent=[]`.
No code, no asset, no compile, no git, no commit, no editor lifecycle action. Milestone stays at
`4620f71`, local and unpushed. Jonathan's go was standing in the dispatch.

**Subject reached by agent verbs only, and pinned as ordered:** `IA_ControlsHelp` →
`menu nav target registered -> 'SiegeControlsHelpWidget_0' … 28 focus stop(s)` → 10 × `IA_MenuDown`
(`MoveFocus(+1): focus moved 9 -> 10 of 28 ('RowButton')`) → `IA_MenuAccept` →
`[ControlsHelp] Detail page open for 'Cards.StackUpgrade' (3 related control(s)).` No substitution.

---

## Acceptance lines → observations

| # | acceptance line | observable chosen | observed (quoted values, evidence path) | pass/fail/unobs |
|---|---|---|---|---|
| 1 | `TASK-1484` / his sentence — *pressing the new button pages the prose down on screen* | decoded pixels, prose band `y44-160` and page band `y40-520`, around ONE isolated press | log said `[ControlsHelp] Detail scroll on 'Cards.StackUpgrade': offset 0.0 -> 67.0 (end 1464.3, step 67.0, view 78.8) - MOVED.` `06.54.53:335`; pixels changed **0.088 %** (prose) / **0.000 %** (related) / **0.222 %** (page), mean \|Δ\| 0.057–0.075 — at the 0.000 % negative-control floor. `…/VER-TASK-1491-prose-band-before-scroll-press.png` → `…-after-scroll-press.png` | **fail** |
| 2 | *…or roll the wheel* (🧑 his own second non-observation) | same instrument, plus arranged geometry | `ui_perform` scroll ×2 on `DetailScrollBox`; prose band **0.986 %**, related band **0.973 %** — both bands equal, i.e. no localisation, and the residual is the mouse cursor the perform parked in the box. Arranged geometry claimed offset 320.0 local (stable at PIE `t=550.1` and `t=623.9`); the pixels did not follow. | **fail** |
| 3 | *`Cards.StackUpgrade` has 1,166 px of prose outside the painted bounds (2.68 × fold)* | ink extent of the rendered page | lowest text row = **424** of 615 in BOTH frames; `Back to the controls list` ~573, `Scroll down — back to the top at the end` ~600. The page's whole content — body, `All the controls that go with it`, and all three related blocks — is on screen at once with ~90 px to spare. | **fail (as rendered)** |
| 4 | the row's own bar — the camera must be shown able to see a change it knows happened | positive + negative control on the same bands | POSITIVE **24.518 %** / **12.592 %**; NEGATIVE **0.000 %**. Declared FIRED. | **pass** |
| 5 | the mechanism fires (already proven, not re-litigated) | the shipped `Log` line | fires on every press, 12 lines this session, both offsets printed. Not in dispute. | pass (inherited) |

### The measurement table the verdict rests on

% of pixels differing by more than 8 (decoded luma, `x` 8–1078):

| pair | PROSE `y44-160` | RELATED `y240-420` | PAGE `y40-520` |
|---|---:|---:|---:|
| **NEGATIVE CONTROL** — same screen, 1.1 s apart, no input | 0.000 % | 0.000 % | 0.000 % |
| **THE SUBJECT** — one press, log says `0.0 -> 67.0 … MOVED` | **0.088 %** | **0.000 %** | **0.222 %** |
| **POSITIVE CONTROL** — two states whose layout differed | 0.129 % | **24.518 %** | **12.592 %** |

The subject sits on the negative-control line, not the positive-control line.

### The number that explains the whole affair

`DetailScrollBox->GetCachedGeometry().GetLocalSize().Y` is **not stable within one session**:

| time | log line |
|---|---|
| `06.40.58:901` (first press after page open) | `offset 0.0 -> 67.0 (end 1464.3, step 67.0, **view 78.8**) - MOVED.` |
| `06.42.50:940` … `06.42.53:156` (8 rapid presses) | `… (end 1166.4 → 1591.3, step 591.0, **view 695.2**) - MOVED.` |
| `06.44.12:081` | `offset 1591.3 -> 0.0 (end 1464.3, step 67.0, **view 78.8**) - WRAPPED to the top.` |
| `06.54.53:335` (first press after a FRESH page open) | `offset 0.0 -> 67.0 (end 1464.3, step 67.0, **view 78.8**) - MOVED.` |
| `2026-09-25 23.58.06:616` (`TASK-1436`, on record) | `offset 0.0 -> 591.0 (end 1166.4, step 591.0, **view 695.2**) - MOVED.` |

Two of twelve presses read `view 78.8`; `end` took four different values (1166.4 / 1464.3 / 1534.7 /
1591.3) for one unchanged page. **Every downstream number — the step, the clamp, `GetScrollOffsetOfEnd()`,
and therefore the "1,166 px outside the painted bounds" that mints this wave — is computed from that
reading.**

---

## Evidence (promoted)

- `.claude/pipeline/playtest-evidence/2026-09-25/VER-TASK-1491-prose-band-before-scroll-press.png`
  — `Cards.StackUpgrade` detail page, focus on `DetailScrollButton`, immediately before the press.
  Header `LMB · Stack a tower taller`, body opening "Hover one of your OWN buildings while holding
  the card that built it…", then `All the controls that go with it` and all three related blocks
  (`1 / 2 / 3 / 4 / 5 / 6 — Play a card`, `Mouse Wheel Up / Mouse Wheel Down — Resize what you are
  placing`, `RMB / Esc — Cancel`), text ending at row 424, then the two buttons. PIE `t=1001.99 s`.
- `.claude/pipeline/playtest-evidence/2026-09-25/VER-TASK-1491-prose-band-after-scroll-press.png`
  — the same page 1.5 s later, after `IA_MenuAccept -> OnClicked.Broadcast() on 'DetailScrollButton'`
  and the `MOVED` log line. **Pixel-wise indistinguishable from the frame above** (0.222 % of the
  page differs, at the 0.000 % noise floor): same first line at the same height, same related
  blocks at the same heights, same lowest text row 424. PIE `t=1003.49 s`.
- `.claude/pipeline/playtest-evidence/2026-09-25/VER-TASK-1491-detail-scroll-painted-offset-0.png`
  and `…-painted-offset-998.png` — the pair used as the camera's **positive control** (24.518 % /
  12.592 %). ⚠️ **Their filenames encode instrument P's reading at capture time, which this report
  refutes**: the `-998` frame is at *requested* offset ~998 and *painted* offset 0; what differs
  between the two is the page's content re-flowing (the gap under the body closing, lowest text row
  513 → 415), **not** a scroll. Read them as "two layouts the camera could tell apart", nothing more.

Superseded / owed for removal (I have no shell and will not delete):
`VER-TASK-1491-detail-scroll-mouse-wheel-painted-offset-320.png` (filename asserts a painted offset
this report refutes) and `VER-TASK-1491-prose-band-after-three-scroll-presses.png` (contaminated —
the press landed on the list page after a `ui_perform`; superseded by the clean pair above).

---

## Hypotheses (not verdicts)

- **H-A — the `!ScrollBar->IsNeeded()` branch is the live one.** `SScrollBox::Tick` does
  `if (!ScrollBar->IsNeeded()) { ScrollPanel->PhysicalOffset = 0.0f; }` (`SScrollBox.cpp:941-945`).
  If the painted layout fits — and the ink extent says it does — that branch pins the painted offset
  at 0 every tick, no matter what `SetScrollOffset` wrote. `TASK-1487` excluded this branch on
  `ViewFraction = 695.2 / 1861.6 = 0.3734`, computed from the same cached-geometry family that also
  reports `view 78.8`. Its reasoning is sound; its input may not be.
- **H-B — there are two layouts, and the widget does its arithmetic against the wrong one.** The
  geometry cache and the framebuffer disagree, and they disagree *consistently*: instrument P said
  the content moved −67.00 local (exactly the written offset) and later −997.9 local, while the
  prose band moved 0.088 % / 0.129 %. The alternating `view 78.8 / 695.2` is the same defect seen
  from inside the widget.
- **H-C — `TASK-1487`'s 15-of-27 census is now SUSPECT, not refuted.** It is calibrated to the single
  runtime number `1861.6`, which comes from this family. I measured **one** page and found it fits;
  I did not measure the other 26 and I am not claiming anything about them.
- **H-D — the earlier "it moved" pixel readings were a settle, not a scroll.** My own first pair
  (A → C) showed 18.5 % change in the related band; the clean pair shows 0.000 % for the same single
  press. The difference is a one-time content re-flow ~56 s after page open, not the view scrolling —
  the prose band was ≤ 0.129 % in *both*. I record this because it nearly became my headline.

**Routing (row spec (5), route 2):** *paint did NOT move, with a controlled camera ⇒ a real finding
about runtime vs the source model. Route it; do not fix it (`SC-§101`).* I name no remedy and have
taken none. This also lands **before** `TASK-1492`: that row's premise — collapse the related bodies
and most of the 15 folds disappear — rests on the census H-C puts in question, and the manager should
see both facts together.

## 🧑 The sentence this row carries to him (cl. 3(b), verbatim, parenthesis included)

> "Open the Tab screen and go into a long row's detail page — does the text actually move when you
> press the new button or roll the wheel? **And two things you should know before you judge that
> button: a SMALLER affordance (a compact `▼`) is STILL AN OPEN OPTION, and it costs you about
> 74 px of visible prose on every detail page — I told you 34 twice and that was WRONG, it is
> roughly DOUBLE.**"

My measured answer to the first half, so he does not have to take it on trust: **no — not for the
button, and not for the wheel.** And the reason is not that the scroll is broken in a way that hides
text from him: on this page, at this window size, **nothing is hidden.** The whole page is on screen.

## Not examined / limitations this run

1. **One page, one window size.** `Cards.StackUpgrade` at 1280×725, DPI 0.670639. No size sweep.
   `Cards.PlacementResize` — the modelled longest page, 3.19 × — was **not** tested (the row pinned
   the subject and forbade substitution).
2. **`PhysicalOffset` was never read directly.** Candidate (i) is unreachable from every verb I hold
   (non-`UObject`, unreflected). I did not substitute a proxy and then call it candidate (i).
3. **I did not determine WHY the geometry cache and the render disagree.** That is the open question
   this report hands on, and it is a programmer's read, not a verifier's.
4. **No automation-suite citation** — coverage for this screen is 0 across `Tests/`, per the dispatch.
5. **Hazard 4 did NOT reproduce:** the match was still live (`SiegeGameMode` / `SiegeGameState`,
   2 players) at PIE `t=971 s` ≈ 16 min, well past the ~8 min self-end seen three times before. No
   victory screen appeared and nothing stole focus during a reading.
6. **Hazard 2 did NOT reproduce:** `ui_snapshot` selectors **did** narrow by name on this build
   (`{"by":"name","value":"DetailScrollBox"}` returned that subtree alone).
7. **Hazard 1 (H1) partially corroborated, once more:** after a `ui_perform`, a following
   `IA_MenuDown` + `IA_MenuAccept` landed on the **list page** instead of the scroll button — the
   focus ring was not where it had been. Consistent with `ui_perform` disturbing focus. I did not
   isolate it, and it cost one contaminated frame (declared above), not a reading.
8. **Hazard 3 not hit.** `unreal_inspector.grep` was used only against engine source and returned
   hits; project-side searches used the project `Grep`.
9. **`ui_snapshot` reports a garbage/unarranged geometry family for culled widgets** (e.g. `abs_x`
   15.02 with sizes unrelated to the arranged ones). I used only real-absolute-family values, but a
   future reader should not treat every `abs_*` it returns as screen space.
10. **Evidence filenames omit the `-t<time>` token** per the dispatch.
