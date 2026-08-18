# TASK-620 — [CR-3] LIGHTING WAVE 1 — D1+D2+D3 dials landed (gameplay-programmer handoff)

Date: 2026-08-17 · Status requested: ready-for-qa (board flip left to the orchestrator per dispatch — I did not edit TASKBOARD.md) · ZERO compile · PPV untouched (CR-R3) · L_Arena untouched (hash lane §3).

Scope executed exactly as specced: **(1)** BP_Torch class defaults via MCP — `TorchIntensity` 12 → **90** cd, `TorchAttenuationRadius` 1200 → **1900**; BP_Torch.uasset saved (the one legal save). **(2)** `Config/DefaultEngine.ini` — the two LocalExposure contrast scales. Nothing else was written anywhere.

---

## 1. D1+D2 — BP_Torch class defaults (MCP session, property-scoped)

Editor: Jonathan's running session, MCP `http://127.0.0.1:8000/mcp` green. Session was short and property-scoped: no level open/close, no PIE/SIE, no viewport touch, no console/`M`/DumpAssistantPrompt (CF-R3), no save prompt for anything but the explicit BP_Torch save (no map-save prompt ever appeared; none would have been accepted).

Surface: `ObjectTools.get_properties`/`set_properties` against `/Game/Blueprints/BP_Torch.BP_Torch` (the toolset resolves the Blueprint ref to the CDO automatically — the same `BP_Torch_C` default surface TASK-617 D3 verified is the spawned class). Save: `AssetTools.save_assets(["/Game/Blueprints/BP_Torch"])` — explicit single path, never the save-all form.

**BEFORE readback (verbatim MCP return):**
```json
{"TorchIntensity":12,"TorchAttenuationRadius":1200,"TorchLightColor":{"r":1,"g":0.72000002861022949,"b":0.41999998688697815,"a":1},"TorchLightRelativeOffset":{"x":50,"y":0,"z":75}}
```
— matches the TASK-616/617 baseline exactly (12 cd / 1,200 / warm (1, 0.72, 0.42) / the ⛔ offset (50, 0, 75)).

**Write:** `set_properties` with exactly `{"TorchIntensity": 90.0, "TorchAttenuationRadius": 1900.0}` → returned true.

**AFTER readback (verbatim MCP return, taken AFTER the save):**
```json
{"TorchIntensity":90,"TorchAttenuationRadius":1900,"TorchLightColor":{"r":1,"g":0.72000002861022949,"b":0.41999998688697815,"a":1},"TorchLightRelativeOffset":{"x":50,"y":0,"z":75}}
```
— the two dials applied; **`TorchLightColor` and `TorchLightRelativeOffset` byte-identical to before** (the offset fence held).

**Dirty/save ledger (chronological):**
| check | value |
|---|---|
| `is_dirty(/Game/Blueprints/BP_Torch)` post-write | true |
| `is_dirty(/Game/Maps/L_Arena)` post-write | **false** |
| `save_assets(["/Game/Blueprints/BP_Torch"])` | true |
| `is_dirty(/Game/Blueprints/BP_Torch)` post-save | false |
| `is_dirty(/Game/Maps/L_Arena)` at session exit | **false** |

No `compile_blueprint` was called — a CDO default-value change needs no graph recompile, and the fence is zero-compile anyway.

## 2. D3 — Config/DefaultEngine.ini (the `:90-91` region)

**Key-name note for QA:** the spec's shorthand `r.LocalExposure.ShadowContrastScale` / `r.LocalExposure.HighlightContrastScale` are the runtime cvars; the serialized project keys at DefaultEngine.ini:90-91 (the exact region the spec cites, verified before editing) carry the `r.DefaultFeature.` prefix — the same two keys TASK-616 §4 quoted. Those are what I changed; there are no other LocalExposure keys in the file.

Diff (the ONLY two changed lines in the file; the surrounding AutoExposure block :86-89 untouched):
```diff
-r.DefaultFeature.LocalExposure.HighlightContrastScale=0.800000
-r.DefaultFeature.LocalExposure.ShadowContrastScale=0.800000
+r.DefaultFeature.LocalExposure.HighlightContrastScale=0.700000
+r.DefaultFeature.LocalExposure.ShadowContrastScale=0.650000
```
(Shadow 0.8 → **0.65**, Highlight 0.8 → **0.70**, per spec; six-decimal style preserved. 617 D1 confirmed the PPV serializes NO LocalExposure override, so the config dial is PPV-proof.)

⚠️ **Boot law for TASK-622:** these keys are read at engine boot — the running editor still renders 0.8/0.8. The 622 capture session MUST be from an editor booted after this edit (batch with 619's fresh-load bounce where possible; Jonathan closes/relaunches himself, standing law).

## 3. THE HASH LANE (CF-R4)

| artifact | value |
|---|---|
| `Content/Maps/L_Arena.umap` SHA256 **ENTRY** (before any MCP touch) | `B3DBC5D9AE484A7BD02CAFAD52B4681DA68B011477479B65EE7781AE459F8268` |
| `Content/Maps/L_Arena.umap` SHA256 **EXIT** (after session + save) | `B3DBC5D9AE484A7BD02CAFAD52B4681DA68B011477479B65EE7781AE459F8268` |
| verdict | **BYTE-IDENTICAL** (and identical to TASK-617's recorded entry/exit pair) |
| `Content/Blueprints/BP_Torch.uasset` before | 24,333 bytes, mod 2026-08-15 (TASK-616 §3 record) |
| `Content/Blueprints/BP_Torch.uasset` after | **24,445 bytes**, mod 2026-08-17 14:09:56, SHA256 `451924C9B59B5D8EAACCCB737EEF4EF47E06EAB3E4CFBD0914A19A5DC4A17AA0` |

## 4. EXPECTED EFFECT (for 622's acceptance frame)

- Intensity ×7.5. At the LIVE pool-centre height 855 uu (617's amended offset): floor nadir E = 90/8.55² ≈ **1.23 lux** (spec's ~1.5 figure uses the paper height); ×~2 in pool-overlap zones — up from 0.17–0.4 lux (moonlight) toward dim-interior readable.
- Floor pool radius √(1900² − 855²) ≈ **1,697 uu** (was 842) — inter-pool valleys close; the corridor's single-torch coverage hole shrinks accordingly (not eliminated — the 7th anchor stays Wave 2).
- LocalExposure at 0.65/0.70 lifts shadows + compresses highlights in-frame — attacks "black floor beside blown walls" without adding lumens. The blown faces are SUN on pale albedo (617 D5), so torch-side changes don't feed them; watch for the ×7.5 warm wall hotspots near mounts (48 → ~360 lux at 0.5 m) — 622's "pools still warm R/B > 1.3, no NEW blowout" gates cover this.

## 5. FENCES HONOURED / FILES TOUCHED

- **Touched (complete list):** `Content/Blueprints/BP_Torch.uasset` (via editor save) · `Config/DefaultEngine.ini` (two values) · this handoff.
- **Not touched:** the PostProcessVolume (CR-R3 — no read, no write, the exposure lock Min=Max=1.0 Bias 0.4 stands for 622's baseline) · `L_Arena` (hash-proven) · `TorchLightRelativeOffset` · any C++ · TASKBOARD.md (per dispatch — orchestrator flips to ready-for-qa) · git · no compile of any kind · no console/`M` (the TASK-571+552 latch untouched by construction).

**QA (TASK-621) should scrutinize:** (a) `git diff Config/DefaultEngine.ini` = exactly the two value lines above; (b) the worktree BP_Torch.uasset LFS diff is the session's save and nothing else rides it (new SHA256 in §3 for the `§25b` oid check); (c) the §1 AFTER readback shows colour/offset unchanged; (d) the key-name note in §2 — the spec's cvar shorthand vs the serialized `r.DefaultFeature.` keys is a naming equivalence, not a deviation.
