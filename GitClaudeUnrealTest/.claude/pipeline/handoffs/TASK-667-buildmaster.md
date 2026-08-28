# TASK-667 — [ROT-6] THE ONE COMPILE + LIVE RE-VERIFY + CAPTURES + THE ROTATION COMMIT (build-master handoff)

**Status: COMPLETE on the RE-RUN — compile PASSES, suite 136/136 (128 ROT-gating green + 8 `Siegebound.Deck.*` recorded, all green), the 663 battery re-ran BYTE-IDENTICAL, the ONE intended live delta is exactly the spawn-facing fix (yaw 0 → 180), the pawn-view money shot shows the gate dead-ahead, and the rotation commit is cut.** Date: 2026-08-28.

## 0. THE TWO RUNS — the record of record

1. **Run 1 (2026-08-28, BLOCKED):** pre-flight ALL GREEN (HEAD `bd222c2`; Source delta exactly the 7 reviewed ROT files + the declared deck-wave files; map == ROT-§2 ledger `9ccd54ef…0e58` ×2). The editor bounce raised a **Save Content modal** listing the three deck-wave WBPs (TASK-669's in-memory load artifacts, disk verified clean) and every safe answer lane was unavailable (desktop locked · remote-exec CDO write permission-blocked · force-stop permission-blocked · UIA inactive · blind click refused). Escape=Cancel posted, editor restored byte-intact, 🚧 posted. **No QA loop spent — infrastructure blocker, not code.**
2. **Jonathan's manual unblock:** he closed the editor himself choosing **Don't Save** (2026-08-28) — verified: no UnrealEditor process, `L_Arena.umap` SHA256 exactly the ROT-§2 ledger. He also ruled **option 3**: the orchestrator added `[/Script/PythonScriptPlugin.PythonScriptPluginSettings] bRemoteExecution=True` to `Config/DefaultEngine.ini` (declared infrastructure cargo, staged with this commit) so every future editor boot carries a graceful-quit lane.
3. **Run 2 (this record):** editor DOWN at entry — the bounce was skipped entirely; started at the compile.

## 1. RE-PRE-FLIGHT (run 2)

- Git: HEAD `bd222c2` entry AND at the commit point — Jonathan had not self-committed; no drift.
- `Content/Maps/L_Arena.umap` SHA256 == ROT-§2 ledger `9ccd54efeb0459df9ed15204fd7e5274797e5f6093f5504a5730c3c9d5ea0e58` at ENTRY, after editor boot, and at EXIT (post-sessions). ⛔ No save was ever issued; `is_dirty` false at exit; the never-save law honoured throughout.
- Porcelain reconciled against the 667 dated rider: ROT cargo + `Config/DefaultEngine.ini` (declared) + **seven** deck-wave Source files (`SiegeDeckSaveGame.{h,cpp}` M · `DeckBuilderWidget.{h,cpp}` M · `DeckSlotEntryWidget.{h,cpp}` new · `Tests/SiegeDeckSlotsTest.cpp` new) + deck handoffs (TASK-669-buildmaster.md + 2 PNGs, TASK-670/671-programmer.md) — **all rider-enumerated, EXPLAINED + EXCLUDED, none refused. Zero unexplained lines.** (Run 1's return said "6 declared deck files"; the rider enumerates seven Source files and the porcelain matches the rider exactly — recorded as a count-wording delta only.) No `WBP_DeckBuilder.uasset` line — TASK-672 had not run, as sequenced.

## 2. COMPILE (the Build.bat log-parse law)

The CLAUDE.md Build.bat line, editor down, no mutex. **`Result: Succeeded`** by log parse — 19 actions, 21.03 s, UBA local. Zero warnings, zero errors. All 8 wave translation units compiled (Castle, SiegeGameMode, SiegeBotController, SiegeCastleTransformTest + the 4 deck units) — **zero deck-attributed diagnostics** (nothing to forward to TASK-673). 664's flagged consumerless constants drew no diagnostic, as QA predicted. No compile-fix diff exists ⇒ no `SC-§27` surface.

## 3. SUITE — 136 performed, the declared split exact

`UnrealEditor-Cmd` headless, the TASK-470 proven recipe (filter `Siegebound`, `-nullrhi`, `-testexit`, the `-ini:` startup-map override protecting `L_Arena`). From `report/index.json`, not a count-by-eye:
- **performed 136 = expected 136** (the 666 WARN's reconcile: **128 ROT-gating** — 127 baseline + `Siegebound.Castle.RotatedTransformPin` — **+ 8 `Siegebound.Deck.*`** co-flight, recorded-not-gating).
- `succeeded=133 · succeededWithWarnings=3 · failed=0 · notRun=0`. **All 128 gating green** (RotatedTransformPin: Success). **All 8 deck tests green too** (early info for 673/674: EmptySlotIllegal, FixedNameContract, Migration{FixedNameRespected,FreshSave,Idempotence,LegacyMapping,Overflow}, ScratchSlotRoundTrip).
- The 3 warning-bearing tests are pre-existing baseline (`Cloud.GuestSafeDefaults`, `Input.QwertyIsPassThrough`, `WarMap.AppendToInputRefusesAndNeverOpensTheConsole`) — their own deliberate negative-path log lines, unrelated to both waves.

## 4. RELAUNCH + THE REMOTE-EXEC VERDICT

Editor relaunched **PID 9072** (06:06), MCP `http://127.0.0.1:8000/mcp` up at 06:07 (raw JSON-RPC lane, session scratchpad client). Level `/Game/Maps/L_Arena` loaded; map hash re-verified == ledger after boot.

**REMOTE-EXEC: ALIVE — protocol-level.** The editor binds UDP 6766 (PID 9072) and its remote-execution node `1A6C031544A9032097684592006F666F` answered a spec `ping` with a `pong` on the standard multicast 239.0.0.1:6766. (Ping only — no editor Python was executed.) **The Save-Content-modal blocker class is permanently retired: every boot of this project now carries a graceful `quit_editor()` lane.** No explicit LogPython remote-exec startup line exists at default verbosity — the bound endpoint + pong is the mechanical proof.

## 5. THE BATTERY RE-RUN — BYTE-IDENTICAL

`handoffs/TASK-663-battery.py` re-run **byte-for-byte** (raw file content, unmodified, via `ProgrammaticToolset.execute_tool_script`, editor world, PIE not running): output **byte-identical** to `handoffs/TASK-663-battery-result.json` — `json.dumps` of the rerun equals the baseline file exactly; deep-equal too; **zero deltas across all 86 rows** (pre-mouth ×3, 17-station lane incl. the known y −3600 2-uu graze, riser-top bisections 29/101.5/148/174, seal-face x3660-occupied/x3665-empty, forward-field ×3 — both castles). The battery is level-geometry-only, so the spawn-facing fix rightly leaves it untouched; **the intended facing delta appears in the live pawn read below, and NOWHERE else.**

## 6. LIVE RE-VERIFY — three sessions, all fences honoured

⛔ No console sentence, no `M` (latch UNSPENT), no pawn input (CF-R3 — all sessions input-free), no property write, no save; the user's viewport camera never moved (posed captures via `CaptureViewport.captureTransform`; the pawn-view used the live game viewport unposed). Direct single tool calls during sessions (the TASK-569 batcher law).

**Session A (PIE, 06:15, warmup 12 s — the ≥10 s nav hold honoured):**
- **THE ONE INTENDED DELTA, enumerated:** `BP_HeroCharacter_C_0` at world **(−21007.816, 0, 98.15)** — byte-identical location to 663 §1 — rotation **(0, 180, 0)**: **yaw 0 → 180 is the entire live delta vs the 663 record.** He now faces his OWN castle; the gate is IN FRONT of him. Branch-3 log tail live: `… -> (-21008, 0, 100), facing yaw 180 toward the own castle (TASK-665).` Branch-2 refusal line fired byte-identical first, as ever. (Red `facing yaw 0`: no live Red client exists in single-player PIE — same instrument residual as 663 §1; the Red row is the mirrored arithmetic.)
- Overlap-proof: capsule box at the pawn contains ONLY the hero. Spawn CLEAN.
- **F1 grep surface, byte-true, both castles:** `F1 discoverability set — 2/2 banners, 0/0 path segments, 0/0 toe rocks spawned` — 664's declared new form. Torch line byte-intact: `furnished — 6 of 6 torch anchors spawned (cap 6), commander spawned`.
- **Nav definitive:** `Nav generation FINISHED … DEFINITIVE post-settle` → `Traversability CONFIRMED (nav settled: 0 pending) … Blue→Red castle path + 6 mine path(s) exist (after 0 cull(s)) [definitive: OnNavigationGenerationFinished]` — 0 culls, 6/6 mines. The parked mine-reachability row did NOT fire (stays parked).
- **Bot anchor live:** `castle-front (19965, …)` placements reproduced — the 663 §6 value, in front of Red's gate.
- GateBlocker armed-centre probes: Blue (−23425, 18, 852) returns Castle_0, Red (+23425, −18, 852) returns Castle_1, Blue mouth-plane control box EMPTY — armed, localized, on the rotated axis.

**Session B (PIE, 06:21):**
- **The money shot for TASK-668: `TASK-667-pie-spawn-pawnview.png`** — the literal player camera at spawn: the open gate mouth DEAD-CENTRE ahead (grass approach, tan apron, dark passage between the brick towers, flanking knolls). The VID-001 discoverability inversion, closed on pixels. (663's VRAM dev-overlay did not appear this session.)
- GateBlocker property reads, both castles: RelativeLocation **(18, −1575, 852)** · RelativeRotation zero · BoxExtent **(900, 405, 678)** — byte-match 663 §4 (own-team-clear/enemy-seal matrix per the standing TASK-350/659 §3b record; `BodyInstance` remains unreadable through this lane, same declared residual as 663 §9).
- Hero transform reproduced: same location, yaw 180.

**Session C (SIE, 06:24):** after-captures `TASK-667-sie-{blue,red}-from-centre.png` (the 662 §3a poses — each gate archway dead-centre, both crimson chevron banners framing each mouth, torches lit, war table visible; Red self-shadowed = the correct mirror under the SE sun) and `TASK-667-sie-{blue,red}-spawn-eyeline.png` (the open mouth filling the frame from each spawn point). Nav definitive reproduced this session too.

**The Play-Again edge (F1 ×3 sessions):** the F1 line appears EXACTLY once per castle per session — 6 lines after three sessions (06:15 ×2, 06:21 ×2, 06:24 ×2). No double-spawn, no missing on replay.

## 7. THE COMMIT

HEAD re-verified `bd222c2` immediately before staging (Jonathan had not self-committed). Explicit-path staging only; the seven deck-wave Source files + deck handoffs verified ABSENT from the staged list by grep; TASKBOARD.md + CONVENTIONS.md staged LAST (ROT-§ + DECK-§ law + status flips — sanctioned manager/docs cargo). LFS: `L_Arena.umap` staged pointer oid == the ROT-§2 ledger hash; every staged PNG pointer oid == its worktree sha256 (verified per-file, never by size). ⛔ Not pushed.

Cargo: `L_Arena.umap` · the 7 reviewed Source files · `Config/DefaultEngine.ini` (declared) · handoffs 662 (md + 12 PNGs) · 663 (md + battery.py + battery-result.json + 11 PNGs) · 664/665 (md) · `qa/TASK-666.md` · this handoff + 5 TASK-667 PNGs · TASKBOARD.md + CONVENTIONS.md.

## 8. EXIT STATE / FOR TASK-672 AND TASK-668

- Editor **UP PID 9072**, MCP green, `/Game/Maps/L_Arena` loaded, `IsPIERunning` false, **`is_dirty` false — decline any save prompt**; the remote-exec graceful-quit lane is LIVE. TASK-672's graph surgery has its editor window; then Jonathan's TASK-668 walk (spawn → gate ahead → walk in → record = VID-002).
- Latch UNSPENT; no console sentence, no `M`, no pawn input ever issued.
- Captures beside this file (5): `TASK-667-pie-spawn-pawnview` · `TASK-667-sie-{blue,red}-from-centre` · `TASK-667-sie-{blue,red}-spawn-eyeline`.
- Deviations: NONE beyond the recorded instrument residuals (Red facing = arithmetic mirror; `BodyInstance`/`ControlRotation` unreadable through the lane — both pre-existing, declared by 663).
