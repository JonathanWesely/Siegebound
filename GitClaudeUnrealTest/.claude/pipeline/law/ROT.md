<!-- MOVED from .claude/pipeline/CONVENTIONS.md on 2026-10-04 by Tools/split_conventions.py. Sections are byte-identical to the original; this comment is the only addition. Cite clauses by tag (e.g. `VER-§3 cl. 6`); CONVENTIONS.md '## Law index' maps tags to files. -->
## ⚖️ CASTLE ROTATION — the gates face the battlefield center (2026-08-27) — namespace **ROT-§**

Jonathan's directive, verbatim: *"So I just realized that the diagnoses that you couldn't enter may have been because I didnt realize that you were supposed to enter from the south entrance. Lets go ahead and rotate the building 90 degrees so that the main entrance is facing towards the center of the battlefield."* Boarded as **TASK-662..668** (TASKBOARD "CASTLE-ROTATION" wave, directly under the VID-001-F1 chain it partially supersedes). Sections cite as `ROT-§N`, never bare `§N`.

### ROT-§0 ⚖️ TWO JONATHAN RULINGS — DECIDED, BINDING, ⛔ NOT RE-OPENABLE

1. **MECHANISM = LEVEL EDIT + SAVE.** The two placed `ACastle` actors in `L_Arena` are ROTATED IN THE LEVEL and the map is SAVED — **his explicit, ordered exception to the standing `L_Arena` never-save law. This is the never-save law's OWNER exercising it, and it is recorded as exactly that** (the 2026-07-29 nav-exception template: a Jonathan-approved, one-time exception with a tight diff gate). Scope: **ONE save event, TASK-662 only**, whose dirty set is ENUMERATED AND DECLARED before saving — nothing swept, nothing ride-along (⛔ the flag-(ix) authored-terrain asymmetry is explicitly NOT in this save's scope — it stays flagged and open). **The never-save law RESUMES IN FULL the instant the TASK-662 save completes**, under the NEW ledger of ROT-§2.
2. **SCOPE = BOTH castles, MIRRORED, gates facing the battlefield center.** Blue (world −25000, 0): gate turns from world −Y to world **+X**. Red (world +25000, 0): gate turns to world **−X**. Expected yaws in ROT-§1 — **measured before pinned** (TASK-662 verifies facing on pixels + bounds BEFORE the save; a wrong sign is corrected live and recorded, never saved wrong).

### ROT-§1 THE EXPECTED TRANSFORMS — arithmetic on the 617 C1 datum; ⛔ MEASURED BEFORE SAVED

- **Datum (617 C1, three-times-confirmed):** at yaw 0 both gates face world −Y; the gate mouth is on castle-LOCAL −Y (WR-§0 frame).
- **Mapping (UE Z-up, positive yaw rotates +X toward +Y):** local (0,−1,0) under yaw θ → world (sin θ, −cos θ, 0). θ = **+90** → **+X**; θ = **−90** → **−X**.
- ⇒ **EXPECTED: `Castle_Blue` (−25000, 0) yaw +90 · `Castle_Red` (+25000, 0) yaw −90.** Location and Z untouched; yaw is the ONLY changed component.
- **⛔ MEASURE, DON'T TRUST (the standing arithmetic-humility law):** TASK-662 confirms, post-rotation and PRE-save, that each gate-corridor void opens toward the centerline (bounds/overlap probe of the mouth face + `GateBlockerVolume` world center landing center-side + pixel captures of both gates from the centerline). The measured values are what get saved and recorded — this section's arithmetic is the prediction, not the record.
- **180° SYMMETRY CLOSURE:** rotating the world 180° about the origin maps (−25000, 0, yaw +90) → (+25000, 0, yaw −90) — Red's transform exactly. **The mirrored-inward configuration IS 180°-rotationally symmetric**, so the long-flagged "both gates face world −Y" asymmetry (Ancient Grounds §1 residual; TASKBOARD ANCIENT-GROUNDS flags (i)/(x)) is **RESOLVED AND CLOSED by this ruling** — dated closure notes at all three sites.

### ROT-§2 ⛔ THE HASH-LEDGER SUPERSESSION — the old ledger RETIRES at the TASK-662 save

- The standing canonical `L_Arena.umap` SHA256 ledger `b3dbc5d9ae484a7bd02cafad52b4681da68b011477479b65ee7781ae459f8268` is **RETIRED at the TASK-662 save**. Every citation of it dated before the save is HISTORICAL RECORD (left as authored — the record is never rewritten); ⛔ no task dated after the save may cite it as the live ledger.
- **THE CANONICAL LEDGER SLOT NOW LIVES HERE**, and the integrator fills exactly one line (a sanctioned, narrow write on the SLACK.md-registry precedent — build-master edits NOTHING else in this file):
- ~~**NEW CANONICAL `L_Arena.umap` SHA256: `9ccd54efeb0459df9ed15204fd7e5274797e5f6093f5504a5730c3c9d5ea0e58`**~~ *(measured post-save, TASK-662 2026-08-27, the one ordered ROT-§0.1 save event — save log 22:05:16, dirty set = exactly `/Game/Maps/L_Arena` out of 3194 probed packages)* ⇒ 🚩 **RETIRED 2026-09-05 — ⛔ SEE THE RE-PIN DIRECTLY BELOW.**

#### 🚩⛔⛔⛔⭐⭐ **`L_Arena.umap` SHA256 — ⛔ RE-PINNED 2026-09-05. ⛔ THE FOUR-PART RECORD `SC-§82` REQUIRES.**

| ⛔ | ⛔ value |
|---|---|
| ⛔ **OLD** | `9ccd54efeb0459df9ed15204fd7e5274797e5f6093f5504a5730c3c9d5ea0e58` |
| ✅ **NEW — ⛔ THE LIVE LEDGER** | ⛔ **`1f78419d888940739c949a60b29ee43451c464915f7ab37942b921fe0af15622`** |
| ⛔ **DATE** | ⛔ **2026-09-05**, ⭐ `TASK-1036` |
| 🧑 **THE RULING THAT MOVED IT** | ⛔ **VERBATIM: *"yes turn on volumetric fog and add the 27 files to git"*** |

- ⛔ **WHAT CHANGED: ⛔ ONE FLAG — `ExponentialHeightFog_0.bEnableVolumetricFog` ⇒ ⛔ `true`.** ⛔ **⛔ Nothing else. ⭐ `TASK-858` is ⛔ CLEARED by that same ruling.**
- ⭐ **THE OLD HASH WAS ⛔ RE-CONFIRMED FROM A ⛔ BYTE COPY TAKEN ⛔ BEFORE THE SAVE — ⛔ NOT merely read off the board.** ⇒ ⛔ **the pair is ⛔ measured at ⛔ both ends, ⛔ not half-cited** (⭐ `SC-§40`).
- ✅⭐⭐ **AND *"ONLY THE FLAG MOVED"* IS ⛔ PROVEN ⛔ THREE WAYS: ⛔ the ⛔ 6 neighbouring fog properties read back ⛔ BIT-IDENTICAL · ⛔ the level was ⛔ `is_dirty = false` ⛔ IMMEDIATELY BEFORE the single `set_properties` call · ⛔⛔ the package ⛔ NAME-TABLE gained ⛔ EXACTLY ONE ENTRY: ⛔ `bEnableVolumetricFog`.**
- ⛔ **⛔ EVERY CITATION OF THE OLD HASH ⛔ DATED BEFORE 2026-09-05 IS ⛔ HISTORICAL RECORD** (⛔ left as authored — ⛔ the record is ⛔ never rewritten). ⛔ **⛔ No task dated after this save may cite it as the ⛔ live ledger.**
- ⚠️ **⛔ THE STANDING CHECK AT `:2813` IS ⛔ UNCHANGED IN FORCE — ⛔ build-master still verifies `L_Arena`'s SHA256 is ⛔ UNCHANGED at every gate. ⛔ ONLY THE ⛔ BASELINE MOVED.** ⇒ ⛔ **a mismatch against the ⛔ NEW value is ⛔ still a defect, and ⛔ still a stop.**
- Until that line is filled, the level is mid-transition: hash checks state "pre-ROT ledger" or "post-ROT pending" explicitly.

### ROT-§3 THE NAV RULING — the navmesh-rebuild law (Castle 3× HOLLOW section) discharged at decomposition, as it demands

A rotation is a footprint-ORIENTATION change ⇒ the saved bake goes stale silently. **Ruling:** TASK-662 waits for the in-editor Dynamic Recast rebuild to settle (`nav settled: 0 pending` — the standing instrument) BEFORE saving; whatever nav object legitimately dirties inside the `L_Arena` package rides the ONE ordered save and is ENUMERATED in the handoff. If the bake lives in a separate package or needs a manual Build > Navigation click (MCP has no nav-build call — M6.6 lane knowledge), that is DECLARED plus a FOR JONATHAN row — ⛔ never swept in silently, never faked. TASK-663 then proves the definitive-nav gate live (0 pending, 0 castle-lane culls, both castles), and TASK-667 re-proves it post-code.

### ROT-§4 CONSEQUENCE LAW — FRAMES, NOT LINE NUMBERS (what the rotation does and does not invalidate)

- **Castle-LOCAL statements stay TRUE** (gate on local −Y, hall dims, torch/commander anchors, GateBlocker relative transform, "+Y is deeper into the keep"). ⛔ They get NO rider — riding a true comment is noise.
- **WORLD-frame gate statements are now FALSE** — headline: the five TASK-637-corrected sites that (correctly, then) wrote "both castles yaw 0, gates face world −Y (617 C1)". **They are DELIBERATELY SUPERSEDED and take dated riders (TASK-664 in `Castle.{h,cpp}`, TASK-665 elsewhere), never silent contradiction** — the 637 record itself stays as authored.
- **WR-§2b live-bounds derivations are EXPECTED rotation-robust** (the colliding AABB re-answers; X/Y extents swap ≈3656.85 ↔ ≈3692.25) — **VERIFIED at TASK-663, never assumed.** The spawn resolver (`GetHeroStartTransform` branches 2/3), `DefendRadius` band, `BotCastleSpawnOffset` band, `CastleQueryInset` all re-measure.
- **The F1 furnishing anchors (TASK-661) are castle-local and ROTATE WITH the castle.** Banners at the mouth stay correct BY CONSTRUCTION (verified at 663 pixels); the TramplePath (premise: guide from the old east-face refusal around to the mouth) and ToeRocks (premise: dress the old east-face seal the spawn approach hit) **LOSE THEIR PREMISE** — dispositions pinned by the ROT ACTIVATION RULING on 663's measured record (provisional defaults in the wave section: retire both by emptying the `EditDefaultsOnly` CDO anchor arrays; mechanism, soft refs and imported assets STAY — reversible by design, the ATorch null-safe law makes an empty family free).
- **The hero-spawn IDEAL (Jonathan's intent, this directive):** the hero spawns seeing his own gate. 663 MEASURES the live post-rotation spawn (branch taken, location, facing, overlap-clear); if he spawns facing away, TASK-665 turns the spawn rotation to face the own gate mouth, derived LIVE from the castle transform (never a hardcode) — a declared behavior change, Jonathan's eye at TASK-668 is final. ⛔ This is a C++ rotation-of-control, NOT a PlayerStart level edit — the old J2 "spawn move is Jonathan's own save grant" distinction stands untouched.

