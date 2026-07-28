# TASK-345 handoff — [CMD-editor] IA_CmdAmbush + F mapping + WBP_HUD prompt bind (+ optional StageTint)

**Status:** ready-for-integration (all three required deliverables + the optional material step DONE) · **Assignee:** art-director · **Date:** 2026-07-27
**Session:** hosted inside TASK-346's editor session (PID 6460, MCP live) per ruling 11 — TASK-344 C++ compiled in before this task started. Editor NOT closed, handed back to build-master for the phase-B PIE matrix. **No Git, no TASKBOARD edit** (TASK-346 owns the single feature commit).
**Law:** CONVENTIONS "Group orders — 3-zone HOLD + AMBUSH" (input-asset clause) · "Material & Niagara lane laws" (stock nodes) · the TASK-276 corrected HUD recipe (EventGraph-only assign, additive-only).

---

## 1. `/Game/Input/Actions/IA_CmdAmbush` — CREATED
- Class `InputAction` (EnhancedInput), created via DataAsset create at `/Game/Input/Actions/`.
- Readback matches IA_CmdAttack byte-for-byte on every relevant property: `ValueType=Boolean` (Digital/bool), `bConsumeInput=true`, `bTriggerWhenPaused=false`, `bReserveAllMappings=false`, `Triggers=[]`, `Modifiers=[]`.
- Saved (`is_dirty=false`). Matches the TASK-344 soft path contract `/Game/Input/Actions/IA_CmdAmbush` character-for-character.

## 2. `/Game/Input/IMC_Hero` — F mapping ADDED
**F-CONFLICT CHECK (the UE 5.8 trap, done first): F is UNMAPPED — clear.**
- Read `defaultKeyMappings.mappings` (the REAL array), NOT the legacy top-level `Mappings` (confirmed present and `[]` — the trap array, ignored).
- Pre-edit inventory (21 entries): SpaceBar(Jump) · W/S/A/D(Move, with instanced Swizzle/Negate modifiers) · Mouse2D(Look, Negate) · LeftShift(Sprint) · LMB(Attack) · One–Six(Card1-6) · RMB+Escape(CancelPlace) · LeftAlt(UICursor) · Q(Rally) · T(CmdAttack) · R(CmdHold) · E(CmdDefend). **No F anywhere** — no flag needed, nothing stomped.
- Appended entry 22: `action=/Game/Input/Actions/IA_CmdAmbush`, `key=F`, no triggers/modifiers, `settingBehavior=InheritSettingsFromAction` — the exact shape of the T/R/E entries.
- Post-edit readback: 22 entries; all 21 originals byte-identical INCLUDING the instanced modifier sub-object refs (`InputModifierSwizzleAxis_0/1`, `InputModifierNegate_0/1/2` — WASD/look unharmed); legacy `mappings` still `[]`. Saved (`is_dirty=false`).

## 3. `/Game/UI/WBP_HUD` — additive OnCommandPromptChanged bind
**ADDITIVE ONLY — granular create_node/connect_pins, zero write_graph_dsl, zero deletions, no designer-tree change, no reparenting.** The fragile EventTick first-frame Construct block, the stance switch, and the existing `OnUnitCommandChanged_Event_0 → UpdateCommandDisplay` chain are byte-untouched (readback-verified via connected-subgraph walk).

Appended at the TAIL of the init chain (after `AssignOnUnitCommandChanged`, whose `then` was the unconnected chain end):
```
... CastToSiegePlayerController(GetOwningPlayer)   [existing K2Node_DynamicCast_9 — REUSED, not duplicated]
    -> AssignOnUnitCommandChanged                  [existing K2Node_AssignDelegate_7]
    -> AssignOnCommandPromptChanged                [NEW K2Node_AssignDelegate_0; self = the SAME cast output pin (fan-out)]
         -- Delegate --> [Custom Event OnCommandPromptChanged_Event (Prompt: String)]   [auto-spawned by the Assign]
                           -> Branch( IsEmpty(Prompt) )
                                True  (empty)      -> UpdateCommandDisplay()            [existing function — stance fallback]
                                False (non-empty)  -> SetText(Text) on CommandIndicatorText, InText = ToText(String)(Prompt)
```
- New nodes (exact): `K2Node_AssignDelegate_0` (`Siegebound|Commands|AssignOnCommandPromptChanged`) · `K2Node_CustomEvent_0` (`OnCommandPromptChanged_Event`, auto) · `K2Node_CallFunction_0` (`Utilities|String|IsEmpty`) · `K2Node_IfThenElse_1` (Branch) · `K2Node_CallFunction_1` (`UpdateCommandDisplay`) · `K2Node_VariableGet_0` (`GetCommandIndicatorText`) · `K2Node_CallFunction_2` (`ToText(String)`) · `K2Node_CallFunction_3` (`Widget|SetText(Text)`).
- The AssignOnX auto-rename watch: the auto handler kept the name `OnCommandPromptChanged_Event` through compile; ALL pins re-verified post-compile by readback (OutputDelegate→Assign.Delegate, then→Branch, Prompt→IsEmpty+ToText, IsEmpty→Condition, True→UpdateCommandDisplay, False→SetText, Get→Target, ToText→InText) — nothing drifted.
- Compiled `warnings_as_errors=true` → CLEAN. Saved (`is_dirty=false`).
- Semantics honored: non-empty prompt overwrites the indicator text; empty prompt (pick over / groups released) re-reads the stance getters via the existing `UpdateCommandDisplay` — the TASK-276 re-read-not-trust-the-param law carries over.

## 4. OPTIONAL — `M_SpellReticle` StageTint: DONE (no wedge)
- Stock nodes ONLY: `MaterialExpressionVectorParameter` named exactly **`StageTint`** (DefaultValue 1,1,1,1) + one `MaterialExpressionMultiply`.
- Wiring: existing `Multiply_0` (ring × color, the old Emissive source) → `Multiply_1.A`; `StageTint` → `Multiply_1.B`; `Multiply_1` → **EmissiveColor**. **Opacity untouched** (still `Subtract_0` — tint can never change the ring alpha). Identity at default ⇒ visually byte-identical until the C++ MID pushes a tint.
- Recompile: instant, clean (the tool raises on shader failure — none), editor responsive throughout, `Failed to compile Material` log grep = 0. Saved (`is_dirty=false`).
- Parameter name matches the TASK-344 MID contract (`SpawnGroupCircleDecal` cpp:2494-2508) character-for-character: the stage tints (Select white / Position green / Attack red) go LIVE in phase B.

## Message Log / QA watch — CLEAN
- `GetHoldLocation|GetHoldRadius` session-log grep: **0 hits** — no BP compiled-on-load bound the deleted getters (WBP_HUD included).
- `Ensure condition failed|Accessed None|Fatal error` grep: 0. `Failed to compile Material` grep: 0.
- LogBlueprint shows only this task's own WBP_HUD compile, no errors/warnings.

## Saved set (exactly the authored assets — nothing else)
1. `/Game/Input/Actions/IA_CmdAmbush` (NEW → `Content/Input/Actions/IA_CmdAmbush.uasset`)
2. `/Game/Input/IMC_Hero` (edited)
3. `/Game/UI/WBP_HUD` (edited)
4. `/Game/Materials/M_SpellReticle` (edited, optional step)

`/Game/Maps/L_Arena`: NEVER saved, `is_dirty=false` confirmed after all work. No PIE was started by this task.

## Notes for build-master (phase B)
- PIE runtime verification deliberately left to the phase-B matrix (per spec): F opens the AMBUSH pick, prompts appear/clear on the HUD, stage tints white/green/red on the circles, stance fallback on empty prompt.
- The four saved assets above are this task's commit set members for the ONE feature commit (TASK-346).
- The HUD prompt shares `CommandIndicatorText` with the stance display by design (prompt overwrites, empty restores stance) — the on-screen pixel check remains owed to Jonathan's eye per the project's UMG lesson.
