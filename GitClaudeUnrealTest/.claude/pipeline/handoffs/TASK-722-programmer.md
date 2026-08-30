# TASK-722 — THE GREEN ELEVATION RAMP — two colour spaces in one file, both named

**Agent:** gameplay-programmer · **Date:** 2026-08-30 · **Status:** ready-for-qa
**Law:** `WM-§8a` / `§8b` / `§8c` / `§8d` · `WM-§2` · `SC-§33` · `W4-R1` · `SHIP-§9`
**Input:** `handoffs/TASK-721-artist.md` (the measured literals + the colour-space finding)

⛔ No compile · ⛔ no editor · ⛔ no MCP · ⛔ no Git · ⛔ no `.uasset` · ⛔ no `Tools/Packaging/`
⛔ `cards.csv`, `SummonedUnit.{h,cpp}`, `ClimbableTower.*`, `SiegeControlsHelpWidget.cpp` untouched.
🔒 Airlock: no `Capture()`, no `EnsureSnapshot()`, Zone A byte-frozen, the 552 latch unspent, no token figure (`AS-§12g`). **M8: client-local display only** — no replicated property, no new class, no RPC.

---

## 1. THE THREE CONSTANTS, AS SHIPPED

```cpp
// WarMapWidget.cpp — namespace SiegeWarMap
static const FLinearColor ElevationGrassDark  = FLinearColor(0.0626f, 0.1000f, 0.0624f, 1.00f);  // DISPLAY / sRGB-encoded → bytes (16, 26, 16)
static const FLinearColor ElevationGrassLight = FLinearColor(0.6257f, 1.0000f, 0.6243f, 1.00f);  // DISPLAY / sRGB-encoded → bytes (160, 255, 159)

static const FLinearColor AncientGroundIconTint = FLinearColor(0.04f, 0.22f, 0.09f, 1.00f);      // TRUE LINEAR (Slate encodes at draw)
```

✅ **Typed VERBATIM from TASK-721 §0 / §4.** ⛔ The DO-NOT-TYPE linear column — `(0.00516, 0.01002, 0.00515)` / `(0.34934, 1.00000, 0.34770)` — appears in this repo **only inside the warning comment that forbids it**, never as a value.

---

## 2. ⭐ CONFIRMED: THE LERP STAYED IN DISPLAY SPACE

`UWarMapWidget::BrightnessToRampColor` runs `FMath::Lerp` **per channel on the display-encoded floats**, then `RoundToInt(x * 255.f)` — the shipped gray-ramp arithmetic, three times instead of once.

- ⛔ **No linear round-trip.** No `FLinearColor::ToFColor(true)` anywhere on this path (it would sRGB-encode already-encoded floats).
- The ramp midpoint stays at display **0.55**, ⛔ not 0.73.
- Both ends are one chromaticity × {0.10, 1.00}, so **hue and saturation are constant along the ramp by construction** and the luminance curve is byte-identical to the gray ramp. ⛔ Not "improved".
- **Arithmetic check** (by hand, ⛔ not by compiler — 731 owns the only compile): mid green = `Lerp(0.1000, 1.0000, 0.5) × 255 = 140.25 → 140`, exactly `(26 + 255) / 2`. A linear-space lerp would land at byte **186**.

---

## 3. THE EXACT COMMENT TEXT AT EACH COLOUR-SPACE SITE

Both sites name their space, why it is that space, and that the other family exists. Full text is in the diff; the load-bearing sentences:

**(a) At the ramp constants (`WarMapWidget.cpp`, the retired-`ElevationFloorLuma` block):**
> `⚠️⚠️ COLOUR SPACE - THESE ARE **DISPLAY-ENCODED (sRGB) VALUES** IN AN FLinearColor CONTAINER. ⛔⛔ THEY ARE NOT LINEAR, DESPITE THE TYPE'S NAME, AND THAT IS CORRECT. … these two colours never reach Slate. They are turned into the raw BYTES of a texture created with Texture->SRGB = true …, and THE BYTES OF AN sRGB TEXTURE ARE sRGB-ENCODED VALUES. … Typing the TRUE-LINEAR equivalents … would drop the light end from byte 255 to 160 and the dark end from 26 to 3: a visibly much darker map, from literals that look perfectly plausible in a diff. ⛔ Do not convert these. ⛔⛔ AND THE OVERLAY TINTS ABOVE … ARE THE OPPOSITE - TRUE LINEAR … TWO SPACES, ONE TYPE, ONE FILE.`

**(b) At the POI tints (`WarMapWidget.cpp`, the `MineIconTint` / `AncientGroundIconTint` block):**
> `⚠️⚠️ COLOUR SPACE - THESE TWO ARE **TRUE LINEAR**, ⛔ NOT the space the elevation ramp's constants below are in, AND THAT DIFFERENCE IS INVISIBLE IN THE TYPE NAME. An overlay tint reaches the screen through SLATE, which sRGB-ENCODES it AT DRAW TIME (PackVertexColor -> FLinearColor::ToFColor(bSRGBVertexColor), and FSlateRHIRenderingPolicy::IsVertexColorInLinearSpace() returns false …). … ⛔ Never copy a value between the two families, and ⛔ never "correct" one into the other: BOTH readings compile, BOTH render, and only one is right at each site.`

**(c) Inline, on the constants themselves** — `// TRUE LINEAR (Slate tint) - …` on both POI tints, `// DISPLAY / sRGB-encoded` implied by the block plus the byte figures. A reader who greps a single line still sees the space.

**(d) Two more sites carry it** so it cannot be lost by a partial read: the `BrightnessToRampColor` declaration doc in `WarMapWidget.h`, and the "⭐⭐ THE LERP IS IN DISPLAY (sRGB) SPACE" block in its definition.

**(e) The structural finding is carried, as instructed** — in the POI-tint block:
> `🚩 KNOWN, MEASURED, ⛔ DELIBERATELY NOT REPAIRED HERE … MineIconTint (1.09:1) and MarkerLabelColor (1.15:1) … were ALREADY under it against the OLD gray ramp (1.33:1 and 1.06:1), so ⛔ the green did not break them. ⭐ THE STRUCTURAL FINDING: the three overlays that fail are EXACTLY the three drawn with NO OUTLINE. The marker GLYPH is fine (12.22:1) because it gets MarkerOutlineColor; its LABEL, drawn by MakeText a few lines later, gets none. … the repair for those two is an OUTLINE, ⛔ not another re-tint - and it is not this task's to make.`

⛔ **Not fixed, per instruction:** `MarkerLabelColor`, `MineIconTint`. ✅ No repair attempted on white POI glyphs (they improve), blue/red dots, or castle tints.

---

## 4. WHAT CHANGED, FILE BY FILE

### `Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.cpp`
| Site | Change |
|---|---|
| `~:166-167` | `AncientGroundIconTint` → `(0.04, 0.22, 0.09, 1.00)`; block comment gains the colour-space paragraph, the WM-§8c ruling with its numbers, and the no-outline structural finding |
| `~:198` | `ElevationFloorLuma = 0.10f` **RETIRED** (⛔ zero remaining code references — grep clean; the name survives only inside the comment that explains its retirement) → replaced by `ElevationGrassDark` / `ElevationGrassLight` + the display-space warning block |
| `~:928` | **NEW** `UWarMapWidget::BrightnessToRampColor` — per-channel display-space lerp, `RoundToInt × 255`, explicit `FColor` channel-order comment |
| `~:1194` | The bake's screen mapping is now one line: `Pixels[SampleIndex] = UWarMapWidget::BrightnessToRampColor(Brightness);` |
| `~:1232` | Log tail `"clamped full white"` → `"clamped to the ramp's light end"` |

### `Source/GitClaudeUnrealTest/Siegebound/WarMapWidget.h`
One new declaration + its doc: `static FColor BrightnessToRampColor(float Brightness);` — `public`, plain static, ⛔ not a `UFUNCTION`, **one parameter, none defaulted** (`SC-§33`). ⛔ No new include: `FColor` arrives through `CoreMinimal.h` exactly as `FVector2D` already does in this header (its own comment at `:73` says so).

### `Source/GitClaudeUnrealTest/Siegebound/Tests/SiegeWarMapTest.cpp`
**Section 27**, appended in place (⛔ never a second war-map test file): `Siegebound.WarMap.BrightnessToRampColorIsGreenDarkToLightAndMonotonic`.

---

## 5. ⛔⛔ `HeightToBrightness` IS BYTE-UNTOUCHED — VERIFY THIS FIRST

`git diff` over `WarMapWidget.cpp` shows **zero lines** inside `float UWarMapWidget::HeightToBrightness(float, float, float)` — body, comments, signature, `public` plain-static form, three parameters, none defaulted, same normalization, same clamp. Its declaration doc in the `.h` is likewise untouched. Section 24 of the test file is untouched.

---

## 6. THE TESTS — RELATIONSHIPS, ⛔ NEVER THE LITERALS

⛔ **The two grass constants are NOT transcribed into the test file.** They stay `.cpp`-local; every claim is derived from the seam's own outputs, so the suite still means something if the measurement is ever re-taken.

| Block | Claim | Catches |
|---|---|---|
| **(a)** | brightness 0 strictly darker than 1 on **every** channel; out-of-range clamps to the same texels as the ends | a reversed or non-total ramp |
| **(b)** | both ends opaque (A = 255) | a hole punched in the map |
| **(c)** | `G > R` **and** `G > B` at all 21 sweep points; and ⛔ never `R == G == B` | a reverted gray/luma ramp · an **R/G or G/B constructor transposition** |
| **(d)** | no channel decreases; WCAG luminance **strictly increases** across the sweep | a plateau — two heights painted the same green |
| **(e)** ⭐⭐ | the **midpoint byte is the average of the two end bytes** on all three channels (±1, half-up rounding) | ⭐ **THE COLOUR-SPACE TRAP** — a "corrected" linear-space lerp lands ~46 bytes high on green and fails all three at once |
| **(f)** ⚠️ | light end is red-over-blue (`R >= B`), dark end does not invert | an **R/B** transposition — ⚠️ **a ONE-BYTE margin, declared weak in the test's own comment**; the real control is the seam's channel-order comment |

**⚠️ Honest limit, stated rather than papered over:** (f) is the only guard on an R↔B swap and it has a 1-byte margin, because the measured chromaticity is nearly symmetric there (721 §5.3 warned of exactly this). ⛔ The ramp's own pixels are therefore **not** a valid channel-order check, and both the seam and the test say so.

### 🔢 SUITE TOTAL — DECLARED FOR TASK-731's GATE
- **Baseline at `HEAD` (`b6d05e6`): 156.**
- **TASK-722 adds exactly +1 ⇒ 157 attributable to this task.**
- ⚠️ **But the tree already carries TASK-724's `Tests/SiegeHighGroundTest.cpp` (9 tests, untracked).** Measured count of `IMPLEMENT_*_AUTOMATION_TEST` across `Siegebound/Tests/` **after** my edit = **166**. ⇒ **If all five wave tasks land together, 731 should expect 166/166** — ⛔ not 157. TASK-726 (`ClimbableTower`) and TASK-729 (`SiegeControlsHelpWidget`) added no test macros as of this write; **731 must re-count rather than trust this line if either changes.**

---

## 7. ⛔ THE FIREWALL (`WM-§8d`) — RE-VERIFIED AT SOURCE

- ⛔ **No gameplay header included** by `WarMapWidget.{h,cpp}` in this diff; ⛔ no `HIGH-§` symbol is called, referenced or named as a dependency anywhere in it (`HIGH-§` appears only in a comment saying it reads `GetActorLocation().Z` and must never read this layer).
- ⛔ **Nothing here is exported to gameplay.** `BrightnessToRampColor` is a display seam consumed by one call site — the texture bake — and its declaration doc states the 1,000 uu clamp and names `SHIP-§9`'s class explicitly, so a future gameplay reader is warned at the signature.
- ⛔ The reverse direction is clean too: no gameplay file was opened or edited.

---

## 8. ⚖️ DECLARED DEVIATIONS — READ THESE, THEY ARE THE JUDGEMENT CALLS

1. **⭐ THE DISPATCH FENCE SAID `WarMapWidget.cpp` ONLY; I ALSO TOUCHED `WarMapWidget.h` AND `Tests/SiegeWarMapTest.cpp`.** ⚖️ Reason: the **board spec item (5) mandates** extending `Tests/SiegeWarMapTest.cpp` in place and **declaring a suite total for 731's gate**, and the board's own ownership line grants this task `WarMapWidget.{h,cpp}` + that test file. The two grass constants are `.cpp`-file-local (internal linkage), so **a test can only reach the mapping through a declared seam** — without the one-line header declaration, item (5) is physically unexecutable and the only alternative is a test that re-types its subject's literals, which item (5) itself forbids. The dispatch's other fences all name **other tasks'** files (723/724/726/729/731/716/700); I read "`WarMapWidget.cpp` only" as that cross-task fence, ⛔ not as a repeal of item (5). 🙋 **If the narrow reading was intended, the header hunk and test Section 27 are one clean, separable revert each** — the ramp, the icon and the comments would survive untouched except that the bake's one line would inline the mapping again.
2. **The mapping moved into a named static rather than staying inline at `:1084-1087`.** ⚖️ `WM-§8a` requires only that it stay **out of `HeightToBrightness`** — it does. `WM-§2`'s own cited doctrine (`W4-R1`: *a testability obligation gets a testability seam*) is what produced the sibling seam; item (5) created that obligation here. **Behaviour is identical**; the call site is one line.
3. **Log-string repair (⛔ not on any change list):** `"clamped full white"` → `"clamped to the ramp's light end"`. ⚖️ It is **runtime output a human reads during the playtest verify** — naming a colour the map no longer contains would misdirect exactly the eye this feature is checked with. Display-only, in-file, one string.
4. **🚩 THREE STALE `"full white"` PHRASES LEFT IN, DELIBERATELY — flagged, ⛔ not fixed:** `WarMapWidget.cpp:851` (inside `HeightToBrightness`), `WarMapWidget.h:572` (its declaration doc) and `WarMapWidget.h:712` (`ElevationReliefCeiling`'s doc), plus Section 24's wording in the test file. ⚖️ **Item (1)'s "BYTE-UNTOUCHED" outranks a wording repair**, and editing a comment inside that function is exactly the diff line a reviewer would read as a violation. ⭐ The staleness is **self-documenting**: Section 27's opening comment says in the test file that "full white" now means the ramp's light green end and points here. 🙋 **Manager/QA ruling wanted** — it is a 4-word fix if the byte-untouched clause is read as behaviour-only.
5. **`AncientGroundIconTint` took the RECOMMENDED candidate (deep emerald), ⛔ not teal or magenta.** 721 explicitly left the "reads well" choice to Jonathan (`AS-§6 A(e)`) and recommended emerald on the numbers; the spec's item (4) said apply 721's replacement. **The alternates are one constant away** if his eye disagrees.
6. **⛔ NO CLAIM IS MADE THAT THE MAP "READS WELL."** Everything above is arithmetic, transcribed from a measurement, or a source read. `WM-§8c` reserves that judgement for Jonathan, and this handoff does not trespass on it.

---

## 9. WHAT QA SHOULD SCRUTINISE

1. ⭐⭐ **The colour-space split** — that the ramp pair is the **display** column and `AncientGroundIconTint` is the **linear** value, ⛔ not the reverse. Both are in `handoffs/TASK-721-artist.md` §0 and §4; a swap would compile, render and review as fine.
2. ⭐ **That the lerp did not sneak into linear space** — read `BrightnessToRampColor` and confirm no `ToFColor`, no `sRGBToLinear`, no `Pow(x, 2.2)`.
3. ⛔ **`HeightToBrightness` byte-untouched** — diff-scope it; this is item (1) and it is a BLOCKER if it moved.
4. ⚠️ **The `FColor` channel order.** `FColor(R, G, B, A)` is the constructor order; `B, G, R, A` is the memory layout the `Memcpy` into `PF_B8G8R8A8` depends on. Both are stated at the seam. ⛔ **Do not check this against pixel values** — 721 §5.3 explains why the ramp cannot detect its own R/B swap.
5. **`ElevationFloorLuma` is fully retired** — `grep -rn "ElevationFloorLuma" Source/` returns exactly one hit, inside the comment documenting the retirement.
6. **Deviation 1** — the header + test files. Rule on it explicitly rather than passing over it.
7. **Deviation 4** — the three stale `"full white"` doc phrases. A ruling either way is fine; ⛔ silence leaves them.
8. **Suite total 166 vs 157** — §6 shows the derivation; 731 needs the right number.
