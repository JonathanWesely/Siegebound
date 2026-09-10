<!--
  README SOURCE for the packaged game (SHIP-§3a).
  - This file is the tracked, durable body. The ship renders it to
    C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\README.md at Phase E,
    filling every double-brace SHIP placeholder from the ship's own evidence. A placeholder
    that survives into the rendered file is a STOP.
  - Every number below carries its citation as an HTML comment beside it, of the form
    "src: file:line". Paths are relative to GitClaudeUnrealTest/ unless they start with
    "Tools/", ".claude/" or "Plugins/" (same root) or name the engine install.
    "Siegebound/" means Source/GitClaudeUnrealTest/Siegebound/.
  - Source of every number: .claude/pipeline/handoffs/TASK-1190-census.md (HEAD 8c444ca)
    plus its Addendum (TASK-1191). Nothing here was typed from memory.
  - Placeholders and who fills them: see handoffs/TASK-1191-programmer.md.
-->

# Siegebound — Win64 {{SHIP:CONFIG}} build

**Zip:** `{{SHIP:ZIP_NAME}}`
**Built:** {{SHIP:DATE}} · Unreal Engine 5.8 · Win64 · **{{SHIP:CONFIG}}** configuration
**Built from commit:** `{{SHIP:HEAD}}` — the previous package was `Siegebound-Win64-Development-2026-08-29.zip`, built from `{{SHIP:DIFF_BASE}}`
**Size:** {{SHIP:SIZE}}

> Two heroes, two castles, one arena: spend gold from a six-card hand to summon units, raise towers, cast spells and fight as your hero until the enemy castle falls. <!-- src: Config/DefaultGame.ini:29 -->

---

## ⚠️ First, an honest correction

The original ask was to zip *"the `GitClaudeUnrealTest` folder without `.claude` / `Tools` / `Docs`"*
so anyone could extract and click.

**That would not have worked.** The raw project folder is a *source project*, not a game. On a
machine without Unreal Engine 5.8 installed there is nothing in it to click: `.uproject` +
`Content/` + `Source/` need the **engine installed and a full C++ compile** before a runnable
program exists at all. Handing someone that folder hands them a build job, not a game.

**So what is in this zip is a COOKED PACKAGE** — the real thing that satisfies "extract and
click": a compiled game program, the content pre-processed into platform-ready `.pak`/`.ucas`
files, and the slice of the Unreal runtime the game needs to boot. **No engine install, no
compiler, no Epic account required.**

---

## How to run it

1. **Extract the whole zip** anywhere (Desktop, Downloads, a USB stick — any folder works).
   ⚠️ Extract it *fully* first. Running the game from inside the zip preview window will fail —
   Windows only unpacks the one file you clicked, and the game needs the folder next to it.
2. Open the extracted folder and go into **`Windows\`**.
3. **Double-click `Windows\GitClaudeUnrealTest.exe`.** <!-- src: Tools/Packaging/ship.ps1:1090-1091 and :1174-1188 (HEAD 8c444ca — the root shim is the click target); .claude/pipeline/CONVENTIONS.md:7264 -->

   ℹ️ **Yes, the file is named `GitClaudeUnrealTest.exe`, not `Siegebound.exe`.** "Siegebound" is
   the game — it is the name on the window title bar <!-- src: Config/DefaultGame.ini:26 --> and in
   the program's identity fields <!-- src: Config/DefaultGame.ini:19 -->. `GitClaudeUnrealTest` is
   the internal project name it was built under, and renaming the files would break the folder
   layout the engine expects. The file you click is a small launcher; the game program it starts
   is `Windows\GitClaudeUnrealTest\Binaries\Win64\GitClaudeUnrealTest-Win64-Shipping.exe`
   <!-- src: .claude/pipeline/CONVENTIONS.md:7264; Tools/Packaging/ship.ps1:836 (HEAD 8c444ca) -->.
   Both belong to this build; the launcher is the intended way in.

### Windows will probably warn you

This build is **not code-signed**, so Windows SmartScreen will likely show
*"Windows protected your PC"*. That is the expected warning for any unsigned executable — it is
not a virus report. To continue: **More info → Run anyway**.

Your antivirus may also scan the folder on first launch, which can make the first start slow.

### Minimum requirements

| | |
|---|---|
| OS | **Windows 10 or 11, 64-bit** (no 32-bit / ARM build included) |
| GPU | **DirectX 12 capable with Shader Model 6** — the build's default graphics API is DirectX 12, targeting Shader Model 6 <!-- src: Config/DefaultEngine.ini:207-210 --> |
| Disk | {{SHIP:SIZE}} (plus the zip itself while unpacking) |
| RAM | Last measured on the 2026-08-29 Development package: about 1.6–2.2 GB resident while running, so roughly 4 GB free is comfortable <!-- src: packagedZIPofGame/README.md (2026-08-29), "Minimum requirements" row RAM -->. A Shipping build of this game measured a 1,640 MB working set at the main menu on 2026-08-30 <!-- src: Tools/Packaging/ship.ps1:841 (HEAD 8c444ca) -->. |
| Other | The Visual C++ redistributable is bundled under `Windows\Engine\Extras\Redist\en-us\` if your machine happens to lack it |

First launch takes noticeably longer than later ones — shaders warm up on the way in.

---

## What was packaged

**The cooked runtime, and only what the game actually reaches:**

- **The game program** — the launcher you double-click plus the real binary under
  `Windows\GitClaudeUnrealTest\Binaries\Win64\` (see *How to run it*).
- **The cooked content** — `Windows\GitClaudeUnrealTest\Content\Paks\`: `.pak` + `.ucas`/`.utoc`.
  Every mesh, texture, material, Blueprint, card table, input mapping and UI widget the game
  loads, converted to platform-ready form. Sizes are in the line at the top.
- **The Unreal 5.8 runtime** — only the engine binaries, shaders and plugin libraries this game
  needs, including the SiegeLlama plugin's native libraries (the plugin ships; its model file does
  not — see *About the in-game AI assistant*).
- **The VC++ redistributable installer**, bundled as a fallback.

**Maps included — an explicit allowlist, not "everything":**

| Map | Role |
|---|---|
| `/Game/Maps/L_MainMenu` | the boot map — what opens when you click the launcher <!-- src: Config/DefaultEngine.ini:10 --> |
| `/Game/Maps/L_Arena` | the battlefield where a match is played <!-- src: Config/DefaultEngine.ini:11-12; Siegebound/SiegeGameMode.cpp:74 --> |

Those are the only two maps the shipped flow can reach. The engine-template and marketplace demo
levels that live in the project (`ThirdPerson`, `Variant_Combat`, `Variant_Platforming`,
`Variant_SideScrolling`, and the asset-pack showrooms) were deliberately **excluded** — they are
scaffolding that came with the templates and asset packs, they are not the game, and cooking them
would have added weight for nothing.

---

## What was left out of the original project folder, and why

| Left out | Why |
|---|---|
| **`Source/` (all C++) and the uncooked `Content/`** | Already *compiled and cooked into* the game binary and the `.pak`/`.ucas` files. Shipping the sources too would only add a copy nobody can run without the engine. |
| **Unused marketplace/asset-pack content** | The project's `Content/` holds large third-party packs (rocks, foliage, VFX, SFX, castle props). Only the assets the two shipped maps and the game actually reference were cooked in; the rest never ships. This is most of the difference between the multi-gigabyte source folder and the package. |
| **`Content/RawAssets/`** | Source `.fbx` and `.png` art files — inputs to the art pipeline, already baked into the cooked assets. |
| **`.claude/`** | The AI agent pipeline — task board, conventions, handoffs, QA reports. Development process, irrelevant at runtime. |
| **`Tools/`** | The asset/automation pipelines (TRELLIS, Meshy, video review, Supabase helpers, the release script). Developer tooling; also the place API tokens would live, so it stays out of anything distributed. |
| **`Docs/`** | Design and setup documentation, including the source of this README. |
| **`Saved/`, `Intermediate/`, `DerivedDataCache/`, `Binaries/`, `Build/`** | Local build scratch and caches — machine-specific, regenerated on demand, and together larger than the game itself. |
| **The LLM model weights (`Models/*.gguf`, about 2.5 GB)** <!-- src: Tools/fetch_llm_model.py:8 --> | Deliberately excluded — see below. |
| **Editor-only and dev-only configuration** | Settings that only mean something inside the Unreal Editor. |

---

## About the in-game AI assistant

The project includes an optional in-match assistant backed by a local language model
(`Qwen3-4B-Q4_K_M.gguf` <!-- src: Plugins/SiegeLlama/Source/SiegeLlama/Public/SiegeLlamaSettings.h:72 -->,
about 2.5 GB of weights <!-- src: Tools/fetch_llm_model.py:8 -->). In a build that has the
model, you press **Enter** to open a chat box, type one sentence such as *"send three archers to
the mid"*, and the assistant turns it into the same unit order the keys would give.

**Those weights are NOT in this zip.** The release procedure refuses to package a model file at
all <!-- src: Tools/Packaging/ship.ps1:2835-2839 (HEAD 8c444ca), gate C4-NO-MODELS -->, so the
assistant is present in the code and inert in the package.

**This does not block anything.** The game detects the missing model at startup and runs
normally — in the code's own words:

> *"the in-match assistant is unavailable this session … **THE MATCH IS FULLY PLAYABLE**: nothing
> is blocked, nothing is retried, and every keyboard command is byte-identical."* <!-- src: Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSubsystem.cpp:1994 -->

What you will observe: every key and every card works exactly as described in *GameDetails*
below; pressing **Enter** gives you no working assistant. Whether the chat box itself opens in
this Shipping build is not something this README claims — with no model present there is nothing
for it to do either way. The Settings toggle *"Confirm AI orders before they execute"* still
exists (default on) <!-- src: Siegebound/SiegeSettingsSubsystem.h:293; Siegebound/SettingsMenuWidget.cpp:33 -->
and has no effect without a model.

If you want the assistant, the code looks for the model at
`<extracted folder>\Windows\GitClaudeUnrealTest\Models\Qwen3-4B-Q4_K_M.gguf`, or at a path passed
on the command line as `-siegellm.model=<path>` <!-- src: Plugins/SiegeLlama/Source/SiegeLlama/Private/SiegeLlamaSubsystem.cpp:1994; Plugins/SiegeLlama/Source/SiegeLlama/Public/SiegeLlamaSettings.h:74 -->.
That route has not been exercised on a packaged Shipping build, so treat it as untested.

---

## Known notes for this build

- **{{SHIP:CONFIG}} configuration.** This is the first Shipping package of the game; the
  2026-08-29 zip was a Development build. A Shipping build has **no console** (the `~` key does
  nothing — the console is compiled out) <!-- src: Config/DefaultInput.ini:84-85; C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/Core/Public/Misc/Build.h:214-215 -->,
  **no developer cheat commands** <!-- src: C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/Engine/Classes/GameFramework/CheatManagerDefines.h:9 -->,
  and it **writes no log file** — Shipping builds of this project are log-silent by design
  <!-- src: .claude/pipeline/CONVENTIONS.md:7043 -->. It is also smaller than the Development
  package for the same reasons; whether a debug-symbol (`.pdb`) file shipped beside the binary
  is recorded in the size line at the top.
- The game writes its save data (accounts, decks, settings) into
  `Windows\GitClaudeUnrealTest\Saved\` next to the executable, so extract somewhere you have
  write permission (not `C:\Program Files`).
- Carried forward from the 2026-08-29 package: on that machine's first runs the audio device
  occasionally failed to open (`OpenAudioStream failed`). It did not stop the game from booting
  or playing; if you get no sound, that is the likely cause.
- The fog card's visibility is known to breathe on a cycle, and the Graphics menu's
  keep-or-revert countdown has never been watched against a real clock — both are described,
  with what to look for, under *What is NOT in this build*.
- Letter keys are **positional**: on any keyboard layout, the physical key in the QWERTY `W`
  position moves you forward, and so on for every letter binding. Digits, modifiers, Enter,
  Escape, Space and the mouse are literal. There is no setting for this and no key-rebinding
  screen. <!-- src: Siegebound/SiegeKeyboardLayoutSubsystem.h:1-80; Siegebound/SiegeKeyboardLayoutSubsystem.cpp:75 -->

---

## What was verified before shipping

{{SHIP:VERIFIED}}

⚠️ Not verified by machine: actually *playing* a match with mouse and keyboard. There is no
automated input lane in this pipeline, so hands-on play is human acceptance, not a claim made here.

---

## What is NOT in this build

A package is a snapshot. This one was built from commit `{{SHIP:HEAD}}`; the previous package
(`Siegebound-Win64-Development-2026-08-29.zip`) was built from `{{SHIP:DIFF_BASE}}`.

### What changed since the 2026-08-29 package

{{SHIP:CHANGED_SINCE}}

*Draft list, written from the repository history (2026-08-30 → 2026-09-09) before the ship ran;
the line above records the ship's own check of it against the real log.* <!-- src: git log 22728c8..8c444ca (56 commits), anchored at the git root one level up -->

- **A controls screen.** Press **Tab** in a match for the full list of keys and what they do; the
  battle keeps running behind it.
- **High ground matters.** Ranged units and towers now hit harder from above (details under
  *How units fight*), and the battlefield's hills are worth climbing. The war map is shaded by
  elevation.
- **The Watch Tower card** — a building with a ladder. Walk your hero or your units into the
  ladder and they climb to a raised platform; archers up there get the height bonus.
- **Ranged reach is much longer** than in the previous package: Archer 2100 <!-- src: Docs/Data/cards.csv:3 -->,
  Longbowman 3600 <!-- src: Docs/Data/cards.csv:12 -->, Wizard 2100 <!-- src: Docs/Data/cards.csv:30 -->.
- **Order circles.** Hold, Ambush and Follow are now given by drawing circles on the ground with
  the mouse wheel; **Recall (B)** channels your hero home; and when your hero dies you keep
  playing as a **ghost** until the respawn. The camera no longer stays tilted during the ghost
  phase.
- **Tower stacking and the placement wheel.** Hover a building you already own with the same
  card to make it taller and tougher instead of placing a second one; scroll the wheel while
  placing to enlarge a building's footprint. The Watch Tower stacks too, to a lower ceiling.
- **The card bar lost its buttons.** Each card shows a key chip; the per-card discard button is
  gone (press **H** to discard the whole hand). A stray oval that used to sit over the card
  hotkeys is gone.
- **The Witch card.** She veils a nearby friendly unit; a veiled unit no longer shows the enemy
  a health bar, hit flash or damage numbers.
- **The Fog and Bright Sun cards** — the two battlefield-wide spells — are real, visible cards.
  Fog now limits what units keep chasing and firing at, not just what they spot; it renders even
  at the lowest Shadows setting; and its look was made uniform across the field and re-coloured.
- **The hero is a knight** — a rigged, textured model replaces the mannequin.
- **The trees look right** (they had been rendering with the engine's placeholder material) and
  carry their normal maps; grass density is doubled.
- **Settings → Graphics** — a full graphics panel (see *Other features*).
- The controls screen's text about closing the war map was corrected.

### Present in the code but inert, or never observed, in this package

Each item says what you will (or will not) see, not what to conclude from it.

- **The Witch, Fog and Bright Sun never come from the opponent.** All three ship with zero copies
  in the default deck <!-- src: Docs/Data/cards.csv:33-35 --> and neither of the bot's two decks
  contains them <!-- src: Siegebound/SiegeBotController.cpp:253-290 -->. You only meet them by
  putting them in your own deck; when you play them they work as described below.
- **The bot's Fireball behaviour never fires.** The bot has a rule for Fireball, but neither of
  its decks contains the card <!-- src: Siegebound/SiegeBotController.cpp:253-290; :747-809 -->, so
  you will never see the bot cast it.
- **19 of the 34 cards are absent from the default deck** (Sapper, Bomb Tower, Ballista Tower,
  Barracks, Deep Mine, Masons, the four hero upgrades, Lightning, Battle Cry, Pickpocket,
  Crystal Tower, Wizard, Watch Tower, Witch, Fog, Bright Sun) <!-- src: Docs/Data/cards.csv:2-35, column DeckCount -->.
  They are reachable only through the Deck Builder.
- **The fog breathes.** With the Fog card up, stand still and watch the far field for three
  minutes or more: it thins and thickens on a cycle of roughly 181 seconds
  <!-- src: .claude/pipeline/TASKBOARD.md:1600 -->, and at its thinnest the far-field visibility
  measure is about a third of its fullest (min/max 0.3399) <!-- src: .claude/pipeline/TASKBOARD.md:1637 -->.
  This is known and its cause is still being diagnosed.
- **The Graphics menu's 10-second revert has never been observed against a real clock.** Change
  the screen resolution or window mode, touch nothing for ten seconds, and see whether the
  display comes back on its own. <!-- src: Siegebound/SiegeGraphicsMenuWidget.h:410 -->
- **A veiled unit still shimmers to the other side.** The enemy cannot target it and its bars
  are hidden, but the veil effect itself is visible to them; a ruling on that is open.
- **The assistant** — inert in this package (no model; see above).
- **Console commands and cheats** — do not exist in a Shipping package (see *Known notes*).
- **Discarding one card at a time** — retired. The code path exists with nothing calling it; the
  only discard is the whole hand for a flat price. <!-- src: Siegebound/SiegePlayerController.h:1760-1767 -->
- **Multiplayer: the joining player is an observer.** See *Other features*.
- **Cloud sync:** {{SHIP:CLOUD_SYNC}}
- **Playing cards while dead** has no code gate and has not been checked at runtime: as the
  ghost, press a card key and see whether a placement completes. <!-- src: Siegebound/SiegePlayerController.cpp:960-980 -->
- **Gamepads** — only keyboard and mouse are described here; whether a controller responds in a
  match was not checked.

### Work that exists on the project board but not in this package

- The diagnosis of the fog's visibility cycle.
- The ruling on the veil shimmer being visible to the enemy.
- A hands-on playtest of the Graphics menu (the keep-or-revert countdown, the frame-rate readout
  while dragging a slider).

---

## GameDetails

### Gameplay

#### The match in one paragraph

Two castles face each other across a long battlefield; you are **Blue** on the west end, the
opponent is **Red** on the east. Gold ticks in every second. You hold a hand of six cards drawn
from a deck of exactly fifty; each card costs gold and puts a unit, a building, a spell or a hero
upgrade on the field. You also control a **hero** in third person who fights, rallies, and gives
your army its orders. The match ends the moment either castle is destroyed — the other side wins.
<!-- src: Siegebound/SiegeGameMode.cpp:523-572; Siegebound/SiegeGameState.cpp:213-214 -->

#### Gold

| Rule | Value |
|---|---|
| Starting gold | **10** <!-- src: Siegebound/SiegePlayerState.h:312 --> |
| Base income | **+1 gold every second** <!-- src: Siegebound/SiegePlayerState.h:316; :320; :324 --> |
| Overtime | from **7:00** (420 s) the base income **doubles** to +2/s <!-- src: Siegebound/SiegeGameState.h:180; Siegebound/SiegePlayerState.h:336 --> — the HUD shows `OVERTIME` |
| Miners | **+1/s per miner** that has arrived at a mine; nothing until it arrives <!-- src: Siegebound/SiegePlayerState.h:340; Siegebound/MinerUnit.h:484 (arrival radius 150) --> |
| Miner cap | **6 alive** per player; a seventh Miner card is refused with *"Miner limit reached"* and costs nothing <!-- src: Siegebound/SiegePlayerState.h:332; Siegebound/SiegePlayerController.cpp:1970-1975 --> |
| Deep Mine | **+2/s** the moment it is built; gone when it is destroyed <!-- src: Siegebound/DeepMine.h:66 --> |
| Gold cap | **999** <!-- src: Siegebound/SiegePlayerState.h:328 --> |
| Discard your hand (**H**) | **20 gold** flat for the whole hand, however many cards; you draw a full replacement; refused while you are placing or aiming <!-- src: Siegebound/SiegePlayerController.h:1786; Siegebound/SiegePlayerController.cpp:1524 --> |
| Reveal the enemy (war map) | **30 gold**, repeatable; refused if you hold less <!-- src: Siegebound/CommanderNpc.h:311; Siegebound/SiegePlayerController.cpp:6790-6792 --> |

The HUD shows your gold, your income rate, and your miner count. In **Sandbox (No Bot)** you are
granted **9999** gold at the start and there is no opponent <!-- src: Siegebound/SiegeGameMode.h:569; Siegebound/SiegeGameMode.cpp:1628-1646 -->.

#### Your deck and hand

- A legal deck is **exactly 50 cards** <!-- src: Siegebound/DeckTypes.h:73; Siegebound/DeckComponent.h:187 -->.
  There is **no per-card copy limit** — the only rule is the total <!-- src: Siegebound/CardRow.h:204-212 -->.
- Your hand holds **6 cards** <!-- src: Siegebound/DeckComponent.h:183 -->. Playing or discarding
  draws immediately; when the draw pile runs out, the discard pile is reshuffled into it
  <!-- src: Siegebound/DeckComponent.h:192-212 -->. The HUD shows the next card to be drawn (name,
  cost, art) <!-- src: Siegebound/CardHandWidget.h:1-80 -->.
- Keys **1–6** play hand slots 1–6 <!-- src: Content/Input/IMC_Hero.uasset (IA_Card1..IA_Card6); Siegebound/SiegePlayerController.cpp:598-609 -->.
- The **default deck** (used until you build your own): Footman 9 · Archer 8 · Wall 4 · Knight 3 ·
  Miner 3 · Arrow Tower 3 · Militia Mob 3 · Pikeman 3 · Cavalry 3 · Longbowman 2 · Cleric 2 ·
  Ogre 2 · Fireball 2 · Sorcerer 2 · Frost Nova 1 = 50 <!-- src: Docs/Data/cards.csv:2-35, column DeckCount -->.
- You can save **10 decks** per account; the active one is used by the next match
  <!-- src: Siegebound/SiegeDeckSaveGame.h:57 -->.

#### Playing a card

- **Units, buildings and economy cards** enter *placement mode*: a ghost of the thing follows
  your cursor; **left-click** confirms; **right-click** or **Escape** cancels at no cost
  <!-- src: Siegebound/SiegeControlsHelpWidget.cpp:429-460; :578-600; Content/Input/IMC_Hero.uasset (IA_CancelPlace) -->.
- **Where you may place:** anywhere inside a square **7380 units** to each side of your own
  castle (the castle interior counts) <!-- src: Siegebound/Castle.h:394; Siegebound/SiegePlayerController.h:2109 -->,
  **or** inside the mid capture zone while your team owns it <!-- src: Siegebound/SiegePlayerController.cpp:6074-6079 -->.
- **Clearances:** a building must be **200 units** from any other building (more if the footprint
  was enlarged) <!-- src: Siegebound/SiegePlayerController.h:2153 --> and **150 units** from
  obstacles <!-- src: Siegebound/SiegePlayerController.h:2206 -->; units need no clearance
  <!-- src: Siegebound/SiegePlayerController.h:2235 -->.
- **Footprint wheel:** while placing a building, the mouse wheel scales its footprint from
  **×1.0 up to ×1.5 in steps of 0.1** — it never shrinks below the authored size
  <!-- src: Siegebound/SiegePlayerController.h:2287-2322 -->. The Watch Tower cannot be resized
  <!-- src: Siegebound/ClimbableTower.h:464 -->.
- **Stacking a building taller:** while placing, hover one of your own buildings of the **same
  card**. Its outline turns blue and your click upgrades it instead of placing a new one: height
  becomes the original × (1 + upgrades), capped at **×5** <!-- src: Siegebound/Building.h:451 -->
  (the Watch Tower caps at **×2** <!-- src: Siegebound/ClimbableTower.cpp:223 -->); maximum health
  is multiplied by **1.5 per upgrade** with no cap, but damage already taken is not repaired
  <!-- src: Siegebound/Building.h:477; Siegebound/Building.cpp:406-489 -->. The price is the
  card's own cost each time <!-- src: Siegebound/SiegePlayerController.cpp:2765-2789 -->.
- **Spells** open an aiming reticle on the ground (radius 150 by default)
  <!-- src: Siegebound/SiegePlayerController.h:1866 -->. Fireball and Frost Nova use the reticle
  only as the **aim point of a bolt fired from your hero**; Lightning and Battle Cry go off at the
  reticle; Pickpocket, Fog and Bright Sun are instant with no reticle
  <!-- src: Docs/Data/cards.csv:24-35, column SpellDelivery; Siegebound/SpellLibrary.cpp:788-800; :602-633 -->.
- **Instant cards** — hero upgrades, Masons, Pickpocket, Fog, Bright Sun — resolve on the key
  press <!-- src: Siegebound/SiegePlayerController.cpp:5041-5071; Siegebound/SpellLibrary.cpp:630-756 -->.
- **Refusals cost nothing.** A refused spell refunds its gold; a hero upgrade at its cap, Masons
  with no castle standing, and a seventh Miner are all refused without a charge
  <!-- src: Siegebound/SiegePlayerController.cpp:3583; :4863; :5021; :5051; :1975 -->.

#### Your hero

| | |
|---|---|
| Health | **200**; out of combat it regenerates **5 HP/s** after **8 s** without damage <!-- src: Siegebound/HeroCharacter.h:1270; :1274-1278 --> |
| Speed | walk **500**, sprint **750** (hold **Shift**) <!-- src: Siegebound/HeroCharacter.h:1103; :1107 -->; jump launch **600** <!-- src: Siegebound/HeroCharacter.h:1126 -->; steps up to **50** high and walks slopes to **50°** <!-- src: Siegebound/HeroCharacter.h:1118-1122 --> |
| Attack (**left mouse**) | a melee cleave: **20** damage to every enemy within **150 units** inside a **60°** cone in front of you, at most once every **0.5 s** <!-- src: Siegebound/HeroCharacter.h:1208-1220 -->. Against the castle it lands at full value <!-- src: Siegebound/HeroCharacter.cpp:641; Siegebound/Castle.cpp:1139-1153 --> |
| Rally (**Q**) | friendly units within **600 units** move **25 % faster for 5 s**; **20 s** cooldown; the HUD shows `Rally: Ready` or the countdown <!-- src: Siegebound/HeroCharacter.h:1224-1236 --> |
| Recall (**B**) | a **10 s** channel; moving more than **25 units** cancels it; on completion you teleport home and heal to full <!-- src: Siegebound/HeroCharacter.h:1328; :1341; Siegebound/HeroCharacter.cpp:1506-1615 --> |
| Death | your hero's death does **not** lose the match. You take control of a **ghost** that can move and look but cannot attack or rally, and the hero respawns at your own castle after **180 s** <!-- src: Siegebound/SiegeGameMode.h:461; Siegebound/SiegeGameMode.cpp:732-772; Siegebound/SiegeGhostPawn.cpp:507-524 -->. Falling out of the world counts as a death <!-- src: Siegebound/SiegeControlsHelpWidget.cpp:352 --> |
| Upgrades | four cards stack permanent upgrades on the hero (see the Hero upgrades table); they survive death and reset only on Play Again <!-- src: Siegebound/HeroCharacter.cpp:1329-1340; Siegebound/SiegeGameMode.cpp:1527 --> |

#### How units fight

- **Spawning.** Every unit you summon joins your hero's **follow group** and does not fight until
  you give an order (T, E, R, F or C — see *Commands*). Exceptions: the Ogre and the Sapper march
  on their own, and the Miner goes mining <!-- src: Siegebound/SiegePlayerController.cpp:4397; :4478; Siegebound/SummonedUnit.cpp:1712-1720; Siegebound/MinerUnit.h:520 -->.
  The bot's units always advance on their own <!-- src: Siegebound/SiegeBotController.cpp:1663-1800 -->.
- **Notice range** (how far a unit spots an enemy): **5000 units** for every card — the card
  table's notice column is blank on every row, so every unit uses the class default
  <!-- src: Siegebound/SummonedUnit.h:914; Siegebound/SummonedUnit.cpp:4537-4550; Docs/Data/cards.csv:2-35, column NoticeRange -->.
  **While fog is up, the value you actually experience is 609.6 units** — the fog's vision
  ceiling is applied as a minimum to every unit's notice, so on a fogged field nothing on either
  side sees farther than that <!-- src: Siegebound/SiegeFogStatics.h:268; Siegebound/SiegeFogStatics.cpp:122; Siegebound/SiegeCombatStatics.cpp:257 -->.
- **Retention range** (how far a unit keeps chasing a target before giving up): the larger of
  the leash (**8000**) and notice × **1.5** (7500), so **8000 units** for every card
  <!-- src: Siegebound/SummonedUnit.h:1448; :1470; Siegebound/SummonedUnit.cpp:4573-4583 -->.
  **Under fog the drop test is clamped the same way, so the retention you experience is 609.6**
  — a unit drops a target it can no longer see <!-- src: Siegebound/SummonedUnit.cpp:1752; :2024 -->.
- **Attack reach under fog:** ranged units fire no farther than **609.6** while fog is up; melee
  (120) is unaffected <!-- src: Siegebound/SummonedUnit.cpp:1785; :1972; :2049 -->. Towers use
  the same rule with their own range <!-- src: Siegebound/Tower.cpp:235-238 -->.
- **Whom a unit attacks (Standard):** the nearest enemy within notice, preferring units and the
  hero over structures unless a structure is nearer by more than **100 units**
  <!-- src: Siegebound/SummonedUnit.cpp:1795-1858; Siegebound/SummonedUnit.h:1474 -->.
- **Siege units** (Ogre, Sapper) ignore units and the hero and walk to the nearest enemy
  building, or the castle when none is left <!-- src: Siegebound/SummonedUnit.cpp:1635-1638; :2737-2740 -->.
- **Support units** (Cleric, Witch) never attack <!-- src: Siegebound/SummonedUnit.cpp:924-927 -->.
  The Miner and the Sorcerer never notice enemies at all <!-- src: Siegebound/MinerUnit.cpp:72; Siegebound/SorcererUnit.cpp:23 -->.
- Units re-think their state every **0.25 s** <!-- src: Siegebound/SummonedUnit.h:1517 -->.
- **Defending** units engage anything inside the castle's half-width plus **1281 units** of
  their own castle <!-- src: Siegebound/SummonedUnit.h:1513; Siegebound/SummonedUnit.cpp:2650-2710 -->.
- **Projectiles** (archers, towers) are homing shots at **1500 units/s** with a **30**-unit
  impact radius, and they are blocked by terrain and obstacles <!-- src: Siegebound/Projectile.h:150; :159; Siegebound/Projectile.cpp:411-447 -->.

#### Keywords and damage rules

A unit's damage is: base × Charge (×2) × Slayer (×2) × War Banner aura (×1.2) × permanent stacks
× height advantage (ranged only) <!-- src: Siegebound/SummonedUnit.cpp:4703-4780 -->.

| Rule | What it does |
|---|---|
| **Charge** (Cavalry) | the first attack after **2 s** of uninterrupted movement deals **×2**; stopping or being blocked loses it <!-- src: Siegebound/SummonedUnit.h:1603-1615; Siegebound/SummonedUnit.cpp:4255-4277 --> |
| **Slayer** (Pikeman) | **×2** against any target whose maximum health is **150 or more** — units, buildings, the castle, the hero <!-- src: Siegebound/SummonedUnit.h:1619-1623; Siegebound/SummonedUnit.cpp:4723-4736 --> |
| **Suicide** (Sapper) | detonates once, on reaching attack range or on death: its damage over its blast radius, as siege damage, then it dies <!-- src: Siegebound/SummonedUnit.cpp:2766; :5526-5556 --> |
| **Swarm** (Militia Mob) | one play spawns the row's count in a **300**-unit circle <!-- src: Siegebound/SiegePlayerController.h:2330; Siegebound/CardRow.h:260 --> |
| **Chain** (Crystal Tower) | an instant zap through up to the row's chain count, losing the row's falloff per hop; each hop reaches an enemy within **350 units** of the previous one <!-- src: Siegebound/Tower.h:114; Siegebound/Tower.cpp:388-501 --> |
| **Siege damage** (Ogre, Sapper) | **×2** against the castle and buildings <!-- src: Siegebound/Castle.cpp:1141-1143; Siegebound/Building.cpp:558-560 --> |
| **Arrows and tower shots** | **half** damage against the castle; full against units, buildings and the hero <!-- src: Siegebound/Castle.cpp:1145-1147; Siegebound/DamageTypes.h:38-40 --> |
| **Spells** | **half** damage against the castle <!-- src: Siegebound/Castle.cpp:1149-1151 --> |
| **Hero melee** | full damage against everything, castle included <!-- src: Siegebound/HeroCharacter.cpp:641; Siegebound/Castle.cpp:1139-1153 --> |
| **Friendly fire** | none: the castle ignores its own team, and every blast, splash and spell gathers hostile targets only <!-- src: Siegebound/Castle.cpp:1120-1124; Siegebound/SiegeCombatStatics.cpp:441-486 --> |
| **Height advantage** | a **ranged** attacker standing above its target deals **+10 % per 152.4 units** of height difference, continuously (not in steps), with no cap; there is no penalty for shooting upward <!-- src: Siegebound/SummonedUnit.h:1687; :1700; Siegebound/SummonedUnit.cpp:4681-4690; :4772-4778 --> |
| **Freeze** (Frost Nova) | frozen units and buildings do nothing for the duration; a frozen Barracks stops spawning <!-- src: Siegebound/SpellLibrary.cpp:286-330; Siegebound/Building.cpp:111; Siegebound/Barracks.cpp:85 --> |
| **Battle Cry** | friendly units in the circle attack **50 % faster** and move **25 % faster** for the duration <!-- src: Siegebound/SummonedUnit.h:1632-1636 --> |
| **Permanent stacks** (Sorcerer / Ancient Ground) | **+5 % base damage per stack**, up to **80 stacks (+400 %)**, kept until the unit dies <!-- src: Siegebound/SummonedUnit.h:1649; :1659 --> |

#### The castle

- Each castle has **2000 health** <!-- src: Siegebound/Castle.h:359 -->. It visibly crumbles in
  stages at **75 % / 50 % / 25 %** <!-- src: Siegebound/Castle.h:757-765; Siegebound/Castle.cpp:1320-1400 -->,
  and the match ends when one reaches zero.
- What damages it, and how much: siege units ×2, arrows/tower shots ×0.5, spells ×0.5, hero melee
  ×1, other melee ×1 (see *Keywords and damage rules*) <!-- src: Siegebound/Castle.cpp:1106-1160 -->.
  Towers never shoot at castles at all <!-- src: Siegebound/Tower.cpp:209-336 -->.
- **Repair:** the Masons card heals your castle **300 health over 10 s** (ticking every 0.2 s);
  refused with no castle standing <!-- src: Siegebound/SiegePlayerController.h:1741; :1745; Siegebound/Castle.h:724; Siegebound/Castle.cpp:1565-1642 -->.
- Each castle is hollow and enterable, lit by **6 torches**, and houses your **commander**
  (see *The AI commander*) <!-- src: Siegebound/Castle.h:563; Siegebound/Castle.cpp:152-153; :714 -->.
- The castles stand at the far ends of the field, about **25000 units** east and west of centre
  <!-- src: Siegebound/BattlefieldScatter.cpp:1458-1462; Siegebound/SiegeBotController.h:477 -->;
  your hero respawns **1500 units** in front of your own castle <!-- src: Siegebound/SiegeGameMode.h:536 -->.
- Castle health bars are shown on the HUD <!-- src: Content/UI/WBP_CastleHealthBar.uasset -->.

#### Fog and Bright Sun

- **Fog** covers the **entire battlefield** the instant it is played — no reticle, no radius
  <!-- src: Siegebound/CardRow.h:80-88 -->. It lasts **300 s**; playing it again while it is up
  **resets** the clock rather than extending it <!-- src: Siegebound/FogVolume.h:999; Siegebound/FogVolume.cpp:351 -->.
  While it is up, every unit on **both** sides notices, chases and fires no farther than the
  **609.6-unit** vision ceiling (see *How units fight*); at that distance only **2 %** of light
  gets through <!-- src: Siegebound/SiegeFogStatics.h:268; :325 -->. Fog is refused (no gold
  spent) while a Bright Sun window is holding the sky clear <!-- src: Siegebound/SpellLibrary.cpp:686-695; Siegebound/FogVolume.cpp:351-360 -->.
- While fog is up the game forces volumetric fog rendering on, whatever your Shadows setting, so
  it cannot be seen through by lowering graphics <!-- src: Siegebound/FogVolume.h:877-883; :1041 -->.
- **Bright Sun** clears any fog instantly and then **prevents new fog** for **120 s, plus 60 s
  for every 1524 units** your living hero stands above the flat-ground level at the moment you
  cast — read once, uncapped; with no living hero the window is the base 120 s
  <!-- src: Siegebound/FogVolume.h:1059; :1072; :1094; :1139; Siegebound/FogVolume.cpp:556-625 -->.
  A cast that would produce a **shorter** window than the one already running is refused at no
  cost <!-- src: Siegebound/FogVolume.cpp:406-420; Siegebound/SpellLibrary.cpp:743 -->. When the
  window ends the field **stays clear** — fog never comes back on its own <!-- src: Siegebound/FogVolume.h (class doc state diagram) -->.

#### Invisibility (the Witch's veil)

- The Witch veils the nearest friendly unit inside her **400-unit** circle that the enemy can
  currently see, over a **3 s** cast that any interruption cancels, one unit at a time
  <!-- src: Siegebound/SummonedUnit.h:1574; :1593; Siegebound/SummonedUnit.cpp:3322-3335; :3496 -->.
- A veiled unit is skipped by enemy targeting and by the enemy's war-map reveal, but it can
  still be hit <!-- src: Siegebound/SummonedUnit.cpp:1850; Siegebound/SiegePlayerController.cpp:7218-7219 -->.
- The veil breaks **permanently** when the veiled unit acts: it attacks, heals, mines, stands on
  an ancient ground as a Sorcerer (it is counted there every second), or — as a Witch — completes
  a veil cast of its own; death ends it as well. It never restores itself — only a new Witch cast
  re-veils <!-- src: Siegebound/SiegeInvisibilityStatics.h:97; :125; :137; :157; :176; :195; :206 (all six reasons); Siegebound/SummonedUnit.cpp:4180; :4210; :5584 (Attack); :3039 (Heal); Siegebound/MinerUnit.cpp:532 (Mine); Siegebound/AncientGround.cpp:259 (Empower); Siegebound/SummonedUnit.cpp:3607 (Cast); :5621 (Death) -->.

#### The cards

There are **34 cards** <!-- src: Docs/Data/cards.csv:2-35 -->. Every number in the tables below is
the card's own row in the shipped card table (the citation on each row covers every cell in it).
The **notice** and **retention** columns are the same for every unit that fights — **5000 (609.6
while fog is up)** and **8000 (609.6 while fog is up)** — because no card overrides them; the
derivation is in *How units fight* <!-- src: Siegebound/SummonedUnit.h:914; :1448; :1470; Siegebound/SiegeFogStatics.h:268 -->.
"Units" are game units (1 unit = 1 cm in the engine's scale); speeds are units per second.

**Units**

| Card | Cost | Health | Damage | Attack range (under fog) | Attacks every | Speed | Notices enemies within (under fog) | Chases up to (under fog) | Targeting | Default deck | Keywords |
|---|---|---|---|---|---|---|---|---|---|---|---|
| Footman | 9 | 80 | 12 | 120, melee | 1.0 s | 400 | 5000 (609.6) | 8000 (609.6) | Standard | 9 | — <!-- src: Docs/Data/cards.csv:2 --> |
| Archer | 12 | 45 | 10 | 2100 (609.6), homing shot | 1.2 s | 350 | 5000 (609.6) | 8000 (609.6) | Standard | 8 | ranged <!-- src: Docs/Data/cards.csv:3 --> |
| Knight | 18 | 200 | 15 | 120, melee | 1.2 s | 300 | 5000 (609.6) | 8000 (609.6) | Standard | 3 | — <!-- src: Docs/Data/cards.csv:4 --> |
| Militia Mob | 15 | 25 each | 6 | 120, melee | 1.0 s | 400 | 5000 (609.6) | 8000 (609.6) | Standard | 3 | Swarm: 4 per play <!-- src: Docs/Data/cards.csv:8 --> |
| Pikeman | 15 | 100 | 30 | 120, melee | 1.5 s | 350 | 5000 (609.6) | 8000 (609.6) | Standard | 3 | Slayer <!-- src: Docs/Data/cards.csv:9 --> |
| Sapper | 15 | 60 | 80 blast | 120, then detonates | once | 500 | 5000 (609.6) | 8000 (609.6) | Siege | 0 | Suicide, blast radius 250 <!-- src: Docs/Data/cards.csv:10 --> |
| Cavalry | 21 | 140 | 20 | 120, melee | 1.0 s | 600 | 5000 (609.6) | 8000 (609.6) | Standard | 3 | Charge <!-- src: Docs/Data/cards.csv:11 --> |
| Longbowman | 18 | 70 | 18 | 3600 (609.6), homing shot | 1.5 s | 300 | 5000 (609.6) | 8000 (609.6) | Standard | 2 | ranged <!-- src: Docs/Data/cards.csv:12 --> |
| Cleric | 18 | 90 | heals 8 HP/s | heals within 400 | continuous | 350 | never attacks | — | Support | 2 | healer <!-- src: Docs/Data/cards.csv:13 --> |
| Ogre | 36 | 500 | 35 | 120, melee | 1.5 s | 250 | 5000 (609.6) | 8000 (609.6) | Siege | 2 | siege damage <!-- src: Docs/Data/cards.csv:14 --> |
| Wizard | 24 | 45 | 15, splash 250 | 2100 (609.6), homing shot | 1.6 s | 350 | 5000 (609.6) | 8000 (609.6) | Standard | 0 | ranged, splash <!-- src: Docs/Data/cards.csv:30 --> |
| Sorcerer | 60 | 70 | 0 | never attacks | — | 350 | never notices (sealed) | — | Standard orders, no combat | 2 | empowers units on an Ancient Ground <!-- src: Docs/Data/cards.csv:31; Siegebound/SorcererUnit.cpp:23 --> |
| Witch | 50 | 70 | 0 | veil circle 400 | 3 s cast | 350 | never attacks | — | Support | 0 | veils one friendly unit at a time <!-- src: Docs/Data/cards.csv:33 --> |

**Buildings**

| Card | Cost | Health | Damage | Range (under fog) | Fires every | Default deck | Special |
|---|---|---|---|---|---|---|---|
| Arrow Tower | 15 | 150 | 15 | 900 (609.6) | 1.5 s | 3 | fires at the nearest enemy unit or hero <!-- src: Docs/Data/cards.csv:6 --> |
| Wall | 12 | 300 | — | — | — | 4 | blocks the ground <!-- src: Docs/Data/cards.csv:7 --> |
| Bomb Tower | 24 | 180 | 25, splash 250 | 800 (609.6) | 2.5 s | 0 | splash damage <!-- src: Docs/Data/cards.csv:15 --> |
| Ballista Tower | 21 | 120 | 45 | 1400 (609.6) | 3.0 s | 0 | blind spot: cannot hit inside 300 <!-- src: Docs/Data/cards.csv:16; Siegebound/Tower.cpp:244 --> |
| Barracks | 30 | 250 | — | — | — | 0 | a free Footman every 8 s; falls apart after 60 s <!-- src: Docs/Data/cards.csv:17 --> |
| Crystal Tower | 27 | 150 | 15, then 10, then 5 | 800 (609.6) | 1.5 s | 0 | Chain through up to 3 enemies, losing 5 per hop, each hop within 350 <!-- src: Docs/Data/cards.csv:29; Siegebound/Tower.h:114 --> |
| Watch Tower | 30 | 250 | — (no attack) | — | — | 0 | climbable: a ladder to a platform 1200 units up <!-- src: Docs/Data/cards.csv:32; Siegebound/ClimbableTower.h:670 --> |

**Economy and utility**

| Card | Type | Cost | Health | Speed | Default deck | Effect |
|---|---|---|---|---|---|---|
| Miner | Economy unit | 24 | 30 | 350 | 3 | walks to an open mine; +1 gold/s once it arrives; cap 6 alive <!-- src: Docs/Data/cards.csv:5; Siegebound/SiegePlayerState.h:340; :332 --> |
| Deep Mine | Economy building | 45 | 200 | — | 0 | +2 gold/s from the moment it is built; no miner slot used <!-- src: Docs/Data/cards.csv:18; Siegebound/DeepMine.h:66 --> |
| Masons | Utility, instant | 24 | — | — | 0 | repairs your castle 300 health over 10 s <!-- src: Docs/Data/cards.csv:19; Siegebound/SiegePlayerController.h:1741; :1745 --> |

**Hero upgrades** (instant; permanent for the match; survive your hero's death)

| Card | Cost | Effect per copy | Copies your hero can hold | Default deck |
|---|---|---|---|---|
| Sharpened Blade | 18 | +10 melee damage <!-- src: Siegebound/HeroCharacter.h:1285 --> | 2 | 0 <!-- src: Docs/Data/cards.csv:20 --> |
| Plate Armor | 18 | +100 maximum health, and heals 100 immediately <!-- src: Siegebound/HeroCharacter.h:1289 --> | 2 | 0 <!-- src: Docs/Data/cards.csv:21 --> |
| Swift Boots | 15 | +25 % walk and sprint speed <!-- src: Siegebound/HeroCharacter.h:1293 --> | 1 | 0 <!-- src: Docs/Data/cards.csv:22 --> |
| War Banner | 24 | aura: friendly units within 600 units of the hero deal +20 % damage, refreshed every 0.5 s <!-- src: Siegebound/HeroCharacter.h:1297; :1301; :1305 --> | 1 | 0 <!-- src: Docs/Data/cards.csv:23 --> |

A copy beyond the cap is refused and costs nothing <!-- src: Siegebound/SiegePlayerController.cpp:5021 -->.

**Spells**

| Card | Cost | Effect | Delivery | Default deck |
|---|---|---|---|---|
| Fireball | 21 | 100 damage to everything the bolt passes through; the castle takes half <!-- src: Docs/Data/cards.csv:24; Siegebound/Castle.cpp:1149-1151 --> | a bolt from your hero, 900 units long and 100 units to either side, sweeping over 0.3 s, through walls and bodies <!-- src: Siegebound/SpellLineSweep.h:103; :112; :121 --> | 2 |
| Frost Nova | 18 | freezes every enemy unit and building the bolt catches for 4 s; the castle is unaffected <!-- src: Docs/Data/cards.csv:25 --> | the same bolt from your hero <!-- src: Siegebound/SpellLineSweep.h:103; :112 --> | 1 |
| Lightning | 24 | 200 damage to each of the 3 enemies with the most health left within 700 units of the reticle (units, buildings, hero — never the castle) <!-- src: Docs/Data/cards.csv:26; Siegebound/SpellLibrary.cpp:344-445 --> | ground reticle | 0 |
| Battle Cry | 15 | friendly units within 400 units of the reticle attack 50 % faster and move 25 % faster for 8 s <!-- src: Docs/Data/cards.csv:27; Siegebound/SummonedUnit.h:1632-1636 --> | ground reticle | 0 |
| Pickpocket | 18 | steals 10 gold from the opponent, or everything they have if less <!-- src: Docs/Data/cards.csv:28; Siegebound/SpellLibrary.cpp:522-570 --> | instant | 0 |
| Fog | 50 | battlefield-wide fog for 300 s; re-cast resets the clock <!-- src: Docs/Data/cards.csv:34; Siegebound/FogVolume.h:999 --> | instant | 0 |
| Bright Sun | 60 | clears fog and holds it off for 120 s + 60 s per 1524 units of hero height <!-- src: Docs/Data/cards.csv:35; Siegebound/FogVolume.h:1059; :1072; :1094 --> | instant | 0 |

#### Card by card — what the game says, and what the code does

The description on each card's detail panel in the Deck Builder is composed from a fixed set of
sentences plus the card's own numbers (health, damage, attack rate, range, speed); the sentences
quoted below are the shipped text <!-- src: Siegebound/DeckBuilderWidget.cpp:71-268 -->.

1. **Footman** — panel shows its stats only. A basic melee line unit: must close to 120 to hit;
   picks the nearest enemy; waits for your order after spawning. <!-- src: Docs/Data/cards.csv:2 -->
2. **Archer** — *"Shots hit a castle for HALF the listed damage - units, heroes and structures take
   the full amount."* <!-- src: Siegebound/DeckBuilderWidget.cpp:264 --> Homing arrows at 1500
   units/s; earns the height bonus from above; reach 2100, cut to 609.6 under fog.
3. **Knight** — stats only. A heavy melee tank (200 health). <!-- src: Docs/Data/cards.csv:4 -->
4. **Miner** — *"Walks to an open gold mine on its own and, once it claims a spot there, adds +1
   gold per second - it earns nothing until it arrives. Killing it, or the mine running dry, ends
   that income. You may have 6 miners alive at once."* <!-- src: Siegebound/DeckBuilderWidget.cpp:76 -->
   It never attacks. It takes miner-flavoured orders: **T** mine the nearest mine, **E** idle in
   the castle, **R**/**F** mine inside the circle you draw, **C** follow the hero
   <!-- src: Siegebound/MinerUnit.cpp:507-560; :1340-1361 -->.
5. **Arrow Tower** — *"Stationary structure: it never moves, physically blocks the ground it
   stands on, and holds until it is destroyed."* and *"Fires on its own at the nearest enemy unit
   or hero within range - it never shoots castles, walls or other structures."* <!-- src: Siegebound/DeckBuilderWidget.cpp:130; :133 -->
6. **Wall** — the structure sentence above; blocks unit pathing; 300 health. <!-- src: Docs/Data/cards.csv:7 -->
7. **Militia Mob** — *"Swarm: one play puts 4 of them on the field at once, spread around a
   300-unit circle, for a single card and a single cost."* <!-- src: Siegebound/DeckBuilderWidget.cpp:147; Docs/Data/cards.csv:8 -->
8. **Pikeman** — *"Slayer: deals double damage to any target with 150 or more maximum health -
   the big units, towers and castles."* <!-- src: Siegebound/DeckBuilderWidget.cpp:141 -->
9. **Sapper** — *"Explodes the moment it reaches its target - or if it is killed on the way in -
   dealing 80 damage to every enemy within 250 units, then dies. It can only blow up once."* plus
   the Siege sentences (see Ogre) <!-- src: Siegebound/DeckBuilderWidget.cpp:144; Docs/Data/cards.csv:10 -->.
   Its blast is siege damage, so it lands doubled on buildings and the castle; it marches on its
   own and takes no orders.
10. **Cavalry** — *"Charge: its first attack after 2 seconds of uninterrupted advancing deals
    double damage. Being blocked or stopping loses the momentum, and it must be rebuilt."* <!-- src: Siegebound/DeckBuilderWidget.cpp:138 -->
    The fastest unit at 600.
11. **Longbowman** — the ranged castle sentence (see Archer); the longest reach of any unit at
    3600 (609.6 under fog). <!-- src: Docs/Data/cards.csv:12 -->
12. **Cleric** — *"Support: it never attacks. It follows your line and heals the most hurt friendly
    unit within 400 units for 8 health per second."* <!-- src: Siegebound/DeckBuilderWidget.cpp:256; Docs/Data/cards.csv:13 -->
    The heal is delivered every 0.1 s and never over-heals <!-- src: Siegebound/SummonedUnit.h:1549; Siegebound/SummonedUnit.cpp:3043 -->;
    healing breaks its own veil.
13. **Ogre** — *"Siege: it ignores enemy units and the enemy hero completely, walking past them
    for the nearest enemy structure - and for the castle when none is left."* and *"Its damage
    lands on castles and structures at DOUBLE the listed amount."* <!-- src: Siegebound/DeckBuilderWidget.cpp:253; :261 -->
    Marches on its own; takes no orders.
14. **Bomb Tower** — the two tower sentences; its shots splash over 250 units. <!-- src: Docs/Data/cards.csv:15 -->
15. **Ballista Tower** — the two tower sentences; the panel adds a blind-spot line: it cannot hit
    anything inside 300 units. <!-- src: Docs/Data/cards.csv:16; Siegebound/Tower.cpp:244 -->
16. **Barracks** — the structure sentence, then *"Summons a free Footman every 8 seconds, on your
    side and at no extra cost - the first one arrives a full interval after it is built."* and
    *"It falls apart on its own after 60 seconds; everything it already summoned stays on the
    field."* <!-- src: Siegebound/DeckBuilderWidget.cpp:153; :156; Docs/Data/cards.csv:17 -->
17. **Deep Mine** — *"Raises your income by +2 gold per second the moment it is built - no walk
    needed, and it does not use up one of your miner slots. It is a destructible structure: raze
    it and that income is gone."* <!-- src: Siegebound/DeckBuilderWidget.cpp:79 -->
18. **Masons** — *"Instant repair: heals your own castle 300 health over 10 seconds. With no castle
    left standing it is refused and costs you nothing."* <!-- src: Siegebound/DeckBuilderWidget.cpp:82 -->
19. **Sharpened Blade** — *"Instantly upgrades your hero: +10 damage on every melee swing."*
    *"Stacking: your hero can hold 2 of these; a further copy is refused and costs you nothing.
    Upgrades last the rest of the match and survive your hero's death."* <!-- src: Siegebound/DeckBuilderWidget.cpp:115; :127; Docs/Data/cards.csv:20 -->
20. **Plate Armor** — *"Instantly upgrades your hero: +100 maximum health, and it heals 100
    straight away."* Holds 2. <!-- src: Siegebound/DeckBuilderWidget.cpp:118; Docs/Data/cards.csv:21 -->
21. **Swift Boots** — *"Instantly upgrades your hero: +25% movement speed, walking and sprinting
    alike."* Holds 1. <!-- src: Siegebound/DeckBuilderWidget.cpp:121; Docs/Data/cards.csv:22 -->
22. **War Banner** — *"Instantly upgrades your hero: friendly units within 600 units of it deal
    +20% more damage."* Holds 1. <!-- src: Siegebound/DeckBuilderWidget.cpp:124; Docs/Data/cards.csv:23 -->
23. **Fireball** — *"Deals 100 damage to every enemy the bolt passes through - units, heroes,
    structures and castles alike. Your own side is never hit."* *"Aimed from your hero: it flies
    out as a bolt roughly 900 units long, catching anything within 100 units to either side, and
    passes straight through walls and bodies. A bolt that catches nothing is still spent."* *"A
    castle caught in it takes HALF damage; everything else takes the full amount."* <!-- src: Siegebound/DeckBuilderWidget.cpp:164; :245; :267 -->
24. **Frost Nova** — *"Freezes every enemy unit and structure the bolt passes through for 4
    seconds - frozen targets cannot move, attack or fire. Castles and heroes are immune."* plus
    the bolt sentence above. <!-- src: Siegebound/DeckBuilderWidget.cpp:170; :245; Docs/Data/cards.csv:25 -->
25. **Lightning** — *"Strikes the 3 enemies with the most health left within 700 units for 200
    damage each. Castles are never struck; towers and other structures take the full amount,
    which is why it kills them outright."* *"Aimed at a spot on the ground: it goes off where you
    place the reticle."* <!-- src: Siegebound/DeckBuilderWidget.cpp:173; :248; Docs/Data/cards.csv:26 -->
26. **Battle Cry** — *"Friendly units within 400 units of the spot you target attack 50% faster
    and move 25% faster for 8 seconds."* Ground reticle. <!-- src: Siegebound/DeckBuilderWidget.cpp:176; Docs/Data/cards.csv:27 -->
27. **Pickpocket** — *"Steals 10 gold from your opponent the instant you play it - no aiming, no
    target. If they hold less than that, you take everything they have."* <!-- src: Siegebound/DeckBuilderWidget.cpp:179; Docs/Data/cards.csv:28 -->
28. **Crystal Tower** — the two tower sentences, then *"Chain: every shot is an instant zap that
    arcs through up to 3 enemies, weakening as it goes (15, 10, 5 damage in turn). Each arc only
    reaches an enemy within 350 units of the previous one, so a lone target takes just the first
    hit."* <!-- src: Siegebound/DeckBuilderWidget.cpp:150; Docs/Data/cards.csv:29 -->
29. **Wizard** — the ranged castle sentence; its shots splash over 250 units. <!-- src: Docs/Data/cards.csv:30; Siegebound/SummonedUnit.cpp:4863 -->
30. **Sorcerer** — *"It never attacks - no order will make it strike, and an enemy walking into it
    is ignored - so it deals no damage of its own. It still takes your unit orders like anything
    else you play, which is how you walk it onto an ancient ground."* *"While it stands inside an
    ancient ground, every friendly unit that fights standing in that same ground hits +5% harder
    for each second it spends there. The gain is permanent - kept in full when that unit walks
    back out, and lost only when it dies - and it stacks up second after second to a hard ceiling
    of +400%. A second sorcerer in the same ground builds it twice as fast. Units that never
    attack - miners, healers and sorcerers themselves - gain nothing."* <!-- src: Siegebound/DeckBuilderWidget.cpp:85; :112; Siegebound/SummonedUnit.h:1649; :1659 -->
31. **Watch Tower** — the structure sentence. It has no attack. Walk your hero or a unit into its
    ladder (within 350 units) and they climb to a platform 1200 units up
    <!-- src: Siegebound/ClimbableTower.h:670; :770 -->; only your own team may climb it. A ranged
    unit on the platform shooting a target at ground level gets about ×1.79 damage from the height
    rule (1 + 0.10 × 1200 / 152.4) <!-- src: Siegebound/SummonedUnit.h:1687; :1700; Siegebound/ClimbableTower.h:670 -->.
    Stacks to ×2, not ×5.
32. **Witch** — the panel shows no keyword sentence for her in this build (the support sentence
    needs a heal value, and hers is 0) <!-- src: Siegebound/DeckBuilderWidget.cpp:1593; Docs/Data/cards.csv:33 -->.
    Her behaviour is under *Invisibility* above; her 400 range is the veil circle, not an attack.
33. **Fog** — *"Raises fog over the ENTIRE battlefield the instant you play it - there is no spot
    to aim at and no radius. While it hangs, every unit on BOTH sides, yours included, only
    notices enemies close to it: ranged units and towers lose their reach and the fight collapses
    to arm's length, while melee units are barely affected. Playing it again while it is already
    up resets the clock rather than adding to it. While the sky is being held clear against fog,
    playing it is refused outright and costs you nothing."* *"The fog lifts on its own after 300
    seconds."* <!-- src: Siegebound/DeckBuilderWidget.cpp:201; :214; Docs/Data/cards.csv:34 -->
34. **Bright Sun** — *"Clears every trace of fog the instant you play it - there is no spot to aim
    at and no radius - and then holds the sky clear, refusing any new fog for a while afterwards.
    It is worth playing with no fog up at all: that refusal window on its own is half the card."*
    *"The higher above the flat ground you stand at the moment you cast it, the longer that window
    runs - your height is read once, at the cast, and there is no upper limit. When the window
    finally ends the battlefield STAYS clear: fog never returns on its own. Casting it again from
    lower ground while a window is still running would shorten it, so that play is refused
    outright and costs you nothing."* *"Cast from the flat ground it holds fog off for 120
    seconds, before any height on top of that."* <!-- src: Siegebound/DeckBuilderWidget.cpp:229; :235; :242; Docs/Data/cards.csv:35 -->

#### Commands

All **28** key and mouse bindings in the game's input map <!-- src: Content/Input/IMC_Hero.uasset (Mappings array, 28 entries; census TASK-1190 §4a) -->.
Letter keys are positional (see *Known notes*).

| Key | What it does |
|---|---|
| **W / S / A / D** | move (4 bindings) <!-- src: Content/Input/IMC_Hero.uasset (IA_Move); Siegebound/SiegeGhostPawn.cpp:507 --> |
| **Mouse** | look <!-- src: Content/Input/IMC_Hero.uasset (IA_Look); Siegebound/SiegeGhostPawn.cpp:519 --> |
| **Space** | jump <!-- src: Content/Input/IMC_Hero.uasset (IA_Jump); Siegebound/HeroCharacter.h:1126 --> |
| **Shift** (hold) | sprint at 750 <!-- src: Content/Input/IMC_Hero.uasset (IA_Sprint); Siegebound/HeroCharacter.h:1107; Siegebound/HeroCharacter.cpp:448-459 --> |
| **Left mouse** | hero attack — or confirm a placement / a circle / drop a war-map mark <!-- src: Content/Input/IMC_Hero.uasset (IA_Attack); Siegebound/HeroCharacter.cpp:479 --> |
| **Q** | Rally <!-- src: Content/Input/IMC_Hero.uasset (IA_Rally); Siegebound/HeroCharacter.cpp:683 --> |
| **B** | Recall <!-- src: Content/Input/IMC_Hero.uasset (IA_Recall); Siegebound/HeroCharacter.cpp:1506 --> |
| **1 – 6** | play hand slot 1–6 (6 bindings) <!-- src: Content/Input/IMC_Hero.uasset (IA_Card1..6); Siegebound/SiegePlayerController.cpp:598-609 --> |
| **Alt** (hold) | show the mouse cursor for HUD clicks; looking is suspended <!-- src: Content/Input/IMC_Hero.uasset (IA_UICursor); Siegebound/SiegePlayerController.cpp:618-620 --> |
| **Right mouse** / **Escape** | cancel a placement, an aim, or a circle pick at no cost (2 bindings); right mouse also deletes a war-map mark <!-- src: Content/Input/IMC_Hero.uasset (IA_CancelPlace); Siegebound/SiegePlayerController.cpp:627 --> |
| **T** | **Attack** — army-wide order <!-- src: Content/Input/IMC_Hero.uasset (IA_CmdAttack); Siegebound/SiegePlayerController.cpp:636 --> |
| **E** | **Defend** — army-wide order <!-- src: Content/Input/IMC_Hero.uasset (IA_CmdDefend); Siegebound/SiegePlayerController.cpp:644 --> |
| **R** | **Hold** — a three-circle pick <!-- src: Content/Input/IMC_Hero.uasset (IA_CmdHold); Siegebound/SiegePlayerController.cpp:640 --> |
| **F** | **Ambush** — a three-circle pick <!-- src: Content/Input/IMC_Hero.uasset (IA_CmdAmbush); Siegebound/SiegePlayerController.cpp:652 --> |
| **C** | **Follow** — a one-circle pick <!-- src: Content/Input/IMC_Hero.uasset (IA_CmdFollow); Siegebound/SiegePlayerController.cpp:661 --> |
| **H** | discard your whole hand for 20 gold and draw 6 <!-- src: Content/Input/IMC_Hero.uasset (IA_DiscardAll); Siegebound/SiegePlayerController.h:1786; Siegebound/DeckComponent.h:183; Siegebound/SiegePlayerController.cpp:727 --> |
| **Enter** | open/close the assistant chat box (inert in this package) <!-- src: Content/Input/IMC_Hero.uasset (IA_AssistantConsole); Siegebound/SiegePlayerController.cpp:673 --> |
| **M** | open/close the war map — only within 400 units of your own commander <!-- src: Content/Input/IMC_Hero.uasset (IA_WarMap); Siegebound/SiegePlayerController.cpp:690; Siegebound/CommanderNpc.h:295 --> |
| **Tab** | the controls screen; the battle keeps running <!-- src: Content/Input/IMC_Hero.uasset (IA_ControlsHelp); Siegebound/SiegePlayerController.cpp:709 --> |

Other inputs that are not in the map but are read directly:

- **Mouse wheel** — while placing a building, scales its footprint (×1.0–1.5, step 0.1)
  <!-- src: Siegebound/SiegePlayerController.h:2287-2322 -->; during a circle pick, resizes the
  circle by **100 units per notch, between 200 and 5000** <!-- src: Siegebound/SiegePlayerController.h:2119-2127 -->;
  on the war map, resizes a mark by 6 px per notch between 12 and 240 px <!-- src: Siegebound/WarMapWidget.h:1292; :1305; :1317 -->.
- **Z** — accepts the assistant's proposed plan (nothing to accept in this package) <!-- src: Siegebound/SiegeControlsHelpWidget.cpp:1168-1175 -->.
- **F11** or **Alt+Enter** — toggle fullscreen <!-- src: Config/DefaultInput.ini:63-64 -->.

**Console commands and developer cheats do not exist in this Shipping package** — the console is
compiled out and no cheat manager is created — so there is nothing to list here
<!-- src: C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/Core/Public/Misc/Build.h:214-215; C:/Program Files/Epic Games/UE_5.8/Engine/Source/Runtime/Engine/Classes/GameFramework/CheatManagerDefines.h:9 -->.

#### The orders in detail

- **Attack (T):** every commandable unit advances on the enemy castle, engaging the nearest
  enemy it notices on the way; every group is cleared <!-- src: Siegebound/SiegePlayerController.h:1681 -->.
- **Defend (E):** every commandable unit falls back and fights within the castle's half-width plus
  1281 units of your castle; every group is cleared <!-- src: Siegebound/SiegePlayerController.h:1690; Siegebound/SummonedUnit.h:1513 -->.
- **Hold (R):** three circles in turn — first the **selection** circle (opens at **1200**) around
  the units you want, then the **station** circle (**700**) where they stand, then the **attack**
  circle (**1500**) they defend; the group drops any target that leaves both circles
  <!-- src: Siegebound/SiegePlayerController.h:2131 (1200); :2135 (700); :2139 (1500); :1684 (the R handler) -->.
- **Ambush (F):** the same three stages; the group chases a live target with no leash, then
  returns to its station <!-- src: Siegebound/SiegePlayerController.h:1687 -->.
- **Follow (C):** one circle; the circled units escort your hero in formation within **900**
  units and do not fight <!-- src: Siegebound/SiegePlayerController.h:451 (FollowFormationRadius); :2569-2583 (the C handler) -->.
- Each stage is confirmed with the left mouse button; the wheel resizes the current circle; right
  mouse or Escape exits the pick. The HUD shows the active command.
- Ogres and Sappers ignore orders; Miners interpret them as mining orders (see the Miner card).

#### The AI commander

Three different things answer to that phrase, and each is described as it ships.

**1. The opponent (the bot).** *Play (vs Bot)* spawns a computer opponent on the **Red** side. It
has **no hero** <!-- src: Siegebound/SiegeGameMode.cpp:1669-1765 (SpawnBot spawns a controller only); Siegebound/SpellLineSweep.h:124-125; Siegebound/SiegeBotController.cpp:772-773 -->.
It thinks every **2 s**; the first of its rules that applies owns that turn
<!-- src: Siegebound/SiegeBotController.h:221; Siegebound/SiegeBotController.cpp:369-389 -->:

| Rule | What the bot does |
|---|---|
| 1 — Defend | if one of your units is on its half of the field: the cheapest affordable defensive card — a tower placed **750 units** in front of its castle toward the intruder, or a unit in a lane spread of **±900 units** <!-- src: Siegebound/SiegeBotController.cpp:433-490; Siegebound/SiegeBotController.h:443; :376 --> |
| 2 — Economy | otherwise a Miner while it has fewer than **3** alive and an open mine exists, else a Deep Mine <!-- src: Siegebound/SiegeBotController.cpp:496-610; Siegebound/SiegeBotController.h:227 --> |
| 3 — Lightning | at one of your towers with **2** or more of your units near it (only the Defensive deck has Lightning) <!-- src: Siegebound/SiegeBotController.cpp:811-853; Siegebound/SiegeBotController.h:273 --> |
| 4 — Attack | with **36** gold or more, the most expensive unit it can afford, at its castle front — bot units always advance on their own <!-- src: Siegebound/SiegeBotController.cpp:856-907; Siegebound/SiegeBotController.h:231 --> |
| 5 — Discard | the most expensive card it cannot play, for **1** gold <!-- src: Siegebound/SiegeBotController.cpp:909-944; Siegebound/SiegeBotController.h:235 --> |

At the start of each match it picks **one of two decks at random** <!-- src: Siegebound/SiegeBotController.cpp:299-337 -->:

- **Bot Aggro Rush** — Footman 12 · Militia Mob 6 · Pikeman 6 · Knight 6 · Archer 6 · Cavalry 4 ·
  Sapper 4 · Wall 4 · Miner 2 <!-- src: Siegebound/SiegeBotController.cpp:253-267 -->
- **Bot Defensive Economy** — Wall 8 · Arrow Tower 8 · Knight 6 · Bomb Tower 4 · Ballista Tower 4 ·
  Miner 4 · Barracks 3 · Crystal Tower 3 · Cleric 3 · Ogre 2 · Deep Mine 2 · Lightning 2 ·
  Longbowman 1 <!-- src: Siegebound/SiegeBotController.cpp:270-288 -->

It places cards in its own castle square, or in the mid zone while Red owns it
<!-- src: Siegebound/SiegeBotController.cpp:1622-1660 -->. Because it has no hero, any line spell
it cast would fire from its castle; but it never casts Fireball, Frost Nova, Fog, Bright Sun or a
hero upgrade, never gives orders, and never veils — none of those cards are in its decks. There
is no bot in a networked match <!-- src: Siegebound/SiegeGameMode.cpp:1678-1684 -->.

**2. Your commander and the war map.** Each castle houses a commander NPC for its own team
<!-- src: Siegebound/Castle.cpp:153; Siegebound/CommanderNpc.h:295 -->. Stand within **400 units**
of yours and press **M** for the war map: a top-down map of the whole arena, shaded by elevation,
with your own units and hero as live dots refreshed **4 times a second**
<!-- src: Siegebound/WarMapWidget.h:1186 -->, seven place markers (your castle, the enemy castle,
mid, the near and far ancient grounds, the nearest mine, your hero), and numbered marks you can
drop with the left mouse button, resize with the wheel and delete with the right mouse button
<!-- src: Siegebound/WarMapWidget.h:1218-1240; :1280-1364 -->. **Reveal enemy positions** costs
**30 gold** and shows every enemy unit your team can currently see (veiled units excluded) and the
enemy hero as a frozen snapshot of up to **512** dots; the snapshot is discarded when the map
closes <!-- src: Siegebound/CommanderNpc.h:311; Siegebound/SiegePlayerController.cpp:147; :7185-7218 -->.
The match does not pause while the map is open <!-- src: Siegebound/SiegeControlsHelpWidget.cpp:72 -->.

**3. The assistant.** The typed-sentence assistant described under *About the in-game AI
assistant* — inert in this package. For the record, its grammar covers seven intents (send, guard,
ambush, follow, charge, fall back, rally) over seven places, unit kinds by name with counts, and
*"when I have at least N …"* triggers held for **120 s** and checked once a second
<!-- src: Siegebound/SiegeAssistantCommand.h:66-75; Siegebound/SiegeAssistantVocabulary.cpp:157-187; Siegebound/SiegeAssistantComponent.h:1704; :1713 -->;
it only ever gives unit orders and never plays a card or spends gold.

#### Areas of interest

The battlefield is **52000 × 24000 units** (26000 × 12000 to each side of centre)
<!-- src: Siegebound/ScatterConfig.h:356 -->. Everything below except the castles and the mid zone
is placed fresh every match — and every *Play Again* — from a new random seed
<!-- src: Siegebound/BattlefieldScatter.cpp:290-306; Siegebound/BattlefieldScatter.h:211 -->.
**Symmetry rule:** every placed thing is generated on the Blue half and then mirrored as its
180°-rotated twin on the Red half (at −X, −Y), so both sides get the same layout
<!-- src: Siegebound/ScatterConfig.h:57-64; :346 -->.

**Gold mines — 6 per match (3 per side)** <!-- src: Siegebound/ScatterConfig.h:415; Siegebound/BattlefieldScatter.cpp:1631-1790 -->

- **Effect:** each holds **300 gold** <!-- src: Siegebound/ScatterConfig.h:448; Siegebound/GoldNode.h:236 -->.
  The first miner to arrive claims the mine for its team; the other team's miners cannot work it
  while it is occupied; it frees when the last miner leaves <!-- src: Siegebound/GoldNode.cpp:114-180 -->.
  Each working miner drains **1 gold per second** — exactly what it pays you
  <!-- src: Siegebound/GoldNode.h:253 -->. At zero the mine is **depleted for the rest of the
  match**, its miners leave to find another, and its glow dims <!-- src: Siegebound/GoldNode.cpp:343-354; Siegebound/GoldNode.h:269 -->.
- **Where:** three positions are drawn on the Blue half, each at least **1500 units** from the
  centre line and no closer than **600** to the field's edge; at least **3000 units** from every
  other mine; outside a **4500**-unit disc around each castle and an **800**-unit disc around the
  player start (both grown by 600); on ground no steeper than **30°**. If no spot is found after
  the allowed attempts, a fallback point at half-field is used. The three twins mirror them
  <!-- src: Siegebound/ScatterConfig.h:428; :440; :452; :462; :382; :386; Siegebound/BattlefieldScatter.cpp:1659-1790; :1422-1487 -->.

**Ancient grounds — 2 per match (1 per half)** <!-- src: Siegebound/ScatterConfig.h:470-476; Siegebound/BattlefieldScatter.cpp:1950-2090 -->

- **Effect:** a rune-marked square, **1680 × 1680 units**, with no collision
  <!-- src: Siegebound/AncientGround.h:178; Siegebound/ScatterConfig.h:494 -->. Every **second**
  it counts the living Sorcerers of each team standing inside; every *other* friendly unit inside
  that can attack gains that many permanent damage stacks (**+5 % each, up to 80 = +400 %**),
  keeps them when it leaves, and loses them only on death
  <!-- src: Siegebound/AncientGround.h:191; Siegebound/AncientGround.cpp:198-300; Siegebound/SummonedUnit.h:1649; :1659 -->.
  It is neutral — never captured, and both teams can be empowered in it at once.
- **Where:** one position on the Blue half between **4000 and 16080 units** from the centre line
  and within **10800** of the middle line, at least **1800 units** from every mine, clear of
  everything within **1200 units**, on a flat surface; up to 48 attempts. Its twin mirrors it
  <!-- src: Siegebound/ScatterConfig.h:503; :552; :565; :576; :588; Siegebound/BattlefieldScatter.cpp:1970-2085 -->.

**The mid capture zone — 1, fixed at the centre of the map** <!-- src: Content/Maps/L_Arena.umap (CaptureZone actor); Siegebound/CaptureZone.h -->

- A **1680 × 1680** square at the map's origin <!-- src: Siegebound/CaptureZone.h:190 -->. Every
  **0.5 s** it counts the units and heroes inside (buildings and mines do not count): one team
  alone owns it; both at once makes it **neutral**; empty keeps the last owner
  <!-- src: Siegebound/CaptureZone.h:194; :204; Siegebound/CaptureZone.cpp:136-230 -->.
- **Effect:** the owner may **place cards inside it** — the only forward placement area on the
  map; the bot uses it too <!-- src: Siegebound/SiegePlayerController.cpp:6074-6079; Siegebound/SiegeBotController.cpp:1622-1660 -->.
  Play Again resets it <!-- src: Siegebound/SiegeGameMode.cpp:1422-1424 -->.

**The castles — 2**, level-placed at the far ends (see *The castle*), each with its **7380**-unit
placement square, its commander and its torches.

**Terrain and scatter** — decorative except where noted; all of it obeys the same seed and
symmetry rule, keeps a clear corridor **1000 units** to either side of the castle-to-castle line
<!-- src: Siegebound/ScatterConfig.h:399 -->, and stays out of the keep-clear discs above. Per
match <!-- src: Content/Data/DA_BattlefieldScatter.uasset (Layers[7], InstanceCount / MinSpacing / bBlocking per layer) -->:

| Layer | Instances | Minimum spacing | Blocks movement | Where |
|---|---|---|---|---|
| Trees | 340 | 300 | yes | biased toward the edges |
| Rocks | 300 | 350 | yes | biased toward the edges |
| Boulders | 30 | 900 | yes | biased toward the edges |
| **Hills** | **40** | 2000 | yes | anywhere |
| Slabs | 40 | 1500 | yes | biased toward the edges |
| Grass | 36750 | 50 | no | anywhere |
| Plants | 2000 | 200 | no | anywhere |

Hills are climbable and are where ranged units earn the height bonus. Blocking scatter carves the
navigation mesh, and the scatter's meshes stop projectiles <!-- src: Siegebound/BattlefieldScatter.cpp:191; Siegebound/Projectile.cpp:411-447 -->.
How far grass and trees are drawn depends on the **Foliage** graphics setting
<!-- src: Siegebound/BattlefieldScatter.cpp:988-1075 -->. The card-summoned structures (Watch
Tower, Barracks, Deep Mine, Wall, the four attack towers) are *not* areas of interest — they are
played from the hand into a placement region. The two gold-node props in the middle of the field
are visual only <!-- src: Content/Maps/L_Arena.umap (SM_GoldNodeProp) -->.

#### Winning, losing, and playing again

- The match ends the instant a castle is destroyed; the **other** team wins, the world freezes,
  and a **Victory!** or **Defeat** screen offers **Play Again** <!-- src: Siegebound/SiegeGameMode.cpp:523-572; :618; Content/UI/WBP_VictoryScreen.uasset -->.
- **Play Again** resets everything — gold, hero upgrades, the capture zone, the castles — and
  re-rolls the mines, ancient grounds and scatter from a new seed <!-- src: Siegebound/SiegeGameMode.cpp:1305; :1422-1424; :1527; Siegebound/SiegePlayerState.cpp:478; Siegebound/BattlefieldScatter.h:211 -->.
- **Sandbox (No Bot)** opens the arena with no opponent and 9999 gold, for trying things out.

#### Where the shipped game differs from its design notes

The game as shipped follows the code, not its design document, wherever the two differ; the
design document itself is not included in this package (see *What was left out*). The
differences a player would notice:

| The design document said | The shipped game does |
|---|---|
| hero respawns 5 s after death at its castle <!-- src: Docs/GDD.md:47 --> | you play a ghost for **180 s**, then respawn <!-- src: Siegebound/SiegeGameMode.h:461 --> |
| discard costs 1 gold per card <!-- src: Docs/GDD.md:133 --> | **20 gold** for the whole hand; no per-card discard <!-- src: Siegebound/SiegePlayerController.h:1786 --> |
| each card has a maximum number of copies per deck <!-- src: Docs/GDD.md:73 --> | no per-card limit; only the 50-card total <!-- src: Siegebound/CardRow.h:204-212 --> |
| units notice enemies within 600 <!-- src: Docs/GDD.md:147 --> | **5000** (609.6 under fog) <!-- src: Siegebound/SummonedUnit.h:914 --> |
| units give up a chase at 900 <!-- src: Docs/GDD.md:159 --> | **8000** (609.6 under fog) <!-- src: Siegebound/SummonedUnit.h:1448 --> |
| defenders engage within 2500 of the castle <!-- src: Docs/GDD.md:223 --> | castle half-width + **1281** <!-- src: Siegebound/SummonedUnit.h:1513 --> |
| placement square half-width 2460 <!-- src: Docs/GDD.md:125 --> | **7380** <!-- src: Siegebound/Castle.h:394 --> |
| high ground gives no stat bonus <!-- src: Docs/GDD.md:437 --> | ranged **+10 % per 152.4 units** of height <!-- src: Siegebound/SummonedUnit.h:1687-1700 --> |
| Archer 700, Longbowman 1200, Wizard 700 range <!-- src: Docs/GDD.md:359; :373; :405 --> | **2100 / 3600 / 2100** <!-- src: Docs/Data/cards.csv:3; :12; :30 --> |
| the Cleric heals on a 1 s timer <!-- src: Docs/GDD.md:374 --> | heals every **0.1 s** at 8 HP/s <!-- src: Siegebound/SummonedUnit.h:1549 --> |
| 30 cards <!-- src: Docs/GDD.md:350 --> | **34** — the Witch, Watch Tower, Fog and Bright Sun were added <!-- src: Docs/Data/cards.csv:2-35 --> |
| gold nodes 800 units from each castle <!-- src: Docs/GDD.md:429 --> | 6 neutral mines placed by the rule above (the document itself later supersedes the old wording <!-- src: Docs/GDD.md:443 -->) |

### Other features

Everything you can do in the game outside a match, from the main menu.

**Main menu** — **Play (vs Bot)** · **Sandbox (No Bot)** · **Deck Builder** · **Multiplayer** ·
**Settings** · **Login** · **Quit** <!-- src: Content/UI/WBP_MainMenu.uasset (label strings and click handlers); Config/DefaultEngine.ini:10 -->.

- **Play (vs Bot)** — a match against the bot described above.
- **Sandbox (No Bot)** — the arena with no opponent and 9999 gold.
- **Deck Builder** — build and save up to **10 decks** per account <!-- src: Siegebound/SiegeDeckSaveGame.h:57 -->.
  A deck must total **exactly 50** cards, with no per-card limit; the panel shows the running
  total out of 50 and the average cost; clicking a card opens its detail panel with the shipped
  description text quoted above; **Play** and **Exit** buttons
  <!-- src: Siegebound/DeckLibrary.cpp (IsDeckLegal); Content/UI/WBP_DeckBuilder.uasset (TotalText, AvgText, Play, Exit) -->.
  Your edits **auto-save** to the slot you are editing <!-- src: Siegebound/DeckBuilderWidget.h:370 -->,
  and **right-clicking a slot** marks it as the active deck the next match uses
  <!-- src: Siegebound/DeckBuilderWidget.h:93; Siegebound/DeckBuilderWidget.cpp:1240 -->.
- **Settings** — one toggle, *"Confirm AI orders before they execute"* (default on; no effect
  without the assistant model), a **Graphics** button, and Back <!-- src: Siegebound/SettingsMenuWidget.cpp:33; :56; :64 -->.
- **Settings → Graphics** <!-- src: Siegebound/SiegeGraphicsMenuWidget.cpp:41-145 -->:
  - **Auto-Detect Quality**;
  - **Overall Quality** — a slider over **five** levels (0–4), showing *Custom* when the groups
    below disagree <!-- src: Siegebound/SiegeGraphicsSettingsSubsystem.h:150-163 -->;
  - **Resolution Scale** — **50–100 %** <!-- src: Siegebound/SiegeGraphicsSettingsSubsystem.h:174-175 -->;
  - **Screen Resolution** — a stepper over the resolutions your display supports
    <!-- src: Siegebound/SiegeGraphicsSettingsSubsystem.cpp:735-738 -->;
  - **Window Mode** — Fullscreen / Borderless Window / Windowed <!-- src: Siegebound/SiegeGraphicsSettingsSubsystem.cpp:843-846 -->;
  - **V-Sync** <!-- src: Siegebound/SiegeGraphicsMenuWidget.cpp:860-871 -->;
  - **Frame Rate Limit** — 30, 60, 90, 120, 144, 165, 240 or Unlimited <!-- src: Siegebound/SiegeGraphicsSettingsSubsystem.h:642 -->;
  - **ten** per-group sliders, five levels each: View Distance, Anti-Aliasing, Shadow, Global
    Illumination, Reflection, Post Process, Texture, Effects, Foliage, Shading
    <!-- src: Siegebound/SiegeGraphicsSettingsSubsystem.cpp:29-38; :87-96 -->;
  - **Show FPS counter during a match** (default off) — a readout refreshed every **0.5 s**
    <!-- src: Siegebound/SiegeGraphicsMenuWidget.cpp:378; Siegebound/SiegeSettingsSubsystem.h:296; Siegebound/SiegeGraphicsMenuWidget.h:374 -->;
  - **Back**.
  - A change to resolution, window mode or similar is applied provisionally and asks **Keep /
    Revert** with a **10-second** countdown ticking once a second; do nothing and it is meant to
    revert (see *What is NOT in this build* — this has not been watched against a real clock)
    <!-- src: Siegebound/SiegeGraphicsMenuWidget.h:410; :418 -->.
- **Login** (accounts) — **Create Account**, **Log into existing account**, **Log In**, **Log
  Out**, **Back** <!-- src: Siegebound/AccountMenuWidget.cpp:30-63 -->. A display name is **3–24
  characters** and a password **at least 4** <!-- src: Siegebound/SiegeAccountSubsystem.cpp:46; :50 -->.
  Each account keeps its own decks and settings; you start as a guest, and nothing in the game
  requires logging in. Accounts are stored locally in the `Saved\` folder.
  - **Cloud sync:** {{SHIP:CLOUD_SYNC}} <!-- resolved by the ship after inspecting the stage for Config/SiegeCloudDev.ini; see Siegebound/SiegeCloudClient.cpp:108-115 and Siegebound/AccountMenuWidget.cpp:45-104 (Link to Cloud, Sync Now) -->
- **Multiplayer** — *present in the build, with a limit:* **Host** / **Join** (an address; default
  port **7777**) / **Back** <!-- src: Content/UI/WBP_SessionMenu.uasset (Multiplayer, Host, Join, Back, AddressTextBox); Siegebound/SiegeSessionSubsystem.h:62 -->.
  It is a listen server over LAN or a direct IP. **The host plays; the joining player observes**
  — a joiner's card plays are refused and building actions are host-only, so the second player
  watches the match state rather than fighting <!-- src: Siegebound/SiegePlayerController.cpp:974; Siegebound/Building.cpp:415 -->.
  There is no bot in a networked match <!-- src: Siegebound/SiegeGameMode.cpp:1678-1684 -->.
- **Quit** — exits the game.

**In a match**

- **The war map** (M, near your commander) — see *The AI commander*.
- **The controls screen** (Tab) — every binding above, grouped: Hero · Cards · Orders · Pick mode
  · Interface <!-- src: Siegebound/SiegeControlsHelpWidget.cpp:303-1482 -->.
- **The FPS counter** — if enabled in Graphics.
- **The HUD** — gold, gold rate, miner count, `OVERTIME`, Rally status, the active order, your
  hand of six with key chips and the next-card preview, castle health bars, and overhead bars on
  every fighter <!-- src: Content/UI/WBP_HUD.uasset; Content/UI/WBP_CardHand.uasset; Content/UI/WBP_CastleHealthBar.uasset; Siegebound/CombatantHealthBarComponent.h -->.
- **Victory / Defeat → Play Again.**
- **Positional keyboard remapping** — always on, no setting (see *Known notes*).

**Present in the build but not yet usable**

- **The assistant chat box** (Enter) — no model file ships (see *About the in-game AI assistant*).
