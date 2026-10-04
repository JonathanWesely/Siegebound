<!-- MOVED from .claude/pipeline/CONVENTIONS.md on 2026-10-04 by Tools/split_conventions.py. Sections are byte-identical to the original; this comment is the only addition. Cite clauses by tag (e.g. `VER-§3 cl. 6`); CONVENTIONS.md '## Law index' maps tags to files. -->
## ⚖️ THE MAIN CHARACTER — the hero stops being a grey mannequin (2026-09-06) — namespace **`CHAR-§`**

**Trigger — 🧑 Jonathan's directive, verbatim (2026-09-06):** *"create the visual art for the main controllable character, I have added some concept art in the 'ArtPipeline' 'Inbox' folder. It is an image in that folder titled 'MainCharacter.png'. This image features several small images that show different views of the art for the main character. I want you to use the Meshy pipeline to create the 3D asset for the main character, it obviously still needs to have the walking and fighting animations as well. Please make sure the art agent takes its time and follows all the different views from the concept art."*

### CHAR-§1 ✅⛔ **THE FILE, AND ⛔ WHAT IS ACTUALLY IN IT — ⛔ READ BY THE MANAGER, ⛔ NOT ASSUMED**

- ⛔ **PATH: `Tools/ArtPipeline/Inbox/MainCharacter.png`.** ⛔ **⛔ NOT `ArtPipeline/Inbox/` — ⛔ the `Tools/` prefix is ⛔ load-bearing and ⛔ nobody hunts for it.**
- ⛔ **SUBJECT: a ⛔ crusader/templar knight** — ⛔ full plate over mail, ⛔ great helm, ⛔ **white tattered surcoat + cloak bearing a ⛔ RED CROSS**, ⛔ arming sword, ⛔ heater shield.
- ⛔ **IT IS A ⛔ 16-TILE CONTACT SHEET WITH ⛔ TEXT CAPTIONS, ⛔ enumerated here so *"all the views"* is ⛔ a checklist and ⛔ not a mood:**

| ⛔ band | ⛔ tiles |
|---|---|
| ⛔ **ORTHOGRAPHIC BODY VIEWS** (⛔ T-pose) | ⛔ `FRONT` · ⛔ `SIDE` · ⛔ `BACK` |
| ⛔ **HELMET column** | ⛔ 4 stacked sub-tiles: ⛔ front · ⛔ ¾ · ⛔ side · ⛔ back |
| ⛔ **DETAIL tiles** | ⛔ `GAUNTLET` · ⛔ `SHOULDER` · ⛔ `CHEST` · ⛔ `HIP / TASSETS` · ⛔ `LEG` · ⛔ `SABATON` · ⛔ `REAR DETAIL` · ⛔ `CLOAK DETAIL` |
| ⛔ **PROPS** | ⛔ `SWORD (SIDE)` · ⛔ `SWORD DETAIL` · ⛔ `SHIELD (FRONT)` · ⛔ `SHIELD (BACK)` |

### CHAR-§2 🚨⛔⛔⛔⭐⭐⭐ **THE SHEET ⛔ CANNOT BE FED TO THE PIPELINE AS-IS, AND THE PIPELINE'S ⛔ OWN README SAYS SO**

- ⛔ **`Tools/ArtPipeline/README.md` §*"Concept image guidance"*, verbatim: *"⛔ ONE SINGLE SUBJECT — ⛔ no scenes, props, companions, or ⛔ text"* · *"Plain or transparent background… ⛔ clutter confuses the cutout"* · *"square ~1024×1024 PNG recommended."***
- ⇒ 🚨⛔⛔ **`MainCharacter.png` is ⛔ 16 SUBJECTS, ⛔ 2 PROPS AND ⛔ 16 TEXT CAPTIONS ON ⛔ ONE CANVAS. ⛔ IT IS THE ⛔ EXACT OPPOSITE OF WHAT THE GENERATOR WANTS.** ⛔ **Handing it to Stage 1 ⛔ unmodified produces a ⛔ mangled blob, ⛔ spends credits, ⛔ and would look like a ⛔ model failure rather than an ⛔ input failure.**
- ✅⛔ **THE LAW, AND THE ⛔ NEW ARTEFACT PATTERN, ⛔ PINNED: a ⛔ MULTI-VIEW concept sheet is ⛔ SPLIT before it is ⛔ generated from.**
  - ⛔ **`Tools/ArtPipeline/Inbox/<AssetName>.png`** = ⛔ the ⛔ SHEET as 🧑 he dropped it. ⛔ **NEVER modified, ⛔ never deleted, ⛔ never overwritten** — ⛔ it is ⛔ his input and the ⛔ acceptance reference.
  - ⛔ **`Tools/ArtPipeline/Inbox/<AssetName>_Front.png` · `_Side.png` · `_Back.png`** = ⛔ the ⛔ EXTRACTED single-subject views: ⛔ one figure, ⛔ caption text ⛔ cropped out, ⛔ flat background, ⛔ square, ⛔ ≥512 px.
  - ⛔ **Precedent, ⛔ not an invention: `Inbox/Ogre_original_4view.png` sits beside `Inbox/Ogre.png` in this very folder — ⛔ this project has ⛔ met a multi-view sheet before.**
- 🚨⛔⛔⭐⭐ **⛔ AND THE DISTINCTION THAT PROTECTS THE GENERATION: ⛔ THE ⛔ THREE BODY VIEWS ARE ⛔ RECONSTRUCTION INPUT. ⛔ THE ⛔ TWELVE DETAIL TILES ARE ⛔ NOT.**
  - ⛔ **⛔ Feeding a ⛔ GAUNTLET CLOSE-UP into a multi-view reconstruction slot tells the model the ⛔ character IS A HAND. ⛔ That is ⛔ not a hypothetical — ⛔ multi-view solvers assume ⛔ every input frames the ⛔ SAME subject at the ⛔ SAME scale.**
  - ✅ **⛔ THE DETAIL TILES ARE THE ⛔ ACCEPTANCE CHECKLIST at ⛔ `CHAR-§5`, ⛔ and (optionally) the ⛔ STYLE REFERENCE for a ⛔ `--mode retexture` pass. ⛔ They are ⛔ USED — ⛔ just ⛔ not as geometry.**
  - ⇒ ⚖️ ***⛔ THAT IS HOW 🧑 *"FOLLOWS ALL THE DIFFERENT VIEWS"* IS ⛔ HONOURED ⛔ RATHER THAN ⛔ OBEYED LITERALLY INTO A GARBAGE MESH: ⛔ EVERY TILE HAS A JOB, ⛔ AND THE JOBS ARE ⛔ DIFFERENT.***

### CHAR-§3 ⛔⛔ **THE SHIPPED MESHY TOOL SENDS ⛔ ONE IMAGE — ⛔ MEASURED AT SOURCE, ⛔ AND IT IS THE ⛔ GATING FACT OF THIS LANE**

- ⛔ **`Tools/ArtPipeline/meshy_generate.py` ⛔ EXISTS and is the ⛔ project's Meshy route** (⛔ *"Second engine per CONVENTIONS.md ⛔ 'Meshy second engine (M7.5)'"*), with ⛔ two modes: ⛔ `--mode retexture` and ⛔ `--mode image3d`.
- 🚨⛔⛔ **`--mode image3d` reads ⛔ `Inbox/<CardID>.png` — ⛔ SINGULAR — and the payload is ⛔ `payload["image_url"] = image_data_uri(concept_path)` (⛔ `:1496`), against ⛔ `EP_IMAGE3D = "/openapi/v1/image-to-3d"` (⛔ `:106`).** ⇒ ⛔⛔ **⛔ THERE IS ⛔ NO MULTI-IMAGE PATH IN THE SHIPPED TOOL. ⛔ A row that says *"run Meshy with all the views"* would be ⛔ UNEXECUTABLE, and the assignee would ⛔ discover that ⛔ after the dispatch.**
- ✅⛔ **⇒ THE ROUTE IS ⛔ DECIDED ⛔ BY MEASUREMENT, ⛔ NOT BY PREFERENCE, AND ⛔ BOTH BRANCHES ARE BOARDED:**
  - ⛔ **BRANCH A (⛔ preferred, ⛔ honours the ask fully): ⛔ add a ⛔ `--mode multiimage` to the ⛔ existing tool** — ⛔ `TASK-1088`, ⛔ a ⛔ CODE row with a ⛔ QA gate, ⛔ inheriting ⛔ every shipped law of that file ⛔ UNCHANGED: ⛔ the ⛔ ENV-ONLY `MESHY_TOKEN` secret law · ⛔ the ⛔ never-disable-TLS law · ⛔ the ⛔ 3-phase ⛔ download→validate→commit artefact guard (`SC-39.1`) · ⛔ the ⛔ exit-code contract (⛔ `0/1/2/3/4/5/6/64`) · ⛔ `Cache/` staging + ⛔ `_rejected/` quarantine · ⛔ **⛔ output ⛔ NEVER lands in `Content/` — ⛔ everything re-enters through ⛔ Stage 2 (⛔ THE INVARIANT).**
  - ⛔ **BRANCH B (⛔ the declared fallback, ⛔ boarded ⛔ in advance so the lane ⛔ cannot brick): ⛔ single-image `--mode image3d` from ⛔ `MainCharacter_Front.png`, ⛔ with the ⛔ loss stated in the handoff** — ⛔ the ⛔ BACK is where the ⛔ tattered cloak and its ⛔ red cross live, so ⛔ Branch B ⛔ demonstrably loses ⛔ the character's ⛔ most distinctive surface.
  - 🚨⛔⛔ **⛔ `TASK-1088` cl. 0 is a ⛔ PREFLIGHT PROBE (⛔ endpoint + balance), ⛔ BEFORE a line is written.** ⇒ ⛔ **⛔ if the endpoint is ⛔ not on his plan, the row ⛔ STOPS AND REPORTS — ⛔ it does ⛔ not write speculative code against an API nobody can call.** (⛔ `--check` is the ⛔ shipped precedent for exactly this.)

### CHAR-§4 🚨⛔⛔⛔⭐⭐⭐ **THE WALKING AND FIGHTING ANIMATIONS ⛔ ALREADY EXIST. ⛔ THE DELIVERABLE IS THAT THEY ⛔ KEEP BINDING — ⛔ NOT THAT THEY ARE AUTHORED.**

**⛔ MEASURED at `Content/Blueprints/BP_HeroCharacter.uasset` (⛔ name-table census, 2026-09-06):**

| ⛔ slot | ⛔ live value |
|---|---|
| ⛔ skeletal mesh | ⛔ **`SKM_Quinn_Simple`** (⛔ `/Game/Characters/Mannequins/Meshes/`) — 🧑 ⛔ **THE GREY MANNEQUIN IN HIS SCREENSHOT** |
| ⛔ anim blueprint | ⛔ **`ABP_Unarmed`** / ⛔ `ABP_Unarmed_C` — ⛔ **the ⛔ WALK/run/idle/jump locomotion** |
| ⛔ attack montage | ⛔ **`AM_ComboAttack`** (⛔ `HeroCharacter.h:1242`: *"Wired on BP_HeroCharacter in TASK-017 (`/Game/Variant_Combat/Anims/AM_ComboAttack`)"*) — ⛔ **the ⛔ FIGHT** |

- 🚨⛔⛔⛔⭐⭐⭐ **⇒ 🧑 HIS *"it ⛔ obviously still needs to have the ⛔ walking and ⛔ fighting animations as well"* IS ⛔ NOT A REQUEST TO ⛔ AUTHOR TWO ANIMATION SETS. ⛔ IT IS A ⛔ REQUIREMENT THAT THE ⛔ MESH SWAP ⛔ NOT BREAK THE ONES HE ⛔ ALREADY HAS.** ⇒ ⛔ **⛔ read as *"author"*, this lane is ⛔ weeks and needs a ⛔ rigger. ⛔ Read as *"preserve"*, it is a ⛔ MESH + ⛔ SKINNING job. ⛔ THE SECOND READING IS THE ⛔ MEASURED ONE.**
- ✅⛔⛔ **⇒ THE BINDING LAW, AND IT IS THE ⛔ SINGLE MOST IMPORTANT SENTENCE IN THIS NAMESPACE: ⛔ THE NEW KNIGHT ⛔ BINDS TO THE ⛔ HERO'S EXISTING SKELETON — ⛔ `SK_Mannequin`, THE RIG ⛔ `SKM_Quinn_Simple` RIDES.** ⛔ **⛔ Bind it to anything else and ⛔ `ABP_Unarmed` and ⛔ `AM_ComboAttack` ⛔ STOP BINDING, and ⛔ both of the things he named ⛔ disappear.**
- ⛔⛔ **⛔ THE TRAP, ⛔ NAMED SO IT IS ⛔ NOT WALKED INTO: ⛔ THIS PROJECT HAS ⛔ TWO SKELETONS, AND THE ⛔ WELL-TRODDEN ONE IS THE ⛔ WRONG ONE HERE.**
  - ⛔ **The ⛔ UNITS ride ⛔ `/Game/Characters/SK_Footman_Skeleton` (⛔ the 21-bone ⛔ SiegeBiped), share ⛔ `ABP_Footman`, and have a ⛔ SHIPPED retarget chain: ⛔ `IK_MeshyBiped` → ⛔ `RTG_MeshyBiped_to_SiegeBiped` → ⛔ `IK_SiegeBiped`.**
  - ⛔ **The ⛔ HERO does ⛔ NOT.** ⛔ Recorded ⛔ verbatim in this file: *"⛔ NO retarget from `SK_Footman_Skeleton` to the ⛔ hero's mannequin skeleton · ⛔ no hero ABP state machine."*
  - ⇒ 🚨⛔⛔ **⛔ REACHING FOR `RTG_MeshyBiped_to_SiegeBiped` BECAUSE IT ⛔ EXISTS WOULD BIND THE HERO TO THE ⛔ UNITS' RIG AND ⛔ DETACH HIM FROM ⛔ HIS OWN LOCOMOTION, HIS ⛔ COMBAT MONTAGE AND ⛔ EVERY HERO-SPECIFIC ANIM PATH ⛔ AT ONCE. ⛔ IT IS THE ⛔ OBVIOUS MOVE AND IT IS ⛔ WRONG.**
  - ✅ **⛔ The ⛔ SiegeBiped chain is the ⛔ PROOF THE TECHNIQUE WORKS on Meshy output (⛔ a shipped precedent worth having), ⛔ NOT the retargeter to use.** ⛔ **A ⛔ `RTG_MeshyBiped_to_Mannequin` is the ⛔ hero's own asset if a retargeter is needed at all** (⛔ `RTG_` → ⛔ `Content/Characters/`).
- ⭐ **⛔ THE SHAPE OF THIS JOB IS ⛔ ALREADY IN THE LAW, ⛔ under a different feature: *"Anims + ABP are ⛔ PRESERVED, ⛔ NOT regenerated: the remaster changes ⛔ ONLY the mesh + textures + material instance… the anims ⛔ already exist, are ⛔ good, and must ⛔ keep binding after the SK is overwritten in place."*** ⇒ ⛔ **⛔ this is a ⛔ REMASTER-shaped row, ⛔ not the ⛔ Wizard-batch anim-authoring shape. ⛔ Grade it as one.**
- 🧑⛔ **⛔ FLAGGED, ⛔ NOT DECIDED — `J-C1`: ⛔ THE ⛔ SWORD AND ⛔ SHIELD.** ⛔ The sheet gives them ⛔ four dedicated tiles, but ⛔ `ABP_Unarmed` is ⛔ named for what it is, and the hero ⛔ today holds ⛔ nothing. ⇒ ⛔ **⛔ PROCEEDING DEFAULT: ⛔ generate them as ⛔ PART OF THE BODY MESH ⛔ only if the concept's ⛔ T-pose shows them held (⛔ it does ⛔ NOT — ⛔ both hands are ⛔ empty in ⛔ FRONT and ⛔ BACK) ⇒ ⛔ **⛔ THE BODY SHIPS ⛔ UNARMED, ⛔ matching ⛔ both the T-pose ⛔ and `ABP_Unarmed`.** ⛔ Sword/shield as ⛔ socketed props is a ⛔ SEPARATE, ⛔ LATER row and ⛔ needs 🧑 his word. ⛔ **⛔ Do ⛔ not silently fuse a sword to a hand — ⛔ it would ⛔ clip through every existing animation.**

### CHAR-§5 🚨⛔⛔⛔⭐⭐⭐ **THE MESH-APPROVAL CHECKPOINT — 🧑 HIS EYE ⛔ BEFORE THE SKINNING, ⛔ NOT AFTER**

- ⛔ **`TASK-1092` is a ⛔ HARD STOP: ⛔ the mesh is ⛔ generated, ⛔ refined and ⛔ rendered to previews, and ⛔ THEN IT WAITS.** ⛔ **⛔ NO import, ⛔ no skinning, ⛔ no retarget, ⛔ no `BP_HeroCharacter` edit ⛔ until 🧑 he approves the body.**
- ⇒ ⚖️ ***⛔ SKINNING AND RETARGETING ARE THE ⛔ MOST EXPENSIVE STEPS IN THIS LANE AND THE ⛔ LEAST REUSABLE. ⛔ DOING THEM TO A BODY HE THEN REJECTS ⛔ THROWS AWAY ⛔ EXACTLY THE WORK THAT ⛔ CANNOT BE SALVAGED — ⛔ THE MESH CAN BE ⛔ REROLLED FOR CREDITS; ⛔ THE RIG WORK CANNOT BE ⛔ REROLLED AT ALL.***
- ⛔ **⛔ WHAT HE IS SHOWN AT THE CHECKPOINT, ⛔ so the ask is ⛔ answerable in ⛔ one sentence: ⛔ turntable/preview renders at ⛔ FRONT, ⛔ SIDE and ⛔ BACK ⛔ placed ⛔ BESIDE the corresponding ⛔ concept tiles, ⛔ plus the ⛔ `CHAR-§1` ⛔ detail-tile checklist ⛔ ticked or ⛔ declared-missing ⛔ item by item.** ⛔ **⛔ A checklist with ⛔ honest ⛔ MISSING rows is ⛔ worth more than a ⛔ uniformly ticked one** — ⛔ and 🧑 he asked ⛔ twice for the ⛔ views to be followed, ⇒ ⛔ **⛔ *"the ⛔ REAR DETAIL cross is ⛔ absent"* is ⛔ exactly the sentence this checkpoint exists to surface.**
- ⛔ **⛔ THE REROLL LEVER IS ⛔ NAMED ON THE ROW: ⛔ `--seed`, ⛔ quota permitting, ⛔ BEFORE Stage 2 — ⛔ the shipped `README` procedure, ⛔ not an improvisation.**

### CHAR-§6 ⛔ **THE CLOSING RUNG IS A ⛔ BEHAVIOUR, ⛔ NOT A PROPERTY** (⭐ `SC-§94` cl. B · ⭐ `SC-§36.1`)

- ⛔ **⛔ *"the mesh imported"* is ⛔ RUNG 1 (⛔ properties). ⛔ *"`BP_HeroCharacter` references it"* is ⛔ RUNG 2 (⛔ reachability). ⛔ BOTH CAN BE ⛔ TRUE WHILE THE HERO ⛔ T-POSES ACROSS THE FIELD.**
- ✅⛔ **⛔ THE OUTCOME CHECK, ⛔ NAMED ON `TASK-1094` ⛔ WITH ITS OWNER: ⛔ (i) he ⛔ WALKS — ⛔ `ABP_Unarmed` ⛔ drives the new mesh, ⛔ observed ⛔ in motion, ⛔ not inferred from a reference · ⛔ (ii) he ⛔ FIGHTS — ⛔ `AM_ComboAttack` ⛔ plays on the new mesh · ⛔ (iii) ⛔ NO T-pose, ⛔ no exploded skinning, ⛔ no inverted normals · ⛔ (iv) the ⛔ VRAM delta (`FIELD-§3`) — ⛔ **a ⛔ NEW textured character is ⛔ NEW resident memory on a budget that is ⛔ ALREADY 828 MB OVERDRAWN.**
- ⛔ **⛔ AND THE ⛔ VRAM CROSS-LINK IS ⛔ DELIBERATE: ⛔ `FIELD-§` and ⛔ `CHAR-§` are ⛔ INDEPENDENT LANES that ⛔ SPEND THE SAME BUDGET. ⛔ Neither may report its delta as if it were the ⛔ only claimant.**

---

