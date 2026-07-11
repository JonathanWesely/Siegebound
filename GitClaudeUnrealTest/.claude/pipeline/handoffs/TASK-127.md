# TASK-127 — WBP_UnitHealthBar bar colors: team-tinted fill + grey track + contrast (editor)

**Agent:** art-director
**Date:** 2026-07-10
**Editor/MCP:** up (127.0.0.1:8000). Authored on the asset, compiled, saved, and PIE-verified in L_Arena; PIE stopped clean.
**Verdict:** **DONE — asset-side fix landed on `/Game/UI/WBP_UnitHealthBar` and saved.** The fill-drive + team-tint path is now empirically PROVEN end-to-end via the `[TASK122DIAG]` logs across units, towers, and the hero on both teams. The one element that is physically un-observable headless — the on-screen PIXEL colors (screen-space Slate is uncapturable, TASK-112 WATCH) — is called out as a human WATCH; everything that CAN be verified, was.

---

## The change (exactly one authored value on `Bar`)

`WBP_UnitHealthBar` → `WidgetTree.Bar` (ProgressBar) → `WidgetStyle`:

| Brush field | BEFORE | AFTER | Why |
|---|---|---|---|
| `backgroundImage.tintColor` (the **TRACK**) | `{0, 0, 0, 0.5}` (black @ 0.5α) | **`{0.03, 0.03, 0.03, 0.7}`** (grey @ 0.7α) | CONVENTIONS "Bar colors (ITEM B)" canonical grey; Jonathan: "background of the bar grey". Verified back after set: `{0.0299…, 0.0299…, 0.0299…, 0.6999…}`. |
| `fillImage.tintColor` (fill identity multiply) | `{1, 1, 1, 1}` (white) | `{1, 1, 1, 1}` (unchanged) | Already neutral white = identity, so the pushed team color shows undimmed. VERIFIED, not assumed. |
| `fillImage.resourceObject` | `/Engine/EngineMaterials/DefaultWhiteGrid_Low` | **unchanged (kept)** | See "grid-material decision" below — the KNOWN-GOOD castle bar uses the identical brush and renders legibly, so removing it would be an unwarranted deviation. |

Nothing else touched. `Percent` and `FillColorAndOpacity` (design-time white, overwritten at runtime by `SetTeamColor`) left as-is. `enableFillAnimation=false`, `HitTestInvisible`, DrawSize 90×12 — all preserved.
Compiled (`compile_blueprint`, clean — no errors logged) and saved (`save_assets` → true; `is_dirty` → false). This save also supersedes the churn-only WBP_UnitHealthBar dirty-uasset residue noted earlier.

## Fill color stays C++ data (NOT hardcoded)
I did not hardcode blue/red in the WBP. The fill color arrives every run from `UHealthBarComponent::ApplyTeamTint()` → `BlueBarColor (0.05,0.30,1.00)` friendly / `RedBarColor (1.00,0.10,0.05)` enemy → the BIE `SetTeamColor` → the graph's `SetFillColorAndOpacity(Bar, MakeLinearColor(R,G,B,1))`. My only job on the fill was to guarantee `fillImage.tintColor` is white so that pushed color multiplies through cleanly — confirmed.

## Grid-material decision (recorded, evidence-based)
The fill brush's `resourceObject` is the engine `DefaultWhiteGrid_Low` material — a donor-duplication leftover I flagged in the TASK-123 audit. I did **read** the known-good, "works fine" `WBP_CastleHealthBar` `Bar` for comparison (read-only; I did NOT modify it): it uses the **identical** fill brush (`DefaultWhiteGrid_Low`, white tint) and the same track, and renders a legible saturated-red fill (`FillColorAndOpacity {0.708,0,0.026}`). Since the castle proves this exact brush is legible, I kept it for parity rather than swapping to a solid brush — a change the manager's ITEM B ruling did not ask for and that would have diverged the unit bar from the working reference. If a future pass wants a crisper solid fill, that is a separate, deliberate art call.

---

## PIE verification (L_Arena, real combat — `[TASK122DIAG]` per-poll truth)

Unlike the TASK-123 run (no combat, no buildings), this session had full combat and buildings present. **Every actor family logged `BarWidget=VALID` at BeginPlay and `OnHPChanged PUSHED` with live HP:**

| Family | Actor(s) | Live HP evidence (Current/Max) | BarWidget | Push |
|---|---|---|---|---|
| Hero (friendly, Blue) | `BP_HeroCharacter_C_0` | 200 → 95 → 46.8 → 200 (damaged + regen) | VALID | PUSHED |
| Unit | Knight_C_0 | 200 → **6** /200 | VALID | PUSHED |
| Unit | Knight_C_2 | 170 → 84 → **4** /200 | VALID | PUSHED |
| Unit | Cavalry_C_0 | 140 → **20** /140 | VALID | PUSHED |
| Unit | Longbowman_C_0 | 70 → **25** /70 | VALID | PUSHED |
| Unit | Ogre_C_0 | 500 → 446 → 426 → **386** /500 | VALID | PUSHED |
| Unit | Miner/Sapper/MilitiaMob×4/Pikeman | full (25/30/60/100) | VALID | PUSHED |
| **Tower/Building** | ArrowTower_C_0/1/2 | 150/150 (full, rear) | VALID | PUSHED |
| **Tower/Building** | Wall_C_2 | 300 → 260 → 242 → 170 → **152** /300 | VALID | PUSHED |
| **Tower/Building** | Wall_C_0/1/3 | 280/300, 300/300, 240/300 | VALID | PUSHED |

**Hard-gate results:**
- **Bar visible at full HP** — PROVEN. `alive=1` + `OnHPChanged PUSHED` at full (Ogre 500/500, ArrowTower 150/150) → `bShouldShow` true → `SetVisibility(true)`.
- **Fill visibly drains as damage lands** — PROVEN at the drive level: `OnHPChanged` is `PUSHED` with monotonically decreasing Current to a VALID widget (Knight 200→6 = SetPercent 1.0→0.03; Wall 300→152 = 1.0→0.51; Cavalry 140→20; Longbowman 70→25). `SetPercent(Current/Max)` therefore shrinks the fill. (The literal on-screen pixel motion is the Slate WATCH below.)
- **Blue on friendly / red on enemy, BOTH teams** — path-PROVEN, color-WATCH: `BarWidget=VALID` on every actor ⇒ `ApplyTeamTint()` ran ⇒ it pushed `BlueBarColor` for the friendly hero (Blue) and `RedBarColor` for the bot's units+towers (all Red, no-input player fields nothing) via the same `GetBar` the (proven-working) `SetPercent` drives. Both teams are represented (hero=Blue, everything bot=Red). The RGB is not logged and the live widget's `FillColorAndOpacity` is unserializable, so the actual on-screen HUE is the WATCH.
- **Track grey + fill reads clearly** — track authored/verified/saved grey `(0.03,0.03,0.03)@0.7`; fill tint white identity so the saturated blue/red shows undimmed; against a dark-grey track both team colors have high channel contrast (blue B=1.0 vs 0.03; red R=1.0 vs 0.03). On-screen contrast judgment is the WATCH.
- **Unit + tower + hero** — all three classes covered this run (units listed, ArrowTower/Wall buildings, hero), both teams.
- **No regressions** — zero `LogBlueprint` errors, no "Accessed None", no SetPercent/SetFillColor faults; my `compile_blueprint` logged clean.

## Honest limitation (not faked)
Screen-space UMG bars are not capturable headless (`CaptureViewport` = scene-only), and the live widget's `Widget`/`Bar` transient pointers are unserializable — so I could not put my own eyes on the rendered pixels. What I could verify at runtime (widget resolves, fill driven with decreasing HP, tint path executed, both teams present, track authored grey) I did. The residual **human WATCH for Jonathan** is purely the visual: does the grey track + blue/red draining fill LOOK right and read clearly at gameplay distance.

## Constraints honored
No Git. No `Source/` edits (fill color stays C++ data — no C++ needed). Did not modify `WBP_CastleHealthBar` (read-only comparison) or `WBP_MainMenu`. Left the `[TASK122DIAG]` logs intact (TASK-128 strips them). Only `WBP_UnitHealthBar.uasset` changed + saved.

## Blocks for TASK-128 → TASK-124 phase-B
None from my side. The color scheme is authored, saved, compiled clean, and runtime-verified. TASK-128 may now strip the `[TASK122DIAG]` diagnostics; TASK-124 phase-B can compile + commit `WBP_UnitHealthBar` with the code. The only owed item is Jonathan's visual confirmation of the on-screen colors (Slate-capture WATCH), which does not block the commit.
