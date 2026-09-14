# TASK-1217 — [AURA-INDEX-IGNORE] — programmer handoff (2026-09-13)

**Status:** ready-for-qa · gate `TASK-1232` · host `TASK-1240`

## What was written
- `Docs/AuraIndexIgnore.txt` (NEW, 146 lines: 102 patterns, 28 comment lines, 16 blank). Canonical, committed copy; `Tools/aura_sync.ps1` (TASK-1219) copies it over `Saved/.Aura/INDEX_IGNORE.txt`.
- This handoff. **Nothing under `Saved/` was written or touched** (read-only `Read` of `Saved/.Aura/INDEX_IGNORE.txt` to learn the template; `git status` shows no `Saved/` entry — it is gitignored regardless).

## Location law (plan *Corrections* row 1 — overrides source doc §4 Phase 2 step 3)
The live file is `<Project>/Saved/.Aura/INDEX_IGNORE.txt`, NOT project root. Verified twice: (a) WebFetch of https://www.tryaura.dev/documentation/project-understanding/ says it lives in `<ProjectName>/Saved/.Aura`; (b) Aura already generated `Saved/.Aura/INDEX_IGNORE.txt` on first run (measured, present on disk). Patterns inside are relative to the PROJECT root (`C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest`), per the template's own header line.

## Comment-syntax finding (acceptance item 4)
The Aura doc page is silent on syntax (no mention of `#`, globs, or `!`). The decisive evidence is Aura's OWN generated template: it contains `# Folders to ignore...`, `# Unreal Engine specific`, etc., plus file globs (`*.generated.h`, `**/.tmp/**`, `**/*.Target.cs`). So `#` comments and globs are accepted by construction; nothing in the template uses `!`, so negation is assumed ABSENT and the keep-set is protected purely by never matching it. Header is 5 comment lines (the row asked for "two-line"; the extra lines carry the trailing-slash rule the orchestrator asked for and the copy-step name — trim to two if QA prefers, no pattern depends on it).

## Plan item 2 exclude count: 22 / 22 present, verbatim
`Content/Fab/` · `Content/MedievalCastleEnvironmentAndSiegeWeaponProps/` · `Content/Megaplant_Library/` · `Content/Realistic_Grass_and_plant/` · `Content/Realistic_Rocks/` · `Content/Tree_Pack_1/` · `Content/sA_ArcheryVfxPack/` · `Content/sA_StylizedWizardSet/` · `Content/Prickly_Knight/` · `Content/RawAssets/` · `Content/ThirdPerson/` · `Content/Variant_*/` · `Content/LevelPrototyping/` · `Models/` · `Intermediate/` · `Saved/` · `Binaries/` · `DerivedDataCache/` · `Tools/ArtPipeline/.venv/` · `Tools/ArtPipeline/Cache/` · `.claude/pipeline/playtest-evidence/` · `packagedZIPofGame/`

- **`Content/Variant_*/` is a glob** (acceptance item 3): expands to the three template variants on disk — `Variant_Combat/`, `Variant_Platforming/`, `Variant_SideScrolling/`. Kept as the plan's glob form rather than three literals because the template proves `*` is honoured.
- **`packagedZIPofGame/` matches NOTHING** and QA should know why: the folder lives at the GIT root (`C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame`), one level ABOVE the project root, so a project-root-relative pattern cannot reach it — it is already outside Aura's index scope. Kept verbatim for parity with the plan (count = 22), with an inline comment saying so. Not a defect; a no-op.

## Superset of Aura's default template (orchestrator's shape correction)
Because `aura_sync.ps1` REPLACES the generated file, the canonical copy carries every useful default forward: engine noise (`**/Intermediate/**`, `**/Binaries/**`, `**__ExternalActors__**`, `Build/`, `*.generated.*`), IDE (`.vs/`, `*.sln`, `.idea/`, `.vscode/`), Python, Git/Plastic/SVN, OS files, the `Plugins/Aura|AuraForUnity|UnrealMCP|Ramen|node_modules/` lines, `Config/`, `Content/{3rdParty,Collections,Art,FirstPersonArms,StarterContent,__ExternalActors__,__ExternalObjects__}/`.
**Two defaults deliberately DROPPED** (both would eat the keep-set): `Content/Characters/` and the blanket `.claude/` + `**/.claude/**`. Duplicated `.git/` / `**/.git/**` lines in the template were collapsed to one pair.
**One addition beyond the template:** `*.slnx` (two `.slnx` files sit at project root beside the `.sln` files the template already drops — same IDE noise).

## `.claude/` — excluded piecewise instead of wholesale
`.claude/pipeline/TASKBOARD.md` (8.4 MB) + a defensive `**/TASKBOARD.md` glob in case a bare file path is not honoured as a folder line · `.claude/pipeline/qa/` (6.6 MB) · `.claude/pipeline/handoffs/` (547 MB — carries evidence PNGs) · `.claude/pipeline/footage/` · `.claude/pipeline/fab/` · `.claude/commands/` · `.claude/agents/` · `.claude/hooks/` · `.claude/tmp/`. What REMAINS indexed under `.claude/`: `pipeline/CONVENTIONS.md` (keep-set, 2.9 MB), `pipeline/SLACK.md`, the three small brief/checkpoint `.md` files, `settings*.json`.

## Keep-set — reasoning per entry (acceptance item 2), simulated + reasoned
Mechanical check: every one of the 102 patterns was run (Python `fnmatch`, folder-prefix and any-segment semantics) against representative paths in each keep entry — **0 hits**. Reasoning per entry:
- `Source/` — no pattern names `Source`; `*.generated.h/.cpp` only exist under `Intermediate/` (UHT output), not in `Source/`; `**/*.Target.cs` matches only `Source/GitClaudeUnrealTest*.Target.cs` (build rules, not gameplay code — Aura's own default, kept); `*.so` / `lib/` etc. have no counterpart under `Source/`.
- `Content/Blueprints`, `Content/UI`, `Content/Data`, `Content/Maps`, `Content/Materials`, `Content/VFX`, `Content/Input`, `Content/Meshes`, `Content/Textures` — every `Content/...` pattern is a literal sibling folder name or the `Variant_*` glob, none of which is a prefix or glob-match of these names (`Content/Art/` ≠ `Content/Materials/`; `Realistic_*`-style globs were NOT used; the plan's literal `Realistic_Grass_and_plant/` and `Realistic_Rocks/` are used instead). `**__ExternalActors__**` matches only the OFPA folder, not `Content/Maps/`.
- `Content/Characters` — the template's `Content/Characters/` line was removed; no remaining pattern names it. (`Characters/Mannequins` and `Characters/Anims` ride along; 227 MB of `.uasset`s — accepted, it is in the keep-set.)
- `Docs/GDD.md` — no `Docs/` pattern exists at all.
- `.claude/pipeline/CONVENTIONS.md` — the blanket `.claude/` and `**/.claude/**` lines are gone; every `.claude/...` pattern is a specific sibling file/folder (`TASKBOARD.md`, `qa/`, `handoffs/`, ...) and `**/TASKBOARD.md` cannot match `CONVENTIONS.md`.

## Things QA / manager may want to rule on (NOT done — outside the plan's verbatim list, `SC-§100`)
1. `Content/Fire_Magic/` (556 MB), `Content/Ice_Magic/` (567 MB), `Content/IceAttack/` (97 MB), `Content/MedievalWeaponsSFX/` (62 MB) are Fab packs too but are NOT in plan item 2 and NOT in the keep-set. Left INDEXED. Total project file count excluding Saved/Intermediate/Binaries/DDC is ~14.3k (the 15 plan Content excludes hold 1,597 files), so the 30k cap is not at risk either way; a manager may still add them for index speed.
2. `Content/__ExternalActors__/` (Aura default, kept) is where `L_Arena`'s OFPA actors live (4.9 MB). Aura's default drops it; if a verifier needs actor-level answers from the index rather than from the live editor, that default could be reconsidered.
3. `Plugins/SiegeLlama/` (218 MB, mostly llama.cpp ThirdParty binaries) is indexed — its C++ is house code; the binaries are `.dll/.lib`, not text.
4. `Config/` is an Aura default and is kept dropped; `DefaultInput.ini`/`DefaultGame.ini` therefore are not indexed.

## Slack
One post in ⚙️ Dev & QA (`C0BF0QZP3CN`, thread `1783116269.740549`).
