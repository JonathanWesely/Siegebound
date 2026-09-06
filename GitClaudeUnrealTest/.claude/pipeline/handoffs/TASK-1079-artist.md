# TASK-1079 — [OVAL-KILL] — art-director handoff

**Marker:** `TASK-1079-OVAL-KILL` · **Date:** 2026-09-06 · **Status on exit:** `ready-for-integration`
**Host for the commit:** `TASK-1080` (build-master)

🧑 **His words:** *"there seems to be a bit of a visual bug where the oval that is used for the 'play again' button appears above the cards wrapped around the numbers 2 through 6 during play, get rid of that oval so it doesn't appear before the match ends."*

---

## 1. WHAT ACTUALLY DRAWS THE OVAL — **CONFIRMED, NOT ASSUMED**

**It is `Btn_Jump` in `/Game/UI/WBP_CardHand`.** The pre-authorised suspect was correct — but it was *confirmed on four independent lines of evidence before a property was changed*, and the named alternative was *refuted at the asset*, not merely doubted.

### 1a. The graph (engine read-back, `read_graph_dsl`)
- `BuildHandTree` reaches the root exactly as `CARDBAR-§11a` records:
  `CastToOverlay(GetParent(GetParent(GetBtn_Jump)))` → the hand box, preview face and refusal text attach to that cast result. **Confirmed live, not inherited from the handoff.**
- 🚨 **There is NO `SetVisibility(Btn_Jump, …)` anywhere in the asset** — not in `EventConstruct`, not in the EventGraph, not in any of the seven functions. ⇒ `Btn_Jump` is `Visible` at runtime with nothing to suppress it. (Earlier handoffs `TASK-041`/`TASK-049` describe a runtime collapse; **that collapse does not exist in the shipped graph.** It exists in `WBP_MainMenu`, not here.)
- `UpdateGoldDisplay` (reached from `OnGoldChanged_Event`) **actively styles it every gold tick**:
  `SetIsEnabled(Btn_Jump, NewGold >= CardCost)` and `SetBackgroundColor(Btn_Jump, green/grey)`. So it is not inert leftover geometry — it is being repainted continuously.

### 1b. The asset (engine read-back, `get_properties`)
- `Btn_Jump.Visibility` = **`Visible`**, `bIsEnabled` = true, `RenderOpacity` = 1.
- Style: `drawAs: RoundedBox`, `outlineSettings.color` = grey **0.695**, `width: 1` — a **thin light-grey outline**. Disabled state: `drawAs: Image`, `resourceObject: /Engine/MobileResources/HUD/VirtualJoystick_Thumb` with `roundingType: HalfHeightRadius` — a **stadium/ring**, i.e. literally an oval.
- `SizeBox_0` = **240 × 80**, `SelfHitTestInvisible`, bottom-centre pad B40 — the hand bar's own attach is bottom-centre pad B24. Exactly as `§11a` recorded.

### 1c. The pixels (the decisive rung — `SC-§94` cl. B rung 4)
Measured in the live PIE frame, band **above** the chip glyph row:
- The arc is a **neutral grey outline with two vertical rails at `x = 562` and `x = 718`**, constant from `y = 657` down to `y = 687` where the card row occludes it.
- **Width = 157 px.** The floating PIE client is 1280 × ~717 ⇒ UMG DPI scale ≈ **0.654**. `240 × 0.654 = 157`. ✅
- **Vertical:** bottom offset ≈ 26 px; `40 × 0.654 = 26.2`. ✅
  ⇒ **Both axes independently reproduce `SizeBox_0`'s 240 × 80 / pad-B40 to the pixel.** This is a quantitative identification, not a resemblance.

### 1d. The named alternative — **REFUTED AT THE ASSET**
`KeyChip`/`KeyChips` are **plain `UMG.TextBlock`s** constructed by `BuildHandTree` (`SetFontSize 12`, `HitTestInvisible`), driven by `UpdateSlotKeyChip` via `SetText`/`SetVisibility` only. **A `TextBlock` has no border or background brush** ⇒ it *cannot* draw a rounded outline. The alternative is not merely unlikely; it is structurally impossible.

### 1e. 🚨 HOW *"2 THROUGH 6"* RECONCILES — the discriminator, answered with numbers
Measured chip-glyph cluster centres in the BEFORE frame (six clusters, i.e. all six chips present):

| chip | 1 | 2 | 3 | 4 | 5 | 6 |
|---|---|---|---|---|---|---|
| centre x | **486** | ~566 | 592 | 673.5 | ~714 | 758 |

Oval rails: **x = 562** and **x = 718**.

⇒ **Chip 1 (x = 486) sits 76 px OUTSIDE the left rail. Chips 2–5 are bracketed by the rails and chip 6 sits under the right shoulder of the arc.** That is precisely his *"wrapped around the numbers 2 through 6"*, and it is the asymmetry the row demanded be accounted for: **one 240-unit-wide element whose left edge lands past chip 1** — whereas a per-chip brush would have appeared on **all six, symmetrically**. His observation was exact; only his attribution was wrong.

> Slot widths vary with card-name length (`Watch Tower` is wide, `Fog` narrow), so the exact chips the rails land between shift hand-to-hand. The invariant — **chip 1 outside on the left, the rest covered** — is what reproduces.

### 1f. 🧑 His attribution, and why his regression worry was already safe
Play Again lives in **`/Game/UI/WBP_VictoryScreen`**, loaded by `ASiegePlayerController` as `/Game/UI/WBP_VictoryScreen.WBP_VictoryScreen_C`. `Btn_Jump` lives in **`/Game/UI/WBP_CardHand`**. **Different assets.** Corroborated by an engine-side reference query: `get_referencers(/Game/UI/WBP_CardHand)` = **`["/Game/UI/WBP_HUD"]`** — `WBP_VictoryScreen` neither references nor is referenced by the asset I edited. **There is no path from this edit to that button.**

---

## 2. THE FIX — one property, the only legal one

`SetVisibility(Btn_Jump, Collapsed)` on `/Game/UI/WBP_CardHand.WBP_CardHand:WidgetTree.Btn_Jump`.

- ⛔ **NOT deleted, NOT reparented, NOT renamed** (`CARDBAR-§11a`). `Btn_Jump` still resolves in the WidgetTree after the edit, and `BuildHandTree` still holds its getter (`K2Node_VariableGet_22`) after compile.
- ⛔ **NOT `Hidden`** — `Collapsed`, so it reserves no layout.
- **Read back from the engine** after the write (`SC-§94` cl. A — never the value passed in): `{"Visibility":"Collapsed","bIsEnabled":true}`.
- `SizeBox_0` deliberately left untouched: `240 / 80 / SelfHitTestInvisible` re-read identical after the edit. **Exactly one property on exactly one widget changed.**
- Blueprint compiled clean (`LogBlueprint: Compiling Blueprint '/Game/UI/WBP_CardHand.WBP_CardHand'`, no errors/warnings), then **saved as a single package** — `save_assets(["/Game/UI/WBP_CardHand"])`, never the empty "save all" list.

---

## 3. THE THREE-PART OUTCOME CHECK

Method: floating-window PIE on `L_Arena` (client 1280 × ~717), same vantage before and after, arc measured as neutral-grey pixels (`max>140`, `max-min<26`) in the band `y 655..678`, strictly above the chip glyph row.

**Zero-control first (`SC-§88`):** three consecutive BEFORE captures, nothing changed —
arc pixels **367 / 359 / 362** (spread 8, ≈2.2%), rails **identical at x=562 / x=718** in all three. That is the noise floor the AFTER result must beat.

### ✅ (a) THE OVAL IS GONE MID-MATCH — **PASS, observed**
| | arc pixels | rails |
|---|---|---|
| BEFORE ×3 | 367 / 359 / 362 | 562, 718 |
| AFTER ×2 | **0 / 0** | none |
| AFTER, match start / low gold | **0** | none |

Zero, not "reduced" — two orders outside the ±8 control band. Evidence `A`, `B`, `C`, `D`, `F`.

**Extra rung nobody asked for, taken because the brushes differ:** the BEFORE frame was at **Gold 83** (button *enabled* ⇒ `RoundedBox` outline). At match start gold is below `CardCost`, so `UpdateGoldDisplay` **disables** the button and Slate switches to the **`disabled`** brush — the `VirtualJoystick_Thumb` ring at `HalfHeightRadius`, the most oval-looking of the four states. Captured that state too (evidence `E`): **also 0.** `Collapsed` removes it from layout, so no state of the brush can draw.

### ✅ (b) 🚨 THE CARD BAR STILL BUILDS — **PASS, observed. The `§11a` catastrophe did NOT happen.**
Three separate PIE sessions, three different dealt hands, **six cards and six chips `1`–`6` in every one** (chip clusters counted numerically, not eyeballed — 6 clusters detected):
- `Watch Tower 30g · Fog 50g · Watch Tower 30g · Archer 12g · Archer 12g · Archer 12g`
- `Archer 12g · Archer 12g · Fog 50g · Watch Tower 30g · Bright Sun 60g · Fog 50g`
- `Archer 12g · Wall 12g · Bright Sun 60g · Wall 12g · Fog 50g · Archer 12g`

Card art, name, cost and the `Next:` preview column all present. ⇒ **`CastToOverlay(GetParent(GetParent(Btn_Jump)))` still resolves through a `Collapsed` widget** — `TASK-808` §8's claim is now confirmed by live observation on this asset, not inherited.

### ⚠️ (c) MATCH-END SCREEN / PLAY AGAIN — **NOT OBSERVED. Declared inability, transferred by name to `TASK-1080`.**

**I could not reach match end, and I will not imply an observation I did not make.** The routes and why each is closed:
- There is **no `UFUNCTION(Exec)` cheat** anywhere in `Source/GitClaudeUnrealTest/Siegebound/`.
- There is **no match timer / time limit** in `SiegeGameMode.h` to advance.
- The match ends only via `FOnCastleDestroyed` → `OnCastleDestroyedHandler` → `HandleMatchEnd`. `ASiegePlayerController::HandleMatchEnd(ETeamId)` is `BlueprintCallable`, **but this MCP bridge exposes no function-invocation tool** — only property get/set, so I cannot call it on the live PIE controller.
- Destroying a castle for real needs sustained gameplay input; driving the PIE window by synthetic input is precisely the instrument this project has recorded as unreliable.

**What I did establish for (c), and the rung it stops at (`SC-§94` cl. B):**
- **Rung 1 (properties):** `WBP_VictoryScreen.uasset` is **byte-identical to `HEAD`** — `git status -- Content/` returns **exactly one line**, `WBP_CardHand.uasset`. I never opened the file.
- **Rung 2 (reachability):** `get_referencers(/Game/UI/WBP_CardHand)` = `["/Game/UI/WBP_HUD"]` ⇒ **no reference path** links my edit to the Play Again widget.
- **Rung 4 (outcome) — NOT REACHED. Owner: `TASK-1080`, spec item (2).** What remains to be seen is a real match end with Play Again present *and clicked*.

---

## 4. FENCES — all verified

| fence | result |
|---|---|
| `L_Arena` never saved | ✅ `1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622` **before AND after** — matches `…5622` exactly |
| `WBP_VictoryScreen` untouched | ✅ never opened; unmodified vs `HEAD` |
| `BP_T19_PlayAgain` untouched | ✅ never opened |
| Vendor packs | ✅ untouched |
| `CONVENTIONS.md` | ✅ untouched |
| C++ | ✅ none |
| Git | ✅ **nothing staged, nothing committed, no push** |
| `checkout --`/`restore`/`stash`/`reset`/`clean` | ✅ never run |
| `.claude/agents/qa-reviewer.md` | ✅ untouched |

**`WBP_CardHand.uasset`** `26166fbd55fc8de56d9150c1f2119279afa6cfd7bb5b2dd085aba5e2d8ce2a8a` → **`7d9b98f609b8663002eb2dfcc111307696401b91bd440e244c46788e30dbf86a`**

---

## 5. 🚨 TWO THINGS `TASK-1080` MUST KNOW BEFORE IT TOUCHES THIS EDITOR

### 5a. ⛔ **DO NOT "SAVE ALL" IN THIS EDITOR. `L_Arena` IS DIRTY IN MEMORY.**
A prior session spawned `BP_SiegeFog_C_1` into the **editor** world (`LogActorFactory … /Game/Maps/L_Arena.L_Arena:PersistentLevel.BP_SiegeFog_C_1`, 02:27:03). The on-disk map is intact, but the in-memory package is modified. **`save_assets([])` with an empty list means "save every dirty package" and would write `L_Arena`.** Always pass an explicit single path. Decline every save prompt.
`WBP_HUD` is also dirty in memory (see 5b) and must likewise not be saved — its on-disk file is unmodified vs `HEAD` and must stay that way.

### 5b. ⭐ **NEW PIPELINE FINDING: `is_dirty` IS NOT EVIDENCE THAT ANOTHER LANE EDITED SOMETHING — A READ-ONLY GRAPH READ SETS IT.**
`WBP_CardHand` read **dirty before my first write**, which the batch's expected-dirty set calls a stop-and-report ("do not absorb another lane's uncommitted work"). **I did not assume it was benign — I ran a controlled experiment.** `WBP_HUD` was measured `is_dirty = false`; I then ran one `list_graphs` + one `read_graph_dsl` against it and nothing else; it measured `is_dirty = true`. ⇒ **inspecting a Blueprint through this bridge dirties its package.** `WBP_CardHand`'s flag was my own read, and git confirmed the on-disk file matched `HEAD` throughout. The finding clause correctly did not fire.
⚖️ *The general lesson: `is_dirty` answers "has this package been touched in memory", not "has someone changed its content" — and the two are routinely different in a long-lived editor. **The on-disk hash vs `HEAD` is the arbiter; the dirty flag is not.*** Worth a `CONVENTIONS.md` line — **the manager owns that file (`SC-§82`), I did not edit it.**

---

## 6. WRITES (the pathspec `TASK-1080` derives its commit from)

```
Content/UI/WBP_CardHand.uasset                        (edited IN PLACE; oid-verify vs sha256 above — SC-§68)
.claude/pipeline/handoffs/TASK-1079-artist.md         (this file)
.claude/pipeline/playtest-evidence/2026-09-06/TASK-1079-A-BEFORE-full-PIE-oval-over-card-hotkeys.png
.claude/pipeline/playtest-evidence/2026-09-06/TASK-1079-B-BEFORE-zoom-oval-rails-x562-x718-chip1-outside.png
.claude/pipeline/playtest-evidence/2026-09-06/TASK-1079-C-AFTER-full-PIE-oval-gone-six-cards-build.png
.claude/pipeline/playtest-evidence/2026-09-06/TASK-1079-D-AFTER-zoom-six-chips-1to6-no-oval.png
.claude/pipeline/playtest-evidence/2026-09-06/TASK-1079-E-AFTER-matchstart-lowgold-disabled-brush-also-gone.png
.claude/pipeline/playtest-evidence/2026-09-06/TASK-1079-F-BEFORE-vs-AFTER-same-vantage.png
```
⛔ **NEVER STAGED:** `Content/Maps/L_Arena.umap` · `Content/UI/WBP_HUD.uasset` · `.claude/agents/qa-reviewer.md`

## 7. EDITOR STATE ON EXIT
**LEFT OPEN** (PID 17008), PIE stopped, MCP live on `127.0.0.1:8000`, `L_Arena` loaded — `TASK-1081` needs it and I was sequenced first as the shorter sitting.
⚠️ I **minimised a "My Library | Fab" window** that was covering the viewport and blocking every capture, and maximised the main editor window. Left minimised deliberately so `TASK-1081`'s viewport captures are not occluded; 🧑 Jonathan restores it from the taskbar in one click. Nothing in it was clicked, browsed or downloaded.
