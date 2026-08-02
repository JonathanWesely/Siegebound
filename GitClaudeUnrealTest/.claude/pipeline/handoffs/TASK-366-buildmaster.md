# TASK-366 — [AG-B1] Compile the batch + boot-PIE smoke + nav-settle measurement

**Agent:** build-master · **Date:** 2026-08-01 · **Status:** COMPLETE (no commit — TASK-378 owns that)
**Upstream:** TASK-365 **PASS** (7/7, 0 BLOCKER / 2 WARN / 4 NIT) — `qa/TASK-365-report.md`

---

## 1. COMPILE — **GREEN**

```
Result: Succeeded
Total execution time: 13.34 seconds   (17 actions, UBA local executor)
```

**Zero errors. Zero warnings.** All eleven touched TUs compiled and linked as one unit, exactly as QA predicted the
batch would need to:

`AncientGround.cpp` · `CaptureZone.cpp` · `CombatantHealthBarWidget.cpp` · `CombatantHealthBarComponent.cpp` ·
`DeckBuilderWidget.cpp` · `BattlefieldScatter.cpp` · `ScatterConfig.cpp` · `SorcererUnit.cpp` ·
`SiegeCheatManager.cpp` · `SummonedUnit.cpp` + 4 module unity chunks → `Link UnrealEditor-GitClaudeUnrealTest.dll`.

**Link is real, not a no-op:** `UnrealEditor-GitClaudeUnrealTest.dll` **2,779,136 → 2,907,136 bytes** (+128 KB),
relinked 18:23:28. UHT emitted reflection for both new UCLASSes — `AncientGround.generated.h` and
`SorcererUnit.generated.h`.

Adaptive non-unity mode excluded exactly the ten edited files from the unity blob, so **every changed file was
compiled standalone** — the strongest form of this check. No cross-task signature drift surfaced (CONVENTIONS §7
held).

### ⚠️ TWO PROCESS TRAPS WORTH INHERITING

1. **`Build.bat` exits 0 on failure.** The first attempt *failed* (`Result: Failed (OtherCompilationError)`) and
   still returned **exit code 0**. **Parse the log for `Result: Succeeded` / `Result: Failed`; treat a missing
   `Result:` line as failure. Never trust the exit code.** Any hook or agent gating on `$?` would have passed a
   batch that never compiled.
2. **Attempt 1 was blocked by the Live-Coding mutex, not by code.**
   `Unable to build while Live Coding is active` (`Global\LiveCoding_C++…UnrealEditor.exe`), 4.87 s, zero TUs
   compiled. **Not** Smart App Control (`VerifiedAndReputablePolicyState = 0`, and the signature is a mutex check,
   not `0x800711C7`). **Ctrl+Alt+F11 is not a substitute here** — UBT logged `Invalidating makefile (source file
   added)` and Live Coding cannot introduce new UCLASS/UPROPERTY, which is precisely what `AAncientGround` and
   `ASorcererUnit` are. **No QA loop was opened**; the 3-loop budget is untouched.

---

## 2. BOOT-PIE SMOKE — **GREEN**

Editor relaunched (PID 23372), `L_Arena` loaded, MCP up. **Match boots and starts:**

```
LogLoad: Game class is 'SiegeGameMode'
LogSiegeNet: [SiegeGameMode_0] Player login 'SiegePlayerController_0' seated as Blue (M8 seat latch)
LogGitClaudeUnrealTest: [SiegeGameMode_0] Spawned bot opponent 'SiegeBotController_0' … Red
```

**Error budget: 0 Error · 0 Fatal · 0 Ensure · 0 AccessedNone.**

### The required token checks — all pass

| Check | Result |
|---|---|
| `mirror=` new mode value | `GenerateScatter seed=20260801 mirror=rot180 layers=7 corridorHalfY=1000` ✅ |
| MinesPass `inj=` token **GONE** | `grep -c "inj="` → **0** ✅ |
| `AncientGroundsPass` P/M exact antipodes | `P=(-4743,1860,0) M=(4743,-1860,0) fb=no culls=4` ✅ |
| **Two** `AAncientGround` actors | `AncientGround_0`, `AncientGround_1` in `UEDPIE_0_L_Arena` ✅ |
| `authoritativeBoost` on host | **`true` on BOTH** grounds ✅ (QA note 6) |
| Mine pairs antipodal | 3 pairs, all exact `(−X,−Y)`: `(-16957,1013)/(16957,-1013)`, `(-5523,-10494)/(5523,10494)`, `(-3367,5852)/(3367,-5852)` ✅ |
| `twinSkipped` | **0 on every layer** ✅ |

### Runtime confirmations beyond the spec

- **`AAncientGround.bReplicates = false` read live in PIE** — CONVENTIONS §2 **Tier C confirmed at runtime**, not
  just in the header.
- **`ZoneHalfExtent (840,840)` == config `AncientGroundHalfExtent (840,840)`** — the PAIRED TUNABLE agrees and the
  divergence guard correctly stayed silent.
- **`DA_BattlefieldScatter` picked up `SymmetryMode = Rotational180`** and **`bMirrorSymmetric` no longer exists on
  the asset** — the silent default flip that IS this batch's behavior change (QA note 3). `AncientGroundClass` and
  `MaxGroundAttempts` are absent, exactly as QA recorded ("correct by absence").
- **Decal wiring verified structurally:** `DecalMaterial = /Game/Materials/M_AncientGround`, `DecalSize (1024,840,840)`,
  `FadeScreenSize 0.001`, `SortOrder 10`, `bVisible true`, and **no soft-load warning** in the log.

### ⚠️ What I did NOT verify — do not read this handoff as a visual pass

**I could not visually confirm the jade rune rings render.** `CaptureViewport` returned bare terrain at the ancient
ground's exact position. That is almost certainly the capture path not compositing deferred decals (the structural
readback is clean and the material soft-load logged no failure) — **but I cannot prove pixels from a readback, and
this project has been burned exactly that way before.** Likewise the full-arena symmetry shot is inconclusive:
HISM instances distance-cull at a 40000-unit camera height, so the field renders near-empty regardless of what was
placed. **The symmetry verdict below rests on the quantitative log evidence, not on my screenshots.**
**TASK-377 is the pixel gate for both.** Expected-and-not-filed: overhead health bars ~2× taller (TASK-362 §5).

---

## 3. DETERMINISM — **PARTIAL. This is the one real finding, and it is NOT an RNG defect.**

Seed used: **`OverrideSeed = 20260801`** (set on the level scatter actor via MCP; **restored to 0 afterwards**,
`L_Arena` never saved). Three same-seed PIE runs, scatter lines stripped of timestamp/frame prefix and diffed.

| Comparison | Verdict |
|---|---|
| **run 2 vs run 3** (both warm) | **BYTE-IDENTICAL** ✅ |
| **run 1 (cold) vs run 2/3** | **DIFFERS** ❌ |

**run 1 was the first PIE after the editor boot. Runs 2 and 3 were warm. Run 3 reproducing run 2 exactly rules out
a random race — this is a systematic cold/warm split, and it is bistable, not chaotic.**

### Exactly what varies, and what does not

| Line | Cold vs warm |
|---|---|
| `GenerateScatter` | **identical in all 3** ✅ |
| `AncientGroundsPass` | **identical in all 3** ✅ |
| `MinesPass` | **differs** ❌ — positions/seed/`mineStream` identical; only per-pair `culls` redistributes `0/4/0` → `0/2/2`. **Total culls = 4 in both.** |
| Layers `Boulders/Hill/Slabs/Trees/Rocks` (all `blocking=true`) | **identical in all 3**, `zMismatch=0`, `twinSkipped=0` ✅ |
| Layer `Grass` (`blocking=false`) | `placed 12246 → 12244`, `zMismatch 27 → 28` ❌ |
| Layer `Plants` (`blocking=false`) | `placed 2000` both, `zMismatch 6 → 1` ❌ |

**Every fairness-critical value is stable across all three runs, cold included:** both mine pairs and both ancient
grounds land at byte-identical exact antipodes, and all five blocking (nav-relevant, collision-bearing) layers are
identical. **What drifts is decorative ground-cover counts and the *attribution* of a fixed total cull count.**

### Mechanism (evidenced, not assumed)

`GroundZAt` (`BattlefieldScatter.cpp:1160`) is a `LineTraceSingleByChannel(ECC_WorldStatic)` that ignores only the
scatter actor. On the **first** PIE after an editor boot some static-world collision is still registering when
`GenerateScatter` runs at `BeginPlay`, so a handful of traces return a different Z (or miss → the `0.f` fallback).
One diverged placement shifts the acceptance sequence, which shifts every later sampled point — hence *different*
mismatch coordinates in run 1 vs run 2, not just different counts. From the second PIE on, the world is warm and
the pass is exactly reproducible.

**This is environmental, not an RNG fault.** QA's determinism audit stands: the RNG streams are provably clean
(`mineStream=1283221892` identical in every run, zero draws in the rotation step).

### ⚠️ OPEN QUESTION I could not close — flagging rather than guessing

**Is the COLD path reproducible cold-to-cold?** This matters more than it sounds: **in a packaged build every boot
is cold**, so the cold path is the one players actually get. I attempted an editor bounce to test a second cold
run; the close was blocked by the permission classifier and I did not work around it. **Recommend the manager task
this** — two fresh-boot first-PIE runs at one seed, diffed. If cold reproduces cold, this is a benign documented
warmup offset; if not, it is a genuine shipping-path determinism gap.

---

## 4. NAV SETTLE — **MEASURED: ~216 s** (baseline ~178 s)

Measured by raising `LogNavigation` to `Verbose` and timing PIE start → last `Building tile:` line.

| Run | PIE start | Last tile | **Settle** | Tiles |
|---|---|---|---|---|
| run 1 (verbose enabled mid-run) | 01:26:51.2 | 01:30:25.382 | **~214.2 s** | 57 observed (partial) |
| **run 2 (verbose from t=0 — the clean figure)** | 01:33:31.949 | 01:37:08.053 | **216.1 s** | **326** |

**~216 s vs the ~178 s TASK-349 loop-4 baseline ⇒ +38 s / +21%.**

⚠️ **Honest caveat on the comparison:** the 178 s baseline was a *different probe definition* — nav-probe
convergence to team-correct polys at the far castle (TASK-349 loop 4) — whereas mine is "queue fully drained, last
tile built". Both track the same distance-sorted tile drain, so the comparison is **indicative, not exact**.
Tiles processed at a steady ~1.5/s throughout. Recorded as a measurement; per spec **this is not a pass/fail gate.**

---

## 5. FINDING FOR THE MANAGER — the arena terrain is not 180°-symmetric (new task candidate)

The `zMismatch` counter is new in TASK-358, so this has never been measurable before. It fires **only on the two
non-blocking decorative layers**, never on the five blocking ones.

**I confirmed the root cause independently of the scatter code**, with two `trace_world` probes at an antipodal
pair that mismatched identically in *every* run:

| Point | Surface Z |
|---|---|
| `(-25731, -623)` | **87.5** |
| `( 25731,  623)` (exact antipode) | **660.1** |

**A 572-unit height difference in the authored static terrain.** The scatter is behaving correctly — it places the
twin at the exact antipode and honestly reports that the ground beneath disagrees. **Perfect rotational symmetry of
ground-hugging decoration is capped by the level geometry, not by this code.** The blocking layers happen to land
where terrain is symmetric; the dense layers sample enough points to find the asymmetric spots (~0.2 % of instances).

**Not a defect in this batch. Not filed against any task here.** Worth a manager decision: either accept and
document the ceiling, or task a terrain pass. Related QA NIT confirmed live: `Grass` `placed 12246 (target 12250)`
— the documented odd-target/attempt shortfall, not an error.

---

## 6. STATE ON EXIT

- **NOTHING COMMITTED. NOTHING PUSHED.** `HEAD = 10f14de`, `origin/main…main = 0`. TASK-378 owns the commit,
  gated on TASK-377.
- **`L_Arena` NEVER SAVED** — `git status Content/Maps/` is **empty**. The `OverrideSeed` edit was in-memory only
  and was **restored to 0**.
- Working tree: the same **33** entries as at session start — I created no repo files beyond this handoff.
- Editor **running** (PID 23372), MCP up, PIE stopped, `L_Arena` loaded. `LogNavigation` left at `Verbose`
  (in-memory only; reverts on restart).
- Build logs: `…/scratchpad/build-attempt1.log` (the Live-Coding refusal) and `build-attempt2.log` (the green build).
  Scatter captures: `run1_scatter.txt`, `run2_scatter.txt`, `run3_scatter.txt`.

## 7. HANDOFF TO TASK-367 (Jonathan, ~90 s)

Nothing in this batch blocks it. `WBP_CombatantHealthBar` is untouched on disk; the widget-tree edit is clear to
start. `SetDamageBoost` on the not-yet-implemented BIE is a safe no-op, so the taller bars are cosmetic until
TASK-367+368 land.
