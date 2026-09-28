Verdict: MEASURED
# Verification — TASK-1404
Editor/Aura state: connected y (Aura `unreal_inspector` + `unreal_editor` both answered); map `/Game/Maps/L_MainMenu` (editor world, not PIE); PIE mode: **none — PIE was NOT spent** (`is_pie_active` → `is_active: false` before the reads; no `start_pie` issued; the row forbids one); editor instance identified per `SC-§118` cl. 9 substitute census (no shell held): in-process `os.getpid()` = **3108**, `sys.executable` = `C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe` (the GUI editor, not a `-game` instance), `LogInit: Command Line:` empty (project argument only), dirty packages `[]`; attempts used 1 of 3; wall time a few minutes; credit not visible. PIE announcement: the dispatch said *"⛔ NO PIE: the row forbids spending one, so no announcement is owed."* — none was owed and none was made; no PIE ran (`VER-3-6-THE-REPORT-RECORDS-THE-ANNOUNCEMENT`). model (self-reported): "Opus 5.5 (1M context)", model ID "claude-opus-5-5[1m]".

**Outcome: (a).** (i) rows 0–5 are cell-for-cell identical to `TASK-1274`'s table, (ii) rows 6–13 are exactly the `TASK-1408` and `TASK-1508` additions by name, (iii) the count is exactly 14. Every row has `trg=0 mod=0`. **`Enter` → `IA_MenuAccept`: still bound** (row 4). The `TASK-1274` prior stands. The row says this discharges `TASK-1405`'s binding precondition. That ruling belongs to the manager's row, and this report amends nothing.

## Reader control (first)
`get_input_mapping_context_keys` on `/Game/Input/IMC_Hero` → `mapping_count: 28` (non-empty; equals the `TASK-1508` figure). The same reader lists modifiers where they exist (`W`: SwizzleAxis · `S`: SwizzleAxis+Negate · `A`: Negate · `Mouse2D`: Negate). The Python read of `DefaultKeyMappings.Mappings` on `IMC_Hero` → len **28**, totals **trg=0 mod=5**, which agrees with the MCP modifier listing (1+2+1+1 = 5). ⇒ The reader works, and the modifier counter can return non-zero. **Control: PASS.**

## Acceptance lines → observations
| # | acceptance line | observable chosen | observed (quoted values, evidence path) | pass/fail/unobs |
|---|---|---|---|---|
| 1 | Line 1 `Verdict:` byte-literal | this file's line 1 | `Verdict: MEASURED` | measured (control: IMC_Hero 28 rows / mod=5) |
| 2 | Rows 0–5 cell by cell vs `TASK-1274` (action, key, trg=0, mod=0); load-bearing `IA_MenuAccept` ← `Enter` / `Gamepad_FaceButton_Bottom` | `get_input_mapping_context_keys` `/Game/Input/IMC_MainMenu` + `DefaultKeyMappings.Mappings` trg/mod | 6/6 rows identical, same order; row 4 `IA_MenuAccept <- Enter trg=0 mod=0`, row 5 `IA_MenuAccept <- Gamepad_FaceButton_Bottom trg=0 mod=0` (table below) | measured (control: IMC_Hero) |
| 3 | Other rows accounted for BY NAME against TASK-1408 / TASK-1508 | same read | rows 6–11 = `IA_MenuLeft` Left/Gamepad_DPad_Left · `IA_MenuRight` Right/Gamepad_DPad_Right · `IA_MenuBack` Backspace/Gamepad_FaceButton_Right (TASK-1408); rows 12–13 = `IA_MenuSecondary` Home/Gamepad_FaceButton_Top (TASK-1508); none unexplained | measured (control: IMC_Hero) |
| 4 | Count exactly 14 | `mapping_count` and `len(DefaultKeyMappings.Mappings)` | `mapping_count: 14`; Python len **14**; deprecated `Mappings` len 0 (the instrument, as the row predicts; also 0 on IMC_Hero) | measured (control: IMC_Hero) |
| 5 | Both/three tables side by side | this report | printed below | measured (report artefact) |
| 6 | Outcome named by letter | this report | **(a)** | measured |
| 7 | Explicit statement of whether PIE was spent | this report | **no** | measured |
| 8 | `VER-§7` cl. 2 declaration | this report | under "Not examined / limitations" | measured |

Every cell is `measured` and none is `pass`. The row pre-writes outcome (a) as `MEASURED`, which is a confirmation that nothing moved, discriminated by the named control. That makes it the verdict per `VER-§1` cl. 5/5a.

### The three tables (array order as read)
| idx | TASK-1274 prior (6) | expected (14) | read 2026-09-27 (14) |
|---|---|---|---|
| 0 | IA_MenuUp ← Up | IA_MenuUp ← Up | IA_MenuUp ← Up · trg=0 mod=0 |
| 1 | IA_MenuUp ← Gamepad_DPad_Up | IA_MenuUp ← Gamepad_DPad_Up | IA_MenuUp ← Gamepad_DPad_Up · trg=0 mod=0 |
| 2 | IA_MenuDown ← Down | IA_MenuDown ← Down | IA_MenuDown ← Down · trg=0 mod=0 |
| 3 | IA_MenuDown ← Gamepad_DPad_Down | IA_MenuDown ← Gamepad_DPad_Down | IA_MenuDown ← Gamepad_DPad_Down · trg=0 mod=0 |
| 4 | **IA_MenuAccept ← Enter** | **IA_MenuAccept ← Enter** | **IA_MenuAccept ← Enter · trg=0 mod=0** |
| 5 | **IA_MenuAccept ← Gamepad_FaceButton_Bottom** | **IA_MenuAccept ← Gamepad_FaceButton_Bottom** | **IA_MenuAccept ← Gamepad_FaceButton_Bottom · trg=0 mod=0** |
| 6 | — | IA_MenuLeft ← Left (1408) | IA_MenuLeft ← Left · trg=0 mod=0 |
| 7 | — | IA_MenuLeft ← Gamepad_DPad_Left (1408) | IA_MenuLeft ← Gamepad_DPad_Left · trg=0 mod=0 |
| 8 | — | IA_MenuRight ← Right (1408) | IA_MenuRight ← Right · trg=0 mod=0 |
| 9 | — | IA_MenuRight ← Gamepad_DPad_Right (1408) | IA_MenuRight ← Gamepad_DPad_Right · trg=0 mod=0 |
| 10 | — | IA_MenuBack ← Backspace (1408) | IA_MenuBack ← Backspace · trg=0 mod=0 |
| 11 | — | IA_MenuBack ← Gamepad_FaceButton_Right (1408) | IA_MenuBack ← Gamepad_FaceButton_Right · trg=0 mod=0 |
| 12 | — | IA_MenuSecondary ← Home (1508) | IA_MenuSecondary ← Home · trg=0 mod=0 |
| 13 | — | IA_MenuSecondary ← Gamepad_FaceButton_Top (1508) | IA_MenuSecondary ← Gamepad_FaceButton_Top · trg=0 mod=0 |

Order: identical to the expected order, and no order-only change was found. All value types read `Boolean`. `Backspace` is stored in that casing, as `handoffs/TASK-1408-art.md` notes, so that is not a difference.

### Digest
`Content/Input/IMC_MainMenu.uasset`: size 7379 B, sha256 `1a1ee5aff7be1b3485510ab88c2097de6120e34f7d182289132b60eb1db8dbaa`, mtime 2026-09-26T16:23:49. This is byte-equal to the after-sha256 recorded in full at `handoffs/TASK-1508-art.md` line 30. The file was read in-process with `hashlib`, read-only.

## Evidence (promoted)
- none. This was an asset read, and no frames were captured. The evidence is the quoted tool output above.

## Hypotheses (not verdicts)
- none needed. The asset is unchanged since `TASK-1508`'s write (the digest matches).

## Not examined / limitations this run
- **`ETriggerEvent::Started` was NOT read.** It is chosen at the C++ bind site and is not stored in the asset, as the row says. This run makes no claim about it.
- **Trigger-count control is weaker than the modifier control.** `IMC_Hero` also reads `trg=0` on all 28 rows. The `trg=0` readings on `IMC_MainMenu` therefore come from a counter that has not been shown to return non-zero in this run. The modifier counter was shown non-zero (5).
- The deprecated `Mappings` property read len 0 on both assets. That describes the instrument, not the assets, as the row predicts.
- **`VER-§7` cl. 2 declaration:** no `run_verification_sequence` was used, and no step outside the `tools:` line was reached through one. Every call was on the granted read set: `execute_unreal_python_readonly` ×3 (census; `DefaultKeyMappings` read + file sha256), `is_pie_active` ×1, `get_unreal_output_logs` ×1, `get_input_mapping_context_keys` ×2 (IMC_Hero, IMC_MainMenu). No `pie_scene_edit` or `call_actor_function` was used. There was no asset write, no editor lifecycle action and no PIE.
- `.sav` net zero: not applicable, because nothing was driven and no game code ran.

## Recipes used
- none (an asset read with no PIE; `Tools/Verify/recipes/` covers PIE UI drives).
