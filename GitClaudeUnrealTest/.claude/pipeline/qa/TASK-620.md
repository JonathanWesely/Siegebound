# QA Report — TASK-620 (Lighting Wave 1: D1+D2+D3) — reviewed under TASK-621
Verdict: **PASS** — 0 blockers, 2 WARN, 1 NIT

Reviewer: qa-reviewer · Date: 2026-08-17 · Inputs: `handoffs/TASK-620-programmer.md` · `Config/DefaultEngine.ini` (worktree bytes) · `Content/Blueprints/BP_Torch.uasset` (worktree, name-table probes) · baselines `handoffs/TASK-616-programmer.md` + `handoffs/TASK-617-buildmaster.md` §4 (lane D) · TASKBOARD TASK-620/621 specs · CR-R3 · orchestrator-relayed git snapshot.

## Scope of this gate — stated per dispatch item (4)
**This gate is DIFFS AND LEDGER ONLY. I cannot verify pixels.** The two ini keys are read at engine boot — the running editor still renders 0.8/0.8, and the ×7.5 torch change has no capture here. Pixel acceptance is TASK-622's lane and REQUIRES an editor booted AFTER the ini edit (the handoff's §2 boot-law note is present and correct). A PASS here certifies the diffs match the CR-R3 contract, not that the lighting reads well.

## (a) ini diff — VERIFIED AT THE BYTES
- `Config/DefaultEngine.ini:90` = `r.DefaultFeature.LocalExposure.HighlightContrastScale=0.700000`; `:91` = `...ShadowContrastScale=0.650000` — the exact spec values (0.8→0.70 / 0.8→0.65), six-decimal style preserved, at the exact `:90-91` region the spec cites.
- These are the ONLY LocalExposure keys in `DefaultEngine.ini` (full-file grep). The surrounding AutoExposure block `:86-89` matches TASK-616 §4's pre-change quote verbatim; downstream line anchors undisturbed (`r.DefaultFeature.LightUnits=1` still at `:97`) ⇒ no insertions/deletions rode the edit.
- The `DefaultEditor.ini:11-13` LocalExposure grep hits are pre-existing `[/Script/AdvancedPreviewScene.SharedProfiles]` editor preview-profile blobs — not project render settings, not part of this change.

### ⚖️ RULING (dispatch item 1) — key-name equivalence ACCEPTED, not a deviation
The spec's `r.LocalExposure.ShadowContrastScale`/`...HighlightContrastScale` shorthand names no key that exists in the file. The serialized project keys (URendererSettings) carry the `r.DefaultFeature.` prefix; the spec pins "the `:90-91` region" and its mandatory READ-FIRST (TASK-616 §4) quotes the `r.DefaultFeature.LocalExposure.*` keys verbatim at 0.800000. Intent is unambiguous. Editing the keys that exist was correct; adding literal `r.LocalExposure.*` lines would have been the actual deviation (new keys, the live 0.8 pair left standing). Programmer's flag was proper disclosure.

## (b) BP_Torch readback — INTERNALLY CONSISTENT + independently corroborated file-side
- BEFORE readback == the TASK-616/617 baseline exactly: 12 cd / 1,200 / linear (1, 0.72000003, 0.41999999) / offset (50, 0, 75). AFTER (post-save): 90 / 1,900, color and offset byte-identical to BEFORE. Exactly the two specced dials; ⛔ offset fence held.
- **Independent W8-R3 name-table probe on the worktree uasset (my own read, not the handoff's):** `TorchIntensity` PRESENT and `TorchAttenuationRadius` PRESENT (both ABSENT in the TASK-616 §3 baseline scan ⇒ new serialized deltas exist on disk); `TorchLightColor` still ABSENT (no color delta was written); `TorchLightRelativeOffset` PRESENT (the pre-existing TASK-556 override, expected). This is exactly the signature of "two properties and only two gained overrides."
- Dirty/save ledger coherent: BP dirty→explicit single-path save→clean; `L_Arena` dirty **false** at every checkpoint. No `compile_blueprint` — correct: a CDO data-default delta needs no graph recompile, and the wave is zero-compile (CR-R4).
- §4 expected-effect arithmetic re-derived and checks: 90/12 = ×7.5; nadir E = 90/8.55² = 1.23 lux at the live 855 height; pool radius √(1900²−855²) = 1,696.8 ≈ 1,697 (was 842); wall 48→360 lux at 0.5 m.

## (c) fences — COMPLIANT on everything file-checkable
- **PPV untouched (CR-R3):** handoff declares neither read nor written; the PPV lives on `L_Arena`, whose SHA256 entry==exit `B3DBC5D9AE484A7BD02CAFAD52B4681DA68B011477479B65EE7781AE459F8268` matches TASK-617's independently recorded entry/exit pair verbatim, and the dirty ledger reads false throughout. Nothing file-side contradicts.
- No C++ diff claimed or found uncommitted; no compile artifact claimed; no console/`M` (the TASK-571+552 latch untouched); TASKBOARD untouched by the programmer (orchestrator flipped).
- **(d) Scope G3:** two BP class defaults + two config values, no new lights, no relight — dials, as ruled.

## Findings
- **[WARN-1]** Git lane (dispatch item 2): the worktree is **CLEAN at HEAD `67b30ab` ("adding account save stuff")** — the TASK-620 artifacts (2 ini lines, BP_Torch.uasset, the handoff) are already inside committed history, evidently swept into Jonathan's own commit (the standing self-commit precedent; the programmer's "no git" declaration stands — this is not a programmer violation, and the owner's own commit is not QA-gated). Consequence: the requested worktree `git diff` isolation proof cannot exist (nothing is uncommitted), and I hold no git tool by design. **Delegated to TASK-622's committer:** diff `f9f0b72..67b30ab` and partition the lighting lane (exactly ±2 lines in `Config/DefaultEngine.ini`; `Content/Blueprints/BP_Torch.uasset` the only lighting-lane Content change) from Jonathan's account work riding the same commit; **check-and-carry, do NOT cut a duplicate lighting commit** if `67b30ab` already carries the files — this report (`qa/TASK-620.md`) is now the residual uncommitted artifact to stage.
- **[WARN-2]** Hashes are the programmer's measurements, unverified by me (no shell): BP_Torch 24,445 B / SHA256 `451924C9B59B5D8EAACCCB737EEF4EF47E06EAB3E4CFBD0914A19A5DC4A17AA0`, and the L_Arena pair. Mitigations accepted: the L_Arena pair matches TASK-617's independent record byte-for-byte, and my own name-table probes confirm the uasset's new deltas. The `§25b` LFS-oid-vs-worktree-sha256 check stays owed by TASK-622 as already specced.
- **[NIT-1]** Record note, no action: the spec's "~1.5 lux nadir" figure uses the paper height (780); the live 855 height gives 1.23 lux — the handoff declares this itself. TASK-622's acceptance anchor is the ×2 tonemapped floor mean (36, 31, 24), which is unaffected.

## Notes for build-master (TASK-622)
1. **Boot law:** capture session MUST be from an editor booted after the ini edit (batch with 619's bounce where possible; Jonathan closes/relaunches himself). The current running editor renders the OLD 0.8/0.8.
2. Live readback expectations: 12 spawned torches (6+6, class `BP_Torch_C`) at Intensity **90** / Attenuation **1,900** / color unchanged (sRGB (1.0, 0.8667, 0.6784)); cvars `r.DefaultFeature.LocalExposure.HighlightContrastScale`=0.70, `...ShadowContrastScale`=0.65.
3. Commit step: see WARN-1 — verify HEAD first, partition `f9f0b72..67b30ab`, no duplicate commit, stage this report + the 621 board flip; wave-record check-and-carry per CF-R1.
4. Watch item from the handoff's own frame: ×7.5 warm wall hotspots near mounts (~360 lux at 0.5 m) — covered by the "no NEW blowout + pools still warm R/B > 1.3" gates.
