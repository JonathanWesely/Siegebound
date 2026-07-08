# TASK-080 handoff — WBP_CardHand: art on the 6 card faces + next-card preview (editor/UMG)

- **Author:** gameplay-programmer
- **Date:** 2026-07-08 (editor session PID 34120, the TASK-081 phase-1 module; IsPIERunning=false at every edit; no editor bounce)
- **Status requested:** complete — orchestrator flips the board (dispatch said NO TASKBOARD edits by me)
- **Asset touched:** `/Game/UI/WBP_CardHand` ONLY (compiled clean incl. warnings-as-errors, saved, is_dirty=false). WBP_HUD and L_Arena verified NOT dirtied. No C++, no Git, no DT_Cards touch.

## What was built (technique = TASK-041/050/064 proven pattern)

Bulk logic in FOUR NEW function graphs (safe fresh-graph `write_graph_dsl`), granular
`create_node`/`connect_pins` ONLY where the class-ambiguous DSL ids forced it, and exactly TWO granular
insertions into the EventGraph handlers. The protected-graph law was honored: **WBP_HUD's M1 gold Construct
was never opened**; WBP_CardHand's EventGraph was never round-tripped through write_graph_dsl (granular only —
all 12 AssignOnClicked delegate bindings verified intact by readback afterward).

### New member variables
- `Img_CardArt` — ARRAY of `UImage` refs, one per hand slot (index = SlotIndex). The board's "Img_CardArt
  (one per hand slot)" name law is carried by this variable; runtime-constructed widgets can't be given
  editor names via ConstructObjectFromClass (no Name pin) — same convention TASK-041 used for SlotBoxes etc.
- `Img_NextCardArt` — single `UImage` ref (preview art).

### New function graphs (all bodies verified by DSL readback)
1. **`WrapCardFace(FaceBox:Widget) → (FaceOverlay:Overlay, ArtImage:Image)`** — constructs an Overlay
   (SelfHitTestInvisible), adds an Image FIRST (z-order 0 = background; **created Collapsed**; overlay slot
   HAlign_Fill/VAlign_Fill — both pin values readback-verified), then adds FaceBox on top (Fill/Fill).
   Returns both.
2. **`UpdateSlotArt(SlotIndex:int, CardID:string, bAffordable:bool)`** — `Img_CardArt[SlotIndex]` → IsValid
   guard (mirrors the handler's existing box guard) → **`GetCardArtTexture(self, CardID)` — the TASK-079
   resolver, called with the event's own CardID, NO BP-side pre-checks or caching** → IsValid(texture):
   - valid: `SetBrushFromTexture(img, tex, bMatchSize=false)` → `SetColorAndOpacity(img,
     SelectColor(A=white, B=(0.35,0.35,0.35,1), bPickA=bAffordable))` → `SetVisibility(img, HitTestInvisible)`
   - null: `SetVisibility(img, Collapsed)` — null is the single hide-art signal (text-only fallback), exactly
     per qa/TASK-079-report.md carry-forward.
3. **`UpdateNextCardArt()`** — same shape on `Img_NextCardArt` via `GetNextCardArtTexture(self)` (no params —
   the C++ LastNextCardID cache). No affordability tint on the preview (spec ties tint to hand slots only).
4. **`ApplyCardTextShadows()`** — one-shot legibility pass (the ruling's allowed "shadow behind text"):
   ShadowColor (0,0,0,0.8) + ShadowOffset (1,1) applied to all SlotNameTexts, all SlotCostTexts,
   PreviewNameText, PreviewCostText.

### BuildHandTree changes (rewritten via write_graph_dsl — flagged decision 1 below)
- Loop body: after the discard-button array-add, `(faceov, faceimg) = WrapCardFace(slotVBox)`; `faceimg`
  appended to `Img_CardArt`; **the OVERLAY (not the VBox) is now added to the hand HorizontalBox**; SlotBoxes
  still stores the VBox (the handler's collapse/show target — unchanged semantics; when the VBox collapses and
  the art image is collapsed, the wrapper overlay has zero desired size = empty slot takes no space, same as
  before).
- Preview: `(prevov, previmg) = WrapCardFace(previewVBox)`; `previmg` → `Img_NextCardArt`; the OVERLAY is what
  attaches bottom-right (HAlign_Right/VAlign_Bottom, margin R24/B24 — unchanged values).
- `ApplyCardTextShadows()` appended at the tail of the root-overlay cast's `:then` (texts all exist by then).
- Everything else byte-equivalent to the TASK-041 graph (readback-diffed): fonts 16/14/12/12/14/20, "Play"/"1"
  labels, hand-box bottom-center margin B24, refusal Collapsed + center attach, CastFailed PrintString.

### EventGraph granular insertions (2 nodes total, readback-verified)
- `EventOnHandSlotUpdated`: `K2Node_CallFunction_15` (**UpdateSlotArt**) spliced INSIDE the existing
  IsValid(slot-box) guard, BEFORE the IsEmpty(CardID) Branch — so the resolver runs exactly once per event
  invocation on the common path (Callable exec placement per QA carry-forward), and empty-CardID pushes reach
  the resolver (which returns null silently) → art hides in the same event that collapses the box. Pins wired:
  Event.SlotIndex/CardID/bAffordable straight to the call (zero conversion nodes).
- `EventOnNextCardUpdated`: `K2Node_CallFunction_16` (**UpdateNextCardArt**) spliced between the event and the
  existing IsEmpty(DisplayName) Branch.
- Node IDs added this task: EventGraph `K2Node_CallFunction_15`, `K2Node_CallFunction_16`; UpdateSlotArt graph
  `K2Node_CallFunction_7` (SetBrushFromTexture, declaring_class=/Script/UMG.Image), `_8` (SetColorAndOpacity,
  Image), `_9` (SelectColor, A/B literals set by pin value); UpdateNextCardArt graph `K2Node_CallFunction_3`
  (SetBrushFromTexture, Image). Everything else came from the four fresh-graph DSL writes.

## WARN-4 hit-test posture (preserved)
Art images: **HitTestInvisible when shown, Collapsed when hidden** (never hit-testable). Wrapper overlays:
SelfHitTestInvisible. Buttons/texts/root/InitForController path untouched. The 12 button OnClicked delegate
bindings re-verified by full EventGraph readback AFTER all edits (AssignOnClicked → OnPlaySlot0..5 /
OnDiscardSlot0..5 all intact).

## PIE verification (direct-boot L_Arena, live module PID 34120, DT_Cards already carrying CardArt)

Two sessions (in-viewport + floating-window for a legibility-grade capture; MCP cannot click/inject input —
TASK-041 precedent — so interactive items are structural + screenshot evidence, listed below for phase 2).

**Observed live (screenshot evidence, floating 1280×760 PIE):**
- All 6 hand faces render card art matching their CardID (observed across the two sessions ≥8 distinct cards:
  BombTower=bomb, Archer=archer figure, Cleric=robed figure, WarBanner=banner, MilitiaMob, Barracks, Militia/
  Miner-family cards in session 1 — art content matches names).
- Name + cost text render OVER the art, white with the new drop shadow — legible at hand-slot size.
- **Preview art live**: "Next: Longbowman" text over the Longbowman green art patch bottom-right.
- Play/"1" buttons render below the art band, unobstructed; M1 gold counter ("Gold: 110"), +2/s rate, 0/6
  miner count, Rally: Ready all intact top HUD.
- Hand refresh works continuously (slots re-push per gold tick → SetBrushFromTexture re-runs, idempotent,
  no flicker observed, no log spam — the resolver's once-per-CardID guards hold).
- **Zero new log errors/warnings attributable to the change** in either session: no CardArt fault-path logs
  (all 22 art soft-loads succeeded), no "Accessed None", no Blueprint runtime errors, no hit-test regressions.

**Pre-existing log entries observed (NOT new, already board-tracked):** DeepMine CardType-2 warning
(TASK-035 watch) when the bot played DeepMine; victory-widget UIOnly focus error at match end (board's known
follow-up list); LogCrowdFollowing RecastNavMesh-at-PIE-boot warning (engine init-order noise, cannot be
caused by a widget edit). Both PIE matches ended in fast Defeats — the board's known bot-rush balance item,
unrelated to this UI task.

**Not directly observable without human input — verified structurally, phase-2/playtest items:**
1. **Grey tint on unaffordable:** cannot fire in an agent PIE (start gold 50 covers every cost ≤12 and only
   the human can spend). Wiring is the SAME `bAffordable` pin that drives the proven `SetIsEnabled` greying,
   through SelectColor A=white / B=(0.35,0.35,0.35,1) — both literals readback-verified. Fires alongside the
   button-disable that Jonathan already playtested in M2.
2. **Null-art collapse:** all 22 cards now have art, so the fallback can't fire naturally; the null path is
   the resolver's (QA-passed) and the BP hide-on-null was readback-verified.
3. **Alt-cursor clickability (WARN-4):** posture preserved by construction (see above); needs a human click
   to re-confirm — same as every prior hand-UI task.

## Flagged decisions (for build-master / next QA)

1. **BuildHandTree was rewritten via write_graph_dsl** (not granular). Justification: the graph was BORN from
   write_graph_dsl in TASK-041, contains zero lossy node types (no GetDataTableRow/delegate-binds — those live
   in the protected WBP_HUD graph and the EventGraph respectively), and the write is atomic (my first attempt
   was REJECTED whole with the graph untouched, proving fail-safe). Post-write readback diffed clean against
   the intended structure; compile clean; PIE clean. The EventGraph — where the real round-trip hazard lives —
   was touched ONLY granularly.
2. **Art image variables are named `Img_CardArt` (array) + `Img_NextCardArt`** — the names-law names, carried
   as variables because runtime-constructed UWidgets can't be named through the MCP construct node.
3. **Slot art is created `Collapsed` and only shown by UpdateSlotArt on a non-null resolve** — pre-first-push
   frames show today's text-only face, never an empty white brush.
4. **Text shadows added** (slot name/cost + preview name/cost) under the ruling's "shadow behind text is
   allowed". The static "Next:" label (12pt) did NOT get a shadow (it isn't stored in any variable; reaching
   it needed a parent-walk cast chain not worth the graph risk). In the capture it still reads fine over the
   art's headroom; if phase-2 PIE finds it washed out, one SetShadowColorAndOpacity on a stored ref is a
   5-minute follow-up.
5. **SelectColor (KismetMathLibrary) with pin-literal colors** instead of two MakeLinearColor nodes — fewer
   nodes, values verified by get_pin_value.
6. **`Appearance|SetBrushfromTexture` / `Appearance|SetColorandOpacity` DSL ids are class-ambiguous** (the DSL
   resolver bound UBorder's SetBrushFromTexture first — pin-count/type error). Worked around with granular
   `create_node` + `declaring_class=/Script/UMG.Image`. **MCP tooling law for future UMG tasks.**
7. **bMatchSize stays false** (the pin's default) on both SetBrushFromTexture nodes — deliberate: matching
   would inflate the face's desired size to 512×512 and balloon the hand layout. The image FILLS the overlay
   via slot alignment instead (art stretches 1:1 — the textures are square and full-bleed per TASK-078).
8. **One new inert `OnClicked_Event_16` stub appeared** in the EventGraph — spawned by a widget compile's
   AssignOnClicked name-collision mechanism (12 "Name is already in use - CustomEvent" LogBlueprint warnings
   per full compile; same cosmetic class as the 13 stubs TASK-041 documented). Unbound, unreachable, harmless;
   M7 sweep fodder. Expect the same 12 warnings if TASK-081 recompiles the widget — compile-time noise, not
   an asset fault.

## Notes for TASK-081 phase 2

- **Editor UI popup pending:** an auto-import toast "22 changes to source content files detected — Import?"
  is sitting in the editor (the TASK-077 PNGs under Content/RawAssets/CardArt/ tripped the source-content
  monitor). **Do NOT click Import** — that would generate unwanted /Game/RawAssets/ textures. Dismiss with
  "Don't Import" (human hand or ignore; it does not affect PIE or saves — both PIE runs and all saves
  completed cleanly with it up).
- Working tree: the editor auto-staged WBP_CardHand.uasset like every editor session — adjudicate at the
  single phase-2 commit per the board.
- Phase-2 PIE spot-checks that need a human/mouse: click-play a card with Alt-cursor (WARN-4), watch the
  played slot's art swap on redraw, spend below a cost and confirm the art greys WITH the button, discard
  refusal at 0 gold still overlays readably (refusal text got no art behind it — untouched center overlay).
- The two fast Defeats + the known DeepMine/focus-error/log lines above are pre-existing observations for the
  record, not new findings.

## MCP invocation notes (repeat of TASK-041's + new)

- `mcp__unreal-mcp__call_tool` shape: `{tool_name, toolset_name, arguments}` — the key is `arguments`.
- New this task: `find_node_types` first, and when an `Appearance|*` id appears more than once in its output,
  assume class ambiguity and go granular `create_node` with `declaring_class` (decision 6).
- `write_graph_dsl` member-function calls: extra INPUT params must be passed as `:PinName value` keywords —
  a positional arg binds to the `self` pin and type-errors (my first BuildHandTree write rejection).
- Variable accessor DSL ids strip underscores: variable `Img_CardArt` → `Variables|Default|GetImgCardArt`.
