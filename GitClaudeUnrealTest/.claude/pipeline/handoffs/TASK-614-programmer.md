# TASK-614 — [CF-2] LANE C — commander/war-table visibility diagnosis + the blue-bar identification (gameplay-programmer handoff)

**Date: 2026-08-17. DIAGNOSIS ONLY — zero code edits, zero compile, zero editor/MCP, zero git (CF-R1/R3 honored; the editor was down the whole task; no engine result below is faked — every claim is file-side, and the live reads are enumerated for TASK-617 in §6).**

---

## 1. HEADLINE VERDICT

**The commander and the war table were PRESENT, CORRECTLY PLACED, AND FULLY FLESHED in Jonathan's own `-game` session — proven from his session's log file on disk, not inferred.** `Saved/Logs/GitClaudeUnrealTest.log` is the 2026-08-17 standalone session (`LogInit: Command Line: -windowed -ResX=1600 -ResY=900 -game`, log line 470) and it contains, at 19:32:58 (lines 1894–1897):

```
[BP_CommanderNpc_C_0] CommanderNpcInit team=Blue P=(-25465, 810, 174) interactRadius=400 revealCost=30
ACastle 'Castle_0' (Blue): furnished — 6 of 6 torch anchors spawned (cap 6), commander spawned (WR-§4/WR-§5; nothing placed in L_Arena).
[BP_CommanderNpc_C_1] CommanderNpcInit team=Red P=(24535, 810, 174) interactRadius=400 revealCost=30
ACastle 'Castle_1' (Red): furnished — 6 of 6 torch anchors spawned (cap 6), commander spawned (WR-§4/WR-§5; nothing placed in L_Arena).
```

The two earlier sessions that evening (backups `GitClaudeUnrealTest-backup-2026.08.17-19.18.53.log` / `-19.20.22.log`) carry the identical quartet. Blue P = (−25465, 810, 174) is exactly castle root −25000 + `CommanderNpcAnchor` (−465, 810, 174) (`Castle.cpp:313`) — the anchor math composed correctly (`Castle.cpp:591`, `CommanderNpcAnchor * CastleMeshTransform`).

**Equally decisive is what the log does NOT contain.** Every degradation in this lane logs by design, and none fired in any of his three sessions:
- no `avatar mesh ... did not resolve` (`CommanderNpc.cpp:272` — one-shot Warning, guaranteed from BeginPlay) ⇒ **SK_Sorcerer resolved and is on the component**;
- no `war table mesh ... did not resolve` (`CommanderNpc.cpp:410`) ⇒ **SM_WarTable resolved and is on the component**;
- no `REF POSE` line of either shape (`CommanderNpc.cpp:336` unresolved-clip, `:375` skeleton-mismatch — both log at `Log`, structurally once from the single BeginPlay call site) ⇒ **rung 2 of the anim ladder succeeded: A_Sorcerer_Idle is playing single-node**;
- no `commander NPC failed to spawn` (`Castle.cpp:611`), no torch shortfall (6 of 6), no `BP_CommanderNpc unavailable` fallback line (`Castle.cpp:508`) ⇒ **the Blueprint class resolved and spawned**.

⇒ **"I cannot find him" is a PERCEPTION/REACH failure with three stacked mechanisms (darkness, eye-below-floor sightline, berm-obstructed approach — §3 M9–M11), not an existence, spawn, asset, or `-game`-specific failure.** No commander-side code repair is indicated by the current evidence (repair sketch in §7 is optional-polish INPUT only).

## 2. FILE-SIDE ASSET GROUND TRUTH (verified this task)

| artifact | state |
|---|---|
| `Content/Meshes/SM_WarTable.uasset` | EXISTS (93,688 bytes); binary name-table scan finds `M_WarTable` ⇒ **the material slot is bound in the saved asset** — the "black unassigned default" hypothesis is refuted file-side |
| `Content/Blueprints/BP_CommanderNpc.uasset` | EXISTS (24,605 bytes); scan finds `SimpleConstructionScript`/`SCS_Node` but **NONE of** `AvatarMeshAsset` / `WarTableMeshAsset` / `AvatarAnimClassAsset` / `AvatarIdleAnimAsset` / `InteractRadius` / `EnemyRevealCost` / `ABP_Footman` ⇒ **no serialized delta on any tunable — the C++ defaults are live (the W8-R3 method)**. In particular `AvatarAnimClassAsset` is still empty: the TASK-591/SC-§35 fix is intact in the shipped BP |
| `Content/Characters/SK_Sorcerer.uasset`, `Content/Characters/Anims/A_Sorcerer_Idle.uasset`, `Content/Blueprints/BP_Torch.uasset`, `Content/UI/WBP_CastleHealthBar.uasset`, `Content/UI/WBP_CombatantHealthBar.uasset` | all EXIST |
| TASK-588 lineage (board `W6-R3`, `:8476-8482`) | the WarTable's falsified premise was about **UCX COLLISION node naming** (`WarTable` + `SM_WarTable` render nodes ⇒ `UCX_<node>_NN` never bound). Collision is irrelevant to this lane (both components ship `NoCollision`, `CommanderNpc.cpp:126/:146`). The MATERIAL record is `handoffs/TASK-566-buildmaster.md:471-481`: `M_WarTable` (Default Lit, vertex-color lerp) **created and assigned to slot `WarTable` ✅** — with WoodColor (0.115, 0.070, 0.040) and ParchmentColor (0.520, 0.430, 0.290). ⚠️ i.e. the table is authored as VERY DARK WOOD, non-emissive — legitimately near-invisible in an unlit hall (feeds lane D, §3 M10) |

## 3. THE MECHANISM TABLE (mechanism → paper verdict → live read for TASK-617)

| # | mechanism | paper verdict | live read for TASK-617 |
|---|---|---|---|
| M1 | Spawn refused / castle never furnished | **REFUTED** — his own session log: `commander spawned`, both castles, all three sessions (§1); spawn is `AlwaysSpawn` (`Castle.cpp:546`) | confirm `SpawnedCommanderNpc` IsValid on both `ACastle` (belt only) |
| M2 | `BP_CommanderNpc` resolves to a broken/overridden BP | **REFUTED** — no serialized delta on any visual/tunable property (§2); log shows `BP_CommanderNpc_C_0/1` init at the exact anchor | dump the BP CDO's four soft paths + `InteractRadius`/`EnemyRevealCost` (expect C++ defaults verbatim) |
| M3 | Avatar mesh unresolved ⇒ deliberately-invisible NPC (`CommanderNpc.cpp:273`'s designed degradation) | **REFUTED** — the one-shot Warning is absent from all three session logs | `AvatarMesh->GetSkeletalMeshAsset()` == SK_Sorcerer |
| M4 | War-table mesh unresolved ⇒ no table prop (`:411`) | **REFUTED** — Warning absent; asset exists | `WarTableMesh->GetStaticMesh()` == SM_WarTable |
| M5 | SM_WarTable material unassigned/black | **REFUTED file-side** (§2) — but the ASSIGNED wood is (0.115, 0.070, 0.040): dark by authorship | read the slot-`WarTable` material live (name each material — the TASK-588 token history is live per the 617 spec); pixel it under a light |
| M6 | Ref-pose / anim failure (post-TASK-591 ladder) | **rung 2 SUCCEEDED** — both fall-through `Log` lines absent (§1). And ref pose would be visible-but-static anyway: what he'd actually see in the fail world is a motionless gray robed figure, i.e. NOT invisibility | `AvatarMesh->GetAnimationMode()` == `AnimationSingleNode`, current asset == A_Sorcerer_Idle |
| M7 | Cooked-build-only soft-path failures (asset not cooked, chunking, PrimaryAssetRules) | **CANNOT explain this session, structurally**: his session is Development, UNCOOKED `-game` on loose `Content/` — `LoadSynchronous` hits the same files the editor does. Empirically refuted anyway by M3/M4. State for the record: this family only re-enters when the project first packages | none needed (unobservable in-editor anyway; declare unobserved per `SC-§35`) |
| M8 | Castle destroyed ⇒ furnishing torn down (`ApplyDestroyedState`) | **REFUTED** — castles at full HP in both shots; no destroy in the logs | none |
| M9 | **SIGHTLINE — the eye below the visual floor plane** (CF-R2's named prediction, lane B): in hull-free interior columns the pawn is supported at z ≈ 0 under the 174 visual floor; a camera at ~150–200 uu is BELOW the plane the commander stands ON, so the floor's near edge occludes everything standing on it | **PLAUSIBLE-STRONG for the interior shot** — `interior-dark-floorseam.png` shows precisely the sub-floor viewpoint (seam at chest, floor edge-on). The commander (feet AT 174) is geometrically unviewable from such a camera except within a few hundred uu of the floor edge | lane-B/617: pose the camera at the reconstructed evidence column at pawn-eye height; ray to (−465, 810, 174+90); state occluder. Also trace the support z under the commander himself (expect the `floor_slab_hall` hull top ≈ 174 there — he does NOT sink; his placement needs no fixing) |
| M10 | **DARKNESS/EXPOSURE (lane D)**: the hall is near-black at game auto-exposure while sunlit faces blow out; the commander is a gray figure and the table is near-black wood, 2,400+ uu deep | **PLAUSIBLE-STRONG — the lead "cannot find" mechanism.** Geometry note for lane D: the commander at (−465, 810) sits only 180 uu from torch anchor 2's floor-pool centre (−465, 990 — `Castle.cpp:267`), well inside its ≈912-uu pool ⇒ he IS in the best-lit spot the interior has, and the pixels still show near-black ⇒ the pool luminance loses to auto-exposure. 6/6 torches DID spawn (his log) — this is intensity/exposure, not missing lights | lane-D/617: interior pixel at the commander's head from eye height; exposure histogram; per TASK-616's checklist |
| M11 | **REACH (lane A)**: the entrance berm swallows the threshold; he plausibly never got a lit, above-floor viewpoint into the hall at all | **PLAUSIBLE contributing** — `entrance-berm-halfbody-sink.png` shows the approach half-blocked. Corroboration for lane A from the 612 record: even the SIE capture notes "interior scatter intact" INSIDE the hall (`handoffs/TASK-612-buildmaster.md:80`) — non-blocking scatter legally enters the keep-clear disc (`BattlefieldScatter.cpp:691`), and the interior grass in the evidence is on pixels | lane-A/617 checklist (TASK-613's) |
| M12 | Distance/size: from the threshold the commander is ~3,160 uu away (threshold y ≈ −2350 → anchor y +810), a ~180-uu figure subtending a few dozen pixels, in the dark, partially framed by the 1,470-uu gate aperture | contributing, not sufficient alone | falls out of the M9/M10 screenshots |

**The `SC-§35` reconciliation the spec demands (612's SIE saw him; his `-game` couldn't find him): BOTH observations are TRUE and neither refutes the other.** The 612 SIE capture posed a free editor camera INSIDE the hall at close range under editor-viewport exposure; Jonathan's `-game` camera is chained to a pawn that lane A blocks at the door and lane B sinks below the floor plane, under game auto-exposure that lane D shows crushing the interior to black. Same world, same actors — different camera and different tonemap. The in-editor-visible vs in-game-not-found split is fully explained without any `-game`-specific code path (and M7 shows no such path exists in an uncooked Development session).

## 4. THE BLUE-BAR IDENTIFICATION

**Verdict: the blue bar is the HERO'S OWN overhead health bar — `AHeroCharacter`'s `UCombatantHealthBarComponent` (named `HPBarWidget`, `HeroCharacter.cpp:86`, attached to the capsule) drawing `WBP_CombatantHealthBar` with the Blue-team fill `BlueBarColor = (0.05, 0.30, 1.00)` (`CombatantHealthBarComponent.h:137`). It is not "in the doorway" at all — it is ~25–30 uu above the hero's head, and the visible actor beneath it is the player character himself.** The doorway placement is compositional coincidence: in both shots Jonathan faces the doorway with the camera behind him, so the bar projects over the dark opening.

Measured on the evidence pixels (this task, both PNGs):
- bar ≈ **80 × 13 px, solid saturated blue** — matches the WBP_CombatantHealthBar health row (component DrawSize (90, 22), ≈12-px health bar + a boost row driven to opacity 0 for a non-boosted owner, `CombatantHealthBarComponent.h:37`; a screen-space widget renders at the widget's desired size, so ~80–90 px is the expected on-screen width);
- **horizontal tracking**: bar centre x ≈ 951 vs hero centre x ≈ 950 (entrance shot); ≈ 960 vs ≈ 965 (interior shot) — the bar follows the hero to within ~10 px across two different world locations;
- **vertical offset**: ~85–95 px above the head top in BOTH shots — consistent with `BarHeightZ` 120 above the capsule CENTRE (≈ feet + 210, ≈ 27 uu above a 183-uu character) at his ~350–400-uu camera distance;
- the component is **always-visible-while-alive by design** (the reversed hide-at-full law, `CombatantHealthBarComponent.h:25`), so it appears in every shot of a living hero.

Every other candidate, adjudicated:

| candidate | verdict |
|---|---|
| `ACastle::HPBarWidget` (`Castle.h:266`, `Castle.cpp:170-177`) | **NO.** Anchored at castle-mesh +9450 Z, screen-space. From his poses (2,000–4,000 uu from the near castle's root) it projects ~70–80° above the camera axis — far off-screen top. The far castle's bar is not along either sightline (his log puts the castles on the ±25,000 X axis while both shots face the local ∓Y doorway axis). Widget desired size ≈ 256 × 32 also mismatches 80 × 13. Its fill colour is authored in WBP (not readable file-side) — 617 should read it while identifying, but position alone eliminates it |
| `ACommanderNpc` | **has NO widget component of any kind** (verified across `CommanderNpc.{h,cpp}` — components are SceneRoot, AvatarMesh, WarTableMesh only). The bar cannot be the commander's; nothing hangs a bar over him |
| `ADamageNumberActor` | transient float-up damage text, not a persistent solid bar — no |
| other `UCombatantHealthBarComponent` owners (`ASummonedUnit`, `ABuilding` + subclasses) | none plausibly present: no blue units/buildings stood at either location (Rally 0/6, no summons in frame), and a unit bar would not track the hero across two locations |

**The live read that settles it** (also §6 item 7): the decisive discriminator needs a pawn, which SIE deliberately lacks — so the definitive instrument is Jonathan's own next session, one zero-cost gesture: **strafe left/right and watch the bar — if it moves with the character, it is the hero's own bar** (it will). 617's SIE half: from the reconstructed camera pose, enumerate every `UWidgetComponent` in the world and confirm NONE projects into the doorway rect — proving the bar in the evidence entered with the pawn.

## 5. INCIDENTAL FINDINGS (recorded, not lane C defects)

1. **Castle_1 appears to be placed at yaw 0, not the yaw 180 the code comments assume.** Red commander world P = (24535, 810, 174) = 25000 + (−465, +810) UNROTATED; a yaw-180 castle would have produced (25465, −810). `Castle.h:194` ("Castle_Red is placed at yaw 180") is therefore a stale doc claim against the live L_Arena — consistent with TASK-612's trace tables reading identical LOCAL columns at both castles unrotated. Harmless to lane C (the transform composition handled it correctly either way; that is what the relative-anchor design is for), but 615/617 should verify the actor yaw and the manager may want the comment corrected in some future code-owning task. It also means BOTH gates face world −Y.
2. **"Interior scatter intact" inside the hall is in the 612 SIE record** (`handoffs/TASK-612-buildmaster.md:80`) — first-party corroboration for lane A's non-blocking-layer mechanism, from before Jonathan's playtest.
3. **The small white dash at top-centre (~x 0.50, y 0.06 of the viewport, ~15 × 4 px, IDENTICAL position in both shots) is UNIDENTIFIED on paper.** Fixed-viewport-position across two camera poses suggests a HUD element rather than a world-projected widget; it is NOT explainable as either castle bar (both project off-screen or off-axis). 617: identify it while enumerating widgets (§6 item 8).

## 6. THE TASK-617 LIVE CHECKLIST — LANE C (enumerated; SIE, read-only, both castles)

1. `SpawnedCommanderNpc` IsValid on both `ACastle`; actor class name (expect `BP_CommanderNpc_C`); world transform vs the log's (−25465, 810, 174) / (24535, 810, 174) and vs `CommanderNpcAnchor * CastleMeshTransform` recomputed live. Record each castle actor's yaw while there (§5.1).
2. `AvatarMesh->GetSkeletalMeshAsset()` (expect SK_Sorcerer) + its material slots resolved, named.
3. `WarTableMesh->GetStaticMesh()` (expect SM_WarTable) + slot `WarTable`'s material (expect `M_WarTable`, NOT WorldGridMaterial/default — name it; TASK-588 token history live).
4. `AvatarMesh->GetAnimationMode()` (expect `AnimationSingleNode`) and the current single-node asset (expect A_Sorcerer_Idle) — confirms §1's silence-implies-success inference positively.
5. Trace the support top directly under (−465, 810) local at both castles (expect the `floor_slab_hall` hull ≈ 174 — the commander does not sink).
6. PIXEL: hall interior screenshot at PLAYER EYE HEIGHT from the corridor mouth AND from above (the 617 spec's pair); plus one from the §3-M9 sub-floor reconstructed pose to demonstrate the occlusion; plus one close-in with exposure noted (lane D pairing).
7. Enumerate every `UWidgetComponent` instance in the simulate world with world position + widget class + projected viewport position from the reconstructed evidence camera; confirm none lands in the doorway rect (the hero-bar identification's SIE half — the pawn half is Jonathan's strafe test, §4).
8. Identify the top-centre white dash while doing (7) (check the fixed HUD widgets too).
9. Read `ACastle::HPBarWidget`'s widget class + WBP_CastleHealthBar's fill colour (closes the last open cell in §4's table).
10. DECLARED UNOBSERVABLE IN SIE (never fake): the hero-attached bar behavior (no pawn exists — `SC-§35`), and every M7 cooked-only mechanism (n/a until the project packages).

## 7. COSTED REPAIR SKETCH — ⛔ INPUT TO THE MANAGER'S SPEC ONLY (CF-R1), NOT AUTHORIZATION

**Primary: lane C needs NO commander-side repair.** The evidence says fix lanes A (berm), B (floor support), D (lighting) and the commander becomes findable for free — he is already standing lit-relative-to-his-hall, animated, at the right spot, with his table.

Optional findability polish, each independently costed, none load-bearing:
- **(a) Zero-code, Jonathan-side confirmation**: after the A/B/D repairs land, walk in and look at (−465, 810) — cost 0. The strafe test (§4) resolves the blue bar in the same session, cost 0.
- **(b) Table-side candle/brazier light**: one small point light on the war table (could reuse the ATorch pattern or an emissive parchment tweak to `M_WarTable`) — makes the commander corner self-marking in any lighting. Cost: small C++ or material edit + compile + QA gate (G4 binds the compile timing), OR possibly config/BP-only if done as a 7th torch anchor near the hall centre — but the 7-anchor route interacts with `MaxTorchesPerCastle` law (WR-§4) and belongs to lane D's dial set, not here.
- **(c) Interact-prompt/nameplate widget on ACommanderNpc** (screen-space label inside InteractRadius): new widget class + ~30 lines C++ + compile + QA. Legal under WR-§5 (purely additive cosmetics; no gate moved). Defer until after A/B/D — likely unnecessary.
- **(d) Local-hero overhead-bar suppression** (the blue-bar confusion killer): the hero already has WBP_HUD HP; hiding the overhead bar for the locally-controlled hero is ~5 lines in `AHeroCharacter` (or `bShowHealthBar=false` on the hero BP — zero compile). ⚠️ Design call — the bar may be wanted for the M8 remote hero; flag to Jonathan, do not default it.

## 8. FILES READ (all read-only)

`Source/GitClaudeUnrealTest/Siegebound/CommanderNpc.{h,cpp}` · `Castle.{h,cpp}` (ctor/furnishing `:97-314`, spawn lane `:460-648`) · `CombatantHealthBarComponent.h` · `HeroCharacter.cpp` (`:81-87`, `:680-727`) · `CastleHealthBarWidget.cpp` (colour grep — no fill colour in C++) · `handoffs/TASK-612-buildmaster.md` · `handoffs/TASK-566-buildmaster.md` · `handoffs/TASK-556-artist.md` · board TASK-588 lineage (`W5-R3`/`W6-R3`) · both evidence PNGs · `Saved/Logs/GitClaudeUnrealTest.log` + the three 2026-08-17 backups (grep-only) · binary name-scans of `BP_CommanderNpc.uasset` / `SM_WarTable.uasset` (never opened for write).
