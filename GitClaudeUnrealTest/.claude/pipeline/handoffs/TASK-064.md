# TASK-064 Handoff — HUD v4: hero-upgrade icon row + stack pips (editor/MCP)

- author: gameplay-programmer
- date: 2026-07-05 (M4 editor wave 4/4 — LAST M4 editor task before TASK-069 final verify)
- status: **complete / ready-for-qa.** Additive hero-upgrade row built + seed-then-bound on `/Game/UI/WBP_HUD`.
  Gold Construct + Rally indicator confirmed byte-intact by full EventGraph readback. Blueprint compiles clean;
  full-match PIE boot in L_Arena with ZERO runtime errors. **Editor left UP (PID 6172, PIE stopped, WBP_HUD saved
  / is_dirty=false) for TASK-069.**

## What was added (ALL additive to WBP_HUD — protected M1 gold Construct NEVER round-tripped)

Model followed TASK-050's Rally add exactly: **bulk logic in NEW function graphs** (fresh-graph `write_graph_dsl`,
safe) + a **minimal granular append** to the EventGraph Tick do-once. **Zero `write_graph_dsl` on the EventGraph**
(the M1 gold Construct's `GetDataTableRowDT_Cards` is the known-lossy node per TASK-033/050 — untouched).

**4 new member vars** (TextBlock object refs, one entry per upgrade): `BladeEntry`, `PlateEntry`, `BootsEntry`,
`BannerEntry`.

**3 new function graphs:**
- **`UpdateUpgradeEntry(Entry: TextBlock, Label: string, Current: int, Cap: int)`** — the per-entry renderer.
  `Current <= 0` → `SetVisibility(Collapsed)` (inactive upgrades hidden ⇒ "one entry per ACTIVE upgrade"); else
  sets text `"<Label> <Current>/<Cap>"` (two nested `BuildString(Integer)` nodes → e.g. `"Blade 2/2"`,
  `"Boots 1/1"`) and `SetVisibility(HitTestInvisible)`.
- **`UpdateUpgradeRow(Blade, Plate, Boots, Banner: int)`** — the delegate-shaped updater (signature matches
  `FOnHeroUpgradesChanged`). Casts `GetOwningPlayerPawn`→`HeroCharacter`, then calls `UpdateUpgradeEntry` ×4,
  pulling each cap from **`GetUpgradeStackCap(CardID)`** (SharpenedBlade / PlateArmor / SwiftBoots / WarBanner —
  DT_Cards `MaxCopies`, never guessed, per CONVENTIONS). `:CastFailed` fallback uses `Cap=Current` so pips still
  render if the pawn isn't a hero.
- **`SetupUpgradeRow()`** — creates the 4 TextBlocks (font 18) + a `VerticalBox`, `AddChild`s them, attaches the
  VBox to the **root Overlay top-left, padding (24, 56)** — i.e. BELOW the gold counter (gold is 24,12) and clear
  of the Rally indicator (top-right). **Seeds** by calling `UpdateUpgradeRow` from the hero's CURRENT stacks
  (`GetSharpenedBladeStacks`/`GetPlateArmorStacks`/`GetSwiftBootsStacks`/`GetWarBannerStacks`). Overlay cast
  `:CastFailed` logs `"WBP_HUD: root Overlay not found; upgrade row not attached"`.
  - **Nesting note (avoids the TASK-050 "unreachable code after branch" trap):** the DSL treats casts as
    multi-exec nodes that terminate the enclosing flow. The seed hero-cast is therefore NESTED inside the
    overlay-attach cast's `:then` (not a sibling), so both run and neither is unreachable.

**EventGraph Tick do-once EXTENDED** (granular `create_node`/`connect_pins`, appended to the previously-free
`then` exec of the existing Rally `AssignOnRallyStateChanged` node — the Rally node itself is unchanged):
`SetupUpgradeRow()` → `CastToHeroCharacter(GetOwningPlayerPawn)` → `BindEventtoOnHeroUpgradesChanged(hero, <event>)`
where `<event>` is a **CreateEvent (K2Node_CreateDelegate) bound to `UpdateUpgradeRow`**. Runs once (reuses the
existing `bCardHandSpawned` do-once guard — no new guard var).

## Seed-then-bind (CONVENTIONS honored)
`SetupUpgradeRow` seeds the row from the hero's live `Get*Stacks` getters FIRST; the Tick chain then binds
`FOnHeroUpgradesChanged` AFTER (SetupUpgradeRow precedes BindEvent in the exec order). The row is never a stale
bind-only widget. At a fresh match all stacks are 0 ⇒ all entries collapse ⇒ empty row (correct initial state).

## Delegate binding — bound directly to a FUNCTION (no custom event)
`FOnHeroUpgradesChanged` (4×int32, MilitiaMob-friendly, no enums) is bound via a **Create Event → `UpdateUpgradeRow`**
delegate rather than a red custom-event handler. `get_create_event_function` confirms the binding = `UpdateUpgradeRow`.
Chosen deliberately (see "MCP lesson" below). On every `ApplyUpgrade`(Applied), `ResetHero` (respawn re-broadcast),
and `ResetUpgrades` (Play Again → all 0s), the hero broadcasts → `UpdateUpgradeRow` repaints the row.

## Acceptance mapping
- **play upgrade → icon+pip updates:** binding graph delivered + boot-verified; the LIVE "play SharpenedBlade →
  'Blade 1/2' appears" is TASK-069/Jonathan (MCP cannot inject a card play).
- **2-stack shows 2 pips (structural):** a 2-stack SharpenedBlade renders `"Blade 2/2"` (current 2 / cap 2).
  See "pip representation" caveat.
- **row clears on Play Again:** `ResetUpgrades` broadcasts (0,0,0,0) → all entries `Current==0` → all Collapsed →
  empty row. Structurally guaranteed.
- **gold counter + M2/M3 HUD + Rally indicator unchanged:** confirmed by full EventGraph readback (below).

## Verification
- **Structural readback (`read_graph_dsl`):** EventGraph `EventConstruct` is IDENTICAL to pre-task (gold text
  create, `SetGoldText`, `UpdateGoldDisplay`, `AssignOnGoldChanged`, `AssignOnClicked`, overlay attach
  `MakeMargin 24,12`, all continuations) — gold Construct byte-intact. Rally chain (`SetupRallyIndicator` →
  `AssignOnRallyStateChanged` → `OnRallyStateChanged_Event_0` → `UpdateRallyDisplay`) unchanged. All 3 new function
  graphs read back correct.
- **`compile_blueprint`:** clean (null).
- **PIE boot (L_Arena, in-viewport, 4 s warmup):** ZERO runtime errors — no "Accessed None", no "Blueprint Runtime
  Error", and **no "upgrade row not attached"** (⇒ the overlay cast succeeded and the VBox attached, and the hero
  cast succeeded so the delegate bound). A full match ran healthy — `LogSiegeBot` shows Set II play + Rule-4
  discards of the upgrade cards **`SharpenedBlade` and `WarBanner`** (confirms the upgrade cards are in the deck +
  reachable). PIE then `StopPIE`'d cleanly (`IsPIERunning=false`).
- **Interactive check is TASK-069/Jonathan** (MCP has no keypress/card-play injection): live "play upgrade → pip
  appears / stacks to 2 / clears on Play Again". The wiring + seed + structure are delivered and boot-verified.

## MCP-stability outcome (headline)
- **SURVIVED end-to-end.** ~60 MCP calls: `add_object_variable`×4, `add_function_graph`×3, param adds, 3×
  `write_graph_dsl` (fresh function graphs), granular `create_node`/`connect_pins`/`delete_node`/`get_node_infos`
  on the EventGraph, `set_create_event_function`, 2× `compile_blueprint`, `save_assets`, and a full StartPIE →
  StopPIE cycle. Transport verified alive at the end (`list_toolsets` full schemas). Discipline held: small
  batches, save + health-ping between phases, stop-on-first-failure, gold Construct never round-tripped.

## MCP lesson — `get_node_type_pins` creates PHANTOM nodes → ICE (do not repeat)
Calling `get_node_type_pins` on `Siegebound|Hero|AssignOnHeroUpgradesChanged` **spawned a persisted but malformed
`K2Node_AssignDelegate_6` + auto `K2Node_CustomEvent_25`** in the EventGraph. Wiring those and compiling produced
an **`ICE SetVariableOnPersistentFrame - No property found`** cascade (the phantom custom-event's param
"Sharpened Blade Stacks" wasn't registered in the ubergraph persistent frame, poisoning the frame so even the
pre-existing Gold/Tick/Rally event params failed). **Fix:** deleted the phantom nodes and rebuilt the bind
deterministically with real `create_node` nodes — a plain **`BindEventtoOnHeroUpgradesChanged` (no auto event) +
`CreateEvent` bound to `UpdateUpgradeRow`**. Recompiled clean. One clean atomic failure, no hammering (TASK-050
discipline). Prefer `create_node` + `find_node_types` for introspection; avoid `get_node_type_pins` on delegate/
cast node types.

## Stale warnings — DO NOT CHASE (not current state)
All from rejected/intermediate edit states before the fix; none recur at the final compile or PIE:
- `00.31.35 ... [Compiler] ICE SetVariableOnPersistentFrame - No property found. {New Gold|Ready|My Geometry|
  Sharpened Blade Stacks}` — the phantom-node compile described above (superseded; final compile is clean).
- `00.34.39 LogScript: Warning: "UpdateUpgradeRow" is not a compatible function. Valid functions: []` — the first
  `set_create_event_function` BEFORE I connected the CreateEvent OutputDelegate to the bind's Delegate pin (order
  requirement); the retry after connecting succeeded.
- The benign `Missing RowStruct` log (per the task) — not chased.

## Cosmetic cruft (harmless, M7 sweep) — do not let it confuse QA
Empty unbound stub custom events accumulated from node churn: `OnRallyStateChanged_Event_1/_2`,
`OnGoldChanged_Event_6/_7`, `OnClicked_Event_6/_7` (plus the TASK-050-documented `_0.._5` set). All empty bodies,
nothing binds them, compile clean. Same class of cruft TASK-050/033 documented.

## For QA to scrutinize
1. **Pip representation:** entries render numeric **`<Label> <Current>/<Cap>`** text (e.g. "Blade 2/2") — a
   blockout-tier text label per the task ("text/labels like 'Blade x2' acceptable"). Literal circular pip GLYPHS
   are M7 visual polish; the DATA path (current/cap driven by live stacks + `GetUpgradeStackCap`) + bindings are
   complete. If QA requires glyph pips now, flag it — it's a small `UpdateUpgradeEntry` string change.
2. **Caps from DT_Cards:** `UpdateUpgradeRow` reads `GetUpgradeStackCap(CardID)` (MaxCopies) — never guessed.
   CardIDs used exactly per `names:` — `SharpenedBlade`, `PlateArmor`, `SwiftBoots`, `WarBanner`.
3. **Gold Construct byte-intact** claim — EventGraph readback in-session confirms it (zero `write_graph_dsl` on
   EventGraph; granular append only onto the Rally node's free `then`).
4. **Row self-contained + additive** so the deferred TASK-041 visual-hand pass can layer later; Rally indicator +
   gold counter untouched.

## Files touched
- `/Game/UI/WBP_HUD.uasset` — modified + saved (is_dirty=false on disk). Only disk change under Content/UI/.
- `.claude/pipeline/handoffs/TASK-064.md` — this file.
- **NOT touched:** the M1 gold Construct (byte-intact), the M3 Rally indicator, WBP_CardHand, L_Arena (loaded/PIE'd
  only), C++, Git, TASKBOARD.md.
- Auto-staged `.uasset` left for TASK-069 to commit (no Git run per constraints). Editor left UP for TASK-069.

## MCP-blocked items for a manual/TASK-069 pass
- Live "play upgrade → pip appears / stacks to 2 / clears on Play Again" — MCP cannot inject a card play; verify in
  TASK-069/Jonathan's playtest.
- Optional M7 polish: glyph pips + premium upgrade icons (deferred, not in scope here).
