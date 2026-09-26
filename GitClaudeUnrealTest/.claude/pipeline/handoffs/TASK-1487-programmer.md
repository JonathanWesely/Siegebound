# TASK-1487 — [DETAIL-SCROLL-VISIBILITY-DIAGNOSE] — programmer handoff

**Agent:** gameplay-programmer · **Date:** 2026-09-25 · **Mode:** READ-ONLY (no code, no asset, no
editor, no PIE, no compile, no git — nothing was written but this file and this row's `status:`)

---

## ROUTE — **A**

> *"A fold exists somewhere ⇒ routes to a re-verification on that page/size. Name the page and the size."*

**A fold exists, and it exists on the very page `TASK-1436` already tested.** It is not a modelled
fold: it is the arithmetic of the shipped log line itself, once the provenance of `end` is settled.

- **Page to re-verify: `Cards.StackUpgrade`** — the one page whose overflow is a *measurement*
  rather than an estimate. `end 1166.4, view 695.2` ⇒ content desired height **1861.6** = **2.68 ×
  the view**. 1166.4 px of prose was outside the painted bounds at the moment that line printed.
- **Size: the same one `TASK-1436` already ran** (whatever viewport produced `view 695.2`). No new
  size is required, and none would help — see the sweep in §4: the fold survives every viewport I
  could model, from a ~1025 px-wide equivalent to a ~1925 px-wide one.
- **Larger-margin alternative if a bigger signal is wanted: `Cards.PlacementResize`** (modelled
  content ≈ 2218, **3.19 × view** — the longest page on the screen, in every parameterisation I
  swept). This one is modelled, not measured; `Cards.StackUpgrade` is the safer target because it
  needs no model at all.

⛔ **I am barred from taking a remedy (`SC-§101`) and I have not taken one.** I name no fix, no
shape, and no mechanism. `TASK-1488` rules.

---

## 1. (b) FIRST — **DOES `DetailScrollBox` ACTUALLY CLIP? YES. THE DANGEROUS WORLD IS CLOSED.**

**The verifier's hypothesis is REFUTED.** The box clips, by engine default, on a chain that the
project never touches. Every link read at source:

| # | Link | `file:line` |
|---|---|---|
| 1 | `UScrollBox::UScrollBox()` sets `SetClipping(EWidgetClipping::ClipToBounds)` — **in the constructor, unconditionally** | `UMG/Private/Components/ScrollBox.cpp:36` |
| 2 | `UWidget::SynchronizeProperties()` pushes that property onto the live Slate widget every rebuild | `UMG/Private/Components/Widget.cpp:1444` (body), `:1491` (`WITH_EDITOR`) / `:1493` (else) |
| 3 | `SWidget::SetClipping()` stores it and calls `OnClippingChanged()` | `UMG/Private/Components/Widget.cpp:476-484` (the UMG-side setter); `SlateCore` `SWidget::SetClipping` |
| 4 | `SScrollBox::OnClippingChanged()` forwards it to the panel: `ScrollPanel->SetClipping(Clipping)` | `Slate/Private/Widgets/Layout/SScrollBox.cpp:327-330` |
| 5 | `SScrollBox` sets `bClippingProxy = true`, so the box itself is *skipped* by the clip-rule switch and the clip is applied **by `SScrollPanel`** | `Slate/Private/Widgets/Layout/SScrollBox.cpp:242`; `SlateCore/Private/Widgets/SWidget.cpp:1426` (`if (!bClippingProxy)`) |

**The `WITH_EDITOR` branch does not change the answer, and I checked it specifically because PIE
compiles `WITH_EDITOR`.** At `Widget.cpp:1463`,
`SafeContentWidget = MyGCWidget.IsValid() ? MyGCWidget.Pin() : MyWidget.Pin()`. `MyGCWidget` is the
`SObjectWidget` and exists only for a `UUserWidget`; `DetailScrollBox` is a plain `UScrollBox`, so
`MyGCWidget` is invalid and `SafeContentWidget` **is** the `SScrollBox`. Editor and non-editor
branches land on the same widget here.

**Project side — nothing overrides it.** `SetClipping` / `EWidgetClipping` / `bClipToBounds` /
`ClipToBounds` appear **0 times** anywhere under `Source/`.

> 🔬 **Control on that zero.** A bare zero from a `grep` is exactly the silent-zero shape this row
> was told to distrust, so the same alternation was re-run with `SetOrientation` added as a
> **positive control**: it returned `SiegeControlsHelpWidget.cpp:2371` and `:3215`. The pattern can
> return hits on that path ⇒ **the zero for the clipping terms is a real zero, not a dead
> instrument.**

⇒ **Route C is refuted.** The question of whether the non-clipping is `TASK-1484`'s doing or
pre-existing **does not arise** — there is no non-clipping. (For completeness: `TASK-1484` could not
have caused it either way; it added a sibling button and touched none of the three authored
properties on this box.)

---

## 2. THE SHIPPED LINE, RE-DERIVED

`[ControlsHelp] Detail scroll on 'Cards.StackUpgrade': offset 0.0 -> 591.0 (end 1166.4, step 591.0, view 695.2) - MOVED.`

- `view 695.2` = `DetailScrollBox->GetCachedGeometry().GetLocalSize().Y`
  (`SiegeControlsHelpWidget.cpp:2764`) — the **`SScrollBox`'s own** local height.
- `step 591.0` = `695.2 × DetailScrollPageFraction` ⇒ the fraction is **0.85**, exactly as the row
  specified.
- `end 1166.4` = `GetScrollOffsetOfEnd()` (`SiegeControlsHelpWidget.cpp:2779`).

---

## 3. (3) `end`-PROVENANCE — **THE SUSPICION IS CONFIRMED AS TO PROVENANCE AND THE FEARED INFERENCE IS REFUTED**

**Confirmed: `end` IS computed from an unclipped desired size.** Chain, read at source:

```cpp
// Slate/Private/Widgets/Layout/SScrollBox.cpp:572-577
float SScrollBox::GetScrollOffsetOfEnd() const
{
    const FGeometry ScrollPanelGeometry = FindChildGeometry(CachedGeometry, ScrollPanel.ToSharedRef());
    const float ContentSize = GetScrollComponentFromVector(GetScrollPanelContentSize());
    return FMath::Max(ContentSize - GetScrollComponentFromVector(ScrollPanelGeometry.Size), 0.0f);
}

// Slate/Private/Widgets/Layout/SScrollBox.cpp:501-511
UE::Slate::FDeprecateVector2DResult SScrollBox::GetScrollPanelContentSize() const
{
    Size = ScrollPanel->GetArrangedSize();
    if (!bAllowContentToShrink || Size.IsNearlyZero()) { Size = ScrollPanel->GetDesiredSize(); }
    return Size;
}
```

`bAllowContentToShrink` defaults to **`false`** (`Slate/Public/Widgets/Layout/SScrollBox.h:634`) and
**UMG never exposes or sets it** — `AllowContentToShrink` / `ContentToShrink` return 0 hits in
`UMG/.../ScrollBox.cpp` and `ScrollBox.h`. ⇒ the `!bAllowContentToShrink` limb is **always taken**,
so `ContentSize` is **always `ScrollPanel->GetDesiredSize()`** — the unclipped desired size.

**But the feared conclusion does not follow, and this is the whole point of settling (b) first.**
The brief's worry was: *"if it is computed from an UNCLIPPED desired size, then `end 1166.4` and
`view 695.2` are both consistent with a box that displays everything."* That reading requires the
box **not** to clip. It clips (§1). Desired size is not a rival to the painted size — it is the
**correct** measure of content extent, and clipping is a paint-time rect intersection that never
shrinks it. So:

> **content desired 1861.6 − arranged 695.2 = 1166.4 px of prose that `ClipToBounds` is cutting off.**
> `end` is load-bearing here **because** the box clips, not in spite of it.

**One caveat I did not hand-wave.** `end` subtracts `ScrollPanelGeometry.Size` (the *panel*) while
the log's `view` is the *box*'s local size — two different widgets. For `Orient_Vertical` the panel
and the scrollbar sit side by side in the horizontal layout, so the panel takes the box's **full
height** and the two agree on the Y axis. The `1861.6` figure depends on that identity; it would not
hold for `Orient_Horizontal`. This box is `Orient_Vertical` (`SiegeControlsHelpWidget.cpp:2371`).

---

## 4. (4) THE OVERFLOW CENSUS

### 4a. What drives the height — the finding that makes the census counter-intuitive

**A related block renders the related row's ENTIRE detail body, not a summary.**
`Entry.Body = ResolveDetailTokens(ComposeDetailForDisplay(*RelatedRow), ComposeChipForId)`
(`SiegeControlsHelpWidget.cpp:1905`), rendered at font 18 with `SetAutoWrapText(true)`
(`:2977-2981`). So a page with 3 related rows carries **four full detail bodies**. This is by design
and documented (`HELP-§2`, "one definition, two renderings") — but it means **page height is driven
by the related set at least as much as by the page's own prose.**

### 4b. Method, and its calibration

All 27 rows' `Row.Detail` strings were extracted from
`Source/GitClaudeUnrealTest/Siegebound/SiegeControlsHelpWidget.cpp` by concatenating each row's
`TEXT("...")` literals (**wrap-tolerant by construction** — the literals are concatenated before any
matching, so a sentence split across two source lines is never missed). Heights were then modelled
with a greedy word-wrap at Roboto constants (line height 1.20 em, mean advance 0.50 em) and the
widget's authored paddings, and the **scroll-panel width was fitted to reproduce the one real
runtime number**, `Cards.StackUpgrade = 1861.6`.

> 🔬 **Control on the extractor — and it fired.** A positive control asserted that a sentence known
> to span two source lines (`"The size you dial in with the wheel … existing width and length."`)
> is present in the extracted text. **It returned `False`**: the extractor was cutting the segment
> at `)));` and thereby dropping the closing `)` of the **final literal of every row**, silently
> under-measuring all 27 pages. Fixed (cut at `)));`+1); the control then returned `True` while the
> negative control (a sentence that is genuinely absent) stayed `False`. **Both controls
> discriminate. Every number below is post-fix.**
>
> 🔬 **Control on the sweep.** The wrap function originally read its constant via a `k=K` default
> argument, which binds at definition time — a parameter sweep over it would have been a silent
> no-op producing an identical table. Caught by a control that asserts two different constant sets
> give two different answers (`1726.0` vs `1957.2` → DISCRIMINATES). Fixed to read the global.

**Calibration result:** best-fit scroll-panel width **1244 px**, residual **0.40 px** against 1861.6.
⚠️ **I am not claiming that residual as validation** — one free parameter fitted to one observation
will always fit. What *is* evidence is that the fitted width is **physically plausible**: 1244 px of
panel implies a ~1369 px-wide viewport at DPI 1.0 (`+112` column padding `+13` scrollbar), i.e. a
perfectly ordinary PIE window. A model that needed an absurd width would have been refuted here.

### 4c. The census (calibrated geometry: panel 1244, view 695.2)

| Page | content | end | × view | |
|---|---:|---:|---:|---|
| `Cards.PlacementResize` | 2218.4 | 1523.2 | 3.19 | **FOLD** (longest) |
| `Cards.StackUpgrade` | 1862.0 | 1166.8 | 2.68 | **FOLD** — *measured 1861.6 / 1166.4* |
| `Orders.Follow` | 1786.4 | 1091.2 | 2.57 | FOLD |
| `Interface.WarMap` | 1694.0 | 998.8 | 2.44 | FOLD |
| `Interface.MapMarks` | 1686.8 | 991.6 | 2.43 | FOLD |
| `Orders.Hold` | 1581.2 | 886.0 | 2.27 | FOLD |
| `Orders.Ambush` | 1558.4 | 863.2 | 2.24 | FOLD |
| `Interface.WarMapMarker` | 1338.8 | 643.6 | 1.93 | FOLD |
| `PickMode.Cancel` | 1209.2 | 514.0 | 1.74 | FOLD |
| `PickMode.Resize` | 1204.4 | 509.2 | 1.73 | FOLD |
| `PickMode.Confirm` | 1184.0 | 488.8 | 1.70 | FOLD |
| `Interface.AssistantConsole` | 1073.6 | 378.4 | 1.54 | FOLD |
| `Interface.AssistantAccept` | 975.2 | 280.0 | 1.40 | FOLD |
| `Cards.Play` | 779.6 | 84.4 | 1.12 | FOLD (marginal) |
| `Interface.WarMapReveal` | 776.0 | 80.8 | 1.12 | FOLD (marginal) |
| `Cards.Discard` | 584.0 | 0 | 0.84 | fits |
| `Interface.ControlsHelp` | 315.6 | 0 | 0.45 | fits |
| `Orders.Attack` | 292.8 | 0 | 0.42 | fits |
| `Hero.Sprint` | 224.4 | 0 | 0.32 | fits |
| `Hero.Move` / `Cards.CursorHold` / `Cards.Cancel` | 201.6 | 0 | 0.29 | fits |
| `Orders.Defend` / `Hero.Look` / `Hero.Attack` | 178.8 | 0 | 0.26 | fits |
| `Hero.Rally` | 156.0 | 0 | 0.22 | fits |
| `Hero.Jump` | 133.2 | 0 | 0.19 | fits |

**15 of 27 detail pages overflow.**

### 4d. Robustness — does the answer survive my modelling choices?

A 3 × 3 grid over the two constants (mean advance 0.46/0.50/0.54, line height 1.15/1.20/1.25), with
the width **re-fitted each time**, and an independent width sweep:

- **Fold count: 15 in all nine parameter cells**; 13–16 across a width sweep from panel 900 → 1800.
- **`Cards.PlacementResize` is the longest page in every single cell and at every width.**
- **`Interface.WarMapReveal` is the marginal page in every cell** — the boundary does not wander.
- Chip-width assumption swept 90 → 260 px: moves the fitted panel only 1184 → 1280 and changes no
  verdict.

⇒ The census's *ranking* and its *headline* are insensitive to everything I had to guess.

### 4e. At what size would the longest page stop folding?

`Cards.PlacementResize` needs a **view of ~2218 px** to swallow its content. The detail column's
chrome above and below the scroll box is roughly 330 px (header 40+~48+8; summary ~53+16; box slot
12; `BackButton` ~49+34; `DetailScrollButton` ~40+34), so that is a **viewport around 2550 px tall**
in widget-local units. **No realistic window reaches it** — and widening the window makes it *worse*
in the sense that matters, because the fold persists to the top of my sweep.

### 4f. The honest bound on all of this

The **only** page whose overflow is *measured* is `Cards.StackUpgrade`. The other 26 rows are
**modelled**, from real authored strings and real authored paddings, but with an assumed font metric
and a fitted width. **What a runtime read would add:** one `GetScrollOffsetOfEnd()` per page (27
reads, no input injection needed) would replace the whole of §4c with measurement and would also pin
the true panel width, collapsing §4b's fitted parameter. That is cheap and it is the natural
companion to the re-verification this row routes to.

---

## 5. WHAT I FOUND WHILE SETTLING (b) THAT NARROWS THE REMAINING PUZZLE

This is **evidence, not a remedy, and not a prescribed shape.** Since route A sends this back for
re-verification, whoever runs it should know that **the widget-side path from the write to the
painted offset is intact** — I read it end to end rather than assuming it:

- `SScrollBox::SetScrollOffset` assigns `DesiredScrollOffset` **raw** (`SScrollBox.cpp:596-602`) —
  confirming `TASK-1484`'s finding that the row's own clamp is load-bearing.
- `SScrollBox::Tick` (`SScrollBox.cpp:886-950`) then computes
  `PhysicalOffset = Clamp(DesiredScrollOffset / ContentSize, 0, 1 − ViewFraction) × ContentSize`.
- With the measured numbers: `ViewFraction = 695.2 / 1861.6 = 0.3734`, so the clamp ceiling is
  `0.6266 × 1861.6 = 1166.4` — **exactly `end`**, a nice internal consistency check. The written
  `591.0` is `0.3175`, **well under the ceiling**, so it is **not clamped** and
  `ScrollPanel->PhysicalOffset` becomes **591.0** on the next tick.
- The one path that would zero it — `if (!ScrollBar->IsNeeded()) { ScrollPanel->PhysicalOffset = 0; }`
  (`SScrollBox.cpp:941-945`) — **does not fire**, because `ViewFraction 0.3734 < 1`.

⇒ **On the source, the pixels should have moved by 591 px.** I have **not** proven that they did —
I cannot, from a read-only lane with no PIE — and I explicitly do **not** claim it. But combined
with §1, it means the two worlds this row was sent to separate are **both** refuted: there *is* a
fold, and the box *does* clip. **What remains unexplained is the observation, not the widget.**

---

## 6. ONE MEASURED NUMBER FOR `TASK-1488`'s BENEFIT (route B is refuted, so it is moot — recorded anyway)

The brief costed the new button at *"~one button-height + 34 px of prose on every detail page."*
Measured from source: `DetailScrollLabelText` font 20 (`:2651`) ⇒ ~24 px line, `ButtonSlot` padding
`(28, 8, 28, 8)` (`:2655`) ⇒ **~40 px button**, plus `VerticalBoxSlot` padding `(56, 0, 56, 34)`
(`:2669`) ⇒ **~74 px total** taken off the scroll box's `Fill` allocation per detail page. The
brief's estimate was right in shape; the number is ~74, not ~34. **Since a fold exists on 15 of 27
pages, the button is paying for a fold that does exist** — but that is `TASK-1488`'s call, not mine.

---

## Not examined / limitations

1. **No runtime read of any kind.** No PIE, no editor, no MCP. `TASK-1489` was running PIE on the
   console screen throughout; I touched nothing. Every statement here is from project source,
   engine source, or arithmetic on `TASK-1436`'s already-shipped log line.
2. **I did not prove that pixels moved.** That is the original open question and it remains open.
   §5 shows the source path is intact; it is not a pixel observation and must not be read as one.
3. **26 of 27 pages are modelled, not measured** (§4f). The font metric (Roboto 1.20 em line height,
   0.50 em mean advance) is assumed, not read from `FSlateFontMeasure`. §4d bounds the damage.
4. **The actual viewport of `TASK-1436`'s run is unknown to me.** I inferred ~1369 × (unknown) px
   local from the fit. `Config/*.ini` contains **no** `UIScaleRule` / `UIScaleCurve` / `NewWindow*`
   entries (grep, 0 hits), so engine defaults apply and I could not pin the DPI scale from source.
   The `× view` ratios are unaffected; the absolute px are the soft numbers.
5. **`Row.OneLine` extraction failed** in my parser (0 of 27) and I did **not** repair it: the
   summary text is a sibling of the scroll box in `DetailColumn`, **outside** the scrolled content,
   so it cannot affect the content census. It does affect the chrome estimate in §4e, which is
   already flagged as approximate.
6. **Chip widths are assumed (~150 px)**, not derived — the real chip strings come from live key
   bindings at runtime. Swept 90–260 px in §4d with no verdict change.
7. **Whether `BackButton`'s padding changed** under `TASK-1484` I could not check: that needs git
   history and git is forbidden to this row. I report only the current authored value, `(56, 0, 56, 34)`.
8. **`{...}` tokens** (e.g. `{Cards.Cancel}`) are counted at their literal source length; at runtime
   `ResolveDetailTokens` swaps them for key chips, shifting a few characters per page. Far below the
   model's resolution.
9. **Scratch scripts** live in the session scratchpad (`census3.py`, `model.py`, `sweep.py`) — not in
   the repo, nothing staged, nothing committed.
