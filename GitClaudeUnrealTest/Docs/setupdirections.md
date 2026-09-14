# Setup Directions — Recreating the AI-Agent Game-Dev Pipeline on a New Machine

This is the complete guide to rebuilding this project's development system — a UE 5.8 C++
game driven by a team of Claude agents — on another device. It is written for **two readers
working together**: you (the human, doing installs, accounts, and dashboard clicks) and a
**fresh Claude Code session** (scaffolding files, wiring config, and running the verify
probes). Steps are tagged **[You]** or **[Claude]**; untagged steps are either's.

Follow the chapters **in order** — each chapter's prerequisites come before it. Where this
repo is cited as the worked example, generalize paths (`<your-repo>`) to your own machine.

**Secrets law for this document:** no tokens, keys, passwords, or project-specific URLs
appear anywhere below. Every secret is referenced by WHERE it lives (an environment-variable
name, a gitignored config file, a provider dashboard) plus how to mint a fresh one.

---

## THE LIST — every application, website, and account you need

Install/create these as you reach the chapter that needs them (chapter number in the last column).

| # | Thing | What it's for | Get it from | Ch. |
|---|-------|---------------|-------------|-----|
| 1 | **Git** | Version control | git-scm.com | 1 |
| 2 | **Git LFS** | Large binary assets (uasset/fbx/png/...) | git-lfs.com | 1 |
| 3 | **GitHub account** | Remote repo host | github.com | 1 |
| 4 | **Epic Games Launcher + Unreal Engine 5.8** | The engine (5.8 ships the built-in MCP plugin). ⚠️ Fab/marketplace asset imports are a HUMAN step — agents cannot drive the Epic Launcher; this project formalized that as a request file the artist writes and the human fulfills | epicgames.com | 2 |
| 5 | **Visual Studio 2022 — "Game development with C++" workload** | Compiling the UE C++ project | visualstudio.microsoft.com | 2 |
| 6 | **Claude Code** (CLI; needs Node.js/npm) + optionally **Claude Desktop** | The orchestrator + agent team. Requires a Claude subscription that includes Claude Code | `npm install -g @anthropic-ai/claude-code` | 2–3 |
| 7 | **Blender 5.1+ (the "Lab" line with the built-in MCP add-on)** | 3D modeling driven by the art-director agent | blender.org | 5 |
| 8 | **Hugging Face account + access token** | TRELLIS.2 image→3D generation (ZeroGPU quota; PRO ~$9/mo recommended for batches) | huggingface.co → Settings → Tokens (read scope is enough) | 6 |
| 9 | **TRELLIS.2 Space** (`microsoft/TRELLIS.2` on Hugging Face Spaces) | The image→3D engine itself — no separate account; your HF token is the credential | huggingface.co/spaces | 6 |
| 10 | **uv** (Python package manager) | The art-pipeline virtualenv (pinned Python 3.12, disposable, recreated by `uv sync`) | astral.sh/uv (or winget — verify the package id on the new machine) | 6 |
| 11 | **Meshy account (paid)** | Second image→3D / retexture engine | meshy.ai → account → API key | 7 |
| 12 | **Python 3.14 (system install) + Pillow** | The video-review frame extractor (system Python by law — NO venv for this lane) | python.org; then `py -m pip install pillow` | 8 |
| 13 | **ffmpeg** | Frame extraction from gameplay videos | `winget install --id Gyan.FFmpeg -e --source winget` | 8 |
| 14 | **Xbox Game Bar** (or any screen recorder) | Capturing gameplay clips for the footage-analyst (Win+G / Win+Alt+R; preinstalled on Windows 11) | Microsoft Store (usually preinstalled) | 8 |
| 15 | **Slack workspace + the Claude Slack connector** | The team's visibility mirror channel | slack.com; connector via claude.ai Settings → Connectors | 9 |
| 16 | **Supabase account (free tier)** + the Claude Supabase MCP connector | Cloud account/save sync backend, driven through Claude's MCP tools | supabase.com; connector via claude.ai Settings → Connectors | 10 |
| 17 | **Aura AI for Unreal account** (trial first → paid tier after the pilot measures credit-per-verification) | Play-In-Editor verification for the `playtest-verifier` agent (input simulation, live actor/UMG state, screenshots). No API key — it authenticates by account login. ⚠️ Decide the training toggle in its privacy settings BEFORE the first index (unlimited Auto Mode requires training ON) | tryaura.dev → account → dashboard installer for **5.8** | 11 |

### ⚠️ The antivirus reality (read BEFORE installing anything network-facing)

If your AV intercepts TLS (Norton-class HTTPS "protection" does — it MITMs every HTTPS
connection with its own certificate), several lanes break with opaque certificate errors.
On the original machine the fix was **HTTPS-exclusion entries in the AV settings** for:

- `huggingface.co` (token auth + model/space API)
- `*.hf.space` (the TRELLIS.2 Space endpoints)
- `*.supabase.co` (the game's cloud client + auth REST calls)

Two related facts proven on the original machine:

- **winget's `msstore` source fails certificate checks** under TLS interception — always
  pass `--source winget` to winget installs.
- `UV_SYSTEM_CERTS` (a user environment variable) makes `uv` trust the system certificate
  store, which is the fallback that works under interception; `SSL_CERT_FILE` pointing at a
  combined CA bundle is the last-resort fallback for other tools. Neither variable holds a
  secret — they are switches/paths, set them as **User environment variables**.

⚠️ **User environment variables live in the Windows registry under your user hive (HKCU) — a
Windows rollback/reset wipes them.** After any system restore, re-check `HF_TOKEN`,
`MESHY_TOKEN`, and `UV_SYSTEM_CERTS` before blaming the pipeline.

---

## Chapter 1 — Git, GitHub, Git LFS, and the repo layout

### 1.1 Install and initialize

1. **[You]** Install Git (git-scm.com) and Git LFS (git-lfs.com).
2. **[You]** Run once per machine:
   ```powershell
   git lfs install
   ```
3. **[You]** Create the folder structure. This project uses a **two-level layout** — the git
   root is a wrapper folder ABOVE the Unreal project folder:

   ```
   <git-root>/                      ← git repo root (e.g. GitClaudeUnrealTesting/)
     .gitattributes                 ← LFS patterns (root — covers everything)
     .gitignore                     ← minimal root ignore (engine junk + testvideo/)
     <ProjectName>/                 ← the Unreal project (e.g. GitClaudeUnrealTest/)
       <ProjectName>.uproject
       .gitignore                   ← the full project-level ignore (the load-bearing one)
       CLAUDE.md                    ← orchestrator law (Chapter 3)
       .claude/agents/              ← the six agent definitions (Chapter 3)
       .claude/pipeline/            ← TASKBOARD / CONVENTIONS / handoffs / qa / SLACK
       Config/  Content/  Source/  Docs/  Tools/
     testvideo/                     ← gameplay clips (gitignored — Chapter 8)
     packagedZIPofGame/             ← cooked builds (gitignored)
   ```

   Why the wrapper level exists: it gives gitignored working areas (`testvideo/`, packaged
   builds, scratch folders) a home that is inside the repo folder but outside the Unreal
   project, so the editor and cooker never scan them.

4. Initialize and connect:
   ```powershell
   cd <git-root>
   git init
   git remote add origin https://github.com/<you>/<your-repo>.git
   ```

### 1.2 `.gitattributes` (git root) — what LFS tracks

This is the actual root `.gitattributes` of the worked example — copy it verbatim:

```
*.uasset filter=lfs diff=lfs merge=lfs -text
*.umap filter=lfs diff=lfs merge=lfs -text
*.fbx filter=lfs diff=lfs merge=lfs -text
*.png filter=lfs diff=lfs merge=lfs -text
*.jpg filter=lfs diff=lfs merge=lfs -text
*.wav filter=lfs diff=lfs merge=lfs -text
*.mp4 filter=lfs diff=lfs merge=lfs -text
*.dll filter=lfs diff=lfs merge=lfs -text
*.lib filter=lfs diff=lfs merge=lfs -text
```

### 1.3 The two `.gitignore` files — the load-bearing rules

**Root `.gitignore`** (small): `Binaries/`, `DerivedDataCache/`, `Intermediate/`, `Saved/`,
`Build/`, `.vs/`, `*.sln`, `*.suo` — plus **`testvideo/`**. That last line is load-bearing:
`*.mp4` is an LFS pattern above, so WITHOUT the ignore a careless `git add` pushes entire
gameplay videos into LFS. Raw videos never enter git; only promoted evidence PNGs do.

**Project-level `.gitignore`** (inside the Unreal project folder) — the rules that matter,
learned the hard way on the original machine:

- **UE generated dirs:** `Binaries/`, `DerivedDataCache/`, `Intermediate/`, `Saved/`,
  `Plugins/*/Binaries/`, `Plugins/*/Intermediate/`, plus IDE junk (`.vs/`, `*.sln`, ...).
- **⛔ LLM model weights are NEVER versioned, in ANY form:** `*.gguf` and the whole
  `/Models/` directory. A weights file is ~2.5 GB and git history is append-only — one
  accidental commit is permanent for every clone. **LFS is NOT the answer** for large
  regenerable artifacts (weights, cooked builds, caches): they should not be versioned at
  all; a fetch script + a pinned version note make them reproducible instead.
  ⚠️ Use the directory form `/Models/` and never add a `!` negation inside it — git does not
  descend into an ignored directory, so negations inside one silently do nothing.
- **Packaged builds:** `/Build/`, `/Builds/`, `/Packaged/`, and the packaging staging folder
  (here `/packagedZIPofGame/` at the git root) — gitignored BEFORE the first cook ever runs.
- **Machine-local Claude settings:** `.claude/settings.local.json` (accumulated permission
  grants — personal to each machine, never committed).
- **The art-pipeline working dirs:** `Tools/ArtPipeline/.venv/` (recreated by `uv sync`),
  `Tools/ArtPipeline/Inbox/*` and `Tools/ArtPipeline/Cache/*` (with `!.gitkeep` negations —
  these use the `dir/*` form precisely so the `.gitkeep` negation works).
- **Real cloud config:** `Config/SiegeCloudDev.ini` (Chapter 10). The committed file is the
  `.example` template only.
- **Misc:** `__pycache__/`, `*.fbm/` (UE writes FBX texture-extraction dirs beside source
  FBX files), `*_BuiltData.uasset`.

A good starting point is the canonical GitHub `UnrealEngine.gitignore`, then add the
project-specific rules above. Note `.gitignore` only affects UNTRACKED files — anything
already committed stays tracked until `git rm --cached`.

### 1.4 Verify

```powershell
git lfs status                 # LFS installed and active in this repo
git lfs ls-files | Select-Object -First 5   # after first commits: assets listed here, not raw blobs
git check-ignore -v Models/anything.gguf    # must report the /Models/ rule
```

### 1.5 House git law (adopt these as rules, they prevent real incidents)

- **Nothing is committed without a PASS QA report** (code) or completed integration check (art).
- **Never push to remote unless the human explicitly asks.** The human may also self-commit
  and push milestones — agents verify `git HEAD`/origin before dispatching a commit so they
  never duplicate or amend his work.
- On each finished milestone, cut a `<milestone>-testable` branch as the playtest fallback;
  develop the next milestone on main.

---

## Chapter 2 — UE 5.8, the C++ project, and the build command

### 2.1 Create the project

1. **[You]** Install Epic Games Launcher → install **Unreal Engine 5.8**.
2. **[You]** Install Visual Studio 2022 with the **Game development with C++** workload.
3. **[You]** Launcher → UE 5.8 → new project → **Games → Third Person (C++)** (or Blank C++),
   created INSIDE the git wrapper folder from Chapter 1. Close the editor before the first
   commit (avoids file locks).
4. Commit the fresh project (the `.gitignore` from 1.3 must be in place first).

### 2.2 The build command — and the two gotchas that will burn you

The canonical compile (adjust names/paths; keep it in your CLAUDE.md so every agent uses
the same line):

```
"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" <ProjectName>Editor Win64 Development -project="<abs-path>/<ProjectName>.uproject" -waitmutex
```

**⛔ Gotcha 1 — the exit-code lie.** `Build.bat` can return **exit code 0 on a FAILED
build** (observed live when Live Coding held the build mutex). Never trust `$LASTEXITCODE`.
**Parse the build output for the line beginning `Result:`** — `Result: Succeeded` or
`Result: Failed` is the only truth. Make this a written law for your build agent.

**⛔ Gotcha 2 — Windows Smart App Control.** If SAC is ON and enforced, every UE C++ build
fails in ~2 seconds with error `0x800711C7` (SAC blocks UnrealBuildTool's unsigned,
just-compiled `ModuleRules.dll`). This is NOT a code error — do not let a QA loop chase it.
Fix (human-only): Windows Security → App & browser control → Smart App Control → **Off**.
(Windows only lets you turn SAC off once — it can't be re-enabled without reinstalling.)

Also useful: with the editor open, prefer letting the editor close before heavy builds
("editor-bounce" compiles) — Live Coding's mutex is the usual cause of weird build states;
`-waitmutex` in the canonical line makes the build wait instead of failing.

### 2.3 Verify

Run the build command; confirm the output contains `Result: Succeeded`. Open the project
once in the editor and confirm it loads.

---

## Chapter 3 — The Claude agent team

This is the heart of the system. One Claude Code session is the **orchestrator**; six
specialist **subagents** do the actual work; they communicate only through **shared files**.
A fresh Claude can re-scaffold all of it from this chapter.

### 3.1 The three layers

1. **Orchestrator** — the session you talk to. It NEVER does specialist work; it routes
   tasks between agents and enforces the pipeline. Its law lives in **`CLAUDE.md`** at the
   project root (loaded automatically when Claude Code starts there).
2. **Specialist agents** — one markdown file each in **`.claude/agents/<name>.md`**, with
   YAML frontmatter (`name`, `description`, optional `tools` allowlist) + a body of role
   instructions. The `description` is what the orchestrator matches when routing — write it
   like a job posting, not a bio. **Least privilege:** the `tools:` line grants each agent
   only what its job needs (QA gets no Edit; the analyst gets no engine MCP; etc.).
3. **Tool layer** — MCP servers (Unreal, Blender, Slack, Supabase) + Bash/file tools.

### 3.2 The roster (copy these roles; the files are in `.claude/agents/`)

| Agent | Does | Never does | Tools posture |
|-------|------|-----------|----------------|
| **manager** | Splits requests/GDD into tasks on the board; owns naming conventions; the only agent that posts top-level in Slack | Code, art, engine access | Read/Write/Edit/Grep/Glob + Slack |
| **gameplay-programmer** | UE C++ / Blueprint / editor-Python logic via files + Unreal MCP | Art prompts, compiling, Git | Full file tools + Unreal MCP |
| **art-director** | Models/textures in Blender MCP + the generation pipelines; imports to Content/ with exact specced names; UMG layout | Gameplay code, integration, Git | Blender MCP + Unreal MCP + files |
| **qa-reviewer** | Reviews code BEFORE it compiles; writes pass/fail reports to `qa/` | Editing code, engine, Git | Read-only + report Write + Slack |
| **build-master** | Compiles, assembles assets+code in scene via Unreal MCP, Git commits | Writing new code/art | Bash (Build.bat, git) + Unreal MCP |
| **footage-analyst** | Extracts frames from gameplay videos; writes evidence-backed diagnosis reports | Editing anything, engine, Git | Bash/Read/Write + Slack (no MCP) |

### 3.3 The pipeline files (how agents "talk")

Subagents cannot talk to each other. They communicate through files in
**`.claude/pipeline/`**, with the orchestrator relaying:

- **`TASKBOARD.md`** — the hub. Every task: unique `TASK-###` id, one assignee, status,
  `blocked-by:` dependencies, `parallel-safe: yes/no`, a spec small enough for one session,
  and a `names:` block with the EXACT class/asset names to use.
- **`CONVENTIONS.md`** — the naming law, owned by the manager. This is what guarantees the
  artist's asset name matches the programmer's code reference character-for-character. It
  grows over time into the project's case law: each feature batch adds a dated section with
  its own namespace (e.g. `FR-§` for footage review) that task specs cite instead of restating.
- **`handoffs/TASK-###-<role>.md`** — per-task completion notes passed downstream.
- **`qa/TASK-###.md`** — QA reports: `Verdict: PASS|FAIL` + `[BLOCKER|WARN|NIT]` findings.
- **`SLACK.md`** — the Slack mirror protocol + thread registry (Chapter 9).
- **`fab/FAB-REQUESTS.md`** — the marketplace lane: agents AUTHOR requests for Fab/UE
  marketplace packs; the human fulfills them in the Epic Launcher (agents can't).

**Task lifecycle:** `backlog → in-progress → ready-for-qa → qa-passed/qa-failed →
integrating → done` (art tasks skip QA: `ready-for-integration`).

### 3.4 The routing law (put this in CLAUDE.md verbatim, adapted)

1. **Every feature request goes to `manager` first.** No exceptions — it returns task IDs.
   (One recorded exception: gameplay-video reports dispatch the footage-analyst directly —
   diagnosis is evidence-gathering, not decomposition; the manager boards fixes from the
   finished report.)
2. Dispatch per the board: programmer + artist tasks marked `parallel-safe: yes` with no
   blockers launch **in parallel**.
3. Code task hits `ready-for-qa` → invoke `qa-reviewer`.
4. `qa-failed` → back to the programmer with the QA report path. Loop until `qa-passed`
   (**max 3 loops, then escalate to the human**).
5. `qa-passed` / `ready-for-integration` → `build-master` compiles, assembles, commits.
6. Build failure → build-master appends errors to the QA report; routed back to the
   programmer (counts as a QA loop).
7. Report the outcome to the human with task IDs and commit hashes.

**Hard gates:** nothing commits without a PASS QA report; the Unreal Editor + MCP server
must actually be running for engine tasks — if unreachable, agents REPORT it, never fake
results; never push unless asked.

### 3.5 GDD mode (full automation from a design doc)

Write the game's master plan in `Docs/GDD.md`, then say **"Read the GDD and build it."**
The manager splits it into **milestones** (playable increments, ~5–15 tasks each) recorded
at the top of TASKBOARD.md, fully decomposes ONLY the first incomplete milestone, and the
pipeline runs it hands-off. At milestone completion the orchestrator **stops and reports**
(commits, next milestone, open questions) — that's the playtest checkpoint; your feedback
becomes new tasks in the current milestone before it's considered complete. A fresh session
resumes from the board ("Continue the task board") — never re-decompose unless the GDD changed.

GDD writing rule that matters most: **numbers over adjectives** ("sprint = 750 units/s"
gets implemented; "fast sprint" gets guessed) plus per-mechanic acceptance criteria (QA
tests against them) and an explicit out-of-scope list (nobody gold-plates).

### 3.6 Scaffolding it on the new machine

**[Claude]** Given this chapter, a fresh Claude Code session can recreate the skeleton:
write `CLAUDE.md` (team table + routing law + hard gates + the build command), the six
`.claude/agents/*.md` files (frontmatter + role body per 3.2's postures), and empty
`.claude/pipeline/` files (TASKBOARD with a task template, CONVENTIONS seeded with your
naming table, empty handoffs/ and qa/ dirs, SLACK.md once Chapter 9 is done).

**First-run expectation [You]:** the first session prompts for permission on builds, git,
and MCP calls — choose "don't ask again" and grants accumulate in the gitignored
`.claude/settings.local.json`. After that, runs are mostly hands-off between checkpoints.

### 3.7 Adapting the roster to other game genres

The five-role core (manager / builder / maker-of-content / reviewer / integrator) is
genre-agnostic; swap the specialist skins:

- **Narrative game (visual novel, RPG):** add a **narrative-designer** (owns story bible,
  branching outlines) and a **dialogue-writer** (writes lines/barks to the designer's specs;
  never touches story structure). QA grows a **continuity-checker** duty (names, facts,
  flags). The art-director shrinks toward UI + portraits; Blender may drop out entirely.
- **2D game (puzzle/platformer):** replace the Blender lane with a **sprite/pixel-artist**
  agent (image-generation pipeline + atlas packing instead of TRELLIS/Meshy; import to
  Paper2D or your sprite system). The build-master's "assemble in scene" step becomes
  tilemap/prefab assembly.
- **Multiplayer-heavy game:** add a **netcode-qa** agent whose review checklist is
  replication-specific (authority checks, RPC validation, rollback safety, bandwidth
  budgets) and gate every gameplay diff on a "replication declaration" (what replicates,
  who owns it). Consider a **load-test runner** agent that drives headless clients.
- **Systems-heavy sim:** add a **balance-analyst** that owns spreadsheets/CSV data files
  and simulates economy curves before the programmer implements them.

Keep the invariants regardless of genre: one task = one owner = one deliverable; naming law
in CONVENTIONS before the task is issued; QA is read-only; only the integrator touches git.

---

## Chapter 4 — Unreal MCP (Claude's hands inside the editor)

UE 5.8 ships an experimental built-in **MCP plugin** that embeds an MCP server in the
editor, letting Claude place actors, edit Blueprints, drive lighting, and run editor Python.

### 4.1 Enable the plugins (recorded from the original setup — verify names in your 5.8 build)

**[You]** Edit → Plugins, enable all three, restart once:

1. **Unreal MCP** (internal identifier `ModelContextProtocol`) — the core server.
2. **Editor Toolset** — auto-enabled as a dependency, but verify: it's what actually exposes
   editor tools to Claude; without it the server runs with nothing to offer.
3. **Terminal** *(optional)* — an in-editor terminal panel that can auto-launch Claude Code;
   handy for one-offs, but prefer an EXTERNAL terminal for long milestone runs (the
   in-editor panel dies if the editor crashes mid-build; an external session survives and
   resumes from the task board).

### 4.2 Configure auto-start + generate the client config

1. **[You]** Edit → Editor Preferences → General → **Model Context Protocol** → enable
   **Auto Start Server**. Default endpoint: **`http://127.0.0.1:8000/mcp`**.
2. **[You]** In the editor console (backtick key):
   ```
   ModelContextProtocol.GenerateClientConfig ClaudeCode
   ```
   This writes `.mcp.json` into the project root pointing at the running server:
   ```json
   {
     "mcpServers": {
       "unreal-mcp": { "type": "http", "url": "http://127.0.0.1:8000/mcp" }
     }
   }
   ```
   (`GenerateClientConfig All` also emits configs for other clients.)
3. If Claude connects but sees no tools: Edit → Project Settings → MCP → enable all toolset
   registries, restart the editor.

### 4.3 The remote-execution block (add this — it saves unattended sessions)

Add to `Config/DefaultEngine.ini`:

```ini
; Python remote execution: lets the agent pipeline issue a graceful in-editor
; quit_editor() (and other editor-Python) over the local multicast lane, so an
; unattended editor never wedges on a Save Content modal. Editor-only plugin
; setting — no effect on packaged builds.
[/Script/PythonScriptPlugin.PythonScriptPluginSettings]
bRemoteExecution=True
```

Why: MCP alone cannot always close a modal-blocked editor. With remote execution on, a
script can ask the editor to quit gracefully instead of someone force-killing it (which
risks asset corruption). House rule worth adopting: **when the human is present, closing
the editor is HIS choice** — agents never force-kill or drive an unprompted close, because
the close dialog is where he picks which dirty assets to save.

### 4.4 What MCP can't do — the headless commandlet lane

Two measured MCP limits on the original machine, and their workarounds:

- **Same-path asset overwrite:** the MCP static-mesh import tool cannot overwrite an
  existing `SM_<Name>` at the same content path (returns "already exists"). The fix is a
  **headless Python commandlet**: `UnrealEditor-Cmd.exe <project> -run=pythonscript
  -script=<script>` using `unreal.AssetImportTask(replace_existing=True, automated=True)` —
  an in-place reimport that preserves the UObject identity so every hard and soft reference
  survives. Worked examples in this repo: `Tools/reimport_meshes.py` (+ its known
  limitation: mesh slot→material-instance pointers don't persist from the commandlet — they
  are finalized over MCP in the relaunched editor via `Tools/reimport_apply_materials_mcp.py`).
- **No input injection:** MCP has no lane to simulate player input, so "PIE gameplay
  matrix" claims can't be made by agents — human playtests (and the footage lane, Ch. 8)
  cover that. Agents must declare honestly what their verification lane could and couldn't see.

Also learned the hard way: **duplicating + reparenting a WidgetBlueprint can silently break
runtime repaint** while looking fine at design time — rebuild widgets fresh instead, and
prefer pixel/human checks for UI-render bugs.

### 4.5 Verify

With the editor open and the server auto-started, run `claude` in the project root and ask:
*"What actors do I have selected?"* — an answer with real editor context means the
connection works. The hard gate stands: if `http://127.0.0.1:8000/mcp` is unreachable,
engine tasks STOP and report; nothing is faked.

---

## Chapter 5 — Blender MCP (the art-director's modeling hands)

### 5.1 The one correct route (two known-wrong routes explicitly warned)

The working setup is: **Blender 5.1+'s official built-in "MCP" add-on** (a TCP server on
`localhost:9876`) + a **repo-owned stdio bridge script** that Claude Code launches.

- ⛔ **NOT `uvx blender-mcp`** (the third-party PyPI package) — it speaks a different,
  incompatible protocol than the official add-on.
- ⛔ **NOT the add-on's own bundled `mcp_bridge.py`** — it speaks ONLY Content-Length (LSP)
  framing over stdio. Claude Desktop happens to use that framing, but Claude Code uses
  newline-delimited JSON-RPC, so the bundled bridge never answers `initialize` and Claude
  Code hangs to a 30 s connection timeout.
- ✅ The repo bridge (`Tools/blender_mcp_bridge.py`, ~stdlib-only, copy it to the new repo)
  **auto-detects the client's framing** on the first message and replies in kind, so it
  works with both Claude Code and Claude Desktop. It forwards to the add-on's socket on
  port 9876 using the add-on's own null-byte-delimited JSON protocol.

### 5.2 Setup

1. **[You]** Install Blender 5.1+ (the Lab line that bundles the MCP add-on).
2. **[You]** Edit → Preferences → Add-ons → enable **MCP**.
3. **[You]** Edit → Preferences → System → enable **Online Access** — the add-on refuses to
   start its socket server without it. The server then auto-starts on `localhost:9876`
   about a second after launch (verify under Preferences → Add-ons → MCP: "Server is running").
4. Copy `Tools/blender_mcp_bridge.py` from this repo into the new repo's `Tools/`.
5. Add the `blender` server to `.mcp.json` alongside `unreal-mcp`:
   ```json
   "blender": {
     "command": "C:/Program Files/Blender Foundation/Blender 5.1/5.1/python/bin/python.exe",
     "args": ["-u", "-X", "utf8", "<abs-path-to-repo>/Tools/blender_mcp_bridge.py"],
     "env": { "PYTHONUTF8": "1", "PYTHONIOENCODING": "utf-8" }
   }
   ```
   (Any Python 3.10+ works for the bridge — Blender's bundled interpreter just matches the
   add-on's recommended setup.) **Fully restart Claude Code after editing `.mcp.json`** —
   it may not hot-reload.

### 5.3 Operating rules (proven on the original machine)

- The add-on exposes exactly **three tools**: `execute_blender_code` (arbitrary `bpy`
  Python — the workhorse; to read anything back the code must assign a JSON-serializable
  dict to a variable named `result`), `get_scene_info`, and `get_object_info`. Everything —
  meshes, modifiers, materials, exports — is written as `bpy` code.
- **The live bridge has a ~30 s socket cap** — it is for quick inspection/preview only.
  Heavy work (remesh, decimate, bakes) runs **headless**:
  `blender.exe --background --python <script>` (that's how the Chapter 6 refine stage runs).
- Blender must be OPEN with the server up for art tasks; if it isn't, the art-director
  reports the outage — never fakes asset creation.
- Run only ONE Claude client against Blender at a time (Desktop or Code) — the add-on's
  socket is single-threaded and two clients on 9876 contend.

### 5.4 Verify

With Blender open, ask Claude to call `get_scene_info` — a scene summary (default cube et
al.) proves the whole chain: Claude → bridge → port 9876 → Blender.

---

## Chapter 6 — The TRELLIS.2 art pipeline (concept image → game-ready mesh)

Lives in **`Tools/ArtPipeline/`** — copy the whole folder (scripts + `pyproject.toml` +
`uv.lock` + `.python-version` + manifests) to the new repo. Full flow:

```
Inbox/<AssetName>.png                 (human drops the concept image)
   │  Stage 1  trellis_generate.py    (HF Space microsoft/TRELLIS.2, gradio client)
   ▼
Cache/<AssetName>/trellis_raw.glb     (+ state.json provenance + api_schema.json)
   │  Stage 2  refine_trellis_glb.py  (HEADLESS Blender: remesh → UV → bake D/N/ORM → FBX)
   ▼
Content/RawAssets/<AssetName>.fbx  +  RawAssets/Textures/<AssetName>/*.png
   │  Stage 3  Unreal import          (art-director via MCP / the Ch. 4.4 commandlet)
   ▼
/Game/Meshes/SM_<AssetName>           (same-path overwrite — references survive)
```

### 6.1 One-time setup

1. **[You]** Install **uv** (astral.sh/uv; a winget package exists — verify the id on the
   new machine). If your AV intercepts TLS, set the `UV_SYSTEM_CERTS` user env var (see THE LIST).
2. **[Claude]** From `Tools/ArtPipeline/`:
   ```powershell
   uv sync
   ```
   uv downloads its own managed **Python 3.12** (pinned in `.python-version` — the gradio
   client is not validated on newer Pythons; the system Python is never touched) and
   installs the locked deps into `.venv/` (gitignored, disposable — `uv sync` recreates it).
3. **[You]** Mint an HF access token: huggingface.co → Settings → Tokens (read scope is
   enough). Store it ONLY as the **`HF_TOKEN` user environment variable** (Windows Settings
   → System → About → Advanced system settings → Environment Variables → User → New). The
   GUI route keeps it off command lines and shell history. Open a NEW terminal afterward —
   running shells don't see new variables.

**The token law (binding):** `HF_TOKEN` is **ENV-ONLY** — never in a file, never on argv,
never in chat/logs/handoffs. The scripts read it from the environment, redact it from all
output, and exit with code 2 (printing the setup steps) if it's unset. Agents only ever
check that the variable EXISTS.

### 6.2 Stage commands + probes

```powershell
uv run trellis_generate.py --check          # tokenless smoke test — no GPU, no quota
uv run trellis_generate.py <AssetName>      # reads Inbox/<AssetName>.png
uv run trellis_generate.py <AssetName> --seed 7   # reroll a bad generation
```

Exit codes are a contract the orchestration reads: `0` success · `1` failure · `2` token
unset · `3` quota exhausted (an EXPECTED PAUSE — the message contains the reset time;
record it and stop, don't retry) · `4` Space API drift · `5` concept image missing · `64`
usage. Free ZeroGPU quota ≈ 5 GPU-min/day ≈ 1–2 assets; HF PRO (~$9/mo) ≈ 40 GPU-min/day.

Stage 2 (headless Blender, per `pipeline_manifest.json` budgets):

```powershell
& "C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --python refine_trellis_glb.py -- --asset <AssetName>
```

**Pre-import gate (mandatory):** read `Cache/<AssetName>/refine_report.json` (tri count vs
budget, bounds, UV layer, PNG inventory) AND eyeball the preview renders before anything
enters the editor — nothing is imported unseen.

Stage 3 import law: textures as `T_<Name>_D` (sRGB) / `_N` (normal) / `_ORM` (linear, sRGB
off); a material instance from a shared master PBR material; the FBX imported OVERWRITING
the existing `/Game/Meshes/SM_<Name>` at the same path (never delete+recreate); Nanite per
your project's law.

### 6.3 Concept image guidance (for the Inbox drops)

One single subject · plain/transparent background · ¾ view · no ground shadow · subject
fills the frame · ~1024×1024 PNG · neutral even lighting · named exactly `<AssetName>.png`.

### 6.4 Fallbacks

- **Space API drift (exit 4):** the `api_schema.json` snapshot shows what the Space exposes
  now — file it with the QA report; use the browser fallback meanwhile.
- **Manual browser fallback:** open the TRELLIS.2 Space in a browser (logged in), generate,
  download the GLB, drop it at `Cache/<AssetName>/trellis_raw.glb` — Stage 2 resumes from
  there (note "manual browser run" provenance in the handoff).
- **Bad generation:** reroll Stage 1 with another `--seed` before touching Stage 2.

### 6.5 Verify

`uv run trellis_generate.py --check` passes tokenless; after setting `HF_TOKEN`, a real
run on a test concept produces `Cache/<AssetName>/trellis_raw.glb`.

---

## Chapter 7 — The Meshy pipeline (second generation engine)

Meshy is the paid second engine — used to retexture a TRELLIS mesh from a style reference,
or as an alternative image→3D route. Lives beside TRELLIS in
`Tools/ArtPipeline/meshy_generate.py`:

```powershell
uv run meshy_generate.py --check                    # probe: key present + API reachable
uv run meshy_generate.py --mode image3d <AssetName>    # Inbox/<AssetName>.png → Cache/<AssetName>/meshy_raw.glb
uv run meshy_generate.py --mode retexture <AssetName>  # donor GLB + style PNG → Cache/<AssetName>/meshy_retex.glb
```

Setup + laws:

1. **[You]** Create a Meshy account (paid — generations consume credits), mint an API key
   in the Meshy dashboard, and store it ONLY as a user environment variable. The canonical
   variable name in this repo is **`MESHY_TOKEN`** (the scripts also accept **`MESHY_API_KEY`**
   as a documented alias, and on Windows they fall back to reading the user-variable
   registry hive so shells opened before the variable was set still work).
2. Same secret law as `HF_TOKEN`: env-only, never on argv, never in any file, all script
   output passes through a redactor. Exit codes mirror the TRELLIS map (`2` = key unset,
   `3` = credits exhausted — an expected pause, surfaced verbatim, never retried).
3. **TLS is never disabled.** If your AV MITMs the Meshy API domain, surface the certificate
   failure verbatim and add an AV exclusion (or use the `SSL_CERT_FILE` combined-bundle
   fallback) — never a verification bypass.
4. **The invariant:** Meshy output NEVER lands in `Content/` directly — everything re-enters
   through Stage 2 (the refine script), so the Stage-2/3 laws hold regardless of engine.

**Verify:** `uv run meshy_generate.py --check` exits 0.

---

## Chapter 8 — The video-review pipeline (playtest footage → diagnosed fixes)

The human records gameplay, describes what went wrong in the terminal, and the
**footage-analyst** agent turns the clip into timestamped pixel evidence + a diagnosis
report the manager boards fixes from. (Claude can't ingest video — the lane is ffmpeg
frame extraction → batched image reads.)

### 8.1 Setup

1. **[You]** Install ffmpeg:
   ```powershell
   winget install --id Gyan.FFmpeg -e --source winget
   ```
   (open a fresh terminal after — or let the tool self-locate via the WinGet Links dir).
2. **[You]** System Python 3.14 from python.org, plus Pillow (`py -m pip install pillow`).
   **Law: this lane runs on the SYSTEM Python, stdlib + ffmpeg only — no venv, no numpy;**
   Pillow is optional (labeled contact sheets and the crop command degrade without it).
3. Create `<git-root>/testvideo/` and make sure the root `.gitignore` ignores it (§1.3 —
   load-bearing: raw videos must never reach LFS).
4. Copy `Tools/VideoReview/extract_frames.py` into the new repo, and the footage-analyst
   agent definition into `.claude/agents/`.
5. **[You]** Record clips with Xbox Game Bar (Win+Alt+R) — files land wherever Game Bar
   saves them; move them into `testvideo/`. Filenames are accepted VERBATIM (Game Bar names
   carry double spaces and parentheses — every consumer quotes paths / uses list argv, and
   `shell=True` is banned in the tool for exactly this reason).

### 8.2 The flow (the footage-review law, in one paragraph)

Drop the clip in `testvideo/` and describe the issue → the orchestrator dispatches the
footage-analyst directly with your verbatim words + the next `VID-###` id (the recorded
exception to manager-first routing — diagnosis is evidence-gathering, not decomposition).
The analyst runs `--check` then `probe`, does a **coarse pass** (timestamp-labeled contact
sheets, 4x3 grid) to find candidate moments, then a **fine pass** (`frames --at t1,t2`,
`crop --scale 2` before ANY claim about UI text), within a budget of ~40 image reads.
It writes `.claude/pipeline/footage/VID-###-<symptom>.md` where every claim carries a frame
reference — observations, never conclusions ("the fill is visibly unchanged at 01:32.5",
not "the delegate isn't firing"); mechanism lines are labeled **hypothesis, not verdict**.
The few frames that PROVE findings are `promote`d into
`.claude/pipeline/playtest-evidence/<date>/` — the ONLY part of this lane that enters git.
The manager then boards every fix from the report through the normal pipeline. The analyst
is diagnose-only: it never edits code, never opens the editor, never runs git.

Tool subcommands: `--check` · `probe` · `sheets` · `frames --at t1,t2 [--run N]` ·
`crop <png> --box x,y,w,h [--scale N]` · `promote <png> --id VID-### --symptom <slug>`.
Exit codes: `0` ok · `2` no ffmpeg (message names the winget command) · `3` video
missing/undecodable · `4` Pillow needed · `5` bad timestamp · `64` usage. The frame cache
(`testvideo/.frames/`) is disposable and gitignored with its parent.

### 8.3 Verify

```powershell
python Tools/VideoReview/extract_frames.py --check
```

then drop any short clip in `testvideo/` and run `probe` — duration/resolution/fps come back.

---

## Chapter 9 — The Slack mirror (team visibility channel)

One Slack channel mirrors the pipeline so the human can watch progress and drop feedback
from anywhere. **Files stay the contract — Slack is visibility only, and a Slack post is
never authorization.**

### 9.1 Setup

1. **[You]** Create (or reuse) a Slack workspace; create a team channel (this project's is
   `#siegeboundue5agentteam`).
2. **[You]** Connect Claude to Slack: claude.ai → Settings → Connectors → add the Slack
   connector and authorize it into your workspace (exact steps per Anthropic's current
   connector docs — verify on the new machine). The Slack MCP tools then appear to Claude
   Code sessions signed into your account.
3. **[Claude]** Record the channel ID and write `.claude/pipeline/SLACK.md` — the protocol
   file (copy this repo's as the template). Create the standing threads (below) and record
   each thread's `thread_ts` in SLACK.md's registry: **a thread is not live until its ts is
   recorded.**
4. Grant the Slack tools in each agent definition's `tools:` line (see 3.2). If an agent's
   connector tools don't surface in a given run (headless), the orchestrator **proxies the
   agent's post verbatim** — proxying is always the lossless fallback; the communication is
   never skipped.

### 9.2 The protocol (the parts that make it work)

- **Main-chat law: top-level posts are the manager + the human ONLY.** Everything else
  lives in a fixed registry of **standing domain threads** (not per-task threads): Planning
  & Feedback · Dev & QA · Art · Build & Git · Blockers · Footage Review. A task's lifecycle
  plays out inside its domain thread; the `TASK-###` is the join key when a task's life
  spans threads (code → QA → build).
- **Identity prefixes are mandatory** — all posts share one Slack account, so the prefix IS
  the speaker: `📋 MANAGER:` · `🔍 QA:` · `⚙️ GAMEPLAY-PROGRAMMER:` · `🎨 ART-DIRECTOR:` ·
  `🔧 BUILD-MASTER:` · `🎬 FOOTAGE-ANALYST:` · `ORCHESTRATOR:`, followed by a status emoji
  (🟦 dispatched · 🔧 in progress · 🧪 ready-for-qa · ✅ done · ❌ failed · 📦 integrating ·
  🚧 blocked) + the TASK-###.
- **Every dispatch prompt includes the agent's Slack duty:** channel ID, its domain
  thread_ts, and at least one completion-or-blocker post.
- **Slack does not wake agents.** The orchestrator reads the channel at session start and
  every checkpoint/task boundary and routes actionable feedback to the manager. Anything
  urgent goes to the orchestrator directly in Claude Code.
- Escalations get one line in the Blockers thread; details stay in the domain thread.

### 9.3 Verify

Have the orchestrator post a checkpoint line into the Planning & Feedback thread and read
it back with the Slack read tool.

---

## Chapter 10 — Supabase (cloud accounts / save sync)

The game's cloud layer (accounts, deck/settings sync). Everything below was proven live on
the original project; the design rules are the transferable part.

### 10.1 Setup

1. **[You]** Create a Supabase account (free tier is enough to start).
2. **[You]** Connect Claude to Supabase: claude.ai → Settings → Connectors → add the
   Supabase connector under YOUR account (verify current steps on the new machine).
3. **The access surface law:** agents reach Supabase ONLY through the orchestrator-held MCP
   tools under the human's account — no agent definition holds standalone Supabase
   credentials. Every provisioning/schema handoff names the MCP tool that produced each figure.
4. **[Claude]** Create a **NEW dedicated project named for the game** via the MCP
   `create_project` tool (clean ownership story — never reuse an unrelated personal sandbox
   project). Note: the MCP region enum may not offer every region name — record the region
   actually created as the region of record.
5. **[You]** If your AV intercepts TLS: add the `*.supabase.co` exclusion BEFORE any
   client-side HTTPS testing — a certificate failure from the game is the environmental
   issue first, a code bug second.

### 10.2 The key law + the config home (⛔ the part that keeps secrets out of git)

- ⛔ **The `service_role` key NEVER touches the repo, the game binary, any committed file,
  or any handoff/report text.** It exists only in Supabase's own dashboard and the
  MCP surface. Standing QA criterion on every cloud diff: grep for `service_role` and for
  secret-key material — zero hits.
- **The game client uses the publishable/anon key + user JWTs + Row Level Security,
  nothing else.** The anon key is public-by-design (RLS is the security boundary, not key
  secrecy) — but it still has exactly ONE ruled home: the **gitignored**
  `Config/SiegeCloudDev.ini`. A hardcoded anon key in a source file is a QA FAIL.
- **The config home pattern:** commit a placeholder template
  `Config/SiegeCloudDev.ini.example`; gitignore the real `Config/SiegeCloudDev.ini`
  (exact line in `.gitignore`). The real file carries `[SiegeCloud]` with `ProjectUrl=` and
  `AnonKey=` filled from the project dashboard. **Missing/unparsable config ⇒ cloud OFF and
  the game behaves byte-identically to its offline build** — cloud gates nothing, ever.
- ⚠️ **The quoted-URL law (a real shipped bug):** `ProjectUrl` MUST be double-quoted —
  `ProjectUrl="https://<project-ref>.supabase.co"` — because UE's ini parser swallows an
  unquoted `//` as an inline comment, silently truncating the URL to `https:` while the
  config still "validates". Quotes are stripped on read, so quoting costs nothing. This
  project pins the rule with an offline unit test that asserts the unquoted form truncates —
  do the same. (`AnonKey` stays unquoted; its alphabet can't contain `/`.)
- **Token posture:** access tokens live in memory only; a refresh token may persist in the
  local save per-profile — stored plaintext-on-disk, the same trust level as any launcher's
  session file, and no artifact may call it "encrypted". Neither token is ever logged.
  Auth goes over Supabase's GoTrue REST endpoints via UE's own HTTP module — no third-party SDK.

### 10.3 Schema discipline

- ⛔ **No ad-hoc DDL.** Every schema change is a numbered SQL file in
  `Tools/Supabase/migrations/` (`0001_init_accounts.sql`, `0002_...`), QA-reviewed BEFORE it
  is applied, applied via the MCP migration tool by the build-master, with the applied text
  verified identical to the file.
- **RLS on every table**, one policy per operation, each bound to `auth.uid()`; no policy
  ever references `service_role` (it bypasses RLS by definition — naming it in a policy is
  a design smell and a QA FAIL). Server time owns the sync clock via a `before insert or
  update` trigger — the client never writes `updated_at`.
- **Post-apply gate:** the MCP security-advisors run must report zero RLS findings; any
  finding blocks the lane and escalates.
- **Local-first remains the law:** local save files stay the source of truth; the cloud is
  a sync layer. No gameplay or menu flow may block on an HTTP round trip; every cloud
  failure degrades to local behavior with one log line and a visible status message.

### 10.4 Verify

**[Claude]** `list_projects` via the MCP shows the new project; `list_tables` after the
first migration shows the schema; the advisors run is clean. **[You]** copy the `.example`
to the real ini, fill it from the dashboard (Project Settings → API), and confirm
`git status` never shows the real ini.

---

## Chapter 11 — Aura (Play-In-Editor verification for the agent team)

Aura AI for Unreal (tryaura.dev) is an in-editor assistant whose Verification Agent can
launch PIE, simulate input, read live actor and UMG state, and screenshot/record the result —
the one lane Chapter 4's MCP cannot provide ("no input injection", §4.4). It joins the team as
a seventh agent, `playtest-verifier`, behind a new `verified` gate. Everything Aura does is
in ADDITION to the existing setup: `unreal-mcp` on `:8000`, Blender on `9876`, the build
command, and the Git law are all untouched.

**Secrets law, unchanged:** Aura authenticates through its own account login — there is no
API key to store anywhere. Nothing in this chapter is a secret; the paths below are paths.

### 11.1 Account + install (engine level, one install per UE_5.8)

1. **[You]** Create the Aura account (THE LIST row 17). The trial is free and card-less;
   the paid tier is decided AFTER the pilot measures credit-per-verification (Appendix B).
2. **[You]** ⚠️ **Decide the training toggle in Aura's privacy settings BEFORE the first
   index.** Unlimited Auto Mode requires training ON; turning it OFF makes Auto Mode
   rate-limited instead. This decides whether the GDD and the game's source are used for
   training — it is a privacy call, and it is recorded on the board in one word.
3. **[You]** Close the editor. From the Aura dashboard download the Windows installer and
   select engine version **5.8**. It installs into the ENGINE (not the project), under
   `<UE_5.8>/Engine/Plugins/Marketplace/Aura/`, so it is available to every 5.8 project;
   re-run the installer after an engine upgrade.
4. **[You]** Open the project → Edit → Plugins → search **Aura** → Enable → restart once.
   The editor adds an `Aura` entry to the `.uproject` `Plugins` array (measured on the
   original machine: `"Name": "Aura", "Enabled": true` plus a `SupportedTargetPlatforms`
   list). That is a committed file — build-master commits it under a task like any config
   change; no agent commits it on its own.
5. **[You]** Click the **Aura** toolbar button and confirm the green 🟢 indicator, then ask
   *"Tell me about this project"* — the answer must name THIS game's classes, not the
   third-person template. If it cannot connect: `Aura.exe` must be running in the system
   tray, nothing else may hold **local port 41200**, and the AV must not block loopback
   (same class of problem as the TLS exclusions in THE LIST).
6. **[You]** Optional: Editor Preferences → Aura Plugin Settings → uncheck *Open Aura in
   Electron Window* to dock it. For unattended runs prefer the standalone app — crash
   recovery only works there, and it does nothing for a Claude-driven session.

### 11.2 Configure for the project

1. **[You]** Aura Settings → **MCP Configuration → Set up Unreal MCP.** This lets Aura call
   Epic's built-in editor tools alongside its own. It is Aura's OWN client connection to
   Epic's MCP — our `unreal-mcp` block in `.mcp.json` (Chapter 4, `:8000`) is untouched.
2. **[You]** Enable the **Filesystem Sandbox** (experimental, 5.8-only; the toggle lives in
   Aura's settings — verify its exact location on first run). Every Aura asset mutation then
   stages in `Intermediate/Sandboxes/AuraSandbox` for Accept/Reject, which gives `.uasset`
   edits an undo that is not Git. ⚠️ It does **not** cover C++.

### 11.3 The machine-local files under `Saved/.Aura` (regenerate, never commit)

Aura reads two per-project files from `<Project>/Saved/.Aura/`: `INDEX_IGNORE.txt` (what
the semantic index skips — the Fab packs put this project over Aura's ~30,000-file default
cap, so this file must exist BEFORE the first index) and `project_memory.txt` (a short digest
Aura injects every turn: the team table, the asset-prefix and texture-suffix tables, the
build command, and the laws it is most likely to violate — never Live Coding, never write
`Content/` from mesh generation, never run Git).

`Saved/` is gitignored, so both are machine-local and would be lost on a fresh clone. The
canonical, committed copies live in `Docs/`:

| Canonical (committed) | Copied to (gitignored) |
|---|---|
| `Docs/AuraIndexIgnore.txt` | `Saved/.Aura/INDEX_IGNORE.txt` |
| `Docs/AuraProjectMemory.md` | `Saved/.Aura/project_memory.txt` |

**[Claude]** `Tools/aura_sync.ps1` copies both (idempotent). Run it as a **session-start
step** on any machine, and again whenever either canonical changes. Edit the `Docs/` copies
only — a hand edit under `Saved/.Aura` is overwritten by the next sync and is never staged
(nothing under `Saved/` ever is). There is no skills mirror: `.claude/skills/` does not
exist in this project, so that step from Aura's docs is dropped.

### 11.4 The bridge into Claude Code — two stdio MCP servers, one config home

1. **[You]** Aura Settings → **MCP Configuration → Add to Editor → Claude Code**, then
   fully restart Claude Code. Paste the `command` and `args` shown on that settings page
   into the chat — they are paths, not secrets.
2. **What the one-click actually does (measured on the original machine):** Aura's own doc
   says it writes `~/.claude/mcp.json` with a `"servers"` key. It does not. It writes the
   two servers into **`~/.claude.json` under `mcpServers`** — Claude Code's USER scope — and
   `~/.claude/mcp.json` is never created. That works, but it leaves the repo without its
   source of truth, so:
3. **[Claude]** ⭐ **House rule: the two blocks live in the project `.mcp.json`** under
   `mcpServers`, next to `unreal-mcp` and `blender`, with both names added to
   `enabledMcpjsonServers` in `.claude/settings.local.json` (Appendix D). Then the
   user-scope copy in `~/.claude.json` is removed so there is exactly one definition, and
   Claude Code is restarted (it does not hot-reload `.mcp.json`). The blocks as measured:

   ```json
   "unreal_inspector": {
     "type": "stdio",
     "command": "C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Marketplace/Aura/PortablePython/Windows/python.exe",
     "args": ["C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Marketplace/Aura/MCP/unreal_inspector.py"]
   },
   "unreal_editor": {
     "type": "stdio",
     "command": "C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Marketplace/Aura/PortablePython/Windows/python.exe",
     "args": ["C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Marketplace/Aura/MCP/unreal_editor.py"]
   }
   ```

   Use Aura's own `PortablePython` — not the system Python 3.14 and not the art-pipeline uv
   venv; Aura's scripts pin their own dependencies. `unreal_inspector` is **read-only**;
   `unreal_editor` is the **mutating** server (it bundles PIE/verify/screenshot tools with
   C++ authoring, Live Coding compile, and shell tools).
4. **[Claude]** `/mcp` in the restarted session must list `unreal_inspector`,
   `unreal_editor`, `unreal-mcp`, and `blender` as connected. Record every tool name the two
   Aura servers expose — the next section and the verifier's `tools:` line are built from
   that census, never from a guess.

### 11.5 The allow-list law (Appendix D — merge, never replace)

- `mcp__unreal_inspector__*` may be allowed **wholesale** — it is read-only.
- `unreal_editor` tools are **enumerated by their real names** from the `/mcp` census: only
  the PIE / verify / screenshot tools, in `permissions.allow` AND in any agent `tools:` line.
- ⛔ **Never `mcp__unreal_editor__*`.** A wildcard there hands every agent C++ authoring,
  Live Coding compile, and a shell — undoing the compile-belongs-to-build-master law and the
  no-Live-Coding law in one line.

### 11.6 The seventh agent — `playtest-verifier` and the `verified` gate

`playtest-verifier` (`.claude/agents/playtest-verifier.md`) runs Aura's PIE verification
against a task's acceptance lines AFTER build-master has compiled and relaunched the editor
on the new binaries (PIE can only test binaries that exist, so it runs after QA, never
before), one verification at a time, never concurrent with an assemble or an import, and
announced first whenever the human is present because it takes over PIE. Every claim in its
report (`.claude/pipeline/qa/TASK-###-verify.md`) carries a screenshot/video path or a
quoted actor/widget value; the verdict is `VERIFIED`, `VERIFY-FAILED` (routes back to the
programmer and counts as a QA loop), or `UNOBSERVABLE` (the honest answer for pure-data or
editor-only tasks — recorded on the row, never treated as a pass). The hard gate: nothing
with a runtime acceptance criterion is committed without a `VERIFIED` report. Aura output
still passes `qa-reviewer`; the verifier is advisory until three of its verdicts match the
human's own playtest, and the human decides when it becomes binding.

### 11.7 Verify

Appendix A rows 12–16. In one line: 🟢 + Siegebound classes named → `/mcp` shows all four
servers → `Tools/aura_sync.ps1` leaves `git status` clean → the verifier returns `VERIFIED`
on a known-good task AND `VERIFY-FAILED` on a deliberately broken one (a verifier that
cannot fail is not a gate) → a Sandbox Reject leaves `Content/` unchanged.

---

## Appendix A — The full verification checklist (run top to bottom on the new machine)

| # | Probe | Expect |
|---|-------|--------|
| 1 | `git lfs status` / `git lfs ls-files \| Select-Object -First 5` | LFS active; binary assets listed |
| 2 | `git check-ignore -v Models/x.gguf` and `git check-ignore -v testvideo/x.mp4` | both ignored |
| 3 | Build.bat run → output contains `Result: Succeeded` | compile works (never trust the exit code) |
| 4 | Editor open → `claude` → "What actors do I have selected?" | Unreal MCP answers with editor context |
| 5 | Blender open → ask Claude for `get_scene_info` | scene summary returns via the bridge on 9876 |
| 6 | `Tools/ArtPipeline>` `uv sync` then `uv run trellis_generate.py --check` | exit 0, tokenless |
| 7 | `uv run meshy_generate.py --check` (after setting the key variable) | exit 0 |
| 8 | `python Tools/VideoReview/extract_frames.py --check` | ffmpeg + Pillow found |
| 9 | Orchestrator posts + reads back a line in the Slack planning thread | connector live, registry recorded |
| 10 | Supabase MCP `list_projects` | the game's project visible |
| 11 | Ask Claude: "Show me the task board status" | the agent team scaffold answers from TASKBOARD.md |
| 12 | Aura toolbar → 🟢 → "Tell me about this project" | Project summary that names `Siegebound` classes, not the template |
| 13 | Claude Code `/mcp` | `unreal_inspector`, `unreal_editor`, `unreal-mcp`, `blender` all connected |
| 14 | `playtest-verifier` on a known-good task | `Verdict: VERIFIED`, a video/screenshot path that exists, quoted widget values |
| 15 | Same on a deliberately broken branch | `VERIFY-FAILED` with the failing observation — a verifier that cannot fail is not a gate |
| 16 | Sandbox on → Aura edits a widget → Reject | Real `Content/` file unchanged (`git status` clean) |

## Appendix B — Things this guide could not verify from the record (check on the new machine)

- **Exact UE 5.8 plugin names/menu paths** (Unreal MCP / Editor Toolset / Terminal) — as
  recorded at setup time (2026-06); verify in your engine build's Plugins browser.
- **The current claude.ai connector setup flows** for Slack and Supabase — UI moves; follow
  Anthropic's current docs.
- **The uv winget package id** — install from astral.sh/uv if unsure.
- **Blender Lab versioning** — this guide assumes the MCP add-on ships in 5.1+; verify the
  add-on exists and its server port (9876) in your Blender build.
- **Meshy dashboard specifics** (where keys are minted, credit pricing) — verify on meshy.ai.
- **Claude Code subscription tier requirements** — verify on anthropic.com.
- The in-editor **Terminal plugin startup commands** recorded on the original machine
  included a TLS-relaxation env var for Node; prefer fixing certificates properly (AV
  exclusions, Appendix/THE LIST) over disabling verification — treat that recorded line as
  a workaround of last resort, not a recommendation.
- **Aura (Chapter 11) — the ⚠️ items no one has measured yet:**
  - **The Fab "$150 upfront, lifetime MCP usage + one-year subscription" SKU** is
    single-sourced (one press article; the Fab listing itself was never found). Verify it on
    Fab before choosing it over the monthly tiers — if real, it dominates for a pipeline whose
    main use is MCP.
  - **Whether Aura's PIE input simulation drives OUR Enhanced Input mappings** (the
    positional-layout keyboard law) or only default bindings — Aura's docs are silent; test
    with one case that needs the positional layout during the pilot.
  - **Whether enabling the Filesystem Sandbox adds a SECOND `.uproject` `Plugins` entry**
    beside `Aura` — only the `Aura` entry was measured; check the diff after the toggle.
  - **Credit per verification run** — the pilot's measurement, and the sole input to the
    tier decision (Pro vs Indie vs the Fab SKU). Credit does not roll over.
  - **The real `mcp__unreal_editor__*` tool names** — obtainable only from `/mcp` after the
    bridge is live; the enumerated allow-list and the verifier's `tools:` line wait on them.
  - **Whether Aura's verification can observe the game's C++ assistant-snapshot / cheat-manager
    state directly** or only actors + UMG (its docs list actors, GAS, UMG, multiplayer).

## Appendix C — House laws worth carrying to any new machine (one line each)

- Files are the contract; Slack/chat is visibility. No post is authorization.
- Nothing commits without a PASS QA report; max 3 QA loops, then escalate to the human.
- Never push unless the human asks; he may self-commit milestones — check HEAD first.
- Parse build logs for `Result:` — exit codes lie.
- Secrets are env-only or gitignored-ini-only; scripts redact; agents check existence, not value.
- Quota exhaustion is an expected pause, not a failure — record the reset time and stop.
- Big regenerable artifacts (weights, cooked builds, caches) are gitignored, never LFS'd.
- Nothing is imported unseen; nothing unverifiable is claimed — report outages instead of faking.
- When the human is present, closing the editor is his call — never force-kill.
- Numbers over adjectives in every spec; acceptance criteria per mechanic.

---

## Appendix D — Claude permissions (what to enable on a fresh machine, and what NOT to)

Claude Code gates tool calls. In **auto mode** an on-the-fly classifier judges each command, so on a new machine an agent chain will stall repeatedly until the recurring operations are pre-approved. Two mechanisms, and the difference matters:

| Mechanism | Where it lives | Lifetime |
|---|---|---|
| **Approving from `/permissions` → "Recently denied"** (arrow to the row, press **Enter**) | in-memory | **This session only** — gone next launch |
| **Allow-rules in a settings file** | `.claude/settings.local.json` (project, **gitignored**) | **Permanent** |

⭐ **Shell allow-rules take precedence over the classifier** (the classifier is only consulted when no rule matches — unless `autoMode.classifyAllShell` is set true), so the settings-file rules below are what actually stop the stalls. Rules match by **command prefix**; `Tool(prefix*)` matches anything starting with that prefix, and a bare `"ToolName"` allows the whole tool.

### D.1 The permanent allow-list (copy into `.claude/settings.local.json`)

Merge into `permissions.allow` — never replace the array. Also set `enabledMcpjsonServers` for the local MCP servers from `.mcp.json`.

```jsonc
{
  "permissions": {
    "allow": [
      // Subagent orchestration — the whole pipeline depends on it.
      // Also immunizes dispatches against classifier outages.
      "Agent",

      // Editor process control — scoped to UnrealEditor BY NAME so it can
      // never be used to kill anything else. Safe because the never-save law
      // + a dirtiness-zero check precede every close (see Chapter 4).
      "PowerShell(Stop-Process -Name UnrealEditor*)",
      "PowerShell(Get-Process UnrealEditor | Stop-Process*)",

      // Read-only probes (zero risk, highest frequency)
      "PowerShell(Get-Process*)", "PowerShell(Get-FileHash*)",
      "PowerShell(Test-Path*)",   "PowerShell(Get-ChildItem*)",
      "PowerShell(Get-Content*)", "PowerShell(Select-String*)",

      // Read-only git ONLY — enumerated deliberately. ⛔ NEVER "git *":
      // that would swallow push/reset/clean and undo the never-push law.
      "Bash(git status*)", "Bash(git log*)", "Bash(git diff*)", "Bash(git show*)",
      "Bash(git check-ignore*)", "Bash(git ls-files*)", "Bash(git rev-parse*)",
      "Bash(git rev-list*)", "Bash(git cat-file*)",
      // …and the same nine as PowerShell(git …*) if that shell is used.

      // The two UE batch files BY FULL PATH (compile + cook) — not "any exe"
      "Bash(\"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat\"*)",
      "PowerShell(& \"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat\"*)",
      "Bash(\"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/RunUAT.bat\"*)",
      "PowerShell(& \"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/RunUAT.bat\"*)",
      // Add the backslash spellings too — prefix matching is literal.

      // The headless editor commandlet lane (Chapter 4)
      "Bash(\"C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe\" *)",
      "PowerShell(& \"C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe\" *)",

      // Python SCOPED TO THIS PROJECT'S Tools/ — our own scripts, not arbitrary code
      "PowerShell(<python> \"<repo>/GitClaudeUnrealTest/Tools/*)",

      // MCP surfaces used constantly
      "mcp__unreal-mcp__call_tool",
      "mcp__claude_ai_Slack__slack_read_channel"
    ]
  },
  "enabledMcpjsonServers": ["unreal-mcp", "blender"]
}
```

⛔ **Keep `.claude/settings.local.json` gitignored** (this repo does so at `GitClaudeUnrealTest/.gitignore:16`) — machine-specific paths, and permissions should never be inherited silently by a collaborator.

### D.2 Session-only approvals (expected, and fine to re-grant each session)

These are one-off operations the classifier stops. They recur because a prefix rule cannot express them cleanly; approving them per session is the intended workflow:

- **Graceful editor quit over the Python remote-exec lane** (and its "wait for exit / enumerate windows" variants) — the polite counterpart to the `Stop-Process` rule above.
- **Posting `Escape` to a "Save Content" modal** — the *recovery* action that un-wedges a stuck editor. Escape = Cancel, never Save; that direction is what makes it safe.
- **Enabling Python remote execution on a live editor** — mostly redundant once `bRemoteExecution=True` is in `DefaultEngine.ini` (Chapter 4).
- **Driving the shipped game for verification** (starting a match, reading its state) — blocked mainly because it takes over the screen while the human is at the machine.
- **Parsing the gitignored cloud-config ini** — the key involved is the *publishable* one (the powerful service-role key is banned by law from ever being fetched), and the command is written not to echo it.
- **Editing `.claude/settings.local.json` itself** — grant this for ONE session to write the permanent rules, then let it lapse so file-editing stays gated. (That is how the D.1 list above was installed.)

### D.3 What to refuse — the triage rule

> Approve if it **reads** anything, or if it **writes only inside this project through a named tool**. Refuse if it could reach the remote, delete recursively, or run arbitrary commands.

⛔ Never allow: bare wildcards (`Bash(*)`, `PowerShell(*)`) · `git push` in any form (an allow-rule would silently undo the never-push law that keeps the human in control of the remote) · `git reset --hard`, `git clean -fdx`, `Remove-Item -Recurse`, `rm -rf` · anything writing outside the repo (registry, system folders, other drives) · arbitrary interpreter execution from temp directories.
⚠️ **Judgment call, default no:** anything that relocates the human's own save/profile data for a test. Agents have a scratch-profile route for almost all of it; if it runs unattended and dies mid-test, real player data is what's at risk.

### D.4 Adjacent grants that are not permission rules

- **MCP connectors** (Slack, Supabase) are authorized once in the Claude client, not here (Chapters 9–10).
- **Local MCP servers** (`unreal-mcp`, `blender`) are trusted via `enabledMcpjsonServers` above (Chapters 4–5).
- **Repo hooks** in `.claude/settings.json` (secret guard + generated-dir guard) run on every Edit/Write and are checked in for the team — a different mechanism from permissions, but part of the same safety story.
- **The editor-close law is a human policy, not a permission**: when the human is present, closing the editor is his call. The rules above exist for unattended runs; they never override his hand.

### D.5 Fresh-machine order

Install the tooling (Chapters 1–2) → clone → **write D.1's allow-list before the first agent chain**, or the first compile/cook will stall → run Appendix A's verification probes → grant D.2 items per session as they surface.
