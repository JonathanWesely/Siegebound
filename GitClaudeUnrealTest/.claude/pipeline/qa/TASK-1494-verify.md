Verdict: MEASURED
# Verification — TASK-1494 [DETAIL-OVERFLOW-PIXEL-CENSUS]

## THE ANSWER, FIRST, AND IT IS THE LONGEST PAGE'S

**NO. The model's longest detail page — `Cards.PlacementResize`, modelled 3.19 × the view — does
not overflow. It fits, in pixels, with room under it.**

MEASURED, on the promoted frame, in a 1086 × 615 composited capture of the 1280 × 725 shipping
viewport:

| quantity on `Cards.PlacementResize` | MEASURED |
|---|---:|
| lowest painted text row | **516** of 615 |
| `Back to the controls list` button block | rows **573–588** |
| `Scroll down — back to the top at the end` button block | rows **596–606** |
| clear vertical gap, last painted text row → top of the first button | **57 px** |
| painted text runs above the buttons | **39** |

Per the row's own bar — *"a page fits if its lowest painted text row sits above the viewport bottom
with the buttons below it"* — this page fits: its last painted text row is 516, and **both** buttons
are painted below it with 57 px of clear space between. (`TASK-1491`'s equivalent on
`Cards.StackUpgrade` was row 424 with buttons at ~573/600.)

**CAMERA CONTROL: FIRED.** Both arms, same decoder, same bands, same session, on the promoted
frames — and it discriminated before I read a negative from it.

| pair | PROSE `y44-160` | RELATED `y240-420` | PAGE `y40-520` |
|---|---:|---:|---:|
| **NEGATIVE** — the same screen 1.30 s later (frames 1699735 / 1699809) | **0.000 %** | 0.001 % | **0.000 %** |
| **POSITIVE** — the subject vs a genuinely different (short) page | **20.525 %** | **12.693 %** | **15.476 %** |

The negative frames are not one capture read twice: distinct frame numbers, distinct game times
(188.84 s / 190.15 s), non-zero mean |Δ| (0.029–0.119). The camera can say **NO** and it can say
**YES**.

---

## AND THE SECOND, INDEPENDENT ARGUMENT — THE ONE A SINGLE PAGE COULD NOT GIVE

One page fitting could mean *"this page fits."* **Six pages fitting on a straight line means
nothing is being clipped at all**, and that is the stronger claim the ladder below supports.

A clipping viewport produces **one common bottom edge**: every overflowing page's text is pinned at
the same row. Six pages were measured. They stop at **six different rows**, spanning 461 px, in the
**exact rank order** the model ranks their content.

| page | modelled × view (`TASK-1487` — **MODELLED**) | lowest painted text row (**MEASURED**) |
|---|---:|---:|
| `Cards.PlacementResize` | 3.19 | **516** |
| `Cards.StackUpgrade` | 2.68 | **424** |
| `PickMode.Cancel` | 1.74 | **306** |
| `Orders.Attack` | 0.42 | **82** |
| `Hero.Sprint` | 0.32 | **69** |
| `Hero.Jump` | 0.19 | **55** |

**DERIVED** (straight-line fit through the two extremes, modelled content px → measured row):
slope 0.2211 px/unit, intercept 25.6. Residuals for the four interior pages: `Hero.Sprint` −6.2,
`Orders.Attack` −8.3, `PickMode.Cancel` +13.1, `Cards.StackUpgrade` −13.3. **Worst residual 13.3 px
on a 461 px span (2.9 %). No saturation, no ceiling, no common edge.**

**Cross-session replication of the one page anyone had measured before.**
`Cards.StackUpgrade`'s lowest painted text row read **424** in my session, with my decoder, from a
navigation I drove independently. `TASK-1491`, a different session on a different day, read **424**.
Identical.

---

## THE WIDGET'S OWN NUMBER FOR THE SUBJECT PAGE — AND WHAT THE PIXELS DID WITH IT

This page's runtime scroll line had never been logged before. It is now, and it agrees with
`TASK-1487`'s model almost exactly:

```
[ControlsHelp] Detail scroll on 'Cards.PlacementResize': offset 0.0 -> 591.0
               (end 1574.9, step 591.0, view 695.2) - MOVED.        07.23.51:622
```

⇒ the widget claims content = `end + view` = **2270.1** local units = **3.27 × the view**
(`TASK-1487` modelled 3.19 ×), i.e. **1,574.9 px of prose outside the painted bounds**.

**Across that exact press, MEASURED in pixels:**

| band | changed | mean \|Δ\| |
|---|---:|---:|
| PROSE `y44-160` | 0.113 % | 0.053 |
| RELATED `y240-420` | **0.000 %** | 0.052 |
| PAGE `y40-520` | 0.228 % | 0.071 |

Lowest painted text row **516 before and 516 after**; **39 text runs before and 39 after**; first
run `(6,9)` in both. That sits on the 0.000 % negative-control line, not the 12.7–35.4 % positive
line. This reproduces `TASK-1491`'s result (0.088 % / 0.000 % / 0.222 %) **on the longest page**,
against a widget claim 2.4 × larger than the one it refuted there.

**DERIVED conversion, stated so it can be checked:** capture 1086 px for a 1280 px viewport ⇒
× 0.8484; 1280 px at DPI scale 0.670639 ⇒ 1908.6 widget-local units across ⇒ **1 local unit =
0.5690 capture px**. The subject's painted prose spans row 64 (first body line) → row 516 =
**452 capture px ≈ 794 local units**, against the widget's claimed 2270.1 — a factor of **2.86**.
And since the painted prose occupies ≥ 452 capture px, the painted box is **≥ 794 local units**
tall while the widget's `view` reads **695.2** ⇒ the widget **over-reads its content and
under-reads its own viewport**, both in the direction of inventing a fold.

---

Editor/Aura state: Aura connected (`editor_connected`). Editor **PID 24652** — identified per
`SC-§118` by the in-process probe `os.getpid() = 24652` returned by the MCP server answering on
`:8000`, so the answering server **is** the owning process; `SystemLibrary.get_command_line()`
returned an **empty string** (length 0) on this build, so the command-line route was unavailable and
the PID probe is the fallback, exactly as at `TASK-1436` and `TASK-1491`. Map `L_Arena` — open at
dispatch, never changed, confirmed open and clean at the end. PIE standalone, window 1280 × 720,
viewport reports 1280 × 725, DPI scale 0.670639. **Two** PIE sessions, **3 attempts of 3**. Wall
time ≈ 50 min. **PID 24652 left UP**, `GameWorld=None` after stop, `DirtyMaps=[] DirtyContent=[]`.
No code, no asset, no compile, no git, no commit, no editor lifecycle action, no evidence frame
renamed or deleted. Milestone stays at `4620f71`, local and unpushed. Jonathan's standing go covered
this run per the dispatch.

**`TASK-1492` had NOT landed when I measured** — checked three ways before the first PIE call:
its row reads `status: backlog` / HELD; `git status` shows no source file modified; and
`SiegeControlsHelpWidget.cpp:1905` still reads
`Entry.Body = ResolveDetailTokens(ComposeDetailForDisplay(*RelatedRow), ComposeChipForId);`
— the un-collapsed form, each related block still rendering the related row's entire body.

**Subject reached by agent verbs only, and identified by the shipped log, never by assumption:**
`IA_ControlsHelp` → 11 × `IA_MenuDown` → `IA_MenuAccept` →
`[ControlsHelp] Detail page open for 'Cards.PlacementResize' (3 related control(s)).` Every frame in
this report is tied to a `Detail page open for '<id>'` line; where a navigation landed somewhere
other than intended, the frame is reported under the page the log names, never the page I aimed at.

**Attempts.** Attempt 1 (PIE #1): the six-page ladder and both control arms — 8 clean frames; ended
when hazard 3 fired (below), contaminating 2 frames which are discarded. Attempt 2 (same session,
post-match): refused by the world — `MoveFocus(+1) declined: menu covered` — 2 frames discarded.
Attempt 3 (PIE #2): the scroll-press corroborator on the subject page, plus the three promoted
frames. Attempt 1's evidence is kept and cited alongside attempt 3's (`VER-§1` cl. 6).

---

## Acceptance lines → observations

| # | acceptance line | observable chosen | observed (quoted values, evidence path) | pass/fail/unobs |
|---|---|---|---|---|
| 1 | line 1 `Verdict:` | — | `Verdict: MEASURED`, byte-literal, no suffix | pass |
| 2 | the CAMERA CONTROL declared FIRED or ABSENT on the face of the report | two-arm pixel control on the promoted frames | **FIRED** — NEGATIVE 0.000 % / 0.001 % / 0.000 %; POSITIVE 20.525 % / 12.693 % / 15.476 %. Declared in the second block of the report. `…/VER-TASK-1494-a3-longest-page-lowest-painted-text-row.png` vs `…-negative-control-same-screen.png` vs `…-short-page-positive-control.png` | pass |
| 3 | the longest page's measured result FIRST | lowest painted text row vs the view bottom, `TASK-1491`'s method reused exactly | `Cards.PlacementResize`: lowest painted text row **516** of 615, buttons **573–588** and **596–606**, **57 px** clear gap, 39 text runs. Replicated at 516 in **five** independent captures across **two** PIE sessions (t=34.22 s, 35.50 s, 251.83 s in session 1; t=25.41 s, 28.05 s, 188.84 s, 190.15 s in session 2) | pass |
| 4 | measured-vs-modelled labelled per row | every table above | modelled figures carry `MODELLED` and their source (`TASK-1487`); pixel figures carry `MEASURED`; the two arithmetic conversions carry `DERIVED` with the factors written out | pass |
| 5 | `## Not examined / limitations` | — | present, 10 entries | pass |
| 6 | the row's question — *does ANY detail page overflow, measured in pixels?* | the ladder + the fit | **NO** for the longest page and for all five others measured. 6 pages, 6 different bottom rows, on a straight line with a 2.9 % worst residual. 21 of 27 pages were **not** measured — see limitation 1 | answered |

---

## Evidence (promoted)

- `.claude/pipeline/playtest-evidence/2026-09-26/VER-TASK-1494-a3-longest-page-lowest-painted-text-row.png`
  — `Cards.PlacementResize`, the model's longest page, freshly opened (log `07.26.37:625`), focus on
  `BackButton`. 1086 × 615, mean luma 51. 39 painted text runs in three blocks (rows 64–137,
  174–297, 357–516) below the header (6–31) and the summary (44–54); the last painted text row is
  **516**; then **57 rows of clear panel**; then the `Back to the controls list` button block at
  **573–588** and the `Scroll down…` button block at **596–606**. PIE `t = 188.84 s`, frame 1699735.
- `.claude/pipeline/playtest-evidence/2026-09-26/VER-TASK-1494-a3-longest-page-negative-control-same-screen.png`
  — the identical screen 1.30 s later, a distinct capture (frame 1699809, `t = 190.15 s`). Same 39
  runs at the same rows, same lowest text row 516, same button blocks. **0.000 % of the page's
  pixels differ** — this is the camera's NEGATIVE arm.
- `.claude/pipeline/playtest-evidence/2026-09-26/VER-TASK-1494-a3-short-page-positive-control.png`
  — `Hero.Sprint` (log `07.26.53:416`, 0 related controls), a genuinely different, much shorter
  layout: **6** painted text runs, lowest text row **69**, then **504 px** of clear panel to the same
  two button blocks at 573–588 / 596–606. Against the subject frame this reads **20.525 % / 12.693 %
  / 15.476 %** — the camera's POSITIVE arm.

Working frames retained in `Saved/` (gitignored, not promoted; the report cites them only as
corroboration): `Saved/AuraVerify/T1494/` — `A_placementresize_1_t34.22s_f1651329.png`,
`A_placementresize_2_t35.50s_f1651402.png`, `G_placementresize_replicate_t251.83s_f1663699.png`
(the same page re-opened 218 s later through a completely fresh navigation — **0.008 % page-wide**
against the first), `F_placementresize_replicate_t216.41s_f1661732.png` (`Cards.StackUpgrade`, row
424), `B_shortpage_t64.23s_f1653004.png` (`PickMode.Cancel`, row 306),
`E_placementresize_replicate_t182.84s_f1659868.png` (`Orders.Attack`, row 82),
`D_shortestpage_t161.54s_f1658629.png` (`Hero.Jump`, row 55),
`L_listpage_t61.00s_f1652815.png` (the list page — 97.8 % against the subject),
`M_before_scrollpress_t25.41s_f1690628.png` / `N_after_scrollpress_t28.05s_f1690762.png` (the press
pair). **Discarded and cited as discarded, not as evidence:** `C_shortestpage_t114.38s_f1655952.png`
(the overlay had closed), `H_before_scrollpress_t575.84s_f1681900.png` /
`I_after_scrollpress_t578.12s_f1681980.png` and `J_before_scrollpress_t670.82s_f1687197.png` /
`K_after_scrollpress_t673.18s_f1687310.png` (the victory screen had taken focus).

No landed evidence frame was renamed, moved or deleted. My own new frames omit the `-t<time>` token
per the dispatch.

---

## An instrument I built, measured, and threw away — stated in full rather than quietly dropped

I found a 5-px-wide vertical band at **x = 1069–1073** that differs from the row background on
**446 of 446** sampled rows, and I read it as the always-shown scrollbar
(`DetailScrollBox->SetAlwaysShowScrollbar(true)`, `SiegeControlsHelpWidget.cpp:2372`). It gave a
clean-looking **thumb/track = 0.9382**, which would have been a tidy second number for "the page
fits."

**It is not a scrollbar and I discarded it.** Two checks killed it: (i) the band is the *same*
— track y 5–570, luma ≈ 99, thumb/track 0.9382 — on the 3.19 × page and on the 0.19 × page, and a
real thumb cannot be identical on pages whose content differs 16-fold; (ii) it spans y = 5, i.e.
*above the header*, where the scroll box cannot reach. It is the detail panel's right border.
Recorded because a band that behaves identically on the longest and the shortest page is exactly the
shape of a confident, meaningless number (`VER-§12` cl. 7). **No figure in this report rests on it.**

---

## Hypotheses (not verdicts)

- **H-A — `TASK-1487`'s census is UNANCHORED in SCALE but CORRECT in RANK, and that distinction is
  the useful one.** Its six modelled pages rank exactly as the pixels rank them, and a single
  straight line maps its modelled content onto my measured rows to within 2.9 %. What is wrong is
  only the *conversion to screen*: the model puts the longest page's content at 2218 local units
  where the pixels put the painted prose at ≈ 794. The fold count follows from the scale, not the
  rank — which is why the ranking survived every robustness sweep in §4d while the headline did not.
- **H-B — the `!ScrollBar->IsNeeded()` branch is the live one, now with the longest page's numbers
  behind it.** `SScrollBox::Tick` pins `ScrollPanel->PhysicalOffset = 0` when the bar is not needed
  (`SScrollBox.cpp:941-945`). On this page the widget wrote 591.0 unclamped and the pixels moved
  0.228 % (page) / 0.000 % (related). If the content genuinely exceeded the view, that branch could
  not fire and the prose would have moved 591 px. This is `TASK-1491`'s H-A, re-observed on the page
  with the largest claimed fold.
- **H-C — the geometry cache disagrees with the framebuffer in BOTH directions at once.** Derived
  above: content over-read ≈ 2.86 ×, viewport under-read (`view 695.2` vs a painted box ≥ 794 local).
  Both errors manufacture a fold. This is a programmer's read of `SScrollBox` / the arranged pass,
  not a verifier's, and I name no remedy (`SC-§101`).
- **H-D — the render is STABLE where the geometry is not.** The same page re-opened 218 s later
  through an entirely fresh navigation differed by **0.008 %** page-wide, and its lowest painted text
  row was 516 in all five captures across two PIE sessions. The instability this epic was built on
  — `view` alternating 78.8 / 695.2, `end` taking four values for one page — is **not** in the
  pixels. It is in the cache.
- **H-E — I saw no truncation signature.** If the related bodies were silently not rendering, the
  three related-bearing pages would fall *below* the line set by the two 0-related pages; they all
  sit well *above* it (`Cards.PlacementResize` 516 vs 408 extrapolated). That is consistent with the
  related bodies being drawn — but it is an extrapolation from two short pages, not a reading of the
  text, and limitation 3 below is the honest bound on it.

**Routing.** The row's question is answered NO, with a controlled camera, so this is a measurement,
not a gate: `MEASURED` never blocks and never bounces (`CLAUDE.md` 5c, `VER-§1` cl. 5a). It is the
input `TASK-1495` is blocked on. I have taken no remedy, named no fix, and touched nothing outside
this report, my own row, and the three promoted frames.

## Hazards, as encountered

1. **Hazard 3 REPRODUCED — and it is no longer "not-reproduced-once."** The match self-ended in
   session 1: `menu nav target registered -> 'WBP_VictoryScreen_C_0'` at `07.20.17:421`, and it
   **stole focus** exactly as warned — my next `IA_MenuDown` + `IA_MenuAccept` landed on its
   `Btn_Jump` ("Play Again"), restarting the match. Two frames contaminated and discarded; the
   follow-up attempt was then refused with `MoveFocus(+1) declined: menu covered`. **Every reading in
   this report predates it**: the subject frames are at PIE t = 34.22 s / 35.50 s (log `07.11.47`),
   inside the row's ~2-minute fence, and the whole ladder is complete by `07.15.31`.
2. **NEW hazard, recorded for the next row: injected input actions issued closer together than about
   one frame are silently coalesced.** 26 × `IA_MenuUp` at ~16 ms apart advanced focus far fewer than
   26 stops and the following `IA_MenuAccept` closed the overlay instead of opening a page (one
   discarded frame). At 0.15 s spacing one press of twelve was still swallowed (I aimed at
   `Cards.PlacementResize` and got `Cards.StackUpgrade` — which is why the cross-session replication
   of `TASK-1491`'s anchor exists at all). **0.25 s spacing was reliable across ~40 presses.** The
   defence that actually worked was never the spacing: it was reading `Detail page open for '<id>'`
   out of the log after every navigation and labelling the frame by what the log said.
3. **Hazard 1 (`ui_perform` clears focus) NOT settled, and deliberately so.** I used **zero**
   `ui_perform` calls this run — all navigation was `inject_input_action` — so this run contributes
   **no evidence either way** and `TASK-1493` stands. The only adjacent observation: across ~9 page
   opens driven purely by injected actions, the focus ring was never in an unexpected place except
   where an input was swallowed by rate (hazard 2), which is a different mechanism. That is a weak
   note, not a result.
4. **Hazard 4 (`unreal_inspector.grep` silent zeros) not hit** — I used the project `Grep`
   throughout and took no zero from `unreal_inspector.grep`.
5. **`start_pie` overflowed the tool-result limit twice** (3,054,913 and 2,235,160 characters) —
   `VER-§12` cl. 6. Both were ignored; PIE state was read from `is_pie_active` instead, and **no
   finding here rests on that output**.
6. **`get_command_line()` empty** (length 0) ⇒ `SC-§118` identity by the in-process PID probe.

## Not examined / limitations this run

1. **21 of 27 pages were not measured.** Six were: `Cards.PlacementResize`, `Cards.StackUpgrade`,
   `PickMode.Cancel`, `Orders.Attack`, `Hero.Sprint`, `Hero.Jump`. The row's instruction was explicit
   — *if the longest page fits, the answer is effectively NO and you are done* — and the longest page
   fits. The claim "no page overflows" therefore rests on the **ladder argument** (six pages, six
   different bottom rows, on one line with no saturation), **not** on 27 measurements. A page that
   overflowed would have to break that line.
2. **One window size only.** 1280 × 725, DPI 0.670639. No size sweep; nothing here speaks for any
   other viewport, and `TASK-1487`'s §4e claim that no realistic window swallows the content is
   modelled and untested by me.
3. **I never read the glyphs.** 🚨 **`attach_pie_frames` and `take_editor_screenshot(mode="pie")`
   both returned `success: true` with `add_to_context: true`, and neither image ever reached my
   context** — so I could not look at a single frame this run. Every statement above is from the
   byte-level PNG decode (zlib inflate + per-row unfilter, all five filter types, luma). **I measured
   where the painted text STOPS; I did not read what it SAYS.** In particular I cannot certify that
   every authored sentence is rendered — H-E is an extrapolation, not a reading. If anyone needs
   *"is the last sentence of the third related block on screen?"*, that is a human eye or a new
   instrument, and it is the one question this method structurally cannot answer.
4. **`PhysicalOffset` was never read directly** — unreachable (non-`UObject`, unreflected), closed by
   `TASK-1491`. `GetViewOffsetFraction()` is `DesiredScrollOffset` (`SScrollBox.cpp:593`), also
   closed. Arranged geometry shares the widget's cache, also closed. I re-derived none of the three
   and substituted no proxy for them (`SC-§138`).
5. **The scroll box's exact bottom edge is not measured**, only bounded. The clear band between row
   516 and the button at 573 contains it; I did not locate it to the pixel, and the discarded border
   band is why (see above). The 57 px figure is *text-to-button*, not *text-to-box-bottom*, and is
   quoted that way throughout. `TASK-1491`'s "~90 px spare" was text-to-box-bottom on a different
   page — **the two numbers are not the same measurement** and should not be differenced.
6. **The two conversions are DERIVED, not measured**: the 0.5690 capture-px-per-local-unit factor
   and everything computed through it (the ≈ 794 local units, the 2.86 × ratio, the ≥ 794 box
   height). They rest on the reported viewport (1280 × 725) and DPI (0.670639) being exact.
7. **The straight-line fit is a fit.** Two free points, four residuals; it is offered as a
   *saturation test*, not as a calibration, and it would be worth nothing if the residuals were not
   small in the presence of a 16-fold range.
8. **`end 1574.9 / view 695.2` for `Cards.PlacementResize` is a NEW runtime datum but it comes from
   the family `TASK-1491` showed unstable** — I observed it once, on one press. It is quoted as the
   widget's claim, never as a measurement of the page.
9. **No automation-suite citation** — coverage for this screen is 0, per the dispatch.
10. **Hazard 2's swallowed inputs mean one intended navigation never happened**: I aimed twice at a
    fresh-open replicate of the subject and got `Orders.Attack` and `Cards.StackUpgrade` instead. Both
    are reported under the page the log names. The subject replicate was obtained on the third try
    (frame G) and again in session 2.
