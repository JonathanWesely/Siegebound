<!-- ARCHIVED from .claude/pipeline/TASKBOARD.md on 2026-10-04 by Tools/archive_board.py. Every row below was in a terminal state when moved; bytes are unchanged and this comment is the only addition. Law: TASKBOARD.md '## Archive'. -->
## W-UNIT-1 tasks (decomposed 2026-07-26) — Wizard AoE fireball unit (TASK-298..305) — ⚠️ RECONSTRUCTED-FROM-HANDOFFS

> # ⚠️ RECONSTRUCTED-FROM-HANDOFFS — 2026-07-26 (manager). THIS IS NOT THE ORIGINAL SECTION.
>
> **What happened:** on the morning of 2026-07-26 a stray `git reset --hard` + `git clean -fd` run by a build-master agent destroyed every uncommitted working-tree file. `TASKBOARD.md` had not been committed since **`e01bc2e`** (TASK-297, 2026-07-25), so the original `## W-UNIT-1 tasks` section (~190 lines, authored earlier that same morning) existed ONLY in the working tree and is **permanently gone**.
>
> **Git is NOT a recovery source for it** — verified directly: `git show 0d717c0:.claude/pipeline/TASKBOARD.md` contains **zero** occurrences of `W-UNIT-1`. Any instruction to "restore this section from git" is wrong; see the corrected 🚨 PIPELINE-FILE REVERT block under Active tasks.
>
> **This section was REBUILT from the surviving COMMITTED artifacts:** `handoffs/TASK-{298,299,307}-programmer.md` · `handoffs/TASK-{300,301,302,303}-artist.md` · `qa/TASK-{298,299,307}.md` · the shipped commit **`0d717c0`** · CONVENTIONS "Wizard unit — AoE fireball caster (2026-07-26)" (itself restored from manager session context, committed in `1a04971`).
>
> **Fidelity contract — read before citing this section.** Every entry below is marked either **EVIDENCED** (traceable to a named artifact) or **⚠️ INFERRED** / **⚠️ NOT RECOVERED**. The original spec wording, the original dispatch order, the original per-task `parallel-safe` flags, and the original `blocked-by` graph are **NOT recoverable verbatim** — what is recorded is the chain the artifacts prove actually ran. **When exact original spec text matters, cite the `handoffs/` file, never this reconstruction.**
>
> **The Wizard itself is SHIPPED and was never at risk:** commit **`0d717c0`** on `main` (NOT pushed) — "Wizard AoE fireball unit (SM/SK/BP/card/anims/LODs, splash via existing AoE path) + systemic feet-grounding fix … Jonathan-approved look; grounded post-fix." This was a documentation loss, not an asset loss.

**Directive (Jonathan, 2026-07-26 — EVIDENCED, quoted in CONVENTIONS):** a new ranged unit *"similar to the Archer but instead of arrows it shoots fireballs that deal Area-of-Effect (splash) damage — the fireball hits a target and damages everything within a radius."* A STANDALONE unit-add batch (label **W-UNIT-1**), NOT part of M7.6/M7.7. The Wizard is the ARCHER behavioural template — a `Standard`-profile ranged `ASummonedUnit` (so it obeys the Shield-Wall ATTACK/HOLD/DEFEND commands exactly like the Archer) — whose homing projectile deals RADIAL splash on impact by REUSING the shipped AoE path, never a new one.

**Manager rulings (recovered from CONVENTIONS "Wizard unit — AoE fireball caster (2026-07-26)", which is the surviving authority for this batch's law):**
1. **AoE = REUSE, not reinvent (LOAD-BEARING).** The splash is the EXISTING TASK-056 projectile-AoE path (`AProjectile::InitProjectile(..., InAoERadius)` → `FSiegeCombatStatics::ApplyRadialDamage`). The ONLY gameplay code change is that `ASummonedUnit::FireProjectileAt` PASSES the row-bound `AoERadius` into `InitProjectile`. Every existing ranged shot has `AoERadius == 0` ⇒ byte-for-byte unchanged.
2. **No new CSV column.** The splash rides the EXISTING `FCardRow.AoERadius`; Wizard default `250` (FLAGGED tunable).
3. **The look is a BP subclass, the gameplay is data.** `ASummonedUnit` gains one optional `TSubclassOf<AProjectile> ProjectileClass` (default null, null-safe fallback to base `AProjectile`); `BP_Unit_Wizard` sets it to `BP_Projectile_Fireball`. The Fire_Magic pack is a READ-ONLY donor (template-donor rule).
4. **Stats are Jonathan-default + FLAGGED** ("tune at a later playtest"): Cost 24 / MaxCopies 4 / HP 45 / Damage 15 / Range 700 / Cadence 1.6 / Speed 350 / Profile Standard / bRanged true / AoERadius 250 / **DeckCount 0** (preserves the `sum(DeckCount)==50` invariant).
5. **Deck-builder description is AUTO-GENERATED** from the row by M7.7's `UDeckBuilderWidget::GetCardDescription` — no authored `Notes`/`Description`. **TRUTH LAW:** the splash line is truthful only once ruling 1 ships, so the batch ships together; QA on the code task confirms the path actually splashes.
6. **⚠️ NOT RECOVERED:** any *additional* rulings the original section carried beyond what CONVENTIONS records (e.g. the original dispatch-map paragraph, the parallel/editor-gated split for this batch, any flagged-for-Jonathan list). The FLEET-REMASTER dispatch map does record one fact about this batch: **"Wizard TASK-304/305"** were EDITOR-GATED and had to serialize with M7.7 TASK-269..272 and TASK-297.

### ✅ BATCH RESULT — WIZARD SHIPPED (EVIDENCED by commit `0d717c0`, main, NO push)

| Deliverable | Path | Evidence |
|---|---|---|
| AoE pass-through + `ProjectileClass` hook | `Source/…/Siegebound/SummonedUnit.{h,cpp}` | `handoffs/TASK-298-programmer.md`, `qa/TASK-298.md` (PASS, loop 2) |
| Card row | `Docs/Data/cards.csv` → `/Game/Data/DT_Cards` | `handoffs/TASK-299-programmer.md`, `qa/TASK-299.md` (PASS) |
| Concept | `Content/RawAssets/Concepts/Wizard.png` + `Tools/ArtPipeline/Inbox/Wizard.png` | `handoffs/TASK-300-artist.md` |
| Static mesh + textures + MI | `/Game/Meshes/SM_Wizard`, `/Game/Textures/T_Wizard_{D,N,ORM}`, `MI_Wizard_PBR` | `handoffs/TASK-301-artist.md` |
| Skeletal runtime + 4 anims + LOD recipe | `/Game/Characters/SK_Wizard`, `/Game/Characters/Anims/A_Wizard_{Idle,Walk,Attack,Death}` | `handoffs/TASK-302-artist.md` |
| Card art | `/Game/UI/CardArt/T_CardArt_Wizard` | `handoffs/TASK-303-artist.md` |
| Blueprints | `BP_Unit_Wizard`, `BP_Projectile_Fireball` | commit `0d717c0` message (⚠️ no build-master handoff survives) |
| Systemic feet-grounding fix (born from this batch's float) | `ResolveSkeletalVisual` | TASK-306/307/307-QA/308 — recorded in **## FLEET-REMASTER tasks** below, shipped in the SAME commit |

---

#### TASK-298 — [W-code] Wizard fireball AoE pass-through + optional per-unit `ProjectileClass` (gameplay-programmer, C++ file-only) — EVIDENCED
- assignee: gameplay-programmer
- status: **done** — qa-passed on loop 2, shipped in `0d717c0`. (Loop 1 FAILED on one blocker: the `SpawnClass` ternary mixed `TSubclassOf<AProjectile>` and `UClass*` ⇒ MSVC **C2445** under UE 5.8 `/permissive-`; fixed with `.Get()` per the `BattlefieldScatter.cpp:1038` precedent. Loop 2 PASS, diff-checked as the only delta.)
- blocked-by: none · parallel-safe: yes (file-only; disjoint from every art task)
- spec: >
    Two surgical, additive, backward-compatible changes to `ASummonedUnit`, nothing else in either file:
    (1) `FireProjectileAt` passes the EXISTING row-bound `AoERadius` as the 5th arg of `InitProjectile` (it was omitted ⇒ defaulted 0 ⇒ every ranged unit single-target). Reuses the shipped TASK-056 `HandleImpact`→`ApplyRadialDamage` path — NO new AoE routine, no friendly fire, castle-side 50% projectile scaling preserved.
    (2) Add `UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Siegebound|Unit") TSubclassOf<AProjectile> ProjectileClass` (default null) + forward-decl; spawn `ProjectileClass.Get()` when set, else `AProjectile::StaticClass()` — the null-safe fallback IS today's behaviour for every existing unit.
    File-only: no compile, no editor, no Git, no MCP. Write `handoffs/TASK-298-programmer.md`. Post in ⚙️ Dev & QA.
- names: > `Source/GitClaudeUnrealTest/Siegebound/SummonedUnit.{h,cpp}` · `AProjectile::InitProjectile` · `ProjectileClass` · law: CONVENTIONS "Wizard unit — AoE fireball caster (2026-07-26)".
- ⚠️ RECONSTRUCTION NOTE: spec text above is the manager's re-statement of what the handoff proves was built. Original spec wording NOT recovered.

#### TASK-298-QA — [W-code QA] Review TASK-298 — EVIDENCED (report), ⚠️ INFERRED (the `-QA` ID form)
- assignee: qa-reviewer
- status: **done — qa-passed** (`qa/TASK-298.md`; loop 1 FAIL → loop 2 PASS, 1 QA loop consumed)
- blocked-by: TASK-298 · parallel-safe: no
- spec: > Pre-compile review. Confirm: no existing `ASummonedUnit` ranged caller has `AoERadius > 0` (⇒ single-target unchanged); the radial path is the EXISTING one, not a re-implementation, with no friendly-fire hole; `ProjectileClass` null-safe fallback; and the TRUTH gate — a `bRanged` row with `AoERadius > 0` genuinely splashes, so M7.7's auto-description is not claiming a rule the code lacks.
- names: > Report `qa/TASK-298.md`.
- ⚠️ The board's original entry may have been titled differently (the FLEET-REMASTER precedent is `TASK-307-QA`, which is why that form is used here). The REPORT PATH is certain.

#### TASK-299 — [W-data] Wizard row in `cards.csv` (gameplay-programmer, data file-only) — EVIDENCED
- assignee: gameplay-programmer
- status: **done** — qa-passed, shipped in `0d717c0` (DT_Cards reimport happened in the build-master step)
- blocked-by: none · parallel-safe: yes
- spec: >
    Append ONE row to `Docs/Data/cards.csv` — 31 columns, no header change, no reorder, no other row touched. Values per the CONVENTIONS Wizard defaults (Cost 24 / MaxCopies 4 / HP 45 / Damage 15 / Range 700 / Cadence 1.6 / Speed 350 / Profile Standard / bRanged true / AoERadius 250 / **DeckCount 0** so `sum(DeckCount)` stays **50**). `CardArt` cell = the full object path. `Notes` must contain NO comma (unquoted-field style). `Docs/Data/cards.csv` is the source of truth; the `/Game/Data/DT_Cards` reimport is the build-master's.
- names: > `Docs/Data/cards.csv` row `Wizard` · `CardArt` = `/Game/UI/CardArt/T_CardArt_Wizard.T_CardArt_Wizard` · law: CONVENTIONS "Data-driven card stats" + "Wizard unit".

#### TASK-299-QA — [W-data QA] Review TASK-299 — EVIDENCED (report), ⚠️ INFERRED (ID form)
- assignee: qa-reviewer
- status: **done — qa-passed** (`qa/TASK-299.md`, zero findings)
- blocked-by: TASK-299 · parallel-safe: no
- spec: > Data review: 31-column schema match + no reorder; values match the CONVENTIONS defaults exactly; `AoERadius 250` present (drives TASK-298's splash + the truthful auto-description); DeckCount sum still 50; CSV integrity (no comma in `Notes`, trailing empty `SpellDelivery`).
- names: > Report `qa/TASK-299.md`.

#### TASK-300 — [W-concept] Wizard concept art (art-director, no-editor) — EVIDENCED
- assignee: art-director
- status: **done** (2026-07-26 — FLUX.1-dev via `concept_generate.py`, seed **71017**, 1024², prompt entry added to `concept_prompts.json`)
- blocked-by: none · parallel-safe: yes (no editor, no MCP, no Git)
- spec: >
    Generate the Wizard concept to the CURRENT roster house style (`concept_prompts.json` `_doc.art_direction`: polished stylized low-poly, Warcraft-Rumble/Fortnite tier), mirroring the **Cleric** robed-caster grammar re-themed holy → offensive fire. **Team-agnostic** — no baked blue/red; the neutral cream hood-mantle/stole is the intended TeamRegion; the warm-orange fireball is an ability colour key, distinct from team-red. Meshy-ready: single centred full-body subject, arms separated from the torso, plain grey background, even light. Write the SAME final image to BOTH the pipeline pickup and the archived acceptance concept.
- names: > `Tools/ArtPipeline/Inbox/Wizard.png` (gitignored pickup) · `Content/RawAssets/Concepts/Wizard.png` (committed acceptance concept) · `Tools/ArtPipeline/concept_prompts.json` (new `Wizard` entry, seed 71017).
- NOTE (carried, still live): Norton TLS interception broke `provider=auto` SSL; fixed with the combined CA bundle at `Tools/ArtPipeline/Cache/_certs/win-ca-bundle.pem` via `SSL_CERT_FILE`/`REQUESTS_CA_BUNDLE`/`CURL_CA_BUNDLE`. Reusable; rebuild after a Windows rollback.

#### TASK-301 — [W-model] `SM_Wizard` game-ready textured static mesh (art-director) — EVIDENCED
- assignee: art-director
- status: **done** — GEN + Stage-2 complete no-editor; the UE import was deliberately DEFERRED to the editor-gated build step (turnkey recipe in the handoff). Shipped in `0d717c0`.
- blocked-by: TASK-300 · parallel-safe: yes for the GEN portion; the UE import is editor-gated (serialize)
- spec: >
    Meshy image-to-3D from `Inbox/Wizard.png` (`meshy_generate.py --mode image3d`; task `019f9fcc-…`, 30 credits) → headless Blender Stage-2 refine → **15,000 tris**, `UVMap`, feet-centre origin (min_z 0.065), two-slot `[0 TeamRegion, 1 WizardPBR]`, D/N/ORM 1024² with the albedo de-light applied. Pre-import eyeball gate against `Concepts/Wizard.png` (M7.5 colour-fidelity bar). Then (EDITOR-GATED) import textures + `MI_Wizard_PBR` from master `M_AssetPBR` + the mesh at `/Game/Meshes/SM_Wizard`, **Nanite OFF**, ≤4 simple hulls, slots assigned in order.
- names: > `/Game/Meshes/SM_Wizard` · `/Game/Textures/T_Wizard_{D,N,ORM}` (D sRGB ON; N/ORM LINEAR) · `MI_Wizard_PBR` ← `M_AssetPBR` · raw `Content/RawAssets/Wizard.fbx`. Law: CONVENTIONS "Wizard unit" + "Textured mesh law" + "Meshy second engine (M7.5)".
- **PATH RECONCILIATION (recorded by the artist, binding):** the naming block says the raw FBX lives at `Content/RawAssets/Characters/Wizard.fbx`; the LIVE two-tier convention (matching every roster unit) puts the **STATIC** FBX at `Content/RawAssets/Wizard.fbx` and reserves `Characters/` for the **RIGGED** variant. The live convention was followed.
- Flagged, non-blocking: the held fireball's flame has thin spiky protrusions (Meshy's read of a 2D flame) — cosmetic caster prop only; the gameplay projectile is the Niagara fireball on `BP_Projectile_Fireball`. WATCH: the caster's footprint (outstretched arm + staff) is wider than a stock humanoid — same class as the Cavalry/Ogre weapon overhang.

#### TASK-302 — [W-rig] `SK_Wizard` skeletal rig + 4 anim clips + SK-LOD recipe (art-director) — EVIDENCED
- assignee: art-director
- status: **done** — headless Blender rig complete; UE import DEFERRED to the editor-gated build step (turnkey recipe in the handoff). Shipped in `0d717c0`.
- blocked-by: TASK-301 · parallel-safe: yes for the GEN portion; the UE import is editor-gated (serialize)
- spec: >
    Rig the TASK-301 mesh via `rig_character.py --card-id Wizard` onto the **SHARED SiegeBiped** (21 bones, armature root exported as the constant `Footman_Rig` per the TASK-212 law) so UE binds to the EXISTING `/Game/Characters/SK_Footman_Skeleton` with no missing-bones warning — **NOT a bespoke skeleton**. Author the 4 per-unit clips; the attack style is **`cast`/hurl** (staff-raise then forward drive), deliberately NOT the Archer bow-draw. Emit the SK-LOD recipe sidecar. Then (EDITOR-GATED) import `SK_Wizard` + the 4 sequences against that same skeleton (root-motion OFF + force_root_lock), apply LOD1 50%@0.4 / LOD2 20%@0.15 (`lod_count == 3`).
    **No `ABP_Wizard` is authored** — `ResolveSkeletalVisual` falls back to the shared `ABP_Footman` for every fleet unit; the Wizard shares the skeleton so the shared locomotion drives it with ZERO new asset.
- names: > `/Game/Characters/SK_Wizard` · skeleton `/Game/Characters/SK_Footman_Skeleton` (shared) · `/Game/Characters/Anims/A_Wizard_{Idle,Walk,Attack,Death}` · shared `ABP_Footman` (no `ABP_Wizard`) · raw `Content/RawAssets/Characters/Wizard.fbx` + `Wizard.lod.json` + `Characters/Anims/Wizard_*.fbx`. Law: CONVENTIONS "Skeletal rig & animation workstream (M7)" + "Wizard unit" + the M7.6 SK-unit LOD law.
- Flagged, non-blocking: a true two-handed fireball hurl (left-hand throw) or a higher-fidelity Meshy cast clip is an optional later tuning pass; the procedural `cast` reads correctly at the baseline. Cadence check: `A_Wizard_Attack` 40f@30fps ≈ 1.33 s fits inside the 1.6 s cadence with no rate scaling.
- **HISTORICAL NOTE (superseded):** this handoff's §5 stated the float-fix was automatic via the v1 `VisualMeshBaseRelativeLocation` copy. That assumption is exactly what FAILED on the Wizard and triggered TASK-306→308's systemic fix. Read §5 as superseded by TASK-307.

#### TASK-303 — [W-cardart] `T_CardArt_Wizard` card art (art-director) — EVIDENCED
- assignee: art-director
- status: **done** — PNG staged + verified 512×512; the UE import was editor-gated and ran in the build step. Shipped in `0d717c0`.
- blocked-by: TASK-300 · parallel-safe: yes (independent of TASK-301/302)
- spec: >
    Match the ROSTER card-art recipe (the 28 existing `T_CardArt_*` are Blender EEVEE renders of low-poly "board-game token" figures on a colour-keyed studio backdrop — TASK-077 recipe), NOT a crop of the full-body concept. 512×512 RGB, no alpha, **no baked text** (runtime overlays DisplayName/cost), team-agnostic palette, one dominant subject with headroom top + floor strip bottom. Token-tier translation of the concept identity: hooded robe, bushy beard, glowing amber eyes, red-orb staff, bright fireball in the raised casting hand; arcane-plum key colour chosen distinct from every key already in use.
- names: > source `Content/RawAssets/CardArt/Wizard.png` → `/Game/UI/CardArt/T_CardArt_Wizard` (Texture Group **UI**, sRGB ON, TC_Default). CSV cell already points at `/Game/UI/CardArt/T_CardArt_Wizard.T_CardArt_Wizard`. Law: CONVENTIONS "Card artwork (hand UI)".

#### TASK-304 — [W-int-1] Compile + editor-gated imports + Blueprint authoring + `DT_Cards` reimport (build-master) — ⚠️ SCOPE INFERRED
- assignee: build-master
- status: **done** — shipped in `0d717c0` (main, NO push)
- blocked-by: TASK-298-QA, TASK-299-QA, TASK-301, TASK-303 · parallel-safe: no (exclusive editor + Git)
- spec: >
    ⚠️ **INFERRED, NOT RECOVERED — no build-master handoff for this task survives.** Reconstructed scope, from three independent artifact statements: `handoffs/TASK-298-programmer.md` ("TASK-304 (build-master) owns the compile + `BP_Unit_Wizard` authoring — `ProjectileClass = BP_Projectile_Fireball`, `VisualMesh = SM_Wizard`"), `qa/TASK-298.md` ("Ready for the TASK-304 compile … pair with the TASK-299 `DT_Cards` reimport"), and `handoffs/TASK-302-artist.md` ("Feeds TASK-304 (build-master assemble); this import serializes behind TASK-301's static-mesh import — `MI_Wizard_PBR` must exist first"):
    compile the TASK-298 code (hard gate); run the deferred editor-gated Stage-3 imports (textures → `MI_Wizard_PBR` → `SM_Wizard`; card art `T_CardArt_Wizard`); reimport `/Game/Data/DT_Cards` same-path from `cards.csv` and read the Wizard row back; author `BP_Unit_Wizard` (`ASummonedUnit` subclass — CardID `Wizard`, `VisualMesh = SM_Wizard`, `ProjectileClass = BP_Projectile_Fireball`) and `BP_Projectile_Fireball` (`AProjectile` subclass, VISUALS ONLY — Fire_Magic Niagara systems soft-referenced, donor never edited).
- names: > `BP_Unit_Wizard` at `/Game/Blueprints/Units/BP_Unit_Wizard` (spawn path `…BP_Unit_Wizard.BP_Unit_Wizard_C`) · `BP_Projectile_Fireball` at `/Game/Blueprints/BP_Projectile_Fireball` · `NS_Fire_Magic_{Projectile,Explosion,Muzzle}` (READ-ONLY donors) · `/Game/Data/DT_Cards`. Commit on main, no push.
- ⚠️ **The exact TASK-304 / TASK-305 split is INFERRED.** What is CERTAIN: both IDs existed, both were editor-gated and had to serialize with M7.7 TASK-269..272 + TASK-297 (recorded in the FLEET-REMASTER dispatch map), and the work landed in `0d717c0`.

#### TASK-305 — [W-int-2] `SK_Wizard` + anims import, LOD apply, in-engine verify, commit (build-master) — ⚠️ SCOPE INFERRED
- assignee: build-master
- status: **done** — shipped in `0d717c0` (main, NO push). **Jonathan-approved look** (commit message), and the Wizard verified GROUNDED only after TASK-307/308 landed in the same commit.
- blocked-by: TASK-302, TASK-304 · parallel-safe: no (exclusive editor + Git)
- spec: >
    ⚠️ **INFERRED, NOT RECOVERED — no build-master handoff survives.** Reconstructed scope: import `SK_Wizard` + the 4 `A_Wizard_*` sequences against `SK_Footman_Skeleton`, apply the SK-LOD chain (`lod_count == 3`) + confirm URO is live from C++, verify in-engine (skeletal visual resolves from CardID, anims bind and play, slot-0 team recolour tints the mantle, splash actually damages a group), and commit on main with explicit pathspecs, NO push.
    **Independent corroboration that a TASK-305 verify happened:** CONVENTIONS' shared-skeleton clause cites "confirmed at TASK-297/305" as the evidence that `SK_Footman_Skeleton` is the shipped shared skeleton.
- names: > `/Game/Characters/SK_Wizard` · `/Game/Characters/Anims/A_Wizard_{Idle,Walk,Attack,Death}` · skeleton `SK_Footman_Skeleton` · commit on main, no push.

---

### ⚠️ WHAT COULD **NOT** BE RECOVERED (do not fill these in from memory — they are gone)

1. **The original spec/`names` prose for every task.** Each entry above is the manager's re-statement built from what the handoffs prove was BUILT. Acceptance criteria that were specced but never mentioned in a handoff are lost.
2. **The original `blocked-by` graph and `parallel-safe` flags.** The dependencies shown are re-derived from artifact statements ("Downstream: TASK-304…", "serializes behind TASK-301's import…"). They are correct in substance, not necessarily in original form.
3. **TASK-304 and TASK-305 — the exact scope split.** No build-master handoff exists for either (they wrote none, or theirs died with the wipe). The two-way split above is INFERRED; the union of the two is certain (it is what `0d717c0` contains).
4. **Whether a QA task ID existed for TASK-298/299, and in what form.** The QA REPORTS are certain (`qa/TASK-298.md`, `qa/TASK-299.md`); the board entry IDs `TASK-298-QA` / `TASK-299-QA` follow the `TASK-307-QA` precedent and are INFERRED.
5. **This batch's dispatch-map paragraph and any flagged-for-Jonathan list.** FLEET-REMASTER preserved one line about it ("Wizard TASK-304/305" = editor-gated, serialize); nothing else survived.
6. **Any status history / timestamps** — the "was: ready-for-qa … was: backlog" trail that board entries normally carry. Only the FINAL state is recoverable, and it is `done` for all eight tasks.
7. **Slack thread ts values** for this batch's posts (they exist in Slack, not in the files — recoverable by reading `#siegeboundue5agentteam` if ever needed).

---

