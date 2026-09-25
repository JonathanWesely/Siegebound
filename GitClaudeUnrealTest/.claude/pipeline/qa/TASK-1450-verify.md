Verdict: MEASURED
# Verification — TASK-1450 — [FOCUS-RING-FULLRES-DISCRIMINATOR]

⭐⭐ **BRANCH (A). `H-DRAWN` CONFIRMED. THE RING WAS ALWAYS BEING DRAWN IN THE INJECTION LANE. THE
DIVERGENCE IS INSTRUMENT-SIDE.**

**The single pixel that settles it:** on the focused button's corner the capture reads **RGB(90, 157, 224)**
where the *same* pixel on an unfocused button reads **RGB(192, 192, 192)**. The brush is `#0070E0` =
(0, 112, 224) — **the blue channel matches to the LSB.** The two things in that neighbourhood are the
button fill (187,187,187) and the ground (241,239,238); **both are neutral greys, and no blend of two
neutral greys is ever (90,157,224).** It is on exactly the focused row, in **three** frames, on **three
different buttons**, at **all four corners**, and on **none** of the eighteen unfocused button-bands.

⚠️ **THIS IS `MEASURED`, NOT A PASS** (`VER-§1` cl. 5a): it proves the numbers were taken under control, not
that anything works. ⛔ **No remedy is chosen here** (`SC-§101`) and no clause is amended.

⚠️ **PROVENANCE OF §A: line 1 and this header block are the ONLY bytes of this file edited after the first
capture.** §A was written to disk in full, as it stands, **before** PIE was started — its original line 1 read
`Verdict: PENDING — PRE-REGISTRATION WRITTEN, CAPTURE NOT YET RUN` and was replaced by a single anchored
`Edit` so that §A is provably pre-registered rather than reconstructed. ⛔ **Nothing in §A was revised after
the fact, including where §A turned out to be wrong** (see §D.1 and Limitation 2).

**Naming (spec (0)):** `H-DRAWN` = the ring IS drawn, sub-pixel, and the instrument cannot see it ⇒ the
divergence is INSTRUMENT-SIDE. `H-NOTDRAWN` = the ring does not reach the composited frame in the injection
lane. ⛔ `TASK-1446`'s own `H2` is dead at source and is not what this row is about.

⛔ **(0a) NOT RE-DERIVED.** That the ring is *asked for* in our lane is settled by `TASK-1446` §2
(cause `Navigation` explicit at `SiegeMenuInputSubsystem.cpp:397`/`:403` ⇒ `ShowFocus` true at
`SlateApplication.cpp:3094` ⇒ `UGameViewportClient::QueryShowFocus` true under the in-force
`RenderFocusRule = NavigationOnly`, which the project overrides nowhere). **Zero minutes spent on it.**

---

# §A — PRE-REGISTRATION (spec (2)). WRITTEN BEFORE THE CAPTURE.

## A.1 The compositing model, written out

A single opaque-coloured stroke composited over an opaque base:

```
out_c = base_c + cov · (ring_c − base_c)          c ∈ {R, G, B}
Δ_c   = cov · (ring_c − base_c)
```

`cov` ∈ [0,1] is the fraction of the pixel covered by the stroke **times the stroke's own alpha**. The brush
is `FSlateRoundedBoxBrush(Transparent, InputFocusRadius, FStyleColors::Primary, InputFocusThickness)`
(`StarshipCoreStyle.cpp:217`) and `FStyleColors::Primary` = **`#0070E0FF`** (`StyleColors.cpp:38,101`) —
**alpha 1.0**, so `cov` is pure geometric coverage.

```
ring = #0070E0 = (R 0, G 112, B 224)
```

**Base — the number the whole dispute turns on.** The orchestrator's input measures the Settings button at
**RGB(201.4, 201.2, 201.1)** — a *light* grey. (I re-measure the base from my own frame in §D and report it;
it is not assumed.) At that base:

| channel | sensitivity `dΔ/dcov` |
|---|---|
| **Red** | `0 − 201.4` = **−201.4** |
| Green | `112 − 201.2` = **−89.2** |
| **Blue** | `224 − 201.1` = **+22.9** |
| **(B−R)** | **+224.3** |

| cov | ΔR | ΔG | ΔB | **Δ(B−R)** |
|---|---|---|---|---|
| **0.57** | **−114.8** | −50.8 | **+13.1** | **+127.8** |
| 0.10 | −20.1 | −8.9 | +2.3 | +22.4 |
| 0.05 | −10.1 | −4.5 | +1.1 | +11.2 |

⭐ **THE DISCRIMINATING CHANNEL IS RED, NOT BLUE.** At this base red is **8.8×** more sensitive than blue,
and **(B−R)** is **9.8×** more sensitive than blue alone. **A blue-only metric under-reads by ~10×.**

## A.2 Reconciling the two forbidden models — deliverable #1

Both inherited numbers are refused as premises and **both are shown to be computed in the wrong channel.**

- **Model (b) — *"a ~0.57 px stroke is < 1 LSB"* (the orchestrator's inference): ARITHMETICALLY FALSE, and
  not by a little.** At cov = 0.57 the red channel moves **−115 LSB**. Even at the smallest coverage on the
  table, cov = 0.05, red moves **−10 LSB** and (B−R) moves **+11 LSB** — an order of magnitude above one
  quantisation step. **There is no coverage ≥ 0.005 at which this stroke is sub-LSB on a light base.**
  ⛔ Not adopted.
- **Model (a) — *"0.57 × (224 − base) is TENS of LSB"* (the manager's): the FORMULA IS RIGHT, the BASE IS
  WRONG for this frame.** `0.57 × (224 − base_B)` is tens of LSB *at a dark base*. Our base is **light
  (≈201)**, so it yields **+13**, not "tens". Model (a)'s *magnitude* claim survives — **but only in RED
  (−115) and in (B−R) (+128), never in blue.**
- ⇒ **THE RECONCILIATION: the two models do not actually disagree about physics. They disagree because both
  were evaluated in the blue channel against a light base, where blue is the one channel that barely moves.**
  Model (b) then read blue's small number as "no signal"; model (a) read the coverage model's big number as
  "blue's number". **Correct the channel and both collapse into one model with no two-order-of-magnitude
  gap: at cov 0.57 the signal is −115 LSB of red / +128 LSB of blue-excess.**
- ⇒ **CONSEQUENCE FOR `TASK-1399`'s `maxBlueDelta = +1.0`:** under `H-DRAWN` the *blue* channel is predicted
  to move only **+13** while red moves **−115**. **That measurement was taken in the channel carrying ~1/10
  of the signal, so its "null" was never worth what it appeared to be.** ⛔ I nevertheless rest **nothing**
  on it: the retrospective pair is closed on independent grounds (uncontrolled, 39,052 px changed frame-wide,
  clouds animate, 2.06 s apart, and an unstable per-frame detector). **Cited, not re-run.**

## A.3 The coverage this run will actually have — and a result that changes the method

`TASK-1446`'s `0.57 px` is re-derived here from the curve itself rather than inherited.

Engine-default `UIScaleCurve` keys (`BaseEngine.ini:1474-1475`, project overrides none): **(720, 0.666),
(1080, 1.0), (8640, 8.0)**, `UIScaleRule = ShortestSide`.

- Segment 1080→8640: slope `7/7560 = 9.2593e-4`; intercept at 1080 = `1.0 − 1080×9.2593e-4` = **0.0000**.
- Segment 720→1080: slope `0.334/360 = 9.2778e-4`; intercept = **−0.002**.

⇒ 🚨 **THE CURVE IS A PROPORTIONAL LINE THROUGH THE ORIGIN: `DPIscale ≈ 9.26e-4 × shortestSide`, to within
0.3% across the whole 720–8640 range.** Therefore, for a 16:9 window of width `W` captured to a fixed
long-edge cap `L_cap`:

```
ring thickness in CAPTURED px = 1.0 × (9.26e-4 · 0.5625·W) × (L_cap / W)
                              = 5.208e-4 × L_cap            ← W CANCELS
```

⇒ 🚨 **UNDER A FIXED CAPTURE CAP, RING THICKNESS IN CAPTURED PIXELS IS INDEPENDENT OF WINDOW SIZE.** At the
observed `L_cap = 1086` it is **0.566 px** whether the window is 1280×725, 1920×1080 or 2560×1440.
⇒ ⛔ **SPEC (3)(iv) — "raise the render surface" — IS ARITHMETICALLY WORTHLESS ON ITS OWN.** It amplifies
the ring and the capture shrinks it back by exactly the same factor. **The only lever that changes coverage
is defeating the downscale (3)(i).** Native capture gives `thickness = 5.208e-4 × W`: **1280 → 0.667 px,
1920 → 1.000 px, 2560 → 1.333 px.**

⭐ Note in passing: this reproduces `TASK-1446`'s inferred **0.57** exactly, by an independent route. It is
still subject to that row's limitation #1 link (a) — *that the DPI scale multiplies the brush outline width*
— which remains **unverified**, and which §D's measured `DPIScale` echo can only partly discharge.

## A.4 ⭐ THE PRE-REGISTERED NUMBER, FIXED NOW

**Method chosen (spec (3)):** **(i) native-resolution capture** — mandatory, probed for explicitly before
the measurement — **plus (ii) same-frame focused-vs-unfocused across the seven buttons**, plus **(iv) window
raised to 1920×1080 so `DPIscale = 1.000`**, which by A.3 is worth exactly nothing unless (i) succeeds and is
therefore declared as a *conditional* amplifier, not a claimed one. **(iii) is ruled out at boarding and was
not touched: engine source is never edited by anyone on this pipeline.**

**Statistic (fixed now, applied identically to all seven buttons in every frame):**
`maxBlueExcess(band) = max over band pixels of (B − R)`, plus `minRed(band)` and
`count(B−R ≥ 40)`.

**PREDICTED under `H-DRAWN`, at native 1920×1080 (stroke = 1.000 device px):**

| case | cov on the hottest pixel | predicted Δ(B−R) | predicted ΔR |
|---|---|---|---|
| stroke lands on the pixel grid | 1.00 | **+224 LSB** | **−201 LSB** |
| **stroke straddles two rows equally (worst case)** | 0.50 | **+112 LSB** | **−101 LSB** |

⇒ ⭐ **PRE-REGISTERED FLOOR: `maxBlueExcess` on the focused button's edge band ≥ +112 LSB above the grey
base, with `minRed` ≤ −100 LSB, on EXACTLY ONE of the seven buttons.**
⇒ ⭐ **DISCRIMINATION THRESHOLD, FIXED BEFORE THE CAPTURE: the focused band's `maxBlueExcess` must exceed
the maximum over the six unfocused bands by ≥ +40 LSB.** (2.8× below the floor in the native lane; 1.6×
below it in the fallback capped lane, where cov = 0.566 ⇒ floor +63.) ⛔ **This number is not revised after
the fact for any reason.**

**ABORT CHECK REQUIRED BY (2):** the predicted delta is **+112 LSB** (native) / **+63 LSB** (capped) —
**both ≫ 1 LSB.** ⇒ **The method passes its own sufficiency test and the capture may proceed.** Had the
probe in §C returned a configuration predicting < 1 LSB, the method would have been changed before capturing.

## A.5 Regions — derived ONCE (spec (5))

Per `RECORD 2`'s law — *an instrument that re-detects its own regions per frame reads its own wobble as
signal* — every rectangle is derived **once**, from a **single structured `ui_snapshot`**, and the
**identical** rectangles are applied to every frame and every button. Two bands per button:
**`BAND_EDGE`** = the annulus 3 px inside to 1 px outside the rect boundary (where the ring lands), and
**`BAND_CORE`** = ≥ 5 px inside (pure fill; establishes the base and must stay clean in every frame).
Rectangles are reported as numbers in §D.

---

# §B — EDITOR / AURA STATE

**Aura connected** (`get_headless_status` = `editor_connected`).
**Editor identified BY COMMAND LINE (`SC-§118`), measured by me, not borrowed:**
`unreal.SystemLibrary.get_command_line()` = `''` — **EMPTY ⇒ a GUI editor**, not `-game`, not a commandlet.
**PID 1792.** `Saved/Logs/GitClaudeUnrealTest.log` line 1 = `Log file open, 09/24/26 13:38:43`. 🧑 **No `-game`
instance was seen, read, touched or closed.** Re-confirmed identical (PID 1792, empty command line) after the run.
**Map:** `/Game/Maps/L_MainMenu` — already open; ⛔ **I called no `load_level` and changed no map.**
**PIE:** `is_pie_active` = **false** before I began ⇒ **no session of his existed; I stopped only the session
I started myself.** Standalone, **one** instance, requested 1920×1080, actual viewport **1920×1085**.
**Serialization (`VER-§2` cl. 1):** re-measured myself — `^- status: \`(integrating|verifying|compiling|importing)\``
over `TASKBOARD.md` ⇒ **0 matches.**
**`VER-§3` go:** GIVEN by 🧑 in the dispatch (*"Yes, go ahead and launch it."*). No PIE call preceded it.
**Attempts used: 1 of 3.** **Wall time ≈ 45 min.** **Blueprint loads: none** — `BROKEN_BP_LOADED = False`
measured **before and after**; ⛔ the `1439`/`1443`/`1445` wedge mechanism was not imported into this lane.
**Dirty packages — MEASURED, not asserted:** before `DIRTY_CONTENT=[]` / `DIRTY_MAPS=[]`; after
`DIRTY_CONTENT=[]` / `DIRTY_MAPS=[]`. ⇒ **Nothing reached disk. No project setting was changed.**
**Left behind:** editor **up**, PID 1792, on `/Game/Maps/L_MainMenu` (**unchanged**), **PIE stopped**, nothing dirty.

---

# §C — METHOD AS EXECUTED (spec (3)), WITH THE NUMBERS

| (3) limb | ruling | what actually happened |
|---|---|---|
| **(i) native capture, no downscale** | REQUIRED | ⭐ **ACHIEVED. `capture_pie_frame(max_dim=4096)` returns 1920×1085 — the full viewport. SCALE FACTOR = 1.000, no resample.** |
| **(ii) same-frame focused-vs-unfocused** | REQUIRED | Used, and **three times over** — three frames, three *different* focused buttons, six in-frame controls each. |
| **(iii) raise `InputFocusThickness`** | RULED OUT | ⛔ **Not touched. Engine source was neither read nor written by this row.** |
| **(iv) raise the render surface** | permitted amplifier | Used: PIE window 1920×1080 ⇒ **measured `dpi_scale = 1.0046296`**. ⛔ **No on-disk setting changed** (dirty `[]` both sides). |

⭐ **A REUSABLE INSTRUMENT FACT, AND IT IS THE ONE THAT UNBLOCKED THIS ROW.** `TASK-1399` recorded
*"`max_dim: 0` does not exceed 1086"* and concluded the capture could not be told not to downscale. **That is
true of `0` and false of the tool.** `1086` is a **default** long-edge cap; passing an **explicit large**
`max_dim` (4096) defeats it and returns the viewport 1:1. ⇒ **the pixel lane was never as blind as it looked.**

⭐ **AND THE MEASURED `dpi_scale` DISCHARGES HALF OF `TASK-1446` LIMITATION #1.** That row flagged the
`UIScaleCurve`'s interpolation mode as an unverified link. §A.3 predicted, from a pure proportional model,
`9.26e-4 × 1085 = 1.00465`. The engine reports **`1.0046296`**. ⇒ **the curve is linear in this band, to five
significant figures. Link (b) is closed.** (Link (a) — that the scale multiplies the brush outline width —
is **not** closed; see Limitation 2.)

---

# §D — THE MEASUREMENT

## D.1 🚨 The regions — and the instrument defect found while fixing them (spec (5))

⛔ **`ui_snapshot`'s `geometry.abs_*` ARE NOT CAPTURED-IMAGE PIXELS, AND USING THEM AS SUCH PRODUCES SILENT
GARBAGE.** Measured ratio abs→image pixels: **2.532 in x, 2.543 in y.**

My first pass mapped the seven `abs` rectangles straight onto the frame. It ran, it returned a full table of
plausible-looking numbers, and **every one of them was meaningless** — the bands had landed on empty sky and
flat ground, ~2.5× too large. ⭐ **It was caught by a validity check, not by luck: `BAND_CORE` is supposed to
be flat button fill, and it came back as (106,140,180) on one row and (240,239,237) on another.** A detector
whose core sample is not the fill is not measuring the button. ⛔ **That first table is discarded in full and
no number from it appears in this report.** (Reported, not hidden: it is the same species of failure as
`RECORD 2`, caught one step earlier.)

⇒ **Rectangles derived ONCE, from the pixels of ONE frame (FRAME B), and applied BYTE-IDENTICALLY to all four
images** (`RECORD 2`'s law — the detector never re-derives itself per frame):

```
x = 895 .. 1024          (width 130 px, identical for all seven)
y0 ∈ { 441, 471, 502, 532, 563, 593, 623 }     height 21 px     spacing 30.4 px
BAND_EDGE = pixels within ±2 px of the rect boundary
BAND_CORE = pixels ≥ 4 px inside
```

⚠️ **Declared deviation from §A.5:** §A.5 pre-registered the bands as "3 px inside to 1 px outside" and
"≥5 px inside", sized for the (wrong) `abs` rectangles. The real buttons are 21 px tall, not 53, so the bands
were re-sized to **±2 px** and **≥4 px**. ⛔ **The band GEOMETRY changed; the STATISTIC and the THRESHOLD did
not.** No threshold was touched after any number was seen.

⭐ **Corroboration that the rectangles are right, before any conclusion rests on them:** a grey-fill detector
run independently over FRAME B returned the seven bands at exactly these coordinates — and returned the
**focused** button as `x=896..1023 (w=128)` while all six unfocused returned `x=895..1024 (w=130)`.
**A 1-px non-grey rim on exactly the focused row, found by a detector that knew nothing about blue.**

## D.2 ⭐ THE RESULT — three frames, three different focused buttons

| frame | evidence file | PIE `t` | focused (structured read) | **focused max(B−R)** | **px ≥100** | max over the other six | **excess** | branch test (≥ +40) |
|---|---|---|---|---|---|---|---|---|
| **A** | `…-button0-focused-native1920.png` | 192.823 | `Button_0` (over **sky**) | **133** | **4** | 75 (`B3`, sky) · **0** (ground rows) | **+58** / +133 | ✅ |
| **B** | `…-button4-focused-injectlane-native1920.png` | 207.952 | `Button_4` (over **ground**) | **134** | **4** | 75 (`B3`, sky) · **0** (ground rows) | **+59** / +134 | ✅ |
| **C** | `…-button6-focused-keypresslane-native1920.png` | 209.568 | `Button_6` (over **ground**) | **128** | **4** | 75 (`B3`, sky) · **0** (ground rows) | **+53** / +128 | ✅ |

**The ≥100 pixels are, in every frame, exactly the FOUR CORNERS of the focused button** — e.g. FRAME C:
`(895,623) (1024,623) (895,643) (1024,643)`, each **RGB(97,161,225)**. That is precisely what
`FSlateRoundedBoxBrush(..., InputFocusRadius = 4.f, InputFocusThickness = 1.0f)` predicts: the corner arcs
concentrate the outline's coverage. ⛔ **Zero ≥100 pixels on all eighteen unfocused button-bands (3 frames × 6).**

**The straight-edge (rim) signal, identical in all three frames regardless of backdrop:**

```
focused   left rim  ->  RGB(161, 168, 177)      B−R = +16      ΔR = −31 vs unfocused
unfocused left rim  ->  RGB(192, 192, 192)      B−R =   0
```

⭐ **The signal MOVES WITH THE FOCUS, which is what kills the "fixed artifact" reading (outcome (D)):**
`Button_4`'s left rim reads **(161,168,177)** in FRAME B where it is focused, and **(192,192,192)** in FRAMES
A and C where it is not — the same pixel, the same rectangle, the same static ground behind it.

**Against the pre-registration:** floor was **≥ +112 LSB**. Observed **+128 … +134** against a grey base of
B−R ≈ 0, and **+112** for FRAME A against its sky base of B−R ≈ +21. ⇒ **THE PRE-REGISTERED FLOOR IS MET IN
ALL THREE FRAMES.** Threshold was **≥ +40 excess**; observed **+53 … +59** even against the sky-contaminated
worst case, **+128 … +134** against same-background controls.

⚠️ **BUT IT IS MET FOR A DIFFERENT REASON THAN §A ASSUMED, AND THAT MATTERS MORE THAN THE PASS.** §A predicted
+112 along the **straight edge**. The straight edge actually carries only **+16**. The floor is met by the
**corners**, which §A did not model at all. ⇒ ⭐ **A MEAN-OVER-BAND DETECTOR WOULD HAVE MISSED THIS ENTIRELY:**
focused `meanBR = +0.74` vs unfocused `−1.48` / `−1.39` — a **+2.2** difference, indistinguishable from noise.
***The signal lives in the MAX, not the mean.*** A row that had averaged its band would have produced exactly
the undiscriminating zero this row exists to avoid. **Offered to the manager as a method-lesson candidate
beside `RECORD 2`'s; ⛔ this row writes no `CONVENTIONS.md`.**

## D.3 ⭐⭐ THE LOOP CLOSED — why `TASK-1399` could not see it, demonstrated rather than hypothesised

A single-variable swap, which is exactly the control `TASK-1446` H4 asked for: **same PIE session, same
window, same focused button, same instant — only `max_dim` changed from 4096 to 1086** (the `TASK-1399` lane).

| statistic on the focused `Button_6` | native (1920×1085) | downscaled (1086×614) |
|---|---|---|
| max(B−R) on the edge band | **128** | **17** |
| pixels ≥ 100 (the corner signature) | **4** | **0** |
| rim pixel | (161,168,177) | (192,194,197) — vs unfocused (205,205,204) |
| **rank against the six unfocused rows** | **1st by 53 LSB** | ⛔ **beaten 4.4× by an UNFOCUSED sky row (75)** |

⇒ 🚨 **THE DOWNSCALE COSTS 7.5× OF THE SIGNAL AND INVERTS THE RANKING.** `TASK-1399`'s free test reported
that an **unfocused** row (`Play`) changed **more** than the expected focused row. **That inversion is
reproduced here on demand, from a frame in which the ring is independently proven present.** ⛔ Its null was
never evidence about the ring; it was a measurement of its own capture path.

## D.4 The two forbidden models, scored against the pixels

- **Model (b), *"< 1 LSB"*: REFUTED by measurement as well as by arithmetic.** The observed corner signal is
  **128–134 LSB**, and even the faint straight edge is **+16**. ⛔ Nothing here is sub-LSB.
- **Model (a), *"tens of LSB"*: RIGHT IN MAGNITUDE, WRONG IN CHANNEL** — exactly as §A.2 predicted before the
  capture. At the observed corner, **blue moves only +33 to +37** (224/225 against a 187–191 base) while
  **red falls 90–101** and **(B−R) moves +128 to +134.** ⇒ ⭐ **A BLUE-ONLY METRIC UNDER-READS THIS SIGNAL BY
  ~3.8× AT THE CORNERS AND BY ~9× ALONG THE RIM — confirmed on pixels, not just on paper.**
- ⇒ **`TASK-1399`'s `maxBlueDelta = +1.0` was doubly handicapped: the wrong channel AND a capture that had
  already thrown 7.5× of the signal away.** ⛔ Still not relied upon: that pair remains closed.

---

# §E — THE CONTROLS (spec (4)), EACH DECLARED

### ⛔ C0 — **DISQUALIFIED, ON THE FACE OF THIS REPORT, AS ORDERED**
`Button_3`'s *"wider and lighter"* render is an **AREA** signal and was once used to license a **1-px STROKE**
claim. An area signal is invariant under resampling; a sub-pixel stroke is destroyed by it — **this run
measured that destruction directly (§D.3: the stroke lost 7.5×; the buttons stayed perfectly legible).**
⛔ **C0 is reported as context and is NOT a control here.** ⚠️ Footnote: in **this** 1920×1085 lane `Button_3`
shows no anomalous render at all — its elevated `B−R` is the **sky behind it** (it straddles the horizon),
present identically in all three frames whether or not it is focused.

### ⭐ C1 — **FIRED.** ⛔ Not inert, and the board's pre-decided "inert ⇒ ABSENT" branch did **not** apply.
`simulate_key_press "Down"` ×2. **Observable — and it is an OUTCOME, not a flag (`VER-§8` cl. 10): the
structured focus read moved `Button_4` → `Button_6`.** Repeated twice, both times. ⛔ `binding_found` is cited
nowhere as the observable; it appears below only as evidence about *mechanism*.
⭐ **And it drew a ring: FRAME C, `Button_6`, max(B−R) = 128, four corners, zero elsewhere.**

### ⭐ C2 — **FIRED, WITH THE NUMBER THAT MAKES A NULL MEAN ANYTHING**
A known thin antialiased feature in the **same frame, same capture settings**: the white glyph strokes of an
unfocused button's label over its flat grey fill.

```
fill 187  ->  glyph peak 255            FULL AMPLITUDE = 68 LSB
max single-pixel horizontal step        = 67 LSB   (188 -> 255 in ONE pixel, at x=943 y=597)
partial-coverage intermediate pixels    = 114 of 1638  (7.0%)
noise floor of the statistic            = max(B−R) 0 on every unfocused same-background band
```

⇒ ⭐ **A ~1-px antialiased feature survives this capture at essentially FULL amplitude (a 67-LSB transition
across a single pixel boundary), and 7% of pixels carry fractional coverage — so sub-pixel detail is
preserved, not quantised away. The ring's 128–134 LSB is ~2× the C2 amplitude and ~43× the noise floor.**
⇒ **This capture can carry a stroke of the ring's class, and the proof is in the same frame as the claim.**

### (extra, not required) — **the downscale swap of §D.3**, a controlled negative for the *old* lane.

---

# §F — (6)(E) THE H7 LINE — RECORDED SEPARATELY, ⛔ NEVER MERGED WITH THE VERDICT

**`simulate_key_press` PRODUCED A RING.** Both input paths draw it, at the same amplitude
(`inject_input_action` → 134 · `simulate_key_press` → 128; the difference is corner sampling, not lane).

🚨 **AND A NEW MEASURED FACT THAT CHANGES WHAT H7 CAN EVER ANSWER.** `get_input_mapping_context_keys` on
`/Game/Input/IMC_MainMenu` returns **`Down` → `IA_MenuDown`** (with `Up`→`IA_MenuUp`, `Enter`→`IA_MenuAccept`,
plus the three gamepad twins). Each `simulate_key_press` reported `bound_actions: ["IA_MenuDown"]`,
`applied_mapping_contexts: ["IMC_MainMenu"]`.

⇒ 🧑 **HIS REAL `Down` KEYSTROKE IS BOUND TO OUR OWN ACTION.** The two "independent" paths are independent in
**input origin** (a raw key into Slate vs a direct Enhanced Input injection) and **converge on the same
`IA_MenuDown`**. ⇒ ⛔ **The outcome cannot discriminate which handler consumed the key — not because nobody
looked, but because the binding makes both readings produce the same result.** That is H7's real answer:
**not "untested", but "not decidable from the outcome, and here is the binding that makes it so."**

⇒ ⭐ **THE SENTENCE THE ROW ASKED FOR, AND IT IS THE OPPOSITE OF THE ONE ANTICIPATED: the two paths do NOT
differ. There is NO pixel difference between the lanes — 128 vs 134 LSB, both on exactly the focused row.
*"Same state, different pixels"* was never the right frame for this epic, because the pixels were never
different. THE INSTRUMENTS WERE.**

---

# §G — (7) THE STATE LIMB, RECORDED BESIDE AND NEVER MERGED (`VER-§8` cl. 4)

⛔ **Not the subject; read anyway.** The structured focus read named the **expected** button before **every**
capture, with all six others `focused: false`:

| capture | expected | `ui_snapshot` read | agree |
|---|---|---|---|
| FRAME A | `Button_0` (top, cold/after 6×`IA_MenuUp`) | `Button_0 focused: true` | ✅ |
| FRAME B | `Button_4` (after 4×`IA_MenuDown`) | `Button_4 focused: true` | ✅ |
| FRAME C | `Button_6` (after 2× key `Down`) | `Button_6 focused: true` | ✅ |
| §D.3 control | `Button_6` (unchanged) | `Button_6 focused: true` | ✅ |

⇒ **No escalation is owed.** The 🚨 Blockers path in (7) was not taken because its trigger did not fire.
⭐ Also measured: the seven button rectangles were **byte-identical in every `ui_snapshot` of the session** —
so the region stability `RECORD 2` demands is a measurement here, not an assumption.

---

# §H — THE BRANCH, BY LETTER (spec (6))

## ⭐ **(A)** — `#0070E0`-family blue on **exactly** the focused row's rectangle, on **none** of the other six, **at or above** the pre-registered amplitude ⇒ **`H-DRAWN` CONFIRMED. The ring always drew; the divergence is INSTRUMENT-SIDE.** Verdict **`MEASURED`** — a control discriminated.

⛔ **(B)/(C)/(D) are each excluded by a measurement, not by preference:** (B) needs "no blue on the focused
row" — there is blue, 128–134 LSB; (C) needs no control to have fired — **both C1 and C2 fired**; (D) needs
blue on unfocused rows too — **zero ≥100 pixels on all eighteen unfocused bands**, and the elevated `B−R` on
the sky-backed rows is present **identically whether or not those rows are focused**, which is what makes it
backdrop rather than detector error.

## 🚨 SAYING IT LOUDLY, AS THE ROW ORDERS: **THE PIXEL LIMB CAN BECOME MACHINE-CHECKABLE.**

The verifier's pixel limb is **not** blind by construction. It was blind by **configuration** — a default
`max_dim` cap and a blue-only metric. Both are now measured, and the recipe that worked here is three numbers:
**capture with an explicit `max_dim` ≥ the viewport's long edge · score `max(B − R)` per region, never the
mean · derive the regions once from the pixels, never from `ui_snapshot`'s `abs_*`.**

⛔ **THE ROWS THIS WOULD FREE — NAMED, AS ORDERED: `TASK-1402` · `TASK-1413` · `TASK-1421` · `TASK-1427` ·
`TASK-1436` · `TASK-1449`.** Each carries a pixel limb that today can only return `UNOBSERVABLE`.
⛔ **I board nothing and amend nothing (`SC-§101`) — this is handed to the manager as a finding.**

---

## Evidence (promoted)

Written **directly** by the capture tool to the evidence folder and **verified present on disk with byte
sizes** (⛔ no path is claimed that does not exist). All four are `1920×1085` native except the last.

- `.claude/pipeline/playtest-evidence/2026-09-24/VER-TASK-1450-mainmenu-button0-focused-native1920.png`
  — PIE t=192.823 (03m12s), 1,317,466 B. Seven grey buttons centred over a blue-sky/white-ground backdrop;
  `Button_0` "Play (vs Bot)" focused. Its four corners carry **RGB≈(90,156,222)**, max `B−R` **133**; the
  other six rows carry no pixel above `B−R` 75 (sky) or 0 (ground).
- `.claude/pipeline/playtest-evidence/2026-09-24/VER-TASK-1450-mainmenu-button4-focused-injectlane-native1920.png`
  — PIE t=207.952 (03m27s), 1,318,688 B. `Button_4` "Settings" focused via `inject_input_action`. Corner
  **RGB(90,157,224)** — blue exact to `FStyleColors::Primary` — max `B−R` **134**; its left rim pixel is
  **(161,168,177)** where every unfocused button's is **(192,192,192)**.
- `.claude/pipeline/playtest-evidence/2026-09-24/VER-TASK-1450-mainmenu-button6-focused-keypresslane-native1920.png`
  — PIE t=209.568 (03m29s), 1,321,073 B. `Button_6` "Quit" focused via `simulate_key_press` (**C1**). Corners
  **RGB(97,161,225)**, max `B−R` **128**; `Button_4` has reverted to the unfocused **(192,192,192)** — the
  ring moved with the focus.
- `.claude/pipeline/playtest-evidence/2026-09-24/VER-TASK-1450-mainmenu-button6-focused-downscaled1086.png`
  — PIE t=690.394 (11m30s), 1086×614, 425,293 B. **Identical state to the frame above, only `max_dim`
  changed.** The corner blue is gone (0 pixels ≥100, max `B−R` **17**) and an **unfocused** sky row now
  outranks the focused row 75 vs 17. **This is the `TASK-1399` blindness, reproduced on demand.**

⚠️ **Not promoted, by choice:** the three in-flight captures in `Saved/AuraVerify/` (`t1450_probe_res.png`,
`t1450_A_cold_button0.png`, `t1450_B_inject_button4.png`, `t1450_C_keypress_down2.png`). They are the
resolution probe and the first (pre-promotion) pass of the same three states; **no finding rests on them** and
`VER-§4` says promote what the report cites. `Saved/` is gitignored.
⚠️ Noted in passing, ⛔ not acted on: `TASK-1399`'s two self-declared misnamed files are **already absent**
from this folder — a `Glob` at the start of this run returned only its three correctly-named PNGs. **Its
owed deletion appears discharged; ⛔ I did not delete anything and I claim no credit for it.**

## Hypotheses (not verdicts)

- ⚠️ **The 2.532× / 2.543× gap between `ui_snapshot`'s `abs_*` and captured pixels is MEASURED; its CAUSE is
  not.** Candidates I can name but did **not** test: a Slate application scale applied to the PIE window; a
  desktop-DPI mismatch between the abs coordinate space and the render target; a render transform on the
  widget tree. ⛔ **I name no cause.** ⚠️ It is a **quiet hazard for every future row** that maps widget
  geometry onto a capture, because it fails by returning confident numbers for the wrong pixels.
- ⚠️ **Why the rim carries only +16 while the corners carry +134** is *consistent with* a stroke well under
  one pixel wide whose coverage doubles where the rounded-box corner arcs fold back. ⛔ **Not measured** — see
  Limitation 2.

## Not examined / limitations this run

1. ⛔ **I cannot state the ring's thickness or coverage as a number, and I decline to infer one.** The rim and
   corner pixels are **three-way** blends (ring + button fill + backdrop) and Slate composites in **linear**
   space while the PNG is **sRGB**-encoded. Solving `cov` from the corner independently per channel gives
   **0.79 (R) / 0.49 (G) / 0.94 (B)** even after an sRGB→linear conversion — mutually inconsistent, so the
   model does not invert. ⇒ ⭐ **PRESENCE IS PROVEN; THICKNESS IS NOT MEASURED.** §A's compositing model was
   good enough to size the experiment and is **not** good enough to invert, and I am not going to pretend
   otherwise after the fact.
2. ⛔ **`TASK-1446` limitation #1 link (a) — that the DPI scale multiplies the brush outline width — is STILL
   NOT VERIFIED.** §C closed link (b) only. This row never needed link (a): it measured the ring's *presence*
   rather than predicting its *width*.
3. ⚠️ **§A's predicted amplitude was met by a feature §A did not model** (corners, not straight edges). The
   pre-registration is reported **as written**, including this mismatch. A stricter reading is that §A got the
   right answer for a partly wrong reason, and the reader is entitled to know that.
4. ⚠️ **The `count(B−R ≥ 40)` statistic is background-contaminated on the four sky-backed rows** (`B1`/`B2`/`B3`
   carry 18 / 443 / 620 in every frame, focused or not). ⛔ Reported rather than suppressed. The discriminating
   statistic is the **≥100 corner signature**, which is clean in all three frames.
5. ⛔ **`start_pie`'s response overflowed the tool-result limit** (180,310 characters / 4,507 lines — a
   recovered film manifest, the same defect `TASK-1399` recorded). **I read ONLY lines 1–12**, enough to
   confirm `status: pie_requested` and the 1920×1080 window. ⛔ **I did not read the remaining ~4,495 lines and
   no finding in this report rests on them.**
6. ⛔ **Nothing was measured in 🧑 his `-game` lane.** The comparison to his 3200×1800 session remains
   arithmetic. This row proves the ring draws in **our** lane; it does not re-measure his.
7. ⛔ **Only `L_MainMenu`'s seven runtime-built buttons were examined.** Nothing here says anything about the
   sub-screens, whose separate finding (no focus at all) is `TASK-1399` §2's and is untouched.
8. ⛔ **The first analysis pass (the `abs_*`-mapped rectangles) is discarded in full** and no number from it
   appears above. It is described in §D.1 because a discarded pass that is never mentioned is a hidden one.
9. ⛔ **No code · no asset · no engine source (read or written) · no compile · no git · no `CONVENTIONS.md` ·
   no editor lifecycle action beyond the PIE session I started myself · no other row's line · no other agent's
   file · no Blueprint load · no remedy chosen · no clause amended · no row boarded.**

### `VER-§7` cl. 2 declaration

**Through `run_verification_sequence` (5 invocations, `record=false` throughout):** `ui_snapshot` ×7,
`capture_pie_frame` ×6, `inject_input_action` ×14, `simulate_key_press` ×4, `wait_pie_seconds` ×17.
**Direct:** `get_headless_status` ×1, `is_pie_active` ×2, `start_pie` ×1, `stop_pie` ×1 (**on the session I
started myself; `is_pie_active` was `false` before I began**), `get_input_mapping_context_keys` ×1.
**Inspector reads:** `execute_unreal_python_readonly` ×7 (**all read-only**; the census, five image-analysis
passes over frames **this row captured**, and the post-run state read), `quicksearch` ×1, `Read`/`Grep`/`Glob`.
⛔ **NO `pie_scene_edit`, NO `set_player_transform`, NO `spawn_actor`, NO `call_actor_function`, NO mutating op
of any kind. NO engine-lifecycle tool. NO generation or plan tool.**
⛔ **`binding_found` is cited nowhere as an observable** (`VER-§8` cl. 10) — C1's observable is the focus move
read by `ui_snapshot`; the flag appears only in §F as evidence about *mechanism*, explicitly labelled as such.
**Budget (cl. 2(b)):** 5 sequences carried ~48 actions; the per-frame image analysis was split one frame per
call after the first decode measured ~6.5 M byte-operations per image.
