# TASK-696 — build-master handoff — THE COOK (fence · UAT BuildCookRun · stage · boot-verify)
2026-08-29 · law `PKG-§1..§6` · runs AFTER TASK-694's commit `b21ccc5`, so **the package contains the war-map flip**

## 1. ⛔ THE FENCE — added BEFORE any cook output existed (`PKG-§3`)

`packagedZIPofGame/` **already existed at the git root**, created by Jonathan —
`C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\` (P4 satisfied; nothing created by me).

Added to the **ROOT** `.gitignore` (the `testvideo/` precedent, same block), with the reasoning inline,
**before the first UAT invocation**:

```
packagedZIPofGame/
```

**Proven, not assumed** — `git check-ignore -v` against the folder *and* against future paths inside it:

```
.gitignore:19:packagedZIPofGame/   packagedZIPofGame/
.gitignore:19:packagedZIPofGame/   packagedZIPofGame/Siegebound-Win64-Development-2026-08-29.zip
.gitignore:19:packagedZIPofGame/   packagedZIPofGame/Windows/GitClaudeUnrealTest.exe
```

`git status --porcelain` never showed the folder at any point during or after the cook (re-checked
mid-cook — the TASK-683 auto-stage trap did **not** fire). ⚠️ Worth recording *why* this mattered
so much here: `.gitattributes` makes **`*.dll` and `*.png` LFS patterns**, so an unfenced staged
build would not merely have bloated git — it would have streamed thousands of engine DLLs into LFS.

📌 **MANAGER RIDER — `PKG-§2`'s staging path is off by one directory level.** The law names
`…\GitClaudeUnrealTesting\GitClaudeUnrealTest\packagedZIPofGame\`, but the **git root is
`C:\GitProjects\GitHub\GitClaudeUnrealTesting\`** (porcelain paths are `GitClaudeUnrealTest/`-prefixed)
and Jonathan's real folder is one level up from the law's path. The law's stated INTENT ("at the git
root") is correct and matches his folder — only the parenthetical is wrong. Fix the parenthetical.

## 2. THE COOK — `BUILD SUCCESSFUL`, but it took TWO passes, and the first one was a defective artifact

⛔ Parsed from the UAT log's **own verdict lines**, never the exit code (the exit-code-lie law,
extended to UAT per `PKG-§6`).

### Pass 1 — succeeded, and shipped a game that could not be played

Command: `RunUAT BuildCookRun -platform=Win64 -clientconfig=Development -build -cook
-map=/Game/Maps/L_MainMenu+/Game/Maps/L_Arena -pak -stage -prereqs -archive`.
Verdict: **`BUILD SUCCESSFUL`** / `AutomationTool exiting with ExitCode=0 (Success)`, 5m55s.
Archived 1.789 GB. Log: `TASK-694/cook-696.log`.

**⭐ AND YET THE ARTIFACT WAS BROKEN — caught only because boot-verify was taken past the menu.**
Booting the packaged exe into `L_Arena` produced, from the package's own log:

```
'/Game/Data/DT_Cards.DT_Cards' not found — deck is EMPTY; the hand deals 6 empty slots
'/Game/UI/WBP_HUD.WBP_HUD_C' not found (built in TASK-011) — continuing without a HUD
BP_HeroCharacter_C' unavailable · BP_CommanderNpc_C' unavailable · BP_Torch_C' unavailable
… mine is invisible but fully functional · the torch is invisible but still lights the room
```

**Root cause, diagnosed not guessed:** an explicit `-map` allowlist cooks each map plus its **hard**
references. Everything this game resolves by **soft path at runtime** — the card table, the HUD and
UI widgets, the hero/commander/torch Blueprints, the input actions, the character meshes — has no
hard reference from either map, so the cooker never saw it and left it out. The build booted to a
menu perfectly and was, underneath, **not the game**: no cards, no HUD, invisible mines.

⚠️ This is precisely the failure a menu-only boot-verify cannot see. `PKG-§6` scopes boot-verify to
"the main menu is reached"; had I stopped there, a package with an empty deck would have shipped to
Jonathan with a clean bill of health. **Recommend `PKG-§6` be widened: boot the ARENA too and sweep
the log for `not found` / `unavailable`.**

### Pass 2 — the allowlist completed (DECLARED DEVIATION, and why it is in-mandate)

The task spec's own words are *"ship `L_MainMenu` + `L_Arena` **and whatever the shipped flow
genuinely needs**"*. A deck-less, HUD-less build does not meet that. The correction adds the
game-owned content directories the flow soft-loads, via the cook commandlet's supported
`-COOKDIR=` switch passed through UAT's `-AdditionalCookerOptions`:

`Content\Data · UI · Blueprints · Input · Characters · Meshes · Materials · Textures · VFX · Audio · LevelPrototyping`

⛔ **No repo file was edited to achieve this** — no `DefaultGame.ini` packaging-settings change, no
plugin surgery (`PKG-§5` honored), no map-allowlist widening. ⛔ The ~8 GB of marketplace/asset-pack
directories (`Realistic_Rocks`, `MedievalCastle…`, `Megaplant_Library`, `Fire_/Ice_Magic`,
`Prickly_Knight`, the showrooms) were **NOT** added — only assets those maps genuinely reference
still ride, which is why the package is 1.9 GB and not 10 GB.

Verdict: **`BUILD SUCCESSFUL`** / `ExitCode=0 (Success)`, 1m15s (incremental — Zen reused pass 1).
Log: `TASK-694/cook-696b.log`.

**Measured effect:** `…-Windows.ucas` **901.1 MB → 1027.8 MB (+126.7 MB)**; archive 1.789 → **1.91 GB**.
Re-boot of the arena from the corrected package: **all five previously-missing assets resolve, zero
`not found` / `unavailable` lines remain, error count 12 → 1.**

## 3. THE MAPS ALLOWLIST (`PKG-§5`) — pasted as required

| Map | Role | Source of truth |
|---|---|---|
| `/Game/Maps/L_MainMenu` | boot map | `DefaultEngine.ini:10` `GameDefaultMap` |
| `/Game/Maps/L_Arena` | the match | `SiegeSessionSubsystem.cpp:20` `ArenaMapPath` (+ `:21` `MenuMapPath`) — the only two the menu flow travels to |

UAT received exactly `-map=/Game/Maps/L_MainMenu+/Game/Maps/L_Arena`. **Template/marketplace levels
kept OUT and verified out:** a case-insensitive sweep of the cook log for `Variant_Combat`,
`Variant_Platforming`, `Variant_SideScrolling`, `Lvl_ThirdPerson` returns **0 hits**. The
`__ExternalActors__`/`__ExternalObjects__` OFPA stubs were never enumerated.

## 4. MODEL POSTURE (`PKG-§4`) — verified twice, at source AND in the artifact

- **Source (cited as required):** `Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSubsystem.cpp:1991-1995`
  — absent model ⇒ one Warning, then: *"THE MATCH IS FULLY PLAYABLE: nothing is blocked, nothing is
  retried, and every keyboard command is byte-identical."*
- **The staged build contains NO `Models/` directory and NO `.gguf`** — swept the whole archive. ✅
  (The 2.33 GB `Models/Qwen3-4B-Q4_K_M.gguf` lives at the *project* root, is gitignored, and is not
  cook content, so it was never a candidate.)
- **⭐ Measured live in the real package, not merely asserted:** the packaged exe logs
  `no model file at '…/Models/Qwen3-4B-Q4_K_M.gguf' -- the in-match assistant is unavailable this
  session … THE MATCH IS FULLY PLAYABLE` and proceeds to the menu. The graceful degrade is real.

## 5. BOOT-VERIFY (`PKG-§6`) — process + log evidence, on the FINAL artifact

Launched `Windows\GitClaudeUnrealTest.exe` **with no arguments**, exactly as a user double-clicking would:

```
Build Configuration: Development      Engine Version: 5.8.0-55116800+++UE5+Release-5.8
LoadMap: /Game/Maps/L_MainMenu
MainMenu up for play
Took 0.037363 seconds to LoadMap(/Game/Maps/L_MainMenu)
```
Process census: the 7 MB launcher shim (PID) + the real 1.6 GB binary. Closed cleanly afterwards.

**Additionally, past the menu — the evidence that the content is genuinely present** (this is what
caught pass 1's defect, and is offered as an argument for widening `PKG-§6`):
```
Took 0.930140 seconds to LoadMap(/Game/Maps/L_Arena)
Traversability CONFIRMED (nav settled: 0 pending) — Blue→Red castle path + 6 mine path(s) exist (after 0 cull(s))
UDeckComponent on 'SiegePlayerController_…': built a 50-card draw pile from 15 card rows (GDD §3…)
Deck select: chose curated deck 0 of 2 'Bot Aggro Rush' (50 cards…)
MinesPass seed=731535169 … minesSpawned=6
```
⚠️ The cook's own navmesh warning (*"RecastNavMesh-Default will be loaded empty … different navmesh
settings"*) was **chased down and is benign**: the packaged run reports `runtimeGen=Dynamic` and then
`Nav generation FINISHED` + `Traversability CONFIRMED`, i.e. the build rebuilds nav at runtime.

⛔ **No gameplay or input claims.** There is no input-injection lane; playing a match by hand remains
Jonathan's acceptance. My test `Saved/` directory was purged from the package before zipping.

## 6. COOK WARNINGS WORTH KEEPING

1. **Navmesh serialize warning ×2** — investigated above, **benign** (runtime-dynamic regeneration).
2. **UE EULA / LLM notice** — informational plugin notice about data sent to an LLM service.
3. `LogInit: Display: Success - 0 error(s), 3 warning(s)` — the cook's own clean summary.

## 7. 🙋 P3 — ANSWERED AS THE SPEC REQUIRES

**`bAllowHighDPIInGameMode=True` WAS still in the working tree at cook time** (uncommitted and
unclaimed, excluded from `b21ccc5` per TASK-689's adjudication), so per `PKG-§5` it **IS baked into
the packaged build's cooked config**. That is the P3 proceeding default ("cook the tree as-is"),
taken deliberately. Config ships inside the pak, not as loose `.ini`, so it cannot be edited
post-hoc in the package — a claim-or-strike ruling from Jonathan would require a re-cook.

## 8. FINDINGS FOR THE MANAGER

1. **⛔ NEW — the soft-reference cook gap (§2).** A map-only allowlist silently under-cooks
   everything loaded by soft path. Worth writing into `PKG-§5` as law, with the corrective
   `-COOKDIR=` recipe, so the next packaging task does not re-ship an empty deck.
2. **⛔ NEW — widen `PKG-§6` boot-verify** to include the arena + a `not found`/`unavailable` log
   sweep. A menu-only gate passed a broken build.
3. **`ProjectName=Third Person Game Template`** (`Config/DefaultGame.ini:3`) — the leftover template
   identity ships in the package and is visible in the exe's properties. Cosmetic but it is the
   first thing a stranger sees. One-line fix, but it is a config edit and therefore not mine.
4. **The exe is `GitClaudeUnrealTest.exe`, not `Siegebound.exe`** — renaming means renaming the
   project target; documented in the README instead.
5. **The 383 MB `.pdb`** (~20% of the package) ships because Development is pinned. A Shipping
   variant, or simply omitting the symbol file, is the single biggest size win.
6. Pre-existing `PKG-§2` path error — §1 rider above.
7. `Config/DefaultEngine.ini:1-7`'s comment claims remote-exec is *"Editor-only … no effect on
   packaged/`-game` builds."* **Measured false for `-game`** (TASK-694 drove the pixel proof over
   exactly that lane). Security-shaped claim in a comment; `Docs/setupdirections.md` repeats it.

## 9. STATE

Cook complete, archive at `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\Windows\`
(1.91 GB, 70 files incl. the README). ⛔ No commit in this task (697 owns it). ⛔ Not pushed.
Git tree carries only the `.gitignore` fence line + the still-excluded `DefaultEngine.ini` +
`Docs/setupdirections.md`; **`packagedZIPofGame/` is invisible to git.**
