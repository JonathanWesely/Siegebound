# TASK-697 — build-master handoff — THE WRAP (README · the zip · secrets sweep · COMMIT B)
2026-08-29 · law `PKG-§1..§6` · follows TASK-696's cook · closes the MAP-MIRROR + SETUP-DOCS + PACKAGED-ZIP wave

## 1. THE ZIP

| | |
|---|---|
| **Absolute path (for Jonathan)** | `C:\GitProjects\GitHub\GitClaudeUnrealTesting\packagedZIPofGame\Siegebound-Win64-Development-2026-08-29.zip` |
| **Size** | **1.313 GB** (1,409,955,049 bytes) |
| **Extracted footprint** | **1.91 GB**, 70 files |
| **Entries in the zip** | 71 (the staged `Windows\` tree + `README.md` at the archive root) |
| **Name** | pinned by `PKG-§2` — `Siegebound-Win64-Development-<YYYY-MM-DD>.zip` ✅ |
| **Built with** | UE 5.8.0 · Win64 · Development (P2 default) · contains TASK-694's war-map flip (cooked from `b21ccc5`) |

**Zip integrity verified by reading the archive back, not by assuming:**
- `Windows\GitClaudeUnrealTest.exe` — 171,520 B — **the file the README tells you to double-click**
- `Windows\GitClaudeUnrealTest\Binaries\Win64\GitClaudeUnrealTest.exe` — 347,694,080 B — the real binary
- `Windows\GitClaudeUnrealTest\Content\Paks\GitClaudeUnrealTest-Windows.ucas` — 1,077,741,296 B
- `README.md` — 8,421 B — at the archive root, first thing you see on extract
- ⛔ **Zero `Models/` or `.gguf` entries** (`PKG-§4` re-verified inside the ZIP itself)
- ⛔ **No test logs**: my boot-verify `Saved/` was purged before zipping; the single remaining
  `Saved` path is `Windows\Engine\Saved\Config\Windows\Manifest.ini`, a standard staged engine
  config manifest, not run output.

Built with .NET `ZipFile.CreateFromDirectory` (ZIP64-capable) rather than PowerShell 5.1's
`Compress-Archive`, which is unreliable near the 2 GB boundary — a deliberate choice, since a
silently-truncated 1.9 GB archive would have been the worst possible failure here.

## 2. SECRETS SWEEP of `Docs/setupdirections.md` (the 697 duty) — **ZERO HITS**

Ran before staging the file, over all 824 lines. **Both pattern sets returned no matches (grep exit 1).**

- **Spec patterns:** `hf_…` · `sk-…` · `eyJ…` · `sb_secret` · `sbp_…` → **0**
- **Extended patterns:** the Slack channel ID `C0BF0QZP3CN` · Jonathan's email · `<ref>.supabase.co`
  live project refs · `xox[baprs]-` Slack tokens · `ghp_…` GitHub tokens ·
  `(password|secret|api_key|token) = <12+ chars>` → **0**
- **Context check (what legitimately DOES appear):** environment-variable *names* only —
  `HF_TOKEN`, `MESHY_TOKEN`, `UV_SYSTEM_CERTS` — always in the context of the doc's own
  env-only token law, plus the generic wildcard `*.supabase.co` in an antivirus-exclusion list.
  No credential material, no project/org IDs. ✅ **Cleared for commit**, confirming TASK-695's
  own declaration independently.

## 3. THE README — full text reproduced (`PKG-§3`: the folder is unversioned, the repo keeps the record)

The file ships at `packagedZIPofGame/README.md` and inside the zip root. Verbatim:

---

# Siegebound — Win64 Development build

**Zip:** `Siegebound-Win64-Development-2026-08-29.zip`
**Built:** 2026-08-29 · Unreal Engine 5.8.0 · Win64 · **Development** configuration
**Extracted size:** ~1.91 GB (70 files)

---

## ⚠️ First, an honest correction

The original ask was to zip *"the `GitClaudeUnrealTest` folder without `.claude` / `Tools` / `Docs`"*
so anyone could extract and click.

**That would not have worked.** The raw project folder is a *source project*, not a game. On a
machine without Unreal Engine 5.8 installed there is nothing in it to click: `.uproject` +
`Content/` + `Source/` need the **engine installed and a full C++ compile** before a runnable
program exists at all. Handing someone that folder hands them a build job, not a game.

**So what is in this zip is a COOKED PACKAGE** — the real thing that satisfies "extract and
click": a compiled game `.exe`, the content pre-processed into platform-ready `.pak`/`.ucas`
files, and the slice of the Unreal runtime the game needs to boot. **No engine install, no
compiler, no Epic account required.**

---

## How to run it

1. **Extract the whole zip** anywhere (Desktop, Downloads, a USB stick — any folder works).
   ⚠️ Extract it *fully* first. Running the `.exe` from inside the zip preview window will fail —
   Windows only unpacks the one file you clicked, and the game needs the folder next to it.
2. Open the extracted folder and go into **`Windows\`**.
3. **Double-click `GitClaudeUnrealTest.exe`.**

   > Full path after extracting: `Windows\GitClaudeUnrealTest.exe`

   ℹ️ **Yes, the file is named `GitClaudeUnrealTest.exe`, not `Siegebound.exe`.** "Siegebound" is
   the game; `GitClaudeUnrealTest` is the internal project name it was built under, and renaming
   the executable would break the folder layout the engine expects. Click it anyway — it is the
   right file. (For the same reason, Windows' file properties may show the leftover template
   name *"Third Person Game Template"*; cosmetic only.)

### Windows will probably warn you

This build is **not code-signed**, so Windows SmartScreen will likely show
*"Windows protected your PC"*. That is the expected warning for any unsigned executable — it is
not a virus report. To continue: **More info → Run anyway**.

Your antivirus may also scan the folder on first launch, which can make the first start slow.

### Minimum requirements

| | |
|---|---|
| OS | **Windows 10 or 11, 64-bit** (no 32-bit / ARM build included) |
| GPU | **DirectX 12 capable with Shader Model 6** — the build's default RHI is DX12/SM6 |
| Disk | ~1.9 GB extracted (plus the zip itself while unpacking) |
| RAM | ~4 GB free — the game measured ~1.6–2.2 GB resident while running |
| Other | The Visual C++ redistributable is bundled under `Windows\Engine\Extras\Redist\en-us\` if your machine happens to lack it |

First launch takes noticeably longer than later ones — shaders warm up on the way in.

---

## What was packaged

**The cooked runtime, and only what the game actually reaches:**

- **The game executable** — `Windows\GitClaudeUnrealTest.exe` (launcher) plus the real binary at
  `Windows\GitClaudeUnrealTest\Binaries\Win64\GitClaudeUnrealTest.exe`.
- **The cooked content** — `Windows\GitClaudeUnrealTest\Content\Paks\`: `.pak` + `.ucas`/`.utoc`
  (~1.03 GB). Every mesh, texture, material, Blueprint, card table, input mapping and UI widget
  the game loads, converted to platform-ready form.
- **The Unreal 5.8 runtime** — only the engine binaries, shaders and plugin libraries this game
  needs, including the SiegeLlama plugin's native libraries.
- **The VC++ redistributable installer**, bundled as a fallback.

**Maps included — an explicit allowlist, not "everything":**

| Map | Role |
|---|---|
| `/Game/Maps/L_MainMenu` | the boot map — what opens when you click the exe |
| `/Game/Maps/L_Arena` | the battlefield where a match is played |

Those are the only two maps the shipped flow can reach. The engine-template and marketplace demo
levels that live in the project (`ThirdPerson`, `Variant_Combat`, `Variant_Platforming`,
`Variant_SideScrolling`, and the asset-pack showrooms) were deliberately **excluded** — they are
scaffolding that came with the templates and asset packs, they are not the game, and cooking them
would have added weight for nothing.

---

## What was left out of the original project folder, and why

| Left out | Why |
|---|---|
| **`Source/` (all C++) and the uncooked `Content/`** | Already *compiled and cooked into* the `.exe` and the `.pak`/`.ucas` files. Shipping the sources too would only add a copy nobody can run without the engine. |
| **~8 GB of unused marketplace/asset-pack content** | The project's `Content/` holds large third-party packs (rocks, foliage, VFX, SFX, castle props). Only the assets the two shipped maps and the game actually reference were cooked in; the rest never ships. This is most of the difference between a ~10 GB folder and a ~1.9 GB package. |
| **`Content/RawAssets/`** | Source `.fbx` and `.png` art files — inputs to the art pipeline, already baked into the cooked assets. |
| **`.claude/`** | The AI agent pipeline — task board, conventions, handoffs, QA reports. Development process, irrelevant at runtime. |
| **`Tools/`** | The asset/automation pipelines (TRELLIS, Meshy, video review, Supabase helpers). Developer tooling; also the place API tokens would live, so it stays out of anything distributed. |
| **`Docs/`** | Design and setup documentation, including the pipeline setup guide. |
| **`Saved/`, `Intermediate/`, `DerivedDataCache/`, `Binaries/`, `Build/`** | Local build scratch and caches — machine-specific, regenerated on demand, and together larger than the game itself. |
| **The LLM model weights (`Models/*.gguf`, ~2.3 GB)** | Deliberately excluded — see below. |
| **Editor-only and dev-only configuration** | Settings that only mean something inside the Unreal Editor. |

---

## About the in-game AI assistant

The project includes an optional in-match AI assistant backed by a local language model
(`Qwen3-4B`, ~2.3 GB of weights). **Those weights are NOT in this zip**, to keep the download
roughly a third of the size it would otherwise be.

**This does not block anything.** The game detects the missing model at startup, writes one log
line, and runs normally — in the code's own words:

> *"the in-match assistant is unavailable this session … **THE MATCH IS FULLY PLAYABLE**: nothing
> is blocked, nothing is retried, and every keyboard command is byte-identical."*

Verified live in this exact build: it logs the notice and reaches the main menu normally. If you
want the assistant enabled, a with-model variant can be produced (adds ~2.3 GB).

---

## Known notes for this build

- **Development configuration**, not Shipping — deliberately, for parity with every test and
  playtest this project has run. It carries a console and extra logging, and it includes a
  **383 MB debug-symbol file** (`GitClaudeUnrealTest.pdb`). That single file is ~20% of the
  package and is only useful for diagnosing crashes; a Shipping build would be substantially
  smaller and is a one-word request.
- The game writes its own logs and save data into `Windows\GitClaudeUnrealTest\Saved\` next to
  the executable, so extract somewhere you have write permission (not `C:\Program Files`).
- On this machine's first runs the audio device occasionally failed to open
  (`OpenAudioStream failed`). It did not stop the game from booting or playing; if you get no
  sound, that is the likely cause.

---

## What was verified before shipping

- The UAT cook reported its own **`BUILD SUCCESSFUL`** verdict.
- The packaged `.exe` was launched and **reached the main menu**, confirmed from the build's own
  log (`LoadMap: /Game/Maps/L_MainMenu` → `MainMenu up for play`).
- The battlefield map was booted from the package: navigation generated and passed its
  traversability check, the mines spawned, and real card decks were built from the shipped card
  table — so the content genuinely made it in, rather than merely booting to a menu.
- Confirmed the package contains **no model weights** and no `Models/` directory.

⚠️ Not verified by machine: actually *playing* a match with mouse and keyboard. There is no
automated input lane in this pipeline, so hands-on play is human acceptance, not a claim made here.

---

## 4. HOW THE README HANDLES JONATHAN'S STATED MODEL (`PKG-§1`)

His ask was to zip *"the GitClaudeUnrealTest folder without .claude/Tools/Docs"*. The README opens
by correcting that **plainly and without hedging** — that folder is a source project, not a game;
without UE 5.8 installed **and a C++ compile** there is nothing in it to click. What ships instead
is a cooked package: compiled `.exe` + pak'd content + the engine runtime slice. The correction is
stated in the README **and** relayed to him in the Slack post and the orchestrator return —
⛔ never silently substituted.

The README also covers, per his verbatim scope: what was packaged (incl. the two-map allowlist),
what was left out **and why** (the uncooked project, ~8 GB of unused marketplace content,
`RawAssets/`, `.claude/`, `Tools/`, `Docs/`, `Saved/`/`Intermediate/`/`DerivedDataCache/`, the LLM
weights, dev-only config), and how to run from just the zip — including the **SmartScreen
"More info → Run anyway"** note an unsigned exe will trigger, the honest minimum requirements
(Win10/11 x64, DX12/SM6, ~1.9 GB disk), the extract-fully-first warning, and a plain explanation
of why the executable is named `GitClaudeUnrealTest.exe` rather than `Siegebound.exe`.

## 5. COMMIT B

Cargo (explicit paths, board LAST): the root `.gitignore` fence line · `Docs/setupdirections.md`
(swept clean) · `packagedZIPofGame/README.md` — **⛔ NOT committable, it is inside the ignored
folder; its full text above IS the repo's record, exactly as `PKG-§3` intends** · the 696 + 697
handoffs · TASKBOARD.

**COMMIT B = `24e0730`** (5 files, 1247 insertions; main **3 ahead** of origin, ⛔ **not pushed**).
Commit A (TASK-694) was `b21ccc5`.

**⛔ INDEX-CLEAN PROOF, taken twice before committing — both clean:**
1. `git status --porcelain | grep -i packagedZIP` → **no match** (the zip and the 1.91 GB build are
   neither staged nor untracked).
2. `git diff --cached --name-only | grep -i packagedZIP` → **no match**; the staged list was exactly
   `.gitignore` · `TASKBOARD.md` · `handoffs/TASK-696-buildmaster.md` ·
   `handoffs/TASK-697-buildmaster.md` · `Docs/setupdirections.md`.
3. `git check-ignore -v` confirms the zip, the exe and `packagedZIPofGame/README.md` all resolve to
   `.gitignore:19`.

Post-commit porcelain carries **only** `Config/DefaultEngine.ini` — still excluded, still Jonathan's
P3 ruling. ⛔ Never pushed.

## 6. STATE

Editor relaunched and left UP with MCP live (post-state per dispatch). `L_Arena` hash == the
ROT-§2 ledger, never saved. The zip is a local artifact; distribution is Jonathan's alone.
