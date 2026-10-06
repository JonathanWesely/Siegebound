# GameDevSetup — Recreating the Siegebound AI-Agent Game-Dev Environment From Scratch

**Status:** consolidated 2026-10-03 (rule-change pass 2026-10-04: editor lifecycle scripts, board archive, law split, amend-by-rewriting, no commit-host rows, bot switch) from `Docs/setupdirections.md` (the previous master, retired by this file), the Aura integration plan, `Docs/AuraProjectMemory.md`, the Obsidian notes *UE5 Agent Team System*, *Agent Team Communication Best Practices*, *UE5 AI Art Pipeline*, *Claude + Git Setup in Unreal Engine 5*, *Blender MCP — official addon vs uvx client*, the live repo configuration (`CLAUDE.md`, `.claude/`, `.mcp.json`, `Config/`, `Tools/`), and the orchestrator's memory of every incident since the project began in June 2026.

This is the one document a fresh Claude Code session plus one human need to stand up a **new Unreal Engine 5.8 C++ game** driven by a Claude agent team, with the same tools Siegebound used: Git + LFS, the built-in Unreal MCP, Blender MCP, the TRELLIS.2 / Meshy / FLUX art pipeline on Hugging Face, Supabase cloud accounts, a Slack mirror, a local llama.cpp assistant, the ffmpeg footage lane, and Aura's Play-In-Editor verification. It is written for **two readers working together**: **[You]** marks a human step (installs, accounts, dashboard clicks, Norton), **[Claude]** marks a step a Claude Code session performs (scaffolding files, wiring config, running probes). Untagged steps are either's. Follow the parts in order; each part's prerequisites come before it. Where Siegebound is cited as the worked example, generalize its paths (`GitClaudeUnrealTesting/GitClaudeUnrealTest`) to your own.

Siegebound-specific laws that a new game would re-derive differently are marked as such; everything else is transferable.

## Table of contents

- [Part 0 — The map: how the tools connect, and when you need each](#part-0-the-map-how-the-tools-connect-and-when-you-need-each)
- [Part 1 — THE LIST: every application, account, and version](#part-1-the-list-every-application-account-and-version)
- [Part 2 — Git, GitHub, and Git LFS](#part-2-git-github-and-git-lfs)
- [Part 3 — Unreal Engine 5.8, the C++ project, and the build command](#part-3-unreal-engine-58-the-c-project-and-the-build-command)
- [Part 4 — Unreal MCP: Claude's hands inside the editor, and how Claude opens and closes it](#part-4-unreal-mcp-claudes-hands-inside-the-editor-and-how-claude-opens-and-closes-it)
- [Part 5 — Claude Code: install, configuration, permissions, hooks, memory](#part-5-claude-code-install-configuration-permissions-hooks-memory)
- [Part 6 — The Claude agent team: roster, pipeline files, routing law, and which agents to create](#part-6-the-claude-agent-team-roster-pipeline-files-routing-law-and-which-agents-to-create)
- [Part 7 — The Slack mirror (team visibility channel)](#part-7-the-slack-mirror-team-visibility-channel)
- [Part 8 — The AI art pipeline: Blender MCP, TRELLIS.2, Meshy, FLUX concept art, rigging, import](#part-8-the-ai-art-pipeline-blender-mcp-trellis2-meshy-flux-concept-art-rigging-import)
- [Part 9 — The footage-review lane (gameplay video → diagnosed fixes)](#part-9-the-footage-review-lane-gameplay-video-→-diagnosed-fixes)
- [Part 10 — Supabase (cloud accounts and save sync)](#part-10-supabase-cloud-accounts-and-save-sync)
- [Part 11 — The in-game local LLM (SiegeLlama + llama.cpp + GGUF weights)](#part-11-the-in-game-local-llm-siegellama-llamacpp-gguf-weights)
- [Part 12 — Aura: Play-In-Editor verification for the agent team](#part-12-aura-play-in-editor-verification-for-the-agent-team)
- [Part 13 — Designing the game so the verifier can always drive it](#part-13-designing-the-game-so-the-verifier-can-always-drive-it)
- [Part 14 — Shipping: the `/ship` command and the packaging gates](#part-14-shipping-the-ship-command-and-the-packaging-gates)
- [Part 15 — Fresh-machine order and the full verification checklist](#part-15-fresh-machine-order-and-the-full-verification-checklist)
- [Appendix B — The permission allow-list, verbatim (`.claude/settings.local.json` as installed on the original machine)](#appendix-b-the-permission-allow-list-verbatim-claudesettingslocaljson-as-installed-on-the-original-machine)
- [Appendix C — The two PreToolUse hook scripts, verbatim](#appendix-c-the-two-pretooluse-hook-scripts-verbatim)
- [Appendix D — House laws worth carrying to any new game (one line each)](#appendix-d-house-laws-worth-carrying-to-any-new-game-one-line-each)
- [Appendix E — Glossary of CONVENTIONS namespaces and recurring terms](#appendix-e-glossary-of-conventions-namespaces-and-recurring-terms)

---

## Part 0 — The map: how the tools connect, and when you need each

```
                 ┌────────────────────────── the human (Jonathan) ──────────────────────────┐
                 │  GDD edits · playtest notes · Game Bar clips · Slack posts · hand checks  │
                 └───────────────┬───────────────────────────────────────────┬───────────────┘
                                 │ Claude Code (external terminal, project root)             │ Slack channel
                                 ▼                                                           ▼
┌──────────────────────── ORCHESTRATOR session ──────────────────────────┐   ┌───────────────────────┐
│ CLAUDE.md law · reads TASKBOARD · dispatches subagents · relays files │◄──│ #<game>agentteam      │
│ remuxes recordings · reads the channel at every checkpoint            │──►│ 6 standing threads    │
└──┬──────────┬──────────────┬─────────────┬────────────┬───────────┬────┘   └───────────────────────┘
   │          │              │             │            │           │
 manager   gameplay-      art-director  qa-reviewer  build-master  footage-analyst   playtest-verifier
 (board,   programmer     (Blender MCP, (read-only,  (Build.bat,   (ffmpeg frames,   (Aura PIE: inject
 CONVEN-   (C++/BP via    art pipeline, inspector    git, Unreal   VID-### report)   actions, read state,
 TIONS)    Unreal MCP)    import)       reads)       MCP assemble)                   screenshots, record)
   │          │              │             │            │           │                      │
   ▼          ▼              ▼             ▼            ▼           ▼                      ▼
 .claude/pipeline/{TASKBOARD,CONVENTIONS,handoffs/,qa/,footage/,fab/,SLACK.md}   ← the shared-file bus
                                 │
        ┌────────────────────────┼─────────────────────────────┬──────────────────────────────┐
        ▼                        ▼                             ▼                              ▼
 Unreal Editor 5.8         Blender 5.1                  Tools/ArtPipeline (uv, py3.12)    Aura (engine plugin
 built-in MCP :8000/mcp    Lab MCP add-on :9876         FLUX.1-dev ─┐ HF Inference API    + Aura.exe :41200)
 Terminal plugin           ◄─ Tools/blender_mcp_bridge  TRELLIS.2 ──┤ HF Space (ZeroGPU)  unreal_inspector (49 granted)
 Python remote-exec        headless blender.exe for     Meshy ──────┘ api.meshy.ai        unreal_editor    (32 granted)
 (graceful quit)           refine / rig / retarget      → Content/RawAssets → same-path   Saved/.Aura ← Tools/aura_sync.ps1
        │                                                 reimport commandlet
        ▼
 Source/ + Content/  ──►  Build.bat (editor CLOSED)  ──►  /ship (RunUAT cook)  ──►  packagedZIPofGame/ (gitignored)
        │
        ├── Plugins/SiegeLlama (llama.cpp, GGUF from Hugging Face → Models/, gitignored)   [only if the game needs a local LLM]
        └── Config/SiegeCloudDev.ini (gitignored) → Supabase project (GoTrue + PostgREST)  [only if the game needs cloud accounts]

 Git root (one level ABOVE the project): .gitattributes (LFS) · .gitignore (testvideo/, packagedZIPofGame/) · GitHub remote, never pushed unasked
 Norton: HTTPS-scan exclusions for huggingface.co, *.hf.space (+ *.huggingface.co, *.hf.co, *.supabase.co recommended); loopback 41200 open
```

### 0.1 When and why each tool is needed

| Tool | Needed when | Why this one | Skip it if |
|------|-------------|--------------|-----------|
| **Git + Git LFS + GitHub** (Part 2) | always | binary `.uasset`/`.umap`/FBX/PNG assets need LFS; the pipeline's gates are enforced at commit time | never |
| **UE 5.8 + VS C++ workload** (Part 3) | always | the engine; 5.8 ships the MCP and Terminal plugins and is Aura's best target | never |
| **Unreal MCP (built-in)** (Part 4) | any engine task: placing actors, editing Blueprints/data tables/materials/widgets, importing, saving | Claude's only hands inside the editor; auto-starts with the project | never |
| **Claude Code + the agent team** (Parts 5–6) | always | the orchestrator and its seven specialists; permissions, hooks and memory live here | never |
| **Slack mirror** (Part 7) | you want to watch progress and drop feedback from your phone | visibility only; files stay the contract | you are always at the terminal |
| **Blender MCP** (Part 8.1) | hand-modeled or procedural meshes, inspection of generated meshes, quick previews | the art-director's live modeling hands; three tools, all `bpy` | a 2D game (replace with a sprite lane) |
| **TRELLIS.2 on Hugging Face** (Part 8.4) | image → 3D for props, buildings, units | free, fast, MIT; the default engine | you buy all meshes on Fab |
| **Meshy** (Part 8.5) | hero assets, multi-view characters, retexturing dark TRELLIS output, animation clips | paid per asset; fixes TRELLIS's weaknesses; UE 5.8's retargeter is broken so Meshy clips feed the Blender retarget | TRELLIS output is good enough |
| **FLUX.1-dev concept art** (Part 8.3) | you have no concept art and want consistent inputs for the mesh engines | one prompt file drives every asset's look | you draw or buy concepts |
| **uv + Python 3.12** (Part 8.2) | any of the three generation scripts | pins `gradio_client`'s supported Python without touching the system install | no AI art |
| **Python 3.14 (system) + ffmpeg** (Part 9) | reviewing gameplay recordings | Claude cannot watch video; frames are the lane | you never record playtests (you should) |
| **Supabase** (Part 10) | accounts, cloud saves, cross-machine sync | free tier, RLS, reachable from UE's own HTTP module, driven by Claude's connector | a single-player, local-save game |
| **Local LLM (llama.cpp)** (Part 11) | the game itself needs on-device language understanding | in-process, Vulkan + CPU, grammar-constrained output | the game has no language feature |
| **Aura** (Part 12) | you want runtime verification of gameplay by an agent, not just text review | the only lane that drives PIE, injects input, reads live state, and records | you accept human-only playtesting (the footage lane still works) |

### 0.2 The laws in one breath

Files are the contract; chat and Slack are visibility. Every feature request goes to the manager first. Nothing commits without a PASS QA report; nothing with a runtime acceptance criterion commits without a VERIFIED report; max three fix loops, then escalate to the human. Compile with `Build.bat`, editor closed, and parse `Result:`; never Live Coding. Close and relaunch the editor only through `Tools/stop_editor.ps1` and `Tools/launch_editor.ps1`. Import with the editor open and MCP up; if unreachable, report, never fake. Secrets are env-only or gitignored-ini-only. Never push unless asked. When the human is mid-edit, the editor is theirs. Numbers over adjectives in every spec. Nothing is imported unseen; nothing unverifiable is claimed.

---

## Part 1 — THE LIST: every application, account, and version

Install or create each item when you reach the part that needs it (last column). Versions are the ones measured on the original machine on 2026-10-03; newer is usually fine, but the pins called out in the text (UE **5.8**, Blender **5.1**, Python **3.12** for the art venv) are load-bearing.

| # | Thing | What it is for | Get it from | Measured version | Part |
|---|-------|----------------|-------------|------------------|------|
| 1 | **Git** | Version control | git-scm.com | 2.56.0.windows.1 | 2 |
| 2 | **Git LFS** | Large binary assets (`.uasset`, `.umap`, `.fbx`, `.png`, `.wav`, `.dll`, …) | git-lfs.com | 3.8.0 | 2 |
| 3 | **GitHub account** | Remote repo host (the remote is only pushed when the human says so) | github.com | — | 2 |
| 4 | **Epic Games Launcher + Unreal Engine 5.8** | The engine. 5.8 ships the built-in **Unreal MCP** plugin, the **Terminal** plugin, and is the best-supported Aura target | epicgames.com | 5.8.0 | 3 |
| 5 | **Visual Studio 2022 (or newer) — "Game development with C++" workload** | Compiles the UE C++ project (UnrealBuildTool picked the MSVC 14.50 toolset on the original machine) | visualstudio.microsoft.com | VS 2022 / MSVC 14.50 | 3 |
| 6 | **Node.js + npm** | Runtime for Claude Code | nodejs.org | Node 24.14.1 / npm 11.11.0 | 5 |
| 7 | **Claude Code** (CLI) with a Claude subscription that includes it | The orchestrator session + every subagent | `npm install -g @anthropic-ai/claude-code` | 2.1.289 | 5 |
| 8 | **Blender 5.1** (the "Lab" line with the built-in **MCP** add-on) | Modeling hands for the art-director (live, via MCP) and the headless refine / rig / retarget scripts | blender.org | 5.1 (5.0 also present) | 8 |
| 9 | **uv** (Python package manager) | Creates the pinned Python 3.12 virtualenv for `Tools/ArtPipeline` | astral.sh/uv | 0.11.26 | 8 |
| 10 | **Hugging Face account + access token** | TRELLIS.2 image→3D (Space `microsoft/TRELLIS.2`), FLUX.1-dev concept images (Inference API), GGUF weight downloads | huggingface.co → Settings → Tokens (read scope) | — | 8, 11 |
| 11 | **Hugging Face PRO** (optional, ~$9/mo) | Raises ZeroGPU quota from ~5 to ~40 GPU-minutes/day for TRELLIS batches | huggingface.co/pricing | — | 8 |
| 12 | **Meshy account (paid credits)** | Second image→3D engine, multi-view characters, retexture, Mixamo-style animation clips | meshy.ai → API settings | API v1 | 8 |
| 13 | **Python 3.14 (system install)** + Pillow | The video-review frame extractor and the repo's standalone `Tools/*.py` scripts (never the art venv) | python.org; `py -m pip install pillow` | 3.14.4 at `C:\Python314\python.exe` | 9 |
| 14 | **ffmpeg** | Frame extraction from gameplay videos | `winget install --id Gyan.FFmpeg -e --source winget` | 9.0.1 (Gyan) | 9 |
| 15 | **Xbox Game Bar** (or any screen recorder) | Capturing gameplay clips for the footage-analyst (Win+Alt+R) | preinstalled on Windows 11 | — | 9 |
| 16 | **Slack workspace + the Claude Slack connector** | The team's visibility mirror channel | slack.com; claude.ai → Settings → Connectors | — | 7 |
| 17 | **Supabase account (free tier) + the Claude Supabase connector** | Cloud accounts and deck/settings sync, driven only through Claude's MCP tools | supabase.com; claude.ai → Settings → Connectors | — | 10 |
| 18 | **Aura AI for Unreal account** (Pro tier was chosen) | Play-In-Editor verification for the `playtest-verifier` agent (input injection, live actor/widget reads, screenshots, recordings) | tryaura.dev → dashboard installer for UE 5.8 | plugin 1.0.6 (`"Version": 74`) | 12 |
| 19 | **Obsidian** (optional) | The human's knowledge vault, where the research notes behind this document live | obsidian.md | — | 0 |

### 1.1 Secrets law for this whole document

No token, key, password, or project-specific credential appears anywhere in this file. Every secret is named by WHERE it lives (an environment-variable name, a gitignored ini, a provider dashboard) plus how to mint a new one. The names:

| Secret | Lives in | Minted at |
|--------|----------|-----------|
| `HF_TOKEN` | Windows **User** environment variable (HKCU) | huggingface.co → Settings → Tokens |
| `MESHY_TOKEN` (alias `MESHY_API_KEY`) | Windows **User** environment variable | meshy.ai → API settings |
| Supabase anon key + project URL | gitignored `Config/SiegeCloudDev.ini` | Supabase dashboard → Project Settings → API (or the MCP `get_publishable_keys` tool) |
| Supabase `service_role` key | **nowhere in the repo, ever** | stays in the Supabase dashboard |
| Aura login | Aura's own account session | tryaura.dev |
| Slack / Supabase connector auth | claude.ai account | claude.ai → Settings → Connectors |

Scripts read secrets from the environment only, redact them from every line of output, and exit with code `2` (printing the setup steps) when a required variable is unset. Agents check that a variable EXISTS; they never print its value. A pre-tool hook (Part 5) blocks any file write that contains a token-shaped string.

### 1.2 The antivirus reality — read before installing anything network-facing

Norton (and any AV that does HTTPS "scanning") performs a TLS man-in-the-middle on every HTTPS connection: it re-signs each site's certificate with its own **"Norton Web/Mail Shield Root"**. Windows trusts that root; Python's `certifi` bundle, Node's bundled CA store, and `uv` do not. The symptom is always an opaque `CERTIFICATE_VERIFY_FAILED` (Python) or `UNABLE_TO_VERIFY_LEAF_SIGNATURE` (Node) that looks like a code bug.

**Add these HTTPS-scan exclusions in Norton before the first run** (the exact Norton menu path was never recorded; look under its Firewall / SSL scanning settings on the new machine):

| Exclusion | Covers | Status on the original machine |
|-----------|--------|--------------------------------|
| `huggingface.co` | token auth, model metadata, the TRELLIS Space handshake | added 2026-07-07, proven |
| `*.hf.space` | the TRELLIS.2 Space endpoints | added 2026-07-07, proven |
| `*.huggingface.co` | `router.huggingface.co` (FLUX concept images), `cdn-lfs*.huggingface.co` (LFS downloads) | **never added** — the pipeline works around it with a CA bundle (Part 8.6) |
| `*.hf.co` | `us.aws.cdn.hf.co`, `*.xethub.hf.co` (large-file redirects) | **never added** — downloads use `curl` (Schannel) instead |
| `*.supabase.co` | the game's cloud client + GoTrue auth REST | listed as a hand-step before cloud testing |
| `*.meshy.ai` | the Meshy API and its download CDN | never needed so far; add if Meshy ever fails on TLS |

Related facts proven on the original machine:

- **winget's `msstore` source fails certificate checks under interception.** Always pass `--source winget`.
- **`UV_SYSTEM_CERTS=true`** (a User environment variable; renamed from the deprecated `UV_NATIVE_TLS`) makes `uv` trust the Windows certificate store, which is the fix for `uv sync` and `uv python install` under interception.
- **`SSL_CERT_FILE=<path to a combined CA bundle>`** is the last-resort fallback for Python tooling against a host that is not excluded. The bundle is `certifi` plus the machine's trust store exported, and it must be regenerated from the live Windows store because **Norton rotates its interception root** (the bundle pinned 2026-07-26 was stale by 2026-08-01). The pipeline keeps it at `Tools/ArtPipeline/Cache/_certs/win-ca-bundle.pem`.
- **Python 3.14 made it stricter:** `VERIFY_X509_STRICT` is on by default, so Norton's non-RFC-5280 root is a hard reject (`Basic Constraints of CA cert not marked critical`) where Python 3.12 quietly accepted it. Expect fetch tooling to break on a Python upgrade alone.
- **Never disable verification.** No `--insecure`, no `verify=False`, no token-stripping allowlist widened to route around a certificate error. The one recorded exception is the in-editor Terminal's `NODE_TLS_REJECT_UNAUTHORIZED=0` startup line (Part 4.4), which is a workaround of last resort; prefer exclusions or `NODE_EXTRA_CA_CERTS`.
- Norton prompted twice on the first ffmpeg runs; the human added exceptions.
- Aura's standalone app talks to the editor plugin over **loopback port 41200**; the AV must not block loopback.
- Distinguish: an HTTP `504` from Hugging Face is a router outage (wait and retry); `CERTIFICATE_VERIFY_FAILED` is the MITM (exclusion or bundle).

### 1.3 Windows gotchas that look like pipeline bugs

- **Smart App Control (SAC).** When enforced, every UE C++ build dies in about two seconds with `0x800711C7` ("An Application Control policy has blocked this file" on UnrealBuildTool's freshly compiled `ModuleRules.dll`). Registry tell: `HKLM\SYSTEM\CurrentControlSet\Control\CI\Policy\VerifiedAndReputablePolicyState` (`1` = enforced). Fix is human-only: Windows Security → App & browser control → Smart App Control settings → **Off**. Windows only lets you turn it off once. This is never a code error; do not open a QA loop on it.
- **User environment variables live in the registry under HKCU.** A Windows System Restore or rollback wipes them (it happened 2026-07-09). After any restore, re-check `HF_TOKEN`, `MESHY_TOKEN`, and `UV_SYSTEM_CERTS` before blaming a script. Set them through the GUI (Settings → System → About → Advanced system settings → Environment Variables → **User variables** → New) so the value never touches a shell history, and open a **new** terminal afterward.
- **uv's managed Python can vanish in the same rollback** while uv keeps a stale record (`error: Python interpreter not found at …`). Fix: `uv python install 3.12.13 --reinstall`.
- **The desktop locks after 300 s idle**, which kills any OS-level input injection into PIE. A bare Alt tap arms the menu accelerator and eats the next key.
- **`rg` is a shell function on the original machine**, so a child Bash sees zero matches for every pattern unless you `export -f rg` first. Prefer the Grep tool or `grep`.
- **Long paths.** `git config --global core.longpaths true` is a sensible precaution on Windows (Fab packs have deep paths). The original machine never set it and never hit the limit; `core.autocrlf=true` is set at system level, which is why the line-ending traps in Part 2.5 exist.

---

## Part 2 — Git, GitHub, and Git LFS

### 2.1 Install and initialize

1. **[You]** Install Git (git-scm.com) and Git LFS (git-lfs.com). GitHub Desktop is optional; everything below is CLI.
2. **[You]** Once per machine:
   ```powershell
   git lfs install
   git config --global user.name  "<Your Name>"
   git config --global user.email "<your@email>"
   git config --global core.longpaths true   # recommended; not set on the original machine, which never hit the 260-char limit
   ```
3. **[You]** Create the folder structure. This project uses a **two-level layout**: the git root is a wrapper folder ABOVE the Unreal project folder.

   ```
   <git-root>/                      <- git repo root (worked example: GitClaudeUnrealTesting/)
     .gitattributes                 <- LFS patterns (root level, covers everything)
     .gitignore                     <- small root ignore (engine junk + testvideo/ + packaged builds)
     README.md
     <ProjectName>/                 <- the Unreal project (worked example: GitClaudeUnrealTest/)
       <ProjectName>.uproject
       .gitignore                   <- the full project-level ignore (the load-bearing one)
       .mcp.json                    <- the four MCP servers (Parts 4, 8, 12)
       CLAUDE.md                    <- orchestrator law (Part 6)
       .claude/agents/              <- the seven agent definitions (Part 6)
       .claude/pipeline/            <- TASKBOARD / CONVENTIONS / handoffs / qa / footage / fab / SLACK
       .claude/hooks/               <- the two PreToolUse guard scripts (Part 5)
       .claude/commands/            <- /ship (Part 14)
       .claude/settings.json        <- committed hooks config
       .claude/settings.local.json  <- gitignored permission allow-list (Part 5)
       Config/  Content/  Source/  Plugins/  Docs/  Tools/
     testvideo/                     <- gameplay clips (gitignored; Part 9)
     packagedZIPofGame/             <- cooked builds (gitignored; Part 14)
   ```

   Why Siegebound's wrapper level exists: it was meant to give gitignored working areas (`testvideo/`, packaged builds, scratch folders) a home inside the repo but outside the Unreal project. That reason turned out not to be real: the editor and the cooker scan `Content/`, `Config/`, `Source/` and `Plugins/`, not arbitrary sibling folders, so an ignored `testvideo/` inside the project folder would have been just as invisible to them.

   ⭐ **Recommendation for a NEW game (2026-10-04): make the Unreal project folder the git root** and put the ignored working folders (`testvideo/`, `Packaged/`) inside it. The two-level layout produced its own family of silent-failure laws in Siegebound (a mis-anchored pathspec prints nothing, `HEAD:<path>` vs `-- <path>` disagree, every commit host anchors "one level up"), and none of that exists in a single-level repo. Siegebound itself stays two-level; moving it is not worth the churn, so the rest of this part documents both.

   ⚠️ **Consequence that bites every session:** the git root is ONE LEVEL UP from the folder Claude Code runs in. A `git` pathspec anchored at the wrong level matches nothing and prints nothing, and an empty result reads exactly like "nothing to commit". Anchor pathspecs at the git root (or use the root-anchored magic form `':/GitClaudeUnrealTest/path'`), and never read an empty result as a negative answer.

4. **[You]** Create the Unreal project inside the wrapper folder (Part 3), close the editor, then:
   ```powershell
   cd <git-root>
   git init
   git remote add origin https://github.com/<you>/<your-repo>.git
   ```
   Put the `.gitattributes` and both `.gitignore` files in place BEFORE the first `git add`. Git history is append-only: a binary committed raw before its LFS rule exists stays raw forever.

### 2.2 `.gitattributes` (git root) — what LFS tracks

Copy verbatim (this is the live root file):

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
GitClaudeUnrealTest/Tools/Packaging/Fixtures/*.log -text
```

Notes: `.uproject` is JSON text and is NOT tracked by LFS. `*.dll` / `*.lib` are listed because the repo vendors a llama.cpp plugin (Part 11); the rule had to land before the commit that added 72 MB of binaries. The last line keeps the packaging test fixtures byte-exact (no CRLF conversion).

### 2.3 The two `.gitignore` files

**Root `.gitignore`** (verbatim):

```
Binaries/
DerivedDataCache/
Intermediate/
Saved/
Build/
.vs/
*.sln
*.suo
# Gameplay footage review lane: raw videos + the .frames/ cache never enter git.
# *.mp4 is an LFS pattern in .gitattributes — without this line a careless add pushes whole videos into LFS.
# Only promoted evidence PNGs under <Project>/.claude/pipeline/playtest-evidence/ are committed.
testvideo/
# Packaged builds: the cooked Win64 package + its zip are MULTI-GB and never enter git in any form.
# LFS is NOT the answer here — nothing in this folder should be versioned at all.
packagedZIPofGame/
```

**Project-level `.gitignore`** — start from the canonical GitHub `UnrealEngine.gitignore`, then add these project rules (each one was learned the hard way):

| Rule | Why |
|------|-----|
| `Binaries/`, `DerivedDataCache/`, `Intermediate/`, `Saved/`, `Plugins/*/Binaries/`, `Plugins/*/Intermediate/`, `.vs/`, `*.sln`, `*.suo`, `.vscode/`, `.idea/` | UE and IDE generated output |
| `.claude/settings.local.json` | machine-local permission grants and MCP enablement; never inherited by a collaborator |
| `Tools/ArtPipeline/.venv/` | the uv virtualenv (700+ files); `uv sync` recreates it |
| `Tools/ArtPipeline/Inbox/*` + `!Tools/ArtPipeline/Inbox/.gitkeep`, same for `Cache/` | local working data; the `dir/*` form is what makes the `.gitkeep` negation work |
| `*.gguf` and `/Models/` | LLM weights (~2.5 GB). **Never versioned in any form.** Use the directory form and never add a `!` negation inside it (git does not descend into an ignored directory, so the negation silently does nothing). LFS is NOT the answer for large regenerable artifacts: a fetch script plus a pinned version note make them reproducible instead |
| `Config/SiegeCloudDev.ini` | the real cloud config (Part 10); only the `.example` template is committed |
| `/Build/`, `/Builds/`, `/Packaged/` | packaged output |
| `__pycache__/`, `*.fbm/`, `*_BuiltData.uasset`, `Docs/GDD-Submission-*.pdf` | Python bytecode, UE's FBX texture-extraction dirs, built lighting data, homework PDFs |
| `/Content/Dev/` | throwaway PIE dev-test Blueprints |
| `!Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/lib/**/*.lib`, `!…/bin/**/*.dll` | re-include the vendored llama.cpp binaries that the global `*.dll` / `*.lib` ignore would swallow (placed AFTER those rules; last match wins), followed by a restated `*.gguf` + `/Models/` so the final word on weights is an ignore |

`.gitignore` only affects UNTRACKED files. Anything already committed stays tracked until `git rm --cached`.

### 2.4 Verify

```powershell
git lfs status
git lfs ls-files | Select-Object -First 5          # after the first commits: assets listed here, not raw blobs
git check-ignore -v Models/anything.gguf            # must report the /Models/ rule
git check-ignore -v testvideo/clip.mp4              # must report the root testvideo/ rule
git check-attr filter -- <Project>/Content/Maps/L_Arena.umap   # filter: lfs
```

### 2.5 House git law (adopt these as rules; each prevented or caused a real incident)

- **Nothing is committed without a PASS QA report** (code) or a completed integration check (art), and nothing with a runtime acceptance criterion is committed without a VERIFIED report (Part 12). Only the build-master runs git.
- **Never push to remote unless the human explicitly asks.** The permission allow-list (Part 5) deliberately contains only read-only git verbs: a `git *` rule would silently undo this law.
- **The human may self-commit and push milestones** with a terse message. Before dispatching any commit, check `git log --oneline -3`, `git status`, and `origin/main` so you never duplicate or amend his work.
- **Milestone preservation:** on each finished milestone cut a `<milestone>-testable` branch as the playtest fallback; develop the next milestone on `main`.
- **Commit by explicit file pathspec, never `git add -A` / `git add .` / `commit -a`.** A directory pathspec is a bounded `add -A` that sweeps in whatever another agent writes into that directory between your status read and your add. Stage files by name whenever any task is in flight.
- **Verify the COMMIT, never the index:** `git show --stat HEAD`. The UE editor's Git source-control plugin (`Provider=Git` in `Saved/Config/WindowsEditor/SourceControlSettings.ini`) runs `git add` on every asset it saves or imports, so the index is a moving target while the editor is open. The durable fix is **Revision Control → Change Revision Control Settings → Provider = None**; the setting lives under gitignored `Saved/`, so a fresh clone has it back at `Git` and you must set it again. (The vault's older Git note recommends enabling the provider; house law overrides that.)
- **Verify an LFS asset by oid vs `sha256`, never by size.** Two different `.uasset` blobs weighed exactly 46,510 bytes; only the pointer-oid comparison proved the difference. `git cat-file -p HEAD:<path>` on an LFS asset returns the pointer, not the bytes.
- **`git diff` is blind to line-ending-only corruption** because `core.autocrlf` normalizes it away. A scripted JSON/INI round-trip once turned a `.uproject` from 859 to 805 bytes (CRLF→LF) with a perfectly clean diff. Prove any "I put the file back" claim by checksum or byte size, and prefer targeted text edits over parse/re-serialize round-trips for config files.
- **Two git path forms are not interchangeable:** `-- <pathspec>` is CWD-relative; `HEAD:<path>` is root-relative. Mixing them yields an empty diff and a vacuous pass. Run diffs from the toplevel or use `':/<path>'`.
- **Secret-scan any document you did not author before staging it** and state the result, including benign hits (`HF_TOKEN` appearing as a variable name in prose).
- **Relaunched editors re-save assets on boot** (new LFS oid, same size), so expect spurious dirty `.uasset` files after every editor bounce; never blind-commit them. A running editor also holds locks on `.uasset` files, so `git checkout -- <asset>` can fail with "unable to unlink"; defer restores to an editor-closed window.
- A committer's gate is that the evidence exists in git, is the right file, and carries no secrets. It does not re-adjudicate the artifacts it commits.

---

## Part 3 — Unreal Engine 5.8, the C++ project, and the build command

### 3.1 Install and create the project

1. **[You]** Install the Epic Games Launcher, then **Unreal Engine 5.8** from its Library tab.
2. **[You]** Install Visual Studio 2022 (or newer) with the **Game development with C++** workload. UE's `.vsconfig` in the project root lists the exact components; Visual Studio offers to install missing ones when you open the solution.
3. **[You]** Launcher → UE 5.8 → New Project → **Games → Third Person (C++)** (Siegebound started from this template; Blank C++ also works). Create it INSIDE the git wrapper folder from Part 2. Close the editor before the first commit (file locks).
4. **[You]** Edit → Plugins, enable, restart once:
   - **Unreal MCP** (internal name `ModelContextProtocol`) — the built-in editor MCP server (Part 4)
   - **Editor Toolset** (`EditorToolset`) — the toolset registry that gives that server its tools
   - **Terminal** — an in-editor terminal panel that can auto-launch Claude Code (Part 4.4)
   - **Python Editor Script Plugin** — ships enabled; needed for remote execution and the headless commandlet lane
   - later: **Aura** (Part 12). Siegebound also enables `StateTree`, `GameplayStateTree`, `ModelingToolsEditorMode`, `ProceduralVegetationEditor`, and its in-repo `SiegeLlama` plugin; those are game choices, not setup requirements.
5. Commit the fresh project (the `.gitignore` files from Part 2 must be in place first).

### 3.2 The build command

The canonical compile. Keep it in `CLAUDE.md` so every agent uses the same line:

```
"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" <ProjectName>Editor Win64 Development -project="<abs-path>/<ProjectName>.uproject" -waitmutex
```

Siegebound's exact line:

```
"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat" GitClaudeUnrealTestEditor Win64 Development -project="C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/GitClaudeUnrealTest.uproject" -waitmutex
```

**The editor must be CLOSED for a C++ compile** (Live Coding holds a lock on the game module DLL). `-waitmutex` makes the build wait on the Live Coding mutex instead of failing immediately; it does not remove the need to close the editor.

### 3.3 The gotchas that burn every new machine

- **Gotcha 1 — the exit-code lie.** `Build.bat` can return exit code **0 on a FAILED build** (observed live when Live Coding held the mutex). Never trust `$LASTEXITCODE`. Parse the output for the line beginning `Result:` — `Result: Succeeded` or `Result: Failed` is the only truth, and a missing `Result:` line is also a failure. Make this a written law for the build agent.
- **Gotcha 2 — Smart App Control** (Part 1.3): a build that dies in ~2 s with `0x800711C7` is Windows, not code.
- **Gotcha 3 — Live Coding cannot add a reflected type.** `Ctrl+Alt+F11` is not an escape hatch when the diff adds a new `UCLASS`/`USTRUCT`/`UENUM` or when UnrealBuildTool logs "source file added" / invalidates the makefile. A new type needs fresh UnrealHeaderTool reflection and a full build with the editor down. House law goes further: **agents never compile via Live Coding at all**; compilation belongs to the build agent via `Build.bat`.
- **Gotcha 4 — the editor-state pairing.** A compile needs the editor **CLOSED**; an asset import or scene assembly over MCP needs it **OPEN with the MCP server up on port 8000**. These are opposite requirements, and the sequence a session actually runs is: save assets → close editor → compile → relaunch editor → wait for MCP → import/assemble. Part 4.6 has the procedure.
- **Gotcha 5 — warnings are errors.** UBT runs with `-WarningsAsErrors`. `C4458` (a local or member shadowing a base-class member, e.g. a `UUserWidget` subclass declaring its own `InputComponent`) is a hard error that no in-repo grep can predict. UE 5.8's `TCheckedFormatString` (`C7595`) rejects a non-literal `FString::Printf` format argument. A `*/` inside a `/** */` comment terminates it early.
- **Gotcha 6 — three non-code build stops that must never open a QA loop:** the Live Coding mutex (editor open), Smart App Control, and a build that compiled zero translation units (the code was not judged at all).
- Parked-but-uncommitted code in `Source/` breaks everyone's compile (UBT compiles the whole module).

### 3.4 Project-level engine settings worth copying

Add to `Config/DefaultEngine.ini` (committed):

```ini
; Python remote execution: lets the agent pipeline issue a graceful in-editor
; quit_editor() (and other editor Python) over the local multicast lane, so an
; unattended editor never wedges on a Save Content modal. Editor-only plugin
; setting — no effect on packaged builds.
[/Script/PythonScriptPlugin.PythonScriptPluginSettings]
bRemoteExecution=True
```

The same key also appears in the gitignored `Saved/Config/WindowsEditor/Engine.ini`, and the editor's shutdown rewrite DROPS it from that file every time; the committed `DefaultEngine.ini` entry is what survives.

Also set, once per machine (it lives under gitignored `Saved/`): **Revision Control → Change Revision Control Settings → Provider: None** (Part 2.5 explains why).

Give the game its displayed identity in `Config/DefaultGame.ini` (committed) under `[/Script/EngineSettings.GeneralProjectSettings]`: `ProjectName` and `ProjectDisplayedTitle` feed the packaged exe's Windows file properties and the game window's title (otherwise a shipped build reads "Third Person Game Template"), and `CompanyName` / `CopyrightNotice` are the human's legal assertions that only they author. This is a displayed identity, not a `.uproject`/module/folder rename, which would be a multi-day refactor of every hardcoded path the pipeline owns. Siegebound also pins `[ConsoleVariables]` `CommonUI.CheckKeyboardFocusAndParentage=1`, `CommonUI.DisallowUserFocusedWidgetForPendingFocusRecipient=1`, `CommonUI.FallbackToDesiredOnAutoRestoreFailure=1` there for predictable UI focus (Part 13.2). Packaging settings are deliberately NOT in `Config/`: the maps allowlist and cook directories live in `Tools/Packaging/ship.ps1` (Part 14).

### 3.5 Verify

Run the build command with the editor closed and confirm the output contains `Result: Succeeded`. Launch the editor once and confirm the project loads.

---

## Part 4 — Unreal MCP: Claude's hands inside the editor, and how Claude opens and closes it

UE 5.8 ships a built-in **Model Context Protocol** plugin that embeds an MCP server in the editor. With it, Claude can place and transform actors, create and edit Blueprints, data tables, materials and widgets, run editor Python, drive lighting, and read project state. This is the "actor/scene surface" used by the gameplay-programmer, art-director, and build-master. It cannot play the game; that is Aura's job (Part 12).

### 4.1 Plugins

Edit → Plugins → enable **Unreal MCP** (`ModelContextProtocol`), **Editor Toolset**, and **Terminal**; restart once. Without Editor Toolset the server runs with nothing to offer. If Claude later connects but sees no tools: Edit → Project Settings → **MCP** (the `MCPToolsetSettings` page) → enable the toolset registries → restart.

### 4.2 Editor Preferences → Model Context Protocol (auto-start)

Edit → Editor Preferences → General → **Model Context Protocol**. The four values, as stored in `Saved/Config/WindowsEditor/EditorPerProjectUserSettings.ini` on the original machine:

```ini
[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]
ServerUrlPath=/mcp
ServerPortNumber=8000
bAutoStartServer=True
bEnableToolSearch=True
```

- **Auto Start Server = on** is what makes the server come up every time the project opens, so Claude never has to be told to start it.
- Endpoint: `http://127.0.0.1:8000/mcp`. Change the port here only if 8000 is taken, and then change it in `.mcp.json` too.
- **Enable Tool Search** lets the client discover tools lazily instead of loading the whole registry up front.

This file is under gitignored `Saved/`, so a fresh clone needs these set again by hand.

### 4.3 Generate the client config → `.mcp.json`

In the editor console (backtick key) or the Output Log's Cmd field:

```
ModelContextProtocol.GenerateClientConfig ClaudeCode
```

It writes `.mcp.json` into the project root:

```json
{
  "mcpServers": {
    "unreal-mcp": {
      "type": "http",
      "url": "http://127.0.0.1:8000/mcp"
    }
  }
}
```

(`GenerateClientConfig All` also emits Cursor / VS Code / Gemini configs.) This file is committed; the other three servers (Blender, and Aura's two) are added to it in Parts 8 and 12. Claude Code must also be told to trust the project's servers: `"enabledMcpjsonServers": ["unreal-mcp", "blender", "unreal_inspector", "unreal_editor"]` in `.claude/settings.local.json` (Part 5). Fully restart Claude Code after any `.mcp.json` edit; it does not hot-reload.

### 4.4 Editor Preferences → Terminal (the four startup commands)

The Terminal plugin adds a terminal panel inside the editor and can auto-launch Claude Code when the panel opens. Edit → Editor Preferences → General → **Terminal** → **Startup Commands** → `+` four times, in this order (live values from the original machine):

```ini
[/Script/Terminal.TerminalSettings]
ShellExecutablePath=
FontFamily=CascadiaMono
FontSize=10
ScrollbackLimit=131072
ColorSchemeName=Default
StartupCommands=set NODE_TLS_REJECT_UNAUTHORIZED=0
StartupCommands=set TERM=xterm-256color
StartupCommands=cd /d "C:\GitProjects\GitHub\GitClaudeUnrealTesting"
StartupCommands=claude
bPreventCloseDuringActivity=True
ActivityTimeoutSeconds=5.000000
```

| # | Command | Why |
|---|---------|-----|
| [0] | `set NODE_TLS_REJECT_UNAUTHORIZED=0` | Lets Claude Code's Node runtime talk to Anthropic through the Norton TLS interception without certificate errors. This disables Node's certificate verification inside that panel only. Prefer the exclusion route or `set NODE_EXTRA_CA_CERTS=<path to the exported Norton root PEM>` if your AV allows it; keep this line only as the last resort it was on the original machine |
| [1] | `set TERM=xterm-256color` | Prevents garbled terminal output |
| [2] | `cd /d "<folder>"` | Where Claude Code starts. ⚠️ The recorded value points at the **git root**, which has no `.mcp.json`, `CLAUDE.md`, or `.claude/agents/`. Claude Code loads those from its starting folder, so on the new machine point this at the **Unreal project folder** (`<git-root>\<ProjectName>`) |
| [3] | `claude` | Auto-launches Claude Code in the panel |

`ShellExecutablePath` empty = the default `cmd.exe`, which is why the commands use `set` and `cd /d`. **Prevent Close During Activity** keeps the editor from closing the panel while Claude is mid-command.

Use the panel for one-offs. For long milestone runs prefer an **external terminal**: the in-editor panel dies if the editor crashes mid-build, while an external session survives the bounce and resumes from the task board. The external launch is simply:

```powershell
cd "C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest"
claude
```

### 4.5 Talking to the server from Claude Code

- The project's permission allow-list grants `mcp__unreal-mcp__call_tool`. That tool takes **`toolset_name` and `tool_name` as two separate arguments**; a single dotted string returns `Tool not found`, which reads like a missing tool rather than a wrong argument shape.
- The hard gate: if `http://127.0.0.1:8000/mcp` is unreachable, engine tasks STOP and report. Nothing is faked. Probe with `Get-Process UnrealEditor*` and `Get-NetTCPConnection -LocalPort 8000` (PowerShell), or an HTTP POST to the endpoint.
- The MCP server wedges if PIE is stopped and restarted within ~7 s; leave a settle gap. A raw HTTP POST carrying the `Mcp-Session-Id` header is the client-independent escape hatch when a client session is confused.
- While ANY PIE session is running, every `EditorAssetSubsystem` call fails with "Asset does not exist"; gate saves on PIE being stopped, and save by explicit path (an empty `save_assets([])` means save-all).
- `set_actor_transform` over MCP does not fire `PostEditChange`; volumes such as `NavMeshBoundsVolume` do not re-register until you toggle a property.
- `DataTableTools.set_rows` stores NULL for soft-object cells passed as `{"refPath": ...}`; pass plain path strings and read back.

### 4.6 Opening and closing the editor from Claude (the lifecycle procedure)

The build-master opens and closes the editor around every compile. The human granted a **standing permission on 2026-08-30 ("I give you permission to close and reopen the editor whenever needed")**, with one condition: when the human says they are mid-edit, wait. Since 2026-10-04 the procedure is encoded in two repo scripts, and those scripts are the only editor-lifecycle commands the permission file grants. The reason is an incident: on 2026-09-09 a kill-by-name (`Stop-Process -Name UnrealEditor`) stopped two of the human's `-game` play sessions, because `UnrealEditor.exe` is one binary in three roles with one window title. The law that followed (`SC-§118`: classify by command line, kill by PID, never by name) sat in the law file while the permission file kept pre-approving the banned command and prompting on the legal one. Putting the law inside a script, and granting only the script, fixed both halves.

**`Tools/stop_editor.ps1`** (PowerShell 5.1, ASCII-only):

```powershell
powershell -NoProfile -File Tools\stop_editor.ps1 -WhatIf          # census only: every UnrealEditor.exe with PID, class, creation time, command line
powershell -NoProfile -File Tools\stop_editor.ps1                  # terminate THIS project's single GUI editor by PID
powershell -NoProfile -File Tools\stop_editor.ps1 -TargetPid 18832 # when more than one editor is up
powershell -NoProfile -File Tools\stop_editor.ps1 -RequireAllDown  # cook pre-flight: exit 3 names anything that survives
```

It classifies each `UnrealEditor.exe` by its command line: `PLAY` (contains `-game`; the human's session, never touched, reported), `HEADLESS` (`-run=`, `-AuraHeadless`, `-RenderOffScreen`, `-unattended`; left alone unless `-IncludeHeadless`), `EDITOR` (names this project's `.uproject`; ours), `OTHER` (another project). It refuses a PID that is not `EDITOR`, refuses to guess between two editors, hashes `Content/Maps/L_Arena.umap` before and after (the never-save law; a changed hash is exit 6), waits for the process to exit, and prints the census again. Terminating is the ruled-correct close when only discardable assets are dirty: it eliminates the Slate "Save Content" modal whose *Save Selected* sits beside *Don't Save*. Exit codes: 0 done · 2 no editor of this project running · 3 `-RequireAllDown` and something survives · 4 refused · 5 did not exit · 6 hash changed · 7 error.

**`Tools/launch_editor.ps1`:**

```powershell
powershell -NoProfile -File Tools\launch_editor.ps1 [-TimeoutSeconds 300] [-Port 8000] [-WhatIf]
```

It refuses if an `EDITOR` instance of this project is already up (exit 2), derives the engine from the `.uproject`'s `EngineAssociation` through the registry (fallback: the stock UE_5.8 path), starts the editor detached on the `.uproject`, and waits until the MCP server **answers an HTTP request** on port 8000 (a listening port is not a live server), then prints `LAUNCH_EDITOR: PID=… SECONDS=…`. That line means "MCP answers", not "editor ready": the plugin's HTTP server comes up early in boot (17 s on the first live run, against the 45–140 s full boots measured before), so a step that needs the level loaded or the asset registry scanned establishes that by its own instrument. Exit 5 = alive but no MCP, exit 6 = the process died (read `Saved/Logs`).

**The sequence for every C++ compile:** `stop_editor.ps1` → `Build.bat` (parse `Result:`) → `launch_editor.ps1` → status `built`. First live run 2026-10-04 (TASK-1602): close in under a second with `hash MATCH`, compile 24.5 s, suite 575/575, relaunch with MCP answering at 17 s. Both scripts and the two Python maintenance tools went through a QA gate the same day (TASK-1604 FAIL → TASK-1605 fix → TASK-1606 PASS): the archiver was not re-run safe and the splitter's sanity check could not fail. Tooling is code; gate it. Both scripts' census output goes in the handoff. Closing the GUI editor can auto-launch a headless Aura twin minutes later, so re-run the census (`-WhatIf`) before a compile, a relaunch, or a cook. The graceful quit over Python remote execution (`unreal.SystemLibrary.quit_editor()`) remains available for a modal-blocked editor, with `Escape` = Cancel on a Save Content dialog; it is not the default route.

Aura's `unreal_inspector` server also carries `launch_unreal_project`, `recompile_unreal_project`, and `shutdown_headless`; those are deliberately granted to no agent (Part 12.5).

### 4.7 What MCP cannot do, and the lanes that fill the gaps

- **Same-path asset overwrite.** The MCP static-mesh import tool cannot overwrite an existing `SM_<Name>` at the same content path ("already exists"), and the MCP Python sandbox has no `import unreal`. Deleting and recreating the asset nulls every Blueprint's hard reference. The fix is a **headless Python commandlet** (editor CLOSED):
  ```
  "C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "<abs>/<Project>.uproject" -run=pythonscript -script="<abs>/Tools/reimport_meshes.py" -unattended -nosplash -nopause -stdout -FullStdOutLogOutput
  ```
  using `unreal.AssetImportTask(replace_existing=True, save=True)`, which preserves UObject identity so every reference survives. Worked example: `Tools/reimport_meshes.py` (+ `Tools/reimport_cards.txt` to pick assets). Two commandlet gotchas: `StaticMeshEditorSubsystem` is None in a commandlet (use `EditorStaticMeshLibrary`), and the post-reimport build resets empty material slots to `WorldGridMaterial`, so slot materials are finalized afterward over MCP in the relaunched editor (`Tools/reimport_apply_materials_mcp.py`, `Tools/reimport_finalize_materials_mcp.py`).
- **The commandlet reaches reflected surface only** (`UPROPERTY` / `UFUNCTION`). A private static or a plain inline getter cannot be called from it. Before offering the commandlet as a PIE substitute, name the reflected symbol it will call.
- **No input injection and no viewport.** The built-in MCP cannot simulate player input or see the game, so no agent may claim "PIE gameplay works" from it. Aura (Part 12) and the footage lane (Part 9) exist for that.
- **Widgets:** MCP cannot create the design-time root panel or exactly-named `BindWidget` targets of a UMG widget; a human drops those in the UMG Designer (about two minutes) and agents configure the rest over MCP. Duplicating and reparenting a WidgetBlueprint can silently break runtime repaint while the Designer preview looks fine; after about two failed fix rounds, rebuild the widget fresh from the intended C++ parent. `BindWidgetOptional` binds null silently on a misspelled name; verify by readback.
- **A Blueprint compile does not dirty the package.** `compile_blueprint` followed by `save_assets` returns true and writes nothing (sha256 and mtime unchanged). Only the Blueprint editor's own Compile → `Ctrl+S` persists the compiled class, so a row whose deliverable is a compiled Blueprint carries a human keystroke. The gate is that the `.uasset` sha256 must CHANGE. A green automation suite, even headless `-nullrhi`, cannot catch a stale on-disk class because PIE recompiles in memory; a cooked build does not, so the stale class ships.
- **Material usage flags:** a material missing `bUsedWithInstancedStaticMeshes` is silently swapped for the default material on instanced/foliage meshes only, warned once per session at PIE start, with every property readback green. Check usage flags first when the same mesh looks right one way and wrong another.
- Full editor Python into a RUNNING editor is possible too: one `ObjectTools.set_properties` on `/Script/PythonScriptPlugin.Default__PythonScriptPluginSettings` (in memory) plus the engine's `remote_execution.py` client. Never validate RPC routing from remote-exec Python: it forces local execution and cannot distinguish a working relay from a broken one.

### 4.8 Verify

With the editor open and the server auto-started, run `claude` in the project folder and ask *"What actors do I have selected?"* An answer with real editor context means the chain works. Then close the editor through the Part 4.6 procedure and relaunch it; the server must come back on its own.

---

## Part 5 — Claude Code: install, configuration, permissions, hooks, memory

### 5.1 Install

1. **[You]** Install Node.js (nodejs.org; the original machine runs Node 24 / npm 11).
2. **[You]** `npm install -g @anthropic-ai/claude-code`, then run `claude` once anywhere and sign in with a Claude subscription that includes Claude Code. (The native installer also works; the team ran 2.1.289.)
3. **[You]** Optional: Claude Desktop, if you want a second client for Blender. Run only one client against Blender at a time (Part 8.1).
4. Connectors (Slack, Supabase) are authorized once at **claude.ai → Settings → Connectors** under your account; their tools then appear in Claude Code sessions as `mcp__claude_ai_Slack__*` and `mcp__claude_ai_Supabase__*`. They are not entries in `.mcp.json`.

### 5.2 Where configuration lives

| File | Scope | Committed? | Holds |
|------|-------|------------|-------|
| `~/.claude/settings.json` | user | no (personal) | default model and effort, auto-update channel, TUI mode, auto-mode preferences, enabled plugins |
| `~/.claude.json` | user | no | onboarding/session state; user-scope `mcpServers` (keep this EMPTY; Part 12.4) |
| `<Project>/CLAUDE.md` | project | yes | orchestrator law (Part 6) |
| `<Project>/.claude/agents/*.md` | project | yes | the seven subagents (Part 6) |
| `<Project>/.mcp.json` | project | yes | the four local MCP servers (Parts 4, 8, 12) |
| `<Project>/.claude/settings.json` | project | yes | the two PreToolUse hooks (5.4) |
| `<Project>/.claude/settings.local.json` | project, this machine | **no** (gitignored) | permission allow-list + `enabledMcpjsonServers` (5.3) |
| `<Project>/.claude/commands/ship.md` | project | yes | the `/ship` command (Part 14) |
| `~/.claude/projects/<slug>/memory/` | user, per project | no | the orchestrator's persistent memory (5.5) |
| `~/.claude/skills/`, `<dir>/.claude/skills/` | user / project | no / optional | installed skills; the original machine has ten project-local skills under `C:\GitProjects\.claude\skills` (Obsidian, browser, MCP-builder, design) that are not part of the game repo |

The original machine's `~/.claude/settings.json`, minus personal noise:

```json
{
  "model": "claude-fable-5-1",
  "effortLevel": "xhigh",
  "autoUpdatesChannel": "latest",
  "tui": "fullscreen",
  "agentPushNotifEnabled": true,
  "enabledPlugins": { "claude-code-setup@claude-plugins-official": true }
}
```

Claude Code starts in a folder and treats it as the project: it loads that folder's `CLAUDE.md`, `.claude/agents/`, `.mcp.json`, and `.claude/settings*.json`. Always launch it from the **Unreal project folder**, not the git root.

### 5.3 Permissions: the allow-list law

Claude Code gates every tool call. In **auto mode** a classifier judges each shell command on the fly, so on a new machine an agent chain stalls repeatedly until the recurring operations are pre-approved. Two mechanisms, and the difference matters:

| Mechanism | Where it lives | Lifetime |
|-----------|----------------|----------|
| Approving from `/permissions` → "Recently denied" (arrow to the row, Enter) | in memory | **this session only** |
| Allow-rules in `.claude/settings.local.json` | project, gitignored | **permanent** |

Shell allow-rules take precedence over the classifier (it is consulted only when no rule matches, unless `autoMode.classifyAllShell` is set), so the file rules are what actually stop the stalls. Rules match by **command prefix**: `Tool(prefix*)` matches anything starting with that prefix; a bare `"ToolName"` allows the whole tool. `Stop-Process -Id <pid>` still prompts when only `-Name` is allowed; fix the dispatch phrasing rather than widening the rule.

**The triage rule:** approve if it READS anything, or WRITES only inside this project through a named tool. Refuse if it could reach the remote, delete recursively, or run arbitrary commands. Never allow: `Bash(*)` / `PowerShell(*)`, `git push` in any form, `git *` (it would swallow push), `git reset --hard`, `git clean -fdx`, `Remove-Item -Recurse`, `rm -rf`, anything writing outside the repo, interpreters run from temp directories. Default-no: anything that relocates the human's own save/profile data for a test.

**Session-only approvals that are expected to recur** (approve per session; a prefix rule cannot express them cleanly): the graceful editor quit over remote-exec Python; posting `Escape` to a Save Content modal; driving the shipped game for verification; parsing the gitignored cloud ini (the command is written not to echo the key); and editing `.claude/settings.local.json` itself (grant for ONE session to install the rules below, then let it lapse).

The permanent allow-list, as installed on the original machine (merge into `permissions.allow`; never replace the array). Appendix B carries it verbatim, ready to paste; in summary:

- `"Agent"` — subagent orchestration; the whole pipeline depends on it.
- PowerShell read probes: `Get-Process*`, `Get-FileHash*`, `Test-Path*`, `Get-ChildItem*`, `Get-Content*`, `Select-String*`.
- Editor lifecycle, granted as the two repo scripts and nothing rawer: `powershell -NoProfile -File Tools\stop_editor.ps1*` and `…\launch_editor.ps1*` in both shells and both path spellings, plus the census probe `PowerShell(Get-CimInstance Win32_Process*)`. No `Stop-Process` rule of any shape (the `-Name` rules were removed on 2026-10-04 after the law and the permission file were found pointing in opposite directions; Part 4.6).
- The maintenance tools: `Tools\sync_mirrors.ps1*`, `Tools\aura_sync.ps1*`, `C:/Python314/python.exe Tools/archive_board.py*`, `C:/Python314/python.exe Tools/split_conventions.py*`.
- Read-only git only, enumerated in both shells: `git status*`, `git log*`, `git diff*`, `git show*`, `git check-ignore*`, `git ls-files*`, `git rev-parse*`, `git rev-list*`, `git cat-file*`.
- The two UE batch files by full path, forward- and back-slash spellings: `Build.bat*`, `RunUAT.bat*`; the headless commandlet `UnrealEditor-Cmd.exe *`.
- Python scoped to this project's `Tools/`: `C:\Python314\python.exe "<repo>\GitClaudeUnrealTest\Tools\*`.
- MCP: `mcp__unreal-mcp__call_tool`, `mcp__claude_ai_Slack__slack_read_channel`, the **49 enumerated** `mcp__unreal_inspector__*` read tools and the **32 enumerated** `mcp__unreal_editor__*` PIE/verify/screenshot/input tools (Part 12.5). Never a wildcard on either Aura server.
- `"enabledMcpjsonServers": ["unreal-mcp", "blender", "unreal_inspector", "unreal_editor"]`.

The `deny` and `ask` lists are empty: an unlisted tool such as `mcp__unreal_editor__compile_blueprint` prompts; it is not blocked.

**Fresh-machine order:** install the tooling → clone → **write the allow-list before the first agent chain** (or the first compile will stall) → run the Part 15.2 checklist → grant session-only items as they surface.

### 5.4 The two PreToolUse hooks (committed, team-wide)

`.claude/settings.json`:

```json
{
  "hooks": {
    "PreToolUse": [
      {
        "matcher": "Edit|Write",
        "hooks": [
          { "type": "command", "shell": "bash", "command": "bash \"${CLAUDE_PROJECT_DIR:-.}/.claude/hooks/guard-secrets.sh\"", "timeout": 15, "statusMessage": "Secret guard" },
          { "type": "command", "shell": "bash", "command": "bash \"${CLAUDE_PROJECT_DIR:-.}/.claude/hooks/guard-generated-dirs.sh\"", "timeout": 15, "statusMessage": "UE generated-dir guard" }
        ]
      }
    ]
  }
}
```

`.claude/hooks/guard-secrets.sh` denies any Edit/Write whose content matches a known token format (AWS `AKIA…`, GitHub `gh[oprsu]_…` / `github_pat_…`, Hugging Face `hf_…`, Meshy `msy_…`, Slack `xox[abprs]-…`, Anthropic/OpenAI `sk-…`, Google `AIza…`, JWTs `eyJ…eyJ`, PEM private keys) or a credential-keyword assignment (`SecurityToken=`, `ApiKey:` …) whose value is 16+ characters and contains a digit (so placeholders like `YOUR_PASSWORD_HERE` pass). It scans the raw hook JSON with `grep -E`, so it needs no `jq`.

`.claude/hooks/guard-generated-dirs.sh` denies any Edit/Write whose `file_path` contains a `Binaries/`, `Intermediate/`, `Saved/`, or `DerivedDataCache/` segment (either slash style). Reading those directories (e.g. `Saved/Logs` for QA) is unaffected.

Both scripts are reproduced in Appendix C. Copy them into the new repo unchanged; they are the mechanical enforcement of the secrets law and of "never hand-edit build output".

### 5.5 Memory and skills

- The orchestrator keeps a persistent memory at `~/.claude/projects/<project-slug>/memory/`: one fact per file with frontmatter (`name`, `description`, `type: user|feedback|project|reference`), plus `MEMORY.md` as a one-line-per-file index that is loaded every session. Lessons that matter across sessions (Norton, uv, the Build.bat lie, the editor-close grant, the Blueprint-save trap…) live there. Never rewrite a memory file with a truncating write; append or edit (two files were destroyed that way).
- Project-wide law lives in the repo instead: `CLAUDE.md` for the orchestrator, `.claude/pipeline/CONVENTIONS.md` for the team's case law.
- The only project-local command the game repo needs is `/ship` (Part 14). Skills from the vault (Obsidian, browser automation, MCP builder) are optional and live outside the repo.

### 5.6 Verify

Launch `claude` in the project folder, run `/mcp`: all four local servers (`unreal-mcp`, `blender`, `unreal_inspector`, `unreal_editor`) must show connected when the editor and Blender are up. Ask *"Show me the task board status"*: the orchestrator should answer from `TASKBOARD.md`. Try to write a file containing a fake `hf_` token: the secret guard must refuse.

---

## Part 6 — The Claude agent team: roster, pipeline files, routing law, and which agents to create

This is the heart of the system. One Claude Code session is the **orchestrator**; seven specialist **subagents** do the work; they communicate only through **shared files**. A fresh Claude can re-scaffold all of it from this part.

### 6.1 The three layers

1. **Orchestrator** — the session you talk to. It NEVER does specialist work; it routes tasks between agents, relays file paths, enforces the pipeline, reads Slack at checkpoints, remuxes recordings, and reports. Its law is `CLAUDE.md` in the project folder (loaded automatically when Claude Code starts there).
2. **Specialist agents** — one markdown file each in `.claude/agents/<name>.md`: YAML frontmatter (`name`, `description`, optional `tools` allow-list, optional `model` and `effort`) plus a body of role instructions. The `description` is what the orchestrator matches when routing: write it like a job posting, not a bio. **Least privilege:** `tools:` grants each agent only what its job needs; an agent with no `tools:` line inherits everything.
3. **Tool layer** — MCP servers (Unreal, Blender, Aura ×2, the Slack and Supabase connectors) plus Bash and file tools.

### 6.2 The roster

| Agent | Does | Never does | `tools:` posture | Model |
|-------|------|-----------|------------------|-------|
| **manager** | Splits requests/GDD into tasks on the board, owns naming conventions, the only agent that posts top-level in Slack | Code, art, engine access | `Read, Write, Edit, Grep, Glob` + 4 Slack tools | inherits |
| **gameplay-programmer** | UE C++ / Blueprint / editor-Python logic via files + Unreal MCP | Art prompts, compiling, Git | no `tools:` line (inherits all) | inherits |
| **art-director** | Models/textures in Blender MCP and the Part 8 pipelines, imports to `Content/` with the exact specced names, UMG layout, authors Fab requests | Gameplay code, integration, Git | no `tools:` line | inherits |
| **qa-reviewer** | Reviews code BEFORE it compiles; writes `Verdict: PASS/FAIL` reports; may INSPECT Blueprint graphs and the editor log | Editing code, engine mutation, Git | `Read, Grep, Glob, Write, Edit` (Edit scoped to its own row and report) + the 49 enumerated `unreal_inspector` read tools + 4 Slack tools | inherits |
| **build-master** | Compiles (`Build.bat`), relaunches the editor, assembles assets + code in the scene via Unreal MCP, runs the suite, copies evidence, commits by pathspec | Writing new code/art, pushing | no `tools:` line | inherits |
| **footage-analyst** | Extracts frames from gameplay videos, writes evidence-backed `VID-###` diagnoses | Editing anything, engine, Git | `Bash, Read, Grep, Glob, Write` + 4 Slack tools (no MCP) | inherits |
| **playtest-verifier** | Drives Aura PIE verification, writes `qa/TASK-###-verify.md` | Editing code/art, compiling, Git, editor lifecycle | `Read, Grep, Glob, Write, Edit` + 49 inspector + 32 editor names + 2 Slack tools | **`claude-opus-5-5[1m]`, `effort: medium`** (pinned in frontmatter; dispatch without a `model` parameter) |

The three agents without a `tools:` line hold everything, which is the design: the programmer, the artist, and the integrator need Bash, files, and all four MCP servers. The reviewers and analysts are fenced by construction.

### 6.3 The seven agent definitions (what each file says)

The frontmatter `description`s, verbatim (copy them; the body outlines follow):

- **manager:** "Product Owner. Breaks down the game design document (GDD) or any feature request into small, bite-sized tasks on the task board, assigns them to the right specialist, and enforces asset naming conventions. ALWAYS use this agent FIRST when the user requests a new feature, system, or asset — before any programmer or artist work begins."
- **gameplay-programmer:** "Writes Unreal Engine C++ code, Blueprint logic, and editor Python scripts. Focuses entirely on gameplay logic, math, and engine API calls. Use for any task assigned to gameplay-programmer on the task board, and for fixing code that failed QA review. Never handles art, textures, models, or asset generation prompts."
- **art-director:** "Handles visual style, UI layout, and 3D/2D asset creation. Creates models and textures in Blender, exports them, and imports them into the correct /Content folders in Unreal with correct naming. Use for any task assigned to art-director on the task board. Never writes gameplay code."
- **qa-reviewer:** "Critiques code written by the gameplay-programmer BEFORE it compiles. Safety filter that catches deprecated UE APIs, logic errors, missing null checks, and naming convention violations, then writes a pass/fail report. Use whenever a task reaches ready-for-qa status. Never edits code itself."
- **build-master:** "Manages Git, file structure, compilation, and engine assembly. Integrates the Artist's assets with the Programmer's code in the scene (e.g. attaching scripts to models), runs builds, and commits completed work. Use when a task chain reaches qa-passed / ready-for-integration, or for any Git operation."
- **footage-analyst:** "Reviews screen-recorded gameplay footage dropped in testvideo/ (git root) by extracting frames with Tools/VideoReview/extract_frames.py, correlates Jonathan's issue description to timestamped pixel evidence, and writes an evidence-backed VID-### diagnosis report that the manager turns into fix tasks. Use when Jonathan provides a gameplay video (or asks for footage review) together with an issue description. Never edits code, never touches the editor or engine, never runs Git."
- **playtest-verifier:** "Runs Aura's Play-In-Editor verification against a task's acceptance criteria and writes an evidence-backed runtime report. Use when a code task is qa-passed and its spec names a runtime-observable acceptance criterion. Never edits code or assets, never compiles, never runs Git."

Body outlines (every body has the same four beats: role, inputs, how you work, how you finish):

- **manager**: inputs = the request or `Docs/GDD.md`, `CONVENTIONS.md` (owns and enforces), `TASKBOARD.md`. Writes every task with the board template: unique `TASK-###` (incrementing from the highest), ONE assignee, a spec small enough for one session, exact names from CONVENTIONS, `blocked-by:`, `parallel-safe:`. Rules: one task = one owner = one deliverable (never bundle code + art); any new asset type gets its naming pattern added to CONVENTIONS BEFORE the task is issued; every code task implies QA; every chain ends in a build-master integration task; replies with task IDs, dependency order, and what can start now. Slack: only top-level poster; prefix `📋 MANAGER:`. GDD mode: milestones of ~5–15 tasks, `## Milestones` at the top of the board with `current / pending / done`, decompose only the current one, playtest feedback becomes tasks in the current milestone.
- **gameplay-programmer**: reads only its own rows; C++ in `Source/<Project>/`, Blueprint via Unreal MCP against the running editor; references assets by the exact spec paths even before they exist; never compiles, builds, or touches Git; never marks its own work passing. Finishes with `handoffs/TASK-###-programmer.md` (what changed, files, assets referenced, what QA should scrutinize) and status `ready-for-qa`. On QA failure fixes every finding or justifies a false positive in the handoff.
- **art-director**: Blender via `bpy` through `execute_blender_code`; exports to `Content/RawAssets/<Name>.fbx`; imports via Unreal MCP to the spec folder; the Part 8 pipeline stages and laws (HF_TOKEN env-only, 30 s bridge cap, pre-import gate, same-path overwrite, two-slot materials, Nanite off, lane isolation); Fab requests; never wires Blueprints or touches Git. Finishes with `handoffs/TASK-###-artist.md` and status `ready-for-integration`.
- **qa-reviewer**: checks deprecated APIs (valid for 5.8), correctness, null safety (`IsValid`, casts, delegates, timers), UE correctness (`UPROPERTY`/`UFUNCTION`, `TObjectPtr`, header/cpp consistency), performance smells, conventions, and treats `Tools/**/*.py` and `*.ps1` as CODE (secret handling, timeouts, write confinement). Output `qa/TASK-###.md`: `# QA Report — TASK-###`, `Verdict: PASS | FAIL`, `## Findings` as `[BLOCKER|WARN|NIT] file:line — issue — fix`, `## Notes for build-master`. FAIL iff ≥1 BLOCKER ("a false pass costs an engine crash; a false fail costs one review cycle"). Flips its own status to `qa-passed`/`qa-failed`; posts `🔍 QA:` in Dev & QA. A `qa-passed` is a TEXT-level verdict: hashes, binary assets, compile results and test counts are accepted-as-declared, and the build-master is the first role that can and must measure them.
- **build-master**: gates (QA PASS for code, `ready-for-integration` for art, smoke tests for tooling, no secret in any commit or log); compiles with the canonical `Build.bat` line and parses `Result:`; on failure does NOT fix code but sets `qa-failed` and appends the errors to the QA report; relaunches the editor (Part 4.6); assembles over Unreal MCP; verifies in-editor; commits by explicit pathspec with message `TASK-###: <summary>`; pushes only if asked; flips rows to `done` + hash; writes `handoffs/TASK-###-buildmaster.md`; posts `🔧 BUILD-MASTER:` in Build & Git.
- **footage-analyst**: Part 9.3, with the report template and the ≤40-image-read budget; posts `🎬 FOOTAGE-ANALYST:` in Footage Review; never edits the board.
- **playtest-verifier**: Part 12.6, with the report template, verdict derivation, board flips, the process-census-first rule, the recipe library, the "Read the PNG" rule, and the recording duty; posts `🎮 VERIFIER:` in Dev & QA.

Every dispatch prompt to any agent carries its Slack duty: channel ID, its domain `thread_ts`, and at least one completion-or-blocker post (Part 7).

### 6.4 The pipeline files (how agents "talk")

Subagents cannot talk to each other. They communicate through `.claude/pipeline/`, with the orchestrator relaying paths:

| File | Role |
|------|------|
| `TASKBOARD.md` | the hub: every LIVE task, its assignee, status, blockers, spec, exact names. Kept small by law: at every milestone checkpoint `python Tools/archive_board.py --apply --rows` moves every section whose rows are all terminal, and every terminal row out of a section that still has live rows, into `archive/` byte-for-byte (one italic note is left under the section heading) |
| `archive/` + `archive/INDEX.md` | finished rows and sections. A `TASK-###` missing from the board is looked up with `grep -rn "TASK-###" .claude/pipeline/archive/`; a `gate:`/`blocked-by:` pointer to an archived row resolves there; the next new id increments from the highest id across board AND archive |
| `CONVENTIONS.md` | the naming tables, the generic process law (quiet-module, git hazards, relayed diagnosis, textured-mesh law…) and a generated `## Law index`, owned by the manager |
| `law/<NAMESPACE>.md` | every feature wave and every namespace, whole (`VER.md`, `SHIP.md`, `PKG.md`, `FR.md`, `ACC.md`, `TL.md`, `KBD.md`, …, `NNN-<slug>.md` for untagged waves). New law for a namespace goes into its file; a new wave may be written in `CONVENTIONS.md` and moved out with `python Tools/split_conventions.py --apply`; `--reindex` after editing any law file. Cite by tag, never a bare § |
| `handoffs/TASK-###-<role>.md` | per-task completion notes passed downstream (`-programmer`, `-artist`, `-buildmaster`, `-manager`) |
| `qa/TASK-###.md` | QA reports (`Verdict: PASS/FAIL`) |
| `qa/TASK-###-verify.md` | runtime verification reports (line 1 = `Verdict:`) |
| `footage/VID-###-<symptom>.md` | footage diagnoses |
| `playtest-evidence/<YYYY-MM-DD>/*.png` | the only promoted frames (LFS) |
| `fab/FAB-REQUESTS.md` | the marketplace request ledger (agents author, the human fulfils) |
| `SLACK.md` | the Slack protocol and standing-thread registry (Part 7) |

**`TASKBOARD.md` head, verbatim:**

```markdown
# Task Board — <Project>

The shared communication hub for the agent team. The **manager** creates tasks here; assignees update their status; the orchestrator routes based on status.

## Status flow

`backlog` → `in-progress` → `ready-for-qa` (code) → `qa-passed` / `qa-failed` → `built` (C++ only: compiled + editor relaunched, no commit) → `verified` / `verify-failed` (only rows whose spec names a runtime acceptance criterion; Blueprint/asset rows go `qa-passed` → `verified` directly; `UNOBSERVABLE` leaves the status where it is and appends `verify: unobservable`; rows with no runtime criterion skip straight to `integrating`) → `integrating` → `done`
Art tasks skip QA: `backlog` → `in-progress` → `ready-for-integration` → `integrating` → `done`

## Task template

### TASK-000 — <short title>
- assignee: gameplay-programmer | art-director | build-master
- status: backlog
- blocked-by: (task IDs or none)
- parallel-safe: yes | no
- spec: >
    What to build, acceptance criteria.
- names: >
    Exact class/asset names + /Game/ paths to use (from CONVENTIONS.md).
```

Also legal: `blocked` (an outage); row annotations `verify: unobservable`, `verify: partial (n/m observable)`, `verify: measured — <finding>`; done rows read `done — COMMITTED <hash>`; milestone statuses `current / pending / done`. In practice rows grew a `[TAG] emoji **title** (assignee) — marker` heading and a `GATE: TASK-###` pointer inside `names:`.

**Write discipline** (the board has no lock and up to ten live agents edit it): (1) a status flip is not a spec edit; an assignee edits ONLY its own `status:` line, and `spec:` / `names:` / `blocked-by:` / `parallel-safe:` / creation / renumbering are the manager's; (2) edit the smallest anchor, one line; (3) re-read immediately before a dependent edit; (4) grep your marker back after writing; (5) never "tidy" another row; (6) a dispatch is not a board entry, so board retroactively and mark `⛔ do-not-re-dispatch`; (7) a scheduling fact never goes in `status:`; resolve "is it committed" against git (`git log --oneline --grep=TASK-###`, anchored at the git root); (8) a commit carrying N task IDs owes N status flips in the same action.

**`CONVENTIONS.md`** opens with the owner line ("Owned by the **manager** agent. All agents MUST follow these. If a needed pattern is missing, the manager adds it here BEFORE issuing the task.") and the tables a new game copies on day one:

```markdown
## Asset prefixes (Content Browser)

| Prefix | Type | Folder |
|--------|------|--------|
| BP_    | Blueprint class | Content/Blueprints/ |
| WBP_   | UMG widget | Content/UI/ |
| SM_    | Static mesh | Content/Meshes/ |
| SK_    | Skeletal mesh | Content/Characters/ |
| M_     | Material | Content/Materials/ |
| MI_    | Material instance | Content/Materials/Instances/ |
| T_     | Texture | Content/Textures/ |
| NS_    | Niagara system | Content/VFX/ |
| S_     | Sound | Content/Audio/ |
| DT_    | Data table | Content/Data/ |
| DA_    | Data asset (UDataAsset instance) | Content/Data/ |
| L_     | Level | Content/Maps/ |
| LS_    | Level Sequence (Sequencer) | Content/Cinematics/ |
| ABP_   | Anim Blueprint | Content/Characters/ |
| A_     | Anim Sequence | Content/Characters/Anims/ |
| AM_    | Anim Montage | Content/Characters/Anims/ |
| SKEL_  | Skeleton (shared rig) | Content/Characters/ |
| IK_    | IK Rig (retargeting) | Content/Characters/ |
| RTG_   | IK Retargeter | Content/Characters/ |
| IA_    | Input action | Content/Input/Actions/ |
| IMC_   | Input mapping context | Content/Input/ |

## Texture suffixes
`T_<Name>_D` base color · `T_<Name>_N` normal · `T_<Name>_R` roughness · `T_<Name>_M` metallic · `T_<Name>_E` emissive · `T_<Name>_ORM` packed Occlusion/Roughness/Metallic (LINEAR — sRGB off)

## C++ (Source/<Project>/)
- Classes: `A` actors, `U` UObjects/components, `F` structs, `E` enums, `I` interfaces
- One class per header/cpp pair; file name = class name without prefix
- Exception: pure data types (enums, structs, UInterfaces) may share a header-only file when they form one concept
- Gameplay-relevant members exposed with UPROPERTY/UFUNCTION; use TObjectPtr for UObject members
- All new gameplay code lives in `Source/<Project>/<Game>/`; never modify template code (subclassing is allowed)

## Blueprint subclasses of C++ classes
- Name = `BP_` + C++ class name without its prefix: `BP_HeroCharacter` (from `AHeroCharacter`)

## Widgets with C++ bases
- `U<Name>Widget` in C++ ↔ `WBP_<Name>` in Content/UI/, reparented to it; widget-facing events are BlueprintImplementableEvents with float/int/bool/byte/FString params only (never enums)

## Delegates (C++)
- `FOn<Owner><Event>`, member `On<Owner><Event>`; broadcast on every ACTUAL value change; UI consumers seed from a getter first, THEN bind

## Logging (C++)
- Gameplay log categories are named `Log<Game><Domain>`, one line per decision for anything an acceptance criterion will grep

## Data-driven stats
- One CSV in `Docs/Data/` is the source of truth, imported as `DT_<Name>`; never hardcode a stat that exists in the table; mechanic RULES are UPROPERTY defaults on the owning class with a `// GDD §x.x` comment

## Template-donor rule
- Template content is READ-ONLY: soft-reference it or duplicate into /Game/ and modify the duplicate

## Numbering
Variants use two digits: `SM_Rock_01`, `SM_Rock_02`

## Cross-discipline rule
The task spec's `names:` block is the single source of truth. Programmer code references and Artist asset names must BOTH come from it, character-for-character.
```

CONVENTIONS then grows into case law: each feature wave adds a dated section with its own namespace (`FR-§` footage review, `VER-§` verification, `PKG-§` packaging, `SHIP-§` the ship command, `ACC-§` accounts, `TL-§` tooling hazards, `SC-§` standing process clauses, …) that task specs CITE instead of restating. Appendix E lists the namespaces. Three maintenance laws learned the hard way (2026-10-04): **keep the law split** (Siegebound's single file reached 12,617 lines before it was split into a 460-line core plus 56 law files; the citation rule is what makes the split free), **amend by rewriting** (a clause is rewritten to its current truth plus one dated `_Changelog: …_` line; strikethrough-forever is retired for law text, because a reader of a five-times-amended clause had to parse every layer to learn the current rule; evidence files stay append-only), and **archive the board** at every checkpoint (Siegebound's reached 43,560 lines before the first archive pass halved it).

### 6.5 The routing law (put this in `CLAUDE.md`, adapted)

1. **Every feature request goes to `manager` first.** No exceptions; it returns task IDs. The one recorded exception: when the human drops a video in `testvideo/` and describes an issue, dispatch `footage-analyst` directly with the verbatim description and the next `VID-###`; when the report lands, route its path to the manager to board fixes.
2. Dispatch per the board: `gameplay-programmer` and `art-director` tasks marked `parallel-safe: yes` with no blockers launch **in parallel** (one message, several Agent calls).
3. A code task hits `ready-for-qa` → invoke `qa-reviewer`.
4. `qa-failed` → back to `gameplay-programmer` with the QA report path. Loop until `qa-passed` (**max 3 loops, then escalate to the human**).
5. `qa-passed` → the compile / verify / commit chain:
   - **5a** `build-master` compiles (`Result: Succeeded`) and, for C++ changes, relaunches the editor on the new binaries (`Tools/stop_editor.ps1` → `Build.bat` → `Tools/launch_editor.ps1`; never Live Coding, never a raw `Stop-Process`). Status → `built`. No commit yet.
   - **5b** if the spec has a runtime acceptance criterion → `playtest-verifier`. `verify-failed` → back to the programmer with the verify report path; counts as a QA loop. Blueprint/asset-only tasks skip 5a and come straight here.
   - **5c** `verified` (or `UNOBSERVABLE`, or `MEASURED`, or no runtime criterion) → `build-master` assembles and commits by explicit pathspec, recording the hash on every row it ships; a commit needs no board row of its own (Siegebound boarded a "host row" per commit until 2026-10-04 and dropped the ceremony once the editor's auto-stage trap was gone). `MEASURED` routes like `UNOBSERVABLE` but is not it: it never blocks and never bounces, and it is earned by a control that discriminated, whereas `UNOBSERVABLE` means the lane could not see at all.
6. Build failure → build-master appends errors to the QA report; back to the programmer (counts as a QA loop).
7. Report the outcome to the human with task IDs and commit hashes.
8. The bare word **"ship"** from the human → run `/ship` (Part 14); every gate is a STOP and an `ADJUDICATE` is not a ship.

**Hard gates:** nothing commits without a PASS QA report (code) or a completed integration check (art); nothing with a runtime acceptance criterion commits without a VERIFIED report (UNOBSERVABLE and MEASURED are recorded on the row, not treated as a pass); Aura verification is announced when the human is present and reported, with no wait for a go (his standing grant); never push unless asked; the editor is closed and relaunched only through the two lifecycle scripts; session start runs `Tools/sync_mirrors.ps1`; the editor must be running with MCP up for engine tasks, else report; gameplay videos are never committed; dispatch the verifier without a `model` parameter.

### 6.6 GDD mode (full automation from a design doc)

Write the game's master plan in `Docs/GDD.md` from `Docs/GDD-TEMPLATE.md` (Overview · Core Loop · Mechanics with per-mechanic **Acceptance:** lines · Characters & Enemies · Levels · Art Style & Audio · UI/UX · Progression & Economy · Milestones · Out of Scope), then say **"Read the GDD and build milestone 1 only."** The manager splits it into milestones (playable increments, ~5–15 tasks each; milestone 1 is the smallest thing you can walk around in), records them at the top of the board, fully decomposes only the first, and the pipeline runs it hands-off. At milestone completion the orchestrator **stops and reports** (what shipped, commit hashes, next milestone, open questions): that is the playtest checkpoint, and your feedback becomes new tasks in the current milestone before it is considered complete. A fresh session resumes from the board ("Continue the task board"); never re-decompose unless the GDD changed. On each finished milestone the build-master cuts a `<milestone>-testable` branch.

GDD rules that matter most: **numbers over adjectives** ("sprint = 750 units/s" gets implemented; "fast sprint" gets guessed); acceptance criteria per mechanic (QA and the verifier test against them, and they must be properties the rig can observe, Part 13); an explicit out-of-scope list; stats in a CSV, rules in code with a `// GDD §` comment. Siegebound's GDD was updated with `*[as-built YYYY-MM-DD: …]*` tags whenever shipped reality diverged, never by deleting the original aspiration.

**What to say to the orchestrator:**

| Situation | Say |
|-----------|-----|
| First run with a finished GDD | `Read Docs/GDD.md and build milestone 1 only.` |
| New session, work in progress | `Continue the task board.` |
| Check progress | `Show me the task board status.` |
| After playtesting | `Playtest notes for milestone N: <notes>. Send them to the manager.` |
| Approve moving on | `Milestone N approved — start milestone N+1.` |
| One-off feature | `Add <feature>.` (still routes through the manager) |
| A recording of a bug | drop it in `testvideo/`, then describe the issue in one message |
| A QA loop hit its limit | `Show me the QA report for TASK-NNN` → fix the acceptance wording in the GDD, not the code |

**How to report so agents can act** (from the communication note, still the best input contract): one issue per message or a numbered batch tagged `BUG` / `TUNE` / `CHANGE` / `QUESTION`; every change starts as a GDD edit, not a chat message; cite the GDD section; state expected vs actual with numbers; give a repro; scope-guard ("touch nothing else"); demand evidence, not claims. Bug template: `BUG (milestone N): <summary>` · `GDD ref: §X.X — "<criterion>"` · `Expected:` · `Actual:` · `Repro:` · `Frequency:` · `Evidence:` · `Severity:`. Change template: `CHANGE (milestone N): <summary>` · `GDD diff: §X.X — was → now` · `Reason:` · `Affects:` · `Do NOT:`. Anti-patterns: "combat feels floaty" → `TUNE: hero swing cooldown 0.5 → 0.35 s`; "fix the miner bug" → the full BUG template; a bare screenshot → screenshot plus expected-vs-actual and a §-ref.

### 6.7 Scaffolding the team on a new machine

**[Claude]** Given this part: write `CLAUDE.md` (team table, communication section, Slack law, routing rules, GDD mode, hard gates, build command), the seven `.claude/agents/*.md` files (frontmatter + body per 6.3; copy Siegebound's as templates and rename the project), and `.claude/pipeline/` (TASKBOARD with the head above, CONVENTIONS seeded with the tables above, empty `handoffs/`, `qa/` with its README, `footage/`, `fab/FAB-REQUESTS.md` with its protocol, `playtest-evidence/`, and `SLACK.md` once Part 7 is done). Add `Docs/GDD-TEMPLATE.md`.

Four pieces of drift found in Siegebound's files on 2026-10-03 were fixed on 2026-10-04, so a scaffold copied from the current files inherits the fixes: the QA report filename is `qa/TASK-###.md` everywhere; `manager.md`'s assignee list names all seven roles; `CLAUDE.md` routes the bare word "ship" to `/ship`; and the no-host-row, archive, law-split and amend-by-rewriting laws are written into `manager.md`. One choice remains yours: the three all-tools agents are fenced by prose only; if you want a hard fence on the programmer, artist, or integrator, give them a `tools:` line from the start, because adding one later means re-censusing every MCP tool they use.

First-run expectation **[You]**: the first session prompts for permission on builds, git, and MCP calls; the allow-list in Part 5.3 prevents most of it, and the rest accumulates in the gitignored `settings.local.json`.

### 6.8 Which agents to create, and when

The five-role core (planner · builder · maker-of-content · reviewer · integrator) is genre-agnostic. Siegebound grew two more because two new EVIDENCE LANES appeared: a video lane (footage-analyst) and a runtime lane (playtest-verifier). That is the rule for adding an agent: **create one when a new tool surface or evidence lane needs its own permission posture**, not when an existing agent could simply hold one more skill. Keep the invariants regardless: one task = one owner = one deliverable; naming law in CONVENTIONS before the task is issued; reviewers are read-only; only the integrator touches git; never two mutating agents on one editor at the same time; every agent's `description` says when to use it and what it never does.

Candidates proven or considered, and the trigger for each:

| Agent | Create it when | Tools posture | Notes |
|-------|----------------|---------------|-------|
| **manager, gameplay-programmer, art-director, qa-reviewer, build-master** | day one | as in 6.2 | the core |
| **footage-analyst** | the human records playtests (they should) | Bash + files + Slack, no MCP | diagnose-only; direct dispatch |
| **playtest-verifier** | Aura is installed | enumerated Aura tools, no shell | the `verified` gate; Opus 1M pin |
| **level-designer** | the game has hand-built levels (Siegebound's arena is procedural, so it never needed one) | Unreal MCP + files | greyboxing, actor placement, lighting; splits level work off the gameplay-programmer |
| **narrative-designer** + **dialogue-writer** | a narrative game | files only | designer owns the story bible and branching; writer writes lines to spec; QA grows a continuity-check duty |
| **sprite / pixel-artist** | a 2D game | image generation + Paper2D import | replaces the Blender lane; the build-master's "assemble" becomes tilemap/prefab assembly |
| **audio-designer** | the game needs more than template sounds | files + Unreal MCP (`S_` assets, MetaSounds) | Siegebound used Fab packs and the art-director; a dedicated agent earns its place when audio has its own naming law |
| **vfx-artist** | heavy Niagara work | Unreal MCP (Niagara tools) | Aura has a Niagara agent, but pipeline law says Aura's mutating agents are granted to no pipeline agent; a Niagara system duplicated via MCP is inert until opened once in the editor |
| **netcode-qa** | multiplayer | read-only + inspector | replication-specific review checklist (authority, RPC validation, bandwidth); every gameplay diff declares what replicates |
| **load-test runner** | multiplayer at scale | Bash (headless clients) | drives `-game` instances; must obey the "a `-game` instance may be the human's" census law |
| **balance-analyst** | a systems-heavy economy | files (CSV) + Python | simulates curves before the programmer implements them; owns `Docs/Data/*.csv` |
| **release-manager** | shipping regularly | Bash (RunUAT) + git | Siegebound folded this into the build-master via `/ship`; split it out when cooks run unattended |
| **data-engineer** | cloud features | the Supabase connector only | owns migrations and RLS review; Siegebound used the build-master for apply and the qa-reviewer for review |

Do not create: an "assistant" agent with every tool (the orchestrator already is one), a second mutating engine agent that would run alongside the build-master, or a verifier that cannot fail.

---

## Part 7 — The Slack mirror (team visibility channel)

One Slack channel mirrors the pipeline so the human can watch progress and drop feedback from anywhere. **Files stay the contract; Slack is visibility only, and a Slack post is never authorization.**

### 7.1 Setup

1. **[You]** Create or reuse a Slack workspace; create the team channel (Siegebound's: `#siegeboundue5agentteam`, ID `C0BF0QZP3CN`).
2. **[You]** Connect Claude to Slack: claude.ai → Settings → Connectors → add the Slack connector and authorize it into the workspace. The tools then appear in Claude Code as `mcp__claude_ai_Slack__slack_send_message`, `slack_read_channel`, `slack_read_thread`, `slack_search_channels` (and more). They are connector tools, not `.mcp.json` entries.
3. **[Claude]** Record the channel ID; write `.claude/pipeline/SLACK.md` from Siegebound's as the template; have the manager post the six standing-thread roots and record each root's `thread_ts` in the registry. **A thread is not live until its ts is recorded.**
4. Grant the Slack tools on each agent's `tools:` line (the three agents without a `tools:` line already have them). Add `mcp__claude_ai_Slack__slack_read_channel` to the permission allow-list.
5. If an agent's connector tools do not surface in a run (headless), the orchestrator **proxies the agent's post verbatim**, prefixed with the agent's identity. Proxying is the lossless fallback, never the norm; all seven agents post directly in interactive sessions (first proven posts are recorded in `SLACK.md`).

### 7.2 The protocol (v2, standing domain threads)

- **The main-chat law: top-level posts are the manager and the human ONLY.** The manager posts channel-wide announcements (milestone kickoff/wrap, protocol changes, new standing threads) prefixed `📋 MANAGER:`. The orchestrator's checkpoint reports go in Planning & Feedback; its escalations in Blockers. All agent task traffic goes in the matching domain thread. **Tasks do not get their own threads.**
- **The standing-thread registry** (Siegebound's `thread_ts` values; a new channel mints its own):

| Thread | thread_ts | Scope | Who posts |
|---|---|---|---|
| 📢 Planning & Feedback | 1783116257.317519 | milestone plans, task breakdowns, playtest-feedback processing, orchestrator checkpoint reports | manager, orchestrator, the human |
| ⚙️ Dev & QA | 1783116269.740549 | code task updates, QA verdicts (+ report path), runtime verification verdicts (`TASK-###-verify.md` path + `Verdict:` line) | gameplay-programmer, qa-reviewer, playtest-verifier |
| 🎨 Art | 1783116278.693139 | asset production, imports, integration handoffs | art-director |
| 🔧 Build & Git | 1783116286.945249 | compile results, scene assembly, commit hashes | build-master |
| 🚨 Blockers | 1783116296.221319 | anything needing the human or the orchestrator: escalations, QA-loop limit, MCP/editor outages | any agent, orchestrator; the human monitors |
| 🎬 Footage Review | 1787798959.639009 | footage dispatches, `VID-###` finding summaries + report paths | footage-analyst |

- **Identity prefixes are mandatory**: all posts share one Slack account, so the prefix IS the speaker: `📋 MANAGER:` · `🔍 QA:` · `⚙️ GAMEPLAY-PROGRAMMER:` · `🎨 ART-DIRECTOR:` · `🔧 BUILD-MASTER:` · `🎬 FOOTAGE-ANALYST:` · `🎮 VERIFIER:` · `ORCHESTRATOR:`.
- **Status emoji** follow the prefix on every task post: 🟦 dispatched · 🔧 in progress · 🧪 ready-for-qa · ✅ qa-passed / integrated / done · ❌ qa-failed / build failed / verify-failed · 🎯 verified (an `UNOBSERVABLE` posts as 🎯 with the word in the line; it never reads as ✅) · 📦 integrating / built · 🚧 blocked. The `TASK-###` is the join key when a task's life spans threads (code → QA → build).
- **Task lifecycle inside a domain thread:** `🟦 TASK-### — <title> → <assignee>` dispatch note → progress/completion posts `<emoji> TASK-### — <note>` → QA verdict in Dev & QA, build result + hash in Build & Git → `✅ TASK-### done`, or `🚧 TASK-###` plus a one-liner in Blockers.
- **Every dispatch prompt includes the agent's Slack duty**: channel ID, its domain `thread_ts`, and at least one completion-or-blocker post.
- **Slack does not wake agents.** Agents exist only while running a dispatched task. The orchestrator reads the channel at session start and every checkpoint/task boundary and routes actionable feedback to the manager; playtest notes become tasks in the current milestone. Anything urgent (stop work, change course, approve a push) goes to the orchestrator directly in Claude Code. If Slack and the files disagree, the files win.

### 7.3 Verify

The orchestrator posts a checkpoint line into Planning & Feedback and reads it back with `slack_read_thread`.

---

## Part 8 — The AI art pipeline: Blender MCP, TRELLIS.2, Meshy, FLUX concept art, rigging, import

Two readers use this part: the human (accounts, installs, concept drops) and the art-director agent (which runs the scripts). The whole pipeline lives in `Tools/ArtPipeline/` plus the two Blender-bridge and reimport scripts in `Tools/`. Copy those folders wholesale into a new repo.

### 8.0 The flow at a glance

```
concept_prompts.json                      (art direction per asset, committed)
   |  Stage 0  concept_generate.py        HF Inference API, black-forest-labs/FLUX.1-dev     [HF_TOKEN + CA bundle]
   v
Tools/ArtPipeline/Inbox/<CardID>.png      <- 2D concept drop zone (AI output or a human drop; the human's drop always wins)
   |
   |- Stage 1    trellis_generate.py      HF Space microsoft/TRELLIS.2 (free ZeroGPU)        [HF_TOKEN; run bare]
   |       -> Cache/<CardID>/trellis_raw.glb
   |
   '- Stage 1.5  meshy_generate.py        Meshy REST API (paid credits)                       [MESHY_TOKEN]
           --mode image3d     Inbox/<CardID>.png                        -> Cache/<CardID>/meshy_raw.glb
           --mode multiimage  Inbox/<CardID>_{Front,Side,Back}.png      -> Cache/<CardID>/meshy_raw.glb
           --mode retexture   donor GLB + Inbox/<CardID>.png            -> Cache/<CardID>/meshy_retex.glb
   |
   v  Stage 2   refine_trellis_glb.py     HEADLESS Blender 5.1: cleanup -> conform -> remesh -> UV -> bake D/N/ORM -> FBX
Content/RawAssets/<CardID>.fbx  +  Content/RawAssets/Textures/<CardID>/T_<CardID>_{D,N,ORM}.png  (both LFS-tracked, committed)
   |
   |- Stage 3   Unreal import             new asset: art-director over Unreal MCP (editor OPEN)
   |                                      same-path overwrite: Tools/reimport_meshes.py commandlet (editor CLOSED),
   |                                      then Tools/reimport_apply_materials_mcp.py over MCP (editor OPEN)
   |       -> /Game/Meshes/SM_<CardID>, /Game/Textures/T_<CardID>_*, /Game/Materials/Instances/MI_<CardID>_PBR
   |
   '- Stage 3b  rig_character.py          HEADLESS Blender: SM_<CardID>.fbx -> shared 21-bone SiegeBiped rig + Idle/Walk/Attack/Death
           Stage 3c  retarget_meshy_to_siegebiped.py   Meshy Mixamo-style clips -> SiegeBiped (bypasses UE 5.8's broken retarget export)
           Stage 4   SK import in the editor (MCP): SK_<CardID>, A_<CardID>_*, AM_ montage, ABP_ authored in-editor
Procedural props (no AI)  build_watchtower.py, build_warroom_props.py, build_entry_dressing_props.py  (headless Blender from literals)
```

Why two mesh engines: TRELLIS.2 is free, fast, MIT-licensed and the default; its weaknesses are dark albedo (shading baked into color), small holes and dense triangulation. Meshy is paid and used per asset: to retexture a TRELLIS donor from the concept (fixes the dark look), for a second opinion on hero assets, for multi-view characters (a single front view makes the solver invent the back), and for its Mixamo-style animation clips. **The invariant:** no engine's output ever lands in `Content/` directly; everything re-enters through Stage 2, so the Stage 2/3 laws hold regardless of engine.

### 8.1 Blender MCP (the art-director's live modeling hands)

The working setup is Blender 5.1's **official built-in MCP add-on** (a TCP server on `localhost:9876`) plus a **repo-owned stdio bridge** that Claude Code launches. Two known-wrong routes:

- ⛔ NOT `uvx blender-mcp` (the third-party PyPI package, ahujasid). Same port, incompatible protocol; it blocks on a telemetry-consent handshake and hangs Claude Code to its 30 s connection timeout.
- ⛔ NOT the add-on's own bundled `mcp_bridge.py`. It speaks only Content-Length (LSP) framing; Claude Code uses newline-delimited JSON-RPC, so `initialize` is never answered.
- ✅ `Tools/blender_mcp_bridge.py` auto-detects the client's framing on the first message and replies in kind (works for Claude Code and Claude Desktop), forwarding to port 9876 with the add-on's null-byte-delimited JSON protocol. Stdlib only; any Python 3.10+ runs it.

Setup:

1. **[You]** Install Blender 5.1 (the Lab line that bundles the MCP add-on; 5.1.2 on the original machine).
2. **[You]** Edit → Preferences → Add-ons → enable **MCP**.
3. **[You]** Edit → Preferences → System → enable **Online Access**. The add-on refuses to start its socket server without it. The server auto-starts about one second after launch; verify under Preferences → Add-ons → MCP ("Server is running").
4. Copy `Tools/blender_mcp_bridge.py` into the new repo.
5. Add the `blender` server to `.mcp.json` (paths as on the original machine):
   ```json
   "blender": {
     "command": "C:/Program Files/Blender Foundation/Blender 5.1/5.1/python/bin/python.exe",
     "args": ["-u", "-X", "utf8", "C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Tools/blender_mcp_bridge.py"],
     "env": { "PYTHONUTF8": "1", "PYTHONIOENCODING": "utf-8" }
   }
   ```
   Add `"blender"` to `enabledMcpjsonServers`, then fully restart Claude Code.

Operating rules:

- The add-on exposes exactly three tools: `execute_blender_code` (arbitrary `bpy` Python; **to read anything back the code must assign a JSON-serializable dict to a variable named `result`**), `get_scene_info`, and `get_object_info`. Everything (meshes, modifiers, materials, exports) is written as `bpy` code. Keep each call idempotent (delete a prior object of the same name before recreating it).
- **The live bridge has a 30 s socket cap**; it is for quick inspection and preview. Heavy work (remesh, decimate, bakes) runs headless: `blender.exe --background --factory-startup --python-exit-code 1 --python <script> -- <flags>`.
- Blender must be open with the server up for art tasks; if it is not, the art-director reports the outage, never fakes asset creation.
- Run only ONE Claude client against Blender at a time; the socket is single-threaded. Repeated timeouts orphan `python`/`uv` processes that hammer the port; kill the stragglers (never `blender.exe`).
- Verify: with Blender open, ask Claude for `get_scene_info`; a scene summary proves Claude → bridge → 9876 → Blender.

### 8.2 One-time setup for the scripts (uv, Python 3.12, tokens)

1. **[You]** Install **uv** (astral.sh/uv). Under TLS interception set the `UV_SYSTEM_CERTS=true` User environment variable first.
2. **[Claude]** From `Tools/ArtPipeline/`:
   ```powershell
   uv sync
   ```
   `pyproject.toml` pins `requires-python = ">=3.12,<3.13"` with dependencies `gradio_client`, `huggingface_hub`, `pillow` (`[tool.uv] package = false`; `.python-version` = `3.12`; `uv.lock` committed). uv downloads its own managed Python 3.12 (`gradio_client` is not validated on 3.14; the system Python is never touched) into `%APPDATA%\uv\python\` and installs into the gitignored `.venv/`. Delete `.venv/` and re-run `uv sync` whenever it looks wrong.
3. **[You]** Hugging Face: create a token at huggingface.co → Settings → Tokens (**read scope is enough**) and store it ONLY as the `HF_TOKEN` User environment variable (GUI route, Part 1.3). Then **accept the FLUX.1-dev license** on the model page `black-forest-labs/FLUX.1-dev` while logged in, or Stage 0 returns HTTP 403 (it is a gated repo under the FLUX.1 [dev] Non-Commercial License). Consider HF PRO before any multi-asset TRELLIS batch.
4. **[You]** Meshy: create an account at meshy.ai, buy credits (the team used the Pro plan), generate an API key (prefix `msy_`), store it ONLY as the `MESHY_TOKEN` User environment variable. `MESHY_API_KEY` is accepted as a deprecated alias, and on Windows the script falls back to reading `HKCU\Environment` via `winreg` so shells opened before the variable existed still work.
5. **[You]** Open a NEW terminal.

**The token law (binding):** `HF_TOKEN` and `MESHY_TOKEN` are ENV-ONLY: never in a file, never on argv, never in chat, logs, or handoffs. Every script redacts the live value and the generic token shapes (`hf_…`, `msy_…`) from all output and exits `2` with the setup steps when the variable is unset. Agents only ever check that the variable EXISTS. The secret-guard hook (Part 5.4) refuses any file write that contains one.

### 8.3 Stage 0 — concept art (`concept_generate.py`)

```powershell
uv run concept_generate.py --check            # deps, prompt schema, degeneracy-guard control, HF_TOKEN presence; tokenless
uv run concept_generate.py --check --probe    # + tokenless model reachability
uv run concept_generate.py Footman            # one CardID -> Inbox/Footman.png
uv run concept_generate.py --all              # every CardID in concept_prompts.json
uv run concept_generate.py Witch --seed 71032 --force
```

- Model `black-forest-labs/FLUX.1-dev` through `huggingface_hub.InferenceClient(...).text_to_image` (host `router.huggingface.co`). Defaults 1024×1024, 40 steps, guidance 3.5. FLUX is guidance-distilled, so negative prompts are recorded but not sent. The tool appends a shared suffix to every prompt: single centered subject, full body, ¾ view, plain grey background, even studio lighting, no ground shadow, no props, no text.
- `concept_prompts.json`: `{ "prompts": { "<CardID>": { "prompt", "negative_prompt"?, "seed"? } } }`; authors describe the subject only; seeds 71001+ per card.
- Output is written temp → validated (black/uniform/unlit guard) → `os.replace`, so a bad frame never overwrites a good `Inbox/<CardID>.png`; rejects go to `Inbox/_rejected/`.
- **Norton:** `router.huggingface.co` is not excluded on the original machine, so this stage needs `SSL_CERT_FILE=<combined CA bundle>` (8.6). Run bare once to capture the verbatim failure, then with the bundle.
- Multi-view characters: the human (or the tool) provides `Inbox/<CardID>_Front.png`, `_Side.png`, `_Back.png`.
- Accepted concepts are archived to `Content/RawAssets/Concepts/<CardID>.png` as the color-fidelity reference. Filename IS the asset ID (PascalCase: `Footman.png`, `CrystalTower.png`).

Good input image: one subject, plain or transparent background, front three-quarter view, no ground shadow, subject fills the frame, square ~1024×1024 PNG, neutral even lighting.

### 8.4 Stage 1 — TRELLIS.2 (`trellis_generate.py`)

```powershell
uv run trellis_generate.py --check                 # tokenless: Client signature, Space reachability, 3 endpoints; rewrites Cache/api_schema.json
uv run trellis_generate.py Footman                 # Inbox/Footman.png -> Cache/Footman/trellis_raw.glb (+ state.json)
uv run trellis_generate.py Footman --seed 7        # reroll a bad generation
uv run trellis_generate.py Castle --resolution 1536
```

- `SPACE_ID = "microsoft/TRELLIS.2"`, endpoints `/preprocess_image` → `/image_to_3d` → `/extract_glb`, run atomically on ONE `gradio_client.Client` (the Space's state is session-coupled; never split a run). Defaults: seed 0, resolution `"1024"` (hero assets pin `"1536"` in the manifest), decimation target 500,000, texture 2048, timeout 30 min, 3 attempts with 20/60/180 s backoff.
- Runs BARE (the `huggingface.co` + `*.hf.space` exclusions cover it).
- Quota: free ZeroGPU ≈ 5 GPU-minutes/day ≈ 1–2 assets; HF PRO ≈ 40 GPU-minutes/day.
- Output GLB is validated (magic, chunks, ≥4 verts/tris) before `os.replace`; rejects quarantine under `Cache/<CardID>/_rejected/`.
- Manual browser fallback: open the Space in a browser while logged in, upload the concept, generate, download the GLB (~500k decimation, 2048 texture), drop it at `Cache/<CardID>/trellis_raw.glb`; Stage 2 proceeds, with "manual browser run" noted as provenance.

### 8.5 Stage 1.5 — Meshy (`meshy_generate.py`)

```powershell
uv run meshy_generate.py --check                                   # REQUIRES the key: balance + endpoint probes, spends no credits
uv run meshy_generate.py --mode image3d   Footman                  # Inbox/Footman.png -> Cache/Footman/meshy_raw.glb
uv run meshy_generate.py --mode multiimage MainCharacter           # Inbox/MainCharacter_{Front,Side,Back}.png -> meshy_raw.glb
uv run meshy_generate.py --mode multiimage Ogre --views Front,Side,Back,Top
uv run meshy_generate.py --mode retexture Footman --donor trellis  # donor GLB + Inbox/Footman.png -> meshy_retex.glb
uv run test_meshy_multiimage.py                                    # offline gate for the multi-view mode (no key, no network)
```

- `API_BASE = https://api.meshy.ai`; endpoints `/openapi/v1/balance`, `/openapi/v1/image-to-3d`, `/openapi/v1/multi-image-to-3d`, `/openapi/v1/retexture`. Stdlib `urllib` with `ssl.create_default_context()` (honours `SSL_CERT_FILE`; verification never disabled). `--ai-model latest` by default; also `--topology {triangle,quad}`, `--target-polycount`, `--no-pbr`, `--no-preserve-uv`, `--timeout-minutes`, `--poll-seconds`, `--attempts`.
- `--views` may ADD a view but never drop `Front`/`Side`/`Back` (exit 64); a named view missing from `Inbox/` is exit 5 and nothing is sent; a 403/404 on the multi-image endpoint is exit 4 and never silently falls back to `image3d`.
- Provenance is merged into `Cache/<CardID>/state.json` (engine, task id, input hashes, credits, view order) without clobbering the TRELLIS record.
- Meshy also supplies Mixamo-style animation preset clips consumed by Stage 3c; they are dropped at `Content/RawAssets/Characters/Meshy/<CardID>/<CardID>_<Clip>.fbx`.

**Shared exit codes** for `concept_generate.py`, `trellis_generate.py`, `meshy_generate.py` (a contract the orchestration reads):

| Code | Meaning | Action |
|------|---------|--------|
| 0 | success (or `--check` passed) | — |
| 1 | generic failure after retries | read the surfaced error |
| 2 | token / API key not set | set the User env var, open a NEW terminal |
| 3 | quota or credits exhausted (Meshy HTTP 402) | **expected pause**: the message carries the reset time; record it and stop, never retry |
| 4 | API drift (endpoint changed shape) | check `Cache/api_schema.json`; update the client; use the manual fallback meanwhile |
| 5 | input missing (concept PNG, donor GLB, a named view) | fix the Inbox |
| 6 | degenerate artifact rejected (black/uniform frame, empty GLB) | reroll with another `--seed` |
| 64 | CLI usage error | (remapped from argparse's 2 so `2` uniquely means "no token") |

### 8.6 The Norton CA bundle (for hosts that are not excluded)

`Tools/ArtPipeline/Cache/_certs/win-ca-bundle.pem` is `certifi`'s bundle plus the machine's trust store (about 115 certificates including the current Norton root). Regenerate it from the live Windows store whenever the fallback itself starts failing (Norton rotates its root; the 2026-07-26 bundle was stale by 2026-08-01), keeping the old one as `.bak-<date>`. Use it as `SSL_CERT_FILE=<path>` for Stage 0; use `curl` (Windows Schannel, OS trust store) for large-file downloads that redirect to `us.aws.cdn.hf.co`; run TRELLIS bare. Never widen a token-stripping host allowlist to route around a certificate error.

### 8.7 Stage 2 — headless Blender refine (`refine_trellis_glb.py`)

```powershell
& "C:\Program Files\Blender Foundation\Blender 5.1\blender.exe" --background --factory-startup --python-exit-code 1 --python Tools\ArtPipeline\refine_trellis_glb.py -- --card-id Footman
```

Flags: `--card-id <ID>` (required; a key of `pipeline_manifest.json`), `--input <glb|fbx>` (default `Cache/<CardID>/trellis_raw.glb`; point it at `meshy_raw.glb` or `meshy_retex.glb` for Meshy output), `--mode bake|native`, `--manifest`, `--smoke` (writes confined to `Cache/<CardID>/smoke/`), `--quick`, `--save-blend`. (The vault's older art note says `--asset`; that flag does not exist.)

Bake mode does: IMPORT → CLEANUP → CONFORM (pre-rotate so the front faces Blender −Y, scale to the manifest's `target_dims_ue`, origin feet-center or ground-center) → SPLIT → REMESH/decimate to the tri budget → UV (`smart_project`, layer named exactly `UVMap`) → BAKE on Cycles CPU (Diffuse → `T_<CardID>_D`, Normal → `T_<CardID>_N`, AO 64 spp + Roughness + Metallic → packed `T_<CardID>_ORM`, R=AO G=Rough B=Metal, linear) → DELIGHT (albedo AO-divide + gamma/gain, the dark-TRELLIS fix) → two material slots `[TeamRegion, <CardID>PBR]` → UCX collision hulls for buildings → FBX export (`axis_forward='-Z'`, `axis_up='Y'`, `apply_unit_scale=True`, with Y-mirror/winding pre-compensation for UE's import) → REPORT (`Cache/<CardID>/refine_report.json` + Workbench previews). `native` mode keeps the engine's mesh/UVs/textures (WebP → PNG) and still conforms, splits, exports.

`pipeline_manifest.json` (29 assets): per asset `category` (unit | building | castle-prop), `mode`, `tri_budget` (units 15,000; buildings 20,000; Castle 27,500; MainCharacter 24,000), `bake_resolution` (units 1024, buildings 2048), `origin`, `fit_mode`, `target_dims_ue`, `pre_rotate_z_deg`, `voxel_size_ue`, `albedo_delight`, `team_region.selectors`, `ucx.boxes`, optional `trellis_resolution` and `engine`. All `*_ue` values are UE centimetres; Blender works in metres.

**Pre-import gate (mandatory):** read `refine_report.json` (tri count vs budget, bounds, UV layer, PNG inventory) AND look at the preview renders before anything enters the editor. Nothing is imported unseen.

Related: `rescale_refined_fbx.py -- --asset Castle --factor 3.0` re-scales a shipped FBX in place (not idempotent; keeps `Cache/<Asset>/<Asset>_pre_rescale.fbx`).

### 8.8 Stage 3 — Unreal import

- Textures: `T_<Name>_D` (sRGB), `T_<Name>_N` (normal), `T_<Name>_ORM` (linear, sRGB off) into `/Game/Textures/`; a material instance `MI_<Name>_PBR` from the shared master `/Game/Materials/M_AssetPBR`; the mesh `SM_<Name>` in `/Game/Meshes/` with slot materials `TeamRegion → MI_TeamColor_<Team>` and `<Name>PBR → MI_<Name>_PBR`; **Nanite OFF** for the gameplay fleet; LOD group `LargeProp`, castle-class explicit chain LOD1 50% @ 0.4 / LOD2 25% @ 0.15; collision per manifest category (units ≤ 4 convex hulls; buildings explicit box hulls from `ucx.boxes`).
- New asset: the art-director imports over Unreal MCP with the editor OPEN.
- Existing asset: **same-path overwrite only** via the headless commandlet (Part 4.7), editor CLOSED, then materials finalized over MCP with the editor OPEN. Never delete-and-recreate; references break silently.
- Fab/marketplace packs: human-only acquisition through the Epic Games Launcher ("Add to Project"); agents author requests in `.claude/pipeline/fab/FAB-REQUESTS.md` (`requested → approved → fulfilled → integrated`, each with a license note). Packs land in `Content/Fab/<Pack>/` as a read-only quarantine: soft-reference or duplicate into `/Game/`, never edit in place; blockout meshes are swapped non-breakingly because Blueprints reference by asset path.

### 8.9 Stage 3b/3c — rigging and animation

```powershell
& "<blender.exe>" --background --factory-startup --python-exit-code 1 --python Tools\ArtPipeline\rig_character.py -- --card-id Footman
& "<blender.exe>" --background --factory-startup --python-exit-code 1 --python Tools\ArtPipeline\retarget_meshy_to_siegebiped.py -- --card-id Footman --check
& "<blender.exe>" --background --factory-startup --python-exit-code 1 --python Tools\ArtPipeline\retarget_meshy_to_siegebiped.py -- --card-id Footman
& "<blender.exe>" --background --factory-startup --python-exit-code 1 --python Tools\ArtPipeline\author_climb_anim.py
```

- `rig_character.py` builds the shared **`SiegeBiped`** 21-bone UE-mannequin-named skeleton (armature object always `Footman_Rig`), skins with bone-heat and a deterministic envelope fallback, authors Idle/Walk/Attack/Death per `rig_manifest.json` (attack styles thrust | swing | overhead | cast | mine), and exports `Content/RawAssets/Characters/<CardID>.fbx` + `Characters/Anims/<CardID>_<Action>.fbx` + `<CardID>.lod.json`. Flags `--smoke`, `--quick`, `--no-anim-fbx`, `--save-blend`. `fix_rig_root.py` batch-renames armature objects in older rigs.
- **UE 5.8's IK Retargeter export is broken** (all three export doors emit root-only motion, ≈7 uu foot amplitude). The sanctioned route is `retarget_meshy_to_siegebiped.py`: Meshy's 24-bone Mixamo-style clips → SiegeBiped, constraint-based then baked to FK, Walk gate = both feet ≥ 40 uu amplitude (`--min-walk-foot-uu`). Keep `IK_*`/`RTG_*` assets for preview only.
- Stage 4 in the editor (MCP): import `SK_<CardID>` onto the shared `/Game/Characters/SK_Footman_Skeleton`, import `A_<CardID>_*`, author the `AM_<CardID>_Attack` montage and `ABP_<CardID>` in-editor.

### 8.10 Health probes and verify

```powershell
cd Tools\ArtPipeline
uv run trellis_generate.py --check        # exit 0, tokenless — the one-command health probe for the whole uv stack
uv run concept_generate.py --check
uv run meshy_generate.py --check          # exit 0 (needs MESHY_TOKEN)
uv run test_meshy_multiimage.py           # exit 0
```

Then drop a test concept at `Inbox/<CardID>.png` and run one asset end to end. Confirm the Blender path in the Stage 2/3b commands matches the install.

---

## Part 9 — The footage-review lane (gameplay video → diagnosed fixes)

Claude cannot watch video. The lane turns a screen recording into timestamped frames the footage-analyst reads as images, and a diagnosis report the manager boards fixes from. It is the human-playtest ground truth that outranks every agent verdict.

### 9.1 Setup

1. **[You]** Install ffmpeg: `winget install --id Gyan.FFmpeg -e --source winget`. Open a fresh terminal (the tool also self-locates ffmpeg under `%LOCALAPPDATA%\Microsoft\WinGet\Links` and `…\WinGet\Packages\Gyan.FFmpeg*`). Norton prompted twice on the first runs; allow it.
2. **[You]** System Python 3.14 from python.org, plus `py -m pip install pillow`. **Law: this lane runs on the SYSTEM Python with stdlib + ffmpeg only, no venv, no numpy.** Pillow is optional (`crop` and labeled contact sheets need it; exit 4 without).
3. Create `<git-root>/testvideo/` and confirm the root `.gitignore` ignores it (Part 2.3). Load-bearing: `*.mp4` is an LFS pattern, so without the ignore a careless add pushes whole videos into LFS.
4. Copy `Tools/VideoReview/extract_frames.py` and the `footage-analyst` agent definition into the new repo.
5. **[You]** Record with Xbox Game Bar (Win+Alt+R) and move the clip into `testvideo/`. Filenames are accepted VERBATIM (Game Bar names carry double spaces and parentheses; every consumer uses list-argv and `shell=True` is banned in the tool for that reason).

### 9.2 The tool

```powershell
C:\Python314\python.exe Tools\VideoReview\extract_frames.py --check
C:\Python314\python.exe Tools\VideoReview\extract_frames.py probe  "<clip>.mp4"
C:\Python314\python.exe Tools\VideoReview\extract_frames.py sheets "<clip>.mp4"                  # coarse pass: timestamp-labelled 4x3 contact sheets
C:\Python314\python.exe Tools\VideoReview\extract_frames.py frames "<clip>.mp4" --at 92.5,93.0 [--run N]
C:\Python314\python.exe Tools\VideoReview\extract_frames.py crop   <png> --box x,y,w,h [--scale 2]  # before ANY claim about UI text
C:\Python314\python.exe Tools\VideoReview\extract_frames.py promote <png> --id VID-### --symptom <slug> [--t MMmSSs]
```

Exit codes: `0` ok · `2` no ffmpeg (the message names the winget command) · `3` video missing/undecodable · `4` Pillow needed · `5` bad timestamp · `64` usage. The frame cache `testvideo/.frames/` is disposable. **Autocrop is on by default and load-bearing:** Game Bar parks the game in the top-left ~992×576 of a 2496×1440 black canvas.

### 9.3 The flow (the footage-review law)

Drop the clip, describe the issue in Claude Code. The orchestrator dispatches the **footage-analyst directly** with the verbatim words, optional clip name and timestamps, and the next `VID-###` id (the one recorded exception to manager-first routing: diagnosis is evidence gathering, not decomposition). The analyst runs `--check` and `probe`, a coarse pass (contact sheets) to find candidate moments, then a fine pass (`frames --at`, `crop --scale 2`), within about 40 image reads. It writes `.claude/pipeline/footage/VID-###-<symptom>.md` where every claim carries a frame reference: observations, never conclusions ("the fill is visibly unchanged at 01:32.5", not "the delegate isn't firing"); mechanism lines are labelled hypothesis. The few frames that PROVE findings are promoted into `.claude/pipeline/playtest-evidence/<YYYY-MM-DD>/VID-###[-t<MM>m<SS>s]-<symptom>.png`, the ONLY part of this lane that enters git (as LFS PNGs, via a named evidence host row). The manager then boards every fix through the normal pipeline. The analyst never edits code, never opens the editor, never runs git.

The same evidence folder and naming scheme is reused by the Aura verifier (`VER-TASK-###…png`, Part 12).

### 9.4 Verify

`--check` passes; `probe` on any short clip returns duration, resolution, and fps.

---

## Part 10 — Supabase (cloud accounts and save sync)

What it is for in Siegebound: **cloud accounts plus cross-machine sync of per-profile decks and settings** ("Accounts Phase 2", shipped 2026-08-23). Not a leaderboard, not matchmaking. Phase 1 was local-first accounts in SaveGames; Phase 2 lets a profile link to a cloud account (email + password) and mirrors decks/settings. **Local-first remains the law:** local save files are the source of truth, the cloud is a sync layer, no menu or gameplay flow blocks on HTTP, and every cloud failure degrades to Phase 1 behavior with one log line and a visible status message. A new game needs this part only if it wants accounts or cloud saves; without the config file the game builds and runs identically offline.

### 10.1 Setup

1. **[You]** Create a Supabase account (free tier).
2. **[You]** Connect the **Supabase connector** at claude.ai → Settings → Connectors under your account. Agents reach Supabase ONLY through those orchestrator-held MCP tools (`mcp__claude_ai_Supabase__list_projects`, `create_project`, `apply_migration`, `list_migrations`, `execute_sql`, `get_project_url`, `get_publishable_keys`, `get_advisors`, `query_logs` …). No agent definition holds standalone Supabase credentials.
3. **[Claude]** Create a NEW dedicated project named for the game via `create_project` (never reuse an unrelated sandbox). The region enum may not offer every name; record the region actually created (Siegebound: `us-west-1`).
4. **[You]** Dashboard → Authentication → turn **"Confirm email" OFF** (otherwise sign-up returns no session).
5. **[You]** Add the `*.supabase.co` AV exclusion BEFORE any client-side HTTPS testing. A certificate failure from the game is the environment first, a code bug second (the client's own error string says so).
6. **[Claude]** Apply the schema (10.3) via the MCP migration tool; run the security advisors; zero RLS findings or the lane stops.
7. **[You]** Copy `Config/SiegeCloudDev.ini.example` to `Config/SiegeCloudDev.ini` and fill it from the dashboard (Project Settings → API) or from `get_project_url` + `get_publishable_keys`.

### 10.2 The key law and the config home

- ⛔ **The `service_role` key NEVER touches the repo, the game binary, any committed file, or any handoff/report.** Standing QA criterion on every cloud diff: grep for `service_role` and for key material; zero hits.
- **The game uses the publishable/anon key + user JWTs + Row Level Security, nothing else.** The anon key is public by design (RLS is the boundary), but it still has exactly one home: the **gitignored** `Config/SiegeCloudDev.ini`. A hardcoded key in a source file is a QA FAIL.
- Commit the template `Config/SiegeCloudDev.ini.example`; gitignore the real file (Part 2.3). The real file:
  ```ini
  [SiegeCloud]
  ProjectUrl="https://<project-ref>.supabase.co"
  AnonKey=<the publishable anon key, unquoted>
  ```
  and NOTHING else (no DB password, no custody comment): it ships inside every pak and any in-editor assistant can read it from disk (Part 12.3), so its contents are bounded by law, not hidden.
- ⚠️ **The quoted-URL law (a real shipped bug):** `ProjectUrl` MUST be double-quoted. UE's ini reader swallows an unquoted `//` as an inline comment and silently truncates the URL to `https:` while the config still "validates". Quotes are stripped on read. The project pins this with an offline unit test that asserts the unquoted form truncates; do the same. `AnonKey` stays unquoted (its alphabet cannot contain `/`).
- Missing or unparsable file ⇒ cloud OFF ⇒ byte-identical offline behavior. Cloud gates nothing, ever.

### 10.3 How the game talks to it

- `USiegeCloudClient : UGameInstanceSubsystem` + `FSiegeCloudSync`, using UE's own **`FHttpModule` + `Json` / `JsonUtilities`** (`Build.cs` adds `HTTP`, `Json`, `JsonUtilities`). No third-party SDK. Log category `LogSiegeCloud`.
- Every request carries `apikey: <AnonKey>` and `Authorization: Bearer <token>` (the anon key before a session, the user's access JWT after).
- GoTrue: `/auth/v1/signup`, `/auth/v1/token?grant_type=password`, `/auth/v1/token?grant_type=refresh_token`, `/auth/v1/logout`. PostgREST: `GET /rest/v1/<table>?…`, upsert `POST /rest/v1/<table>` with `Prefer: resolution=merge-duplicates`.
- Token posture: access token in memory only (~1 h, callers drive refresh); refresh token persisted per profile in the local save in plaintext, the same trust level as a launcher's session file, and never called "encrypted"; nothing is ever logged.

### 10.4 Schema discipline

- ⛔ **No ad-hoc DDL.** Every schema change is a numbered file in `Tools/Supabase/migrations/` (`0001_init_accounts.sql`, `0002_…`), QA-reviewed BEFORE it is applied, applied verbatim by the build-master through the MCP `apply_migration` tool, and verified identical to the file afterward.
- The live schema: `profiles (id uuid PK → auth.users, display_name, updated_at)`, `decks (id uuid PK, user_id → auth.users, deck_name, payload jsonb, updated_at, unique(user_id, deck_name))`, `settings (user_id uuid PK → auth.users, payload jsonb, updated_at)`; a `touch_updated_at()` function with a `before insert or update` trigger per table (server time owns the sync clock; the client never writes `updated_at`); RLS enabled on every table with one policy per operation bound to `auth.uid()`; no policy ever references `service_role`. Idempotent by construction (`IF NOT EXISTS`, `OR REPLACE`).
- Post-apply gate: the MCP security advisors must report zero RLS findings.

### 10.5 Verify

`list_projects` shows the game's project; `list_tables` after the first migration shows the schema; the advisors are clean; the real ini never appears in `git status`; an in-game sign-up/login round trip succeeds.

---

## Part 11 — The in-game local LLM (SiegeLlama + llama.cpp + GGUF weights)

Siegebound's in-match assistant lets the player type a sentence ("send the footmen to the left tower") and have it turned into unit orders. That runs on a **local, in-process LLM**: an in-repo plugin `Plugins/SiegeLlama/` wrapping a vendored **llama.cpp** C API, with model weights fetched on demand. Only build this lane if the game needs on-device language understanding; everything else in this document stands without it.

### 11.1 What is in the repo

- `Plugins/SiegeLlama/` (FriendlyName SiegeLlama, 0.1.0): "Local in-process LLM inference, backed by a vendored llama.cpp (C API only, Vulkan + CPU). Knows nothing about gameplay: the contract is (prompt, gbnf) → string." Grammar-constrained (GBNF) output is what makes the result parseable.
- `Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/` — the vendored upstream **release build** (not a local build): tag `b10235`, commit `221f0f6…`, artifact `llama-b10235-bin-win-vulkan-x64.zip` with its SHA-256 pinned in `VERSION.md`. Backend **Vulkan + CPU, never CUDA**; CRT `/MD` to match UE. Linking only the C API is what makes the import libraries toolset-agnostic (three different MSVC toolsets linked clean).
- Those DLLs/LIBs are the reason `*.dll` and `*.lib` are LFS patterns and why the project `.gitignore` re-includes `Plugins/SiegeLlama/Source/ThirdParty/LlamaCpp/{lib,bin}/**` after the global ignores.
- `Tools/fetch_llm_model.py` — stdlib-only downloader: `--check`, `--repo <org/repo> --file <name.gguf> [--dest D:/Models]`; resumable (`.part` + HTTP Range), sha256-verified against the Hugging Face LFS oid, authenticates only to `huggingface.co`/`hf.co` and strips the token on cross-host redirects. Runs bare (the `huggingface.co` exclusion). Accepts `HF_TOKEN` (or `HUGGING_FACE_HUB_TOKEN` / `HUGGINGFACEHUB_API_TOKEN`) for gated repos.

### 11.2 Laws

- ⛔ **Weights are never versioned, in any form.** `*.gguf` and `/Models/` are ignored (directory form, no negations; Part 2.3). A GGUF is ~2.5 GB and git history is append-only; a 70 MB DLL sweep already had to be repaired once while unpushed. LFS is not the answer for regenerable artifacts.
- Model choice (CONVENTIONS "THE FINE-TUNE RUNG"): **Qwen3.5-4B-Instruct Q4_K_M** (Apache 2.0). Purpose-built function-calling models under non-commercial licenses (xLAM-2, Hammer, Arch-Function) are banned.
- Model documentation lives in `Docs/` (`Docs/ModelManifest.md`, declaring the shipped artifact with its sha256), never in `Models/`; the manifest is created only when the artifact ships.
- Large-file downloads that redirect to `us.aws.cdn.hf.co` go through `curl` (Schannel) under Norton; the sha256 comes from `huggingface.co` over a verified connection.

### 11.3 Verify

```powershell
C:\Python314\python.exe Tools\fetch_llm_model.py --check
C:\Python314\python.exe Tools\fetch_llm_model.py --repo Qwen/Qwen3.5-4B-Instruct-GGUF --file model-Q4_K_M.gguf
git check-ignore -v Models\model-Q4_K_M.gguf     # must hit the /Models/ rule
```

---

## Part 12 — Aura: Play-In-Editor verification for the agent team

### 12.0 What Aura is and why it is in the stack

**Aura AI for Unreal** (tryaura.dev, by Ramen) is an in-editor AI assistant for UE 5.4–5.8 (Windows launcher builds). It installs at ENGINE level as a plugin plus a standalone `Aura.exe` tray app that talks to the plugin over loopback port **41200**, and it exposes its tools to Claude Code as two stdio MCP servers:

- `unreal_inspector` — project inspection: asset metadata and graphs, Blueprint review, asset/code search, logs, read-only Python queries. ⚠️ Its name says "inspector" but it also carries engine lifecycle (launch / recompile / shutdown), image and mesh generation, and plan bookkeeping: **13 non-read tools** out of 62.
- `unreal_editor` — mutation: Blueprint and C++ authoring, materials, Niagara, data tables, UMG, Enhanced Input, Unreal Python, Live Coding compile, a shell, **plus Play-In-Editor control and verification** (start/stop PIE, inject input actions, simulate keys and sticks, read live actor and widget properties, UI snapshots and gestures, screenshots and recordings). 114 tools.

Why it was bought: before Aura, `qa-passed` was a verdict over TEXT (the reviewer reads a diff and never watches the game), the built-in Unreal MCP has no input injection and no viewport, and `/ship` stopped at a human adjudication of one screenshot. A feature could be written, reviewed, compiled and committed with nobody having SEEN it run. Aura's PIE lane closes that hole: it is the instrument behind the seventh agent, `playtest-verifier`, and the `verified` gate (Part 6). Everything else Aura can do (Tripo-backed mesh generation, its own Blueprint/C++/material agents, crash recovery) is additive and is NOT used by the pipeline: production meshes stay on Part 8, compilation stays with the build-master via `Build.bat`, and Aura's mesh output never writes `Content/`.

Cost model: per-seat subscription with a monthly premium credit (Trial free 2 weeks · Indie ~$10–20 · **Pro $40/mo, the tier chosen 2026-09-21** · Ultimate $200). Credit does not roll over; MCP usage from Claude Code is charged to the Aura subscription, not the Claude one. The credit cost per verification run is UNMEASURED: no tool reply carries a credit field, so the only reading is a human dashboard read or an overage top-up receipt. A decided tier is not a characterized cost.

Privacy: **"Unlimited Auto Mode" requires training-on-your-conversations to be ON** (toggle in Aura's privacy settings; off makes Auto Mode rate-limited). Decide before the first index; Siegebound kept it ON. Aura's file tools read the disk on demand regardless of any ignore list, so anything on the project tree can enter a chat turn (12.3).

### 12.1 Account and install (engine level, once per UE_5.8)

1. **[You]** Create the account at tryaura.dev (the trial is free and card-less). Decide the training toggle.
2. **[You]** Close the editor. From the Aura dashboard download the Windows installer and select engine version **5.8**. It installs into `C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Marketplace\Aura\` (`Aura.uplugin`, `MCP\unreal_inspector.py`, `MCP\unreal_editor.py`, `PortablePython\Windows\python.exe` = Python 3.12.10, `INDEX_IGNORE.txt` template). It is available to every 5.8 project; re-run the installer after an engine upgrade. The plugin self-updates (1.0.5 → 1.0.6 happened unannounced); read `Aura.uplugin` for the current version, never cite a document.
3. **[You]** Open the project → Edit → Plugins → search **Aura** → Enable → restart. The editor adds `{"Name": "Aura", "Enabled": true, "SupportedTargetPlatforms": ["Win64", "Mac"]}` to the `.uproject` Plugins array; that is a committed file, committed under a task. ⚠️ The entry carries no `TargetAllowList`, so UBT enables Aura's 23 dependency plugins for every target; Aura's own two modules are `EditorNoCommandlet` and cannot ship, but 9 of the dependencies add 19 runtime modules to a packaged build. The ruling was to keep the entry as is and have the ship gate name the module delta plugin by plugin and stop on anything unexplained.
4. **[You]** Click the **Aura** toolbar button, confirm the green indicator, ask *"Tell me about this project"*: the answer must name THIS game's classes, not the template. If it cannot connect: `Aura.exe` must be in the tray, nothing else may hold port 41200, the AV must not block loopback.
5. **[You]** Optional: Editor Preferences → Aura Plugin Settings → uncheck *Open Aura in Electron Window* to dock it (`bOpenAuraInElectronWindow`, `bAutoShutdownAuraApplication` live there). Prefer the standalone app for unattended runs; crash recovery works only there and only for Aura's own sessions.
6. **[You]** Aura Settings → **MCP Configuration → Set up Unreal MCP** lets Aura call Epic's built-in editor tools alongside its own (Aura's OWN client connection; our `unreal-mcp` block in `.mcp.json` is untouched).
7. **[You]** Optional: enable the **Filesystem Sandbox** (experimental, 5.8-only): every Aura asset mutation then stages in `Intermediate/Sandboxes/AuraSandbox` for Accept/Reject. It does not cover C++. Whether it was ever enabled on the original machine was never measured.

### 12.2 Index settings

Aura builds a semantic index of the project. Editor Preferences → type **`Index`** in the search box → section **"Aura - Index Settings"** (searching for "aura" does NOT surface it). Buttons: **Delete Previous Index**, then **Sync Files**; progress shows on hover over the green circle. Settings are saved to `Saved/Config/WindowsEditor/AuraSettings.ini` (`[/Script/Aura.AuraIndexSettings]`: `bEnableAutoIndexing=True`, `AutoIndexingFileCountThreshold=30000`, `bSyncIndexOnStartup/Save/Delete/Rename/Import=True`, `bUseExperimentalIndexer=False`). The project was at 17,808 files against the 30,000 default cap, with the Fab packs excluded. After any change to the ignore list: Delete Previous Index + Sync Files (a startup sync does not drop already-indexed files).

### 12.3 The machine-local files under `Saved/.Aura/` and the sync script

Aura reads two per-project text files from `<Project>/Saved/.Aura/`:

| Canonical (committed, edit THIS) | Copied to (gitignored, never edit, never stage) | Purpose |
|---|---|---|
| `Docs/AuraIndexIgnore.txt` | `Saved/.Aura/INDEX_IGNORE.txt` | what the semantic index skips (one pattern per line, relative to the project root, folders end with `/`, globs accepted, no `!` negation) |
| `Docs/AuraProjectMemory.md` | `Saved/.Aura/project_memory.txt` | a ≤150-line digest Aura injects every turn: the team table, asset prefixes and texture suffixes, the C++ naming rule, the build command, and the laws Aura must never break (never compile via Live Coding; never write generated meshes into `Content/`; never run Git) |

`Saved/` is gitignored, so both would be lost on a fresh clone. **`Tools/aura_sync.ps1`** is the ONE sanctioned writer of that folder: it resolves paths from its own location, fails closed if a source is missing (exit 2), creates `Saved/.Aura/`, copies each pair only when the SHA-256 differs and re-hashes the destination (exit 3 on mismatch), supports `-WhatIf`. Run it as a **session-start step** on any machine and after any edit to either canonical:

```powershell
powershell -NoProfile -File Tools\aura_sync.ps1
```

The canonical ignore list excludes every Fab/marketplace pack under `Content/`, `Content/RawAssets/`, template leftovers, `Models/`, `Intermediate/`, `Saved/`, `Binaries/`, `DerivedDataCache/`, the uv venv and cache, `.claude/pipeline/playtest-evidence/`, the board, QA, handoffs, footage, fab, commands, agents, hooks (but KEEPS `.claude/pipeline/CONVENTIONS.md`), `Config/SiegeCloudDev.ini`, generated headers, IDE and Python junk. Keep-set: `Source/`, `Content/{Blueprints,UI,Data,Maps,Materials,VFX,Characters}`, `Docs/GDD.md`, `CONVENTIONS.md`, `Config/`.

Three rulings a new project must inherit:

- **The ignore list is not a secret fence.** With the cloud ini excluded by both pattern shapes, Aura still read it from disk in one tool call and reported its section and key names. Anything on the tree is reachable by an in-editor assistant, and under training-ON may enter a chat turn. Keep secrets out of the tree (the cloud ini holds only the publishable pair by law).
- **The "project index" Aura answers from is a curated document**: `project_memory.txt` is a byte-identical copy of the hand-authored `Docs/AuraProjectMemory.md`, so it cannot acquire a token nobody typed into it. The index is clean; the disk is not.
- There is no skills mirror: `.claude/skills/` does not exist in this repo, so the step from Aura's docs is dropped.

### 12.4 The bridge into Claude Code (two stdio servers in `.mcp.json`)

1. **[You]** Aura Settings → **MCP Configuration → Add to Editor → Claude Code**, then fully restart Claude Code.
2. **What the one-click actually does:** Aura's doc says it writes `~/.claude/mcp.json` with a `"servers"` key. It does not. It writes both servers into `~/.claude.json` under `mcpServers` (Claude Code's USER scope); `~/.claude/mcp.json` is never created and is not read.
3. **[Claude]** House rule: the two blocks live in the project `.mcp.json` next to `unreal-mcp` and `blender`, both names go into `enabledMcpjsonServers`, and the user-scope copy is removed so there is exactly one definition:
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
   ```powershell
   claude mcp remove unreal_inspector -s user
   claude mcp remove unreal_editor -s user
   ```
   Use Aura's own `PortablePython`, not the system Python 3.14 and not the uv venv. Restart Claude Code.
4. **[Claude]** `/mcp` must list `unreal_inspector`, `unreal_editor`, `unreal-mcp`, and `blender` connected. Write down every tool name both servers expose (the deferred-tool list IS the census) into `handoffs/AURA-MCP-CENSUS.md`; every grant below is built from that file, never from a guess. A plugin update re-fires the census (new file `AURA-MCP-CENSUS-<VersionName>.md`, old file untouched because its sha256 is pinned); the 1.0.6 re-census found zero added/removed/renamed names. A name census cannot see a schema change.

### 12.5 The allow-list law (merge into `settings.local.json` and the agent `tools:` lines)

- ⛔ **Never `mcp__unreal_editor__*` wholesale, anywhere.** A wildcard hands every agent C++ authoring, Live Coding compile, and a shell, undoing the compile-belongs-to-build-master law and the no-Live-Coding law in one line.
- ⛔ **Never `mcp__unreal_inspector__*` wholesale either.** It carries `launch_unreal_project`, `recompile_unreal_project` (a compile path), `shutdown_headless`, `cancel_operation`, seven credit-spending image/mesh generators, and two plan tools. Those 13 are granted to no agent.
- **The grant is enumerated, one name per entry**: the **49** inspector read tools (`execute_unreal_python_readonly`, the `fetch_*` skills, `get_*` readers, `grep`, the `import_*_understanding` set, `query_unreal_project_assets`, `quicksearch`, `read_datatable_*`, `review_blueprint`, `search_geometry_scripts`) and the **32** editor PIE/verify/screenshot/input tools (PIE lifecycle 6 · verification 10 · screenshot/recording 9 · input simulation 7: `start_pie`, `stop_pie`, `is_pie_active`, `load_level`, `wait_pie_frames`, `wait_pie_seconds`, `run_verification_sequence`, `verification_agent`, `get_actor_by_name_in_pie`, `get_actor_property_in_pie`, `get_widget_property_in_pie`, `survey_pie_scene`, `get_player_transform`, `set_player_transform`, `get_input_mapping_context_keys`, `ui_snapshot`, `ui_perform`, `ui_wait_for`, `capture_pie_frame`, `attach_pie_frames`, `take_editor_screenshot`, `get_screenshot_of_objects_for_verification`, `start_pie_recording`, `stop_pie_recording`, `start_state_recording`, `stop_state_recording`, `record_burst`, `inject_input_action`, `simulate_key_press`, `simulate_button_press`, `simulate_left_stick`, `simulate_right_stick`). The full list is in Appendix B.
- Judgment calls recorded with the census: `pie_scene_edit` (PIE-only world mutation) and `create_easy_level_for_verification` / `spawn_blueprint_actors` (they write the editor level) are OUT; `load_level` is in under lifecycle.
- The same 49 inspector names go on the `qa-reviewer`'s `tools:` line so QA can inspect a Blueprint graph or the editor log instead of accepting a declaration; its no-mutation posture is unchanged.
- The sequence runner is a grant surface: a step reachable THROUGH `run_verification_sequence` or `pie_scene_edit` (e.g. `call_actor_function`) is reachable, not granted; the verifier declares such a reach under "Not examined". The grant question for `call_actor_function` was put to the human and declined.
- Unlisted is not denied: `mcp__unreal_editor__compile_blueprint` or `execute_unreal_python` prompt when called; whether a `deny` rule honours an MCP name is unmeasured.

### 12.6 The seventh agent — `playtest-verifier` — and the `verified` gate

`.claude/agents/playtest-verifier.md` runs Aura's PIE verification against a task's acceptance lines AFTER the build-master has compiled and relaunched the editor on the new binaries (PIE can only test binaries that exist, so it runs after QA, never before). Frontmatter: `model: claude-opus-5-5[1m]`, `effort: medium`, and a `tools:` line of exactly `Read, Grep, Glob, Write, Edit` + the 49 + the 32 + `mcp__claude_ai_Slack__slack_send_message`, `mcp__claude_ai_Slack__slack_read_thread`. **Dispatch it without a `model` parameter**: a per-invocation model overrides the frontmatter pin.

How it works:

1. ONE verification at a time; never concurrent with a build-master compile/assemble or an art-director import.
2. Pre-flight: the editor is up (via `unreal_inspector`), PIE is not already running (if it is, that is the human's session: report and wait), the board row reads `built` (C++) or `qa-passed` (Blueprint/asset-only), the process census by command line names the GUI editor and no `-game` instance, and no Blueprint in memory is in `BS_ERROR` (a resident error Blueprint raises a modal "unresolved compiler errors" dialog at PIE start that wedges the game thread while the connection still reports healthy).
3. Maps each acceptance line to an observable (an actor that exists or moves, a widget value, a log line, a frame). No runtime signal ⇒ `unobs` with the reason; it never invents a proxy.
4. Drives PIE, batching predictable input into ONE `run_verification_sequence` (round trips cost 8–70 s of PIE clock; the bot kills an undriven hero from t≈83 s, so observables land before t≈60 s). ≤3 attempts; every attempt's evidence is kept.
5. Reports observations with evidence paths and quoted values ("castle health widget read 87 after the third hit at t=0:41"), never conclusions; mechanisms are hypotheses. It `Read`s each captured PNG itself (the capture tools report success but deliver no image) with a positive-control frame.
6. Writes `.claude/pipeline/qa/TASK-###-verify.md`. **Line 1 is the verdict, byte-literal**: `Verdict: VERIFIED | VERIFY-FAILED | UNOBSERVABLE | MEASURED`. Then `Editor/Aura state:` (connection, map, PIE mode, the editor PID and command line, build configuration, attempts, wall time, credit if visible, what the dispatch said about the PIE announcement, the self-reported model), the acceptance table `| # | acceptance line | observable chosen | observed (quoted values, evidence path) | pass/fail/unobs |`, `## Evidence (promoted)`, `## Hypotheses (not verdicts)`, `## Not examined / limitations this run`, `## Recipes used`, `## Recipe candidates`.
7. Flips only its own row's `status:` (`verified` / `verify-failed` / `blocked`) and posts once in the Dev & QA Slack thread prefixed `🎮 VERIFIER:`.

The verdict vocabulary, mechanically derived from the table:

| Verdict | Means | Routing |
|---|---|---|
| `VERIFIED` | ≥1 `pass`, no `fail` (partial: `verify: partial (n/m observable)`) | commit proceeds (5c) |
| `VERIFY-FAILED` | any `fail` (a statement about OUR CODE) | **blocks the commit**; back to the programmer as a QA loop (max 3, then escalate) |
| `UNOBSERVABLE` | nothing PIE can see (pure data, editor-only, doc rows); status does not move; `verify: unobservable` | never blocks, never a pass; routes to 5c like a row with no runtime criterion |
| `MEASURED` | a CONTROLLED NEGATIVE: the probe fired, a NAMED control discriminated, the observable did not move, and "it did not move" was the answer sought (a statement about the world or the rig); flips to `verified` with `MEASURED` in the first sentence | never blocks, never bounces, never a pass; a `MEASURED` without a named control is read as `UNOBSERVABLE` |

An editor-down or Aura-disconnected run is an OUTAGE (`🚧`, row → `blocked`, no verdict), never `UNOBSERVABLE` and never a fabricated run. A verifier that has never returned `VERIFY-FAILED` on a deliberately broken input is not a gate; the pilot's exhibit caught a data-only break (a card cost edited 50 → 5000) 0.81 s after the press, invisible to compile, suite and text QA. The gate has been **binding** since 2026-09-14 on the human's one-word ruling.

Standing grants that shape the lane: the human granted **"permission to drive PIE whenever you feel it is necessary from now on"** (2026-09-20), so the orchestrator ANNOUNCES before dispatch and the verifier REPORTS, with no wait for a go; any `-game` instance is still his; `.sav` files must be proven net-zero (sha256 and mtime, or a byte comparison from the save-class path to EOF for write-then-restore runs, with a pre-run byte copy); nobody pushes.

Evidence promotion: Aura writes under `Saved/AuraVerify/<run>/` (frames, `recording.h264`, `recording_index.json`). Proving stills are promoted BY COPY into `.claude/pipeline/playtest-evidence/<YYYY-MM-DD>/VER-TASK-###[-a<N>][-t<MM>m<SS>s]-<observable-slug>.png` by the commit host (the verifier holds no shell), proven sha256(source) = sha256(target) = the LFS pointer oid in the commit. Video is never promoted.

**Recording law:** every PIE-driving dispatch records (`start_pie_recording` after PIE starts, `stop_pie_recording` before `stop_pie`, or an in-batch `record_burst` after a level travel, since a film armed before `start_pie` may not survive the travel). The recorder writes a raw `.h264` elementary stream and no `.mp4`; **the orchestrator remuxes** in the run folder and hands the human the path:

```powershell
ffmpeg -framerate 30 -i recording.h264 -c copy recording.mp4
```

The `.mp4` stays in `Saved/`. The human watches runs live and replays them, so this is not optional.

**Recipes:** `Tools/Verify/recipes/README.md` + `RCP-*.md` hold measured input sequences (menu → deck builder; slot and card edits by batched double-click; set-active by keyboard; vs-bot capture-center-and-summon; play a unit card from hand). Each recipe states its source run, plugin version, preconditions, exact steps, read-backs, fences, resolution-dependent coordinates (PIE 1280×720 requested → 1280×725 viewport), and hazards. A recipe is a CLAIM re-verified in-run after a plugin update or a screen edit; the verifier proposes candidates and gameplay-programmer rows write them.

### 12.7 Instrument facts every new project will rediscover (so read them first)

- **Input injects BELOW Slate.** `simulate_key_press` is delivered to the player controller / Enhanced Input, never to Slate: `Tab` moved focus not at all while an injected `IA_MenuDown` did. On a menu map with `FInputModeUIOnly`, `applied_mapping_contexts` is `[]` for raw keys. `inject_input_action` on YOUR OWN input actions is the reliable lane (it drove `IMC_Hero` and `IMC_MainMenu`).
- `ui_perform` `click`, `press`/`release` never fire `UButton.OnClicked` (`down handled=true / up handled=false`, re-measured identical under 1.0.6); **`double_click` fires it exactly once per gesture** (measured 2026-09-26). Rapid batches DROP gestures (42 of 49 at 3-frame spacing; 27 of 30 at 20 frames): read the state back and top up. The right mouse button has no route through `ui_perform` or `simulate_key_press`. `simulate_button_press`'s surface is unmeasured; `simulate_right_stick` reached the controller but nothing bound it.
- **`ui_perform` clears Slate keyboard focus (the call, not the step).** Any focus reading after a `ui_perform` is void until an injected `IA_MenuDown`/`IA_MenuAccept` re-establishes it.
- A `ui_perform` `name_path` selector that misses does not error: it resolves to the root and acts at screen centre, reporting success. Resolve targets with `ui_snapshot` first. Two same-class widgets (a second `WBP_MainMenu` after a sub-screen exit) cannot be addressed by name; fall back to the read-only Python lane (`ObjectIterator` + `IsInViewport()`).
- Raw key polls (`WasInputKeyJustPressed(EKeys::LeftMouseButton)`) cannot be driven by an input action; the composition that works is `pie_scene_edit → call_actor_function → APlayerController::SetMouseLocation(X,Y)` plus `simulate_key_press "LeftMouseButton"` **inside one `run_verification_sequence`** (the aim does not latch across round trips: ≈648 uu scatter vs ≤1.2 uu in-batch). This moves the REAL cursor; the human keeps hands off during a run. `binding_found: false` is not evidence a key failed to reach the game (the mouse wheel fired at `false`).
- Capture tools (`attach_pie_frames`, `take_editor_screenshot`) return `success: true` and deliver no image; the PNG is on disk, `Read` it. The default 1086-px composite loses thin UI (an outline invisible at 1086×615, visible at 1280×725): capture with `max_dim` ≥ 1280. A "Video memory has been exhausted" banner can cover an observable; quote numbers from tool replies, not pixels under a banner. `ui_snapshot` geometry is not capture pixels (≈2.5× ratio).
- `start_pie`, `stop_pie_recording`, and `ui_perform` replies can exceed the tool-result limit (264k–2.8M chars); read the head; the observable is always the state read after.
- `get_unreal_output_logs` may serve the PREVIOUS editor instance's log; name the log file by matching its `Log file open` timestamp to the driven PID's creation time. A log-line ABSENCE is evidence of nothing until the build configuration is declared (`UE_LOG` at `Log` verbosity is compiled out of Shipping) and the category's verbosity is checked (a `Verbose` emit under a `Log` category is a VOID, not a ZERO).
- The read-only Python lane blocks `import subprocess` and `import ctypes`, so a process census (`Get-CimInstance Win32_Process`) belongs to the build-master; the verifier corroborates from inside with `os.getpid()`.
- CDO reflection reports `on_key_down` on every `UserWidget` whether overridden or not, and is blind to Blueprint graph functions in both directions; `function_graphs` is not exposed to Python; `WidgetBlueprintLibrary` is not exposed to the read-only lane.
- Closing the GUI editor can auto-launch a headless `-RenderOffScreen -AuraHeadless -unattended` editor a few minutes later; re-census processes immediately before a compile, before a relaunch, and before a cook.
- The plugin's unauthenticated indexing call logs an Error ~20 s after boot and reds whichever automation test is running at that moment in a `-nullrhi` suite lane; keep Aura out of the suite lane.
- `execute_unreal_python` (the mutating twin, not granted to the verifier) RE-RUNS a script after a force-delete and returns only the last run; destructive scripts must be idempotent and run-logged.

### 12.8 Verify

Aura toolbar green + the project's classes named → `/mcp` shows all four servers → `Tools/aura_sync.ps1` leaves `git status` clean → the verifier returns `VERIFIED` on a known-good task AND `VERIFY-FAILED` on a deliberately broken one (a verifier that cannot fail is not a gate; prove the deliberate break by hash both ways, never by size) → the orchestrator remuxes the run's `.h264` and the human can play the `.mp4`.

---

## Part 13 — Designing the game so the verifier can always drive it

Siegebound spent several milestones retrofitting features so the Aura verifier could reach them (five of seven main-menu flows were verifier-blind at the pilot; right-click-only actions, mouse-only confirms, non-focusable widgets, and hard-coded help numbers each cost a fix wave). A new game avoids all of that by treating **"the verifier can drive and observe this"** as an acceptance criterion from the first milestone. The rules below are what was learned, each bought by a real run.

### 13.1 Input: every player action needs an Enhanced Input ACTION door

- The rig's one reliable input lane is `inject_input_action` on your own `IA_*` actions. Raw key simulation injects below Slate; `ui_perform` clicks mostly do not fire buttons; gamepad simulation reaches the controller but binds nothing. **Therefore every interaction a player can perform must be reachable through an Input Action**, including menus.
- **Build a menu Input Mapping Context from day one** (`IMC_MainMenu` with `IA_MenuUp/Down/Left/Right/Accept/Back/Secondary`, bound to arrows, D-pad, Enter, gamepad face buttons), installed by a world subsystem on menu maps at `OnWorldBeginPlay`, placing initial focus on the first button. `IA_MenuAccept` calls `UButton::OnClicked.Broadcast()` on the focused button so Blueprint handlers run unchanged. The same six actions should be listened to by every in-match screen (help, console, deck builder, victory). This doubles as keyboard/gamepad navigation for players, which is why it is the cheapest door.
- **Every mouse gesture needs a keyboard/action twin that reaches the same implementation.** Right-click set-active got `IA_MenuSecondary` (Home / gamepad Y on a focused slot); the right-click half stays verifier-blind forever. Add a held-key repeat filter so auto-repeat does not re-fire one-shot actions.
- **Never gate a confirm on a raw key poll** (`WasInputKeyJustPressed(EKeys::LeftMouseButton)`) if you can bind it to an action; if you must, expose the confirm as a `UFUNCTION(BlueprintCallable)` so the rig can reach it, and know that the only working composition is a `SetMouseLocation` aim plus `simulate_key_press "LeftMouseButton"` inside one batched sequence.
- Use `UButton` for anything the rig must click: `ui_perform` `double_click` fires `UButton.OnClicked` once per gesture; checkboxes, sliders, and custom widgets are unmeasured. Give every actuable widget a stable, unique `Name` (`Button_0`, `SlotButton`, `Btn_Jump`): the rig addresses widgets by name, and two same-class widgets with the same name cannot be told apart.
- Keyboard fences that kept the design sane: no letter keys in menu bindings; leave Tab to Slate; `Escape` only where explicitly specified (an Escape on a victory screen was a blocker); author mapping assets in the editor and let code only load them (never `MapKey`/`UnmapKey` on shipped paths). Watch for key collisions across contexts (Enter bound to menu-accept in one context and to the console in another).
- Provide **non-shipping cheat/test hooks** as `UFUNCTION(exec)` on a `UCheatManager` (`SummonTestUnit`, `ApplyTestDamage`, `AddTestGold`) or cvars, not only as menu flows: the PIE launcher ignores URL options and `-ExecCmds` splits on commas.

### 13.2 Focus: deterministic and visible

- **Focus the button, never the widget root.** `FInputModeUIOnly` with `SetWidgetToFocus` on a `UUserWidget` root (CDO `bIsFocusable = false`) focuses nothing and logs an engine error. Resolve the button's own `SWidget`, guard with `SupportsKeyboardFocus`, and set `IsFocusable = true` on the asset (that asset change is a human Blueprint-editor save).
- Place initial focus on the first entry of every screen and keep a short re-entry poll, so both a real-key user and the rig start from a known stop. Recipes count steps from that stop; a menu that gains, loses, or reorders an entry invalidates them, so name the change in the task.
- Draw a visible focus ring with a known color (Siegebound: `#0070E0`, corner pixels `RGB(90,157,224)` vs `RGB(192,192,192)` unfocused) so a pixel limb can be specced with a pre-registered threshold written to disk before PIE. Capture at `max_dim` ≥ the viewport's long edge; score the max channel difference, not the mean.
- Expect end-of-match screens to steal focus; name that false positive in any hand check.
- When a screen closes, know how the rig closes it: give sub-screens an action-reachable Exit (a `UButton` or `IA_MenuBack`), or each one-way screen costs a whole PIE session.

### 13.3 Deterministic PIE start state

- Pin the maps in `DefaultEngine.ini` (`GameDefaultMap`, `EditorStartupMap`, `GlobalDefaultGameMode`) and give each level's controller its own input-mode posture at startup (menu: UI-only + visible cursor; arena: game-only + hidden cursor); travel entries stay posture-agnostic.
- Provide a **direct route into gameplay** that skips menus (`load_level L_Arena` into a vs-bot match) so core mechanics stay verifiable even when a menu regresses.
- Make randomness readable: if the hand is shuffled, expose `Hand`/`DrawPile`/`DiscardPile` as reflected properties and have the verifier read them FIRST; a batch cannot branch on its own result, so design observables the rig can tolerate (an in-batch wait that raises gold until any card is affordable).
- Provide **test fixtures the verifier can build through the real UI** and save: Siegebound keeps 50-card test decks in fixed slots (`deck4` = 50× Archer, `deck8` = 50× Footman) plus an empty slot as a standing refusal control, and backs up the `.sav` files before any run that may write. Prove net-zero saves by hash and mtime (or by byte comparison past the auto-rewritten header).
- Keep PIE clean: no Blueprint in `BS_ERROR` anywhere in editor memory (a resident one raises a blocking modal at PIE start; delete demo content that will not compile), dirty-asset list empty, PIE stopped, no `-game` instance alive.
- Know the clock: an idle hero dies to the bot from t≈83 s; the match self-ends around 3.4 min; one MCP round trip costs 8–70 s of PIE clock. Front-load observables to the first ~11 s and finish inside ~60 s, or batch everything into one sequence.
- **Better: give the verifier a dev-only switch for every clock.** Siegebound added `ASiegePlayerController::SetBotEnabled(bool)` as a `UFUNCTION(Exec, BlueprintCallable)` (reachable through `call_actor_function` on the player controller, declared per row) plus a `siege.BotEnabled` console variable, non-shipping by construction, so a verification can simply turn the opponent off instead of racing it (TASK-1600, 2026-10-04). Design every such switch as a reflected function on an actor the rig can address, never only as a console command or a menu.
- Budget VRAM: PIE alongside a human play session shows the "Video memory has been exhausted" banner over frames.

### 13.4 Observability: make state readable and loggable

- **Every claim the design makes must have a reflected read.** Make gameplay state `UPROPERTY` (`VisibleInstanceOnly, Transient` is enough) so `get_actor_property_in_pie` / `get_widget_property_in_pie` / the read-only Python lane can read it; a plain C++ member can be neither read nor force-set. Expose getters the test needs as `UFUNCTION`s (`GetSelectedActionId`, `IsHelpOpen`, `HasKeyboardFocus`); non-reflected logic is not callable.
- Read tunables from their owners, never hard-code them in UI text or help pages: a help-page number that duplicates a constant was wrong twice; the fix was owner-file accessors verified by five-way equality (class default property = getter = native = live property = live getter).
- **Log lines are instruments.** Declare log categories at `Log` verbosity and document the exact format string and arguments at each emit site, because the verify lane greps them (`IA_MenuDown -> HandleMenuDown() entered`, `played card 'Footman' for 9 gold — spawned 'BP_Unit_Footman_C_0' at (…)`, `SetActiveDeck('deck5')` refusals). A `Verbose` emit under a `Log` category is silence by construction, and `UE_LOG` is compiled out of Shipping; declare the build configuration before scoring any absence.
- Write acceptance criteria as **properties, never indices or coordinates**: "Down reaches the target and wraps; Up traverses the same set in reverse; no index repeats twice in a row", not "17 → 18".
- Every negative needs a control that could have fired (an injected action in the same process that did print; a positive-control refusal bracketing a probe). A negative without a discriminating control is `unobs`, never `measured`.
- Pre-register thresholds to disk before the instrument runs.

### 13.5 Boarding rules that keep the lane honest

- **Declare the ceiling at boarding, never discover it at verification.** A row whose acceptance is a Slate keypress, a UMG hover, a right-click, a "does it look right" judgment, or a compiled-Blueprint save says so in its own spec, splits the criterion at the ceiling (the state-read half is a real discriminating line), and specs the human's one-sentence hand check in advance with its false positives named in a parenthesis: *"Open the deck builder, press Down once, then Enter — does the deck total go up by one? (An outline appearing, or the details panel opening, is a NO.)"* Word exclusions as "a click ON a menu entry", never "any click". Put hand-check prompts through a question with options, not plain text.
- Some checks are a human's forever: if the missing tool existed tomorrow, would its output be the CLAIM or a PROXY for the claim? ("Is the Play Again button on screen" is a proxy for "does clicking it restart the match".)
- A ceiling measured on one input lane is not a ceiling on the human; a row asserting "a human cannot do X" names the layer measured and enumerates the layers not measured (OS → Slate → focused widget → viewport client → player controller → Enhanced Input → raw polls). A prohibition on asking the human is the most expensive sentence a handoff can carry.
- Ceilings are candidates for removal, not facts: before proposing a new tool, enumerate the verbs of the tools you already hold, quoted from their own schema (two ceilings fell to an already-granted `call_actor_function` nobody had enumerated), and probe with a run that can still fail. Re-measure ceilings after every plugin update.
- The verifier's evidence is gathered BEFORE the human's playtest and never replaces it; footage from the human's own recording outranks a `VERIFIED`.

---

## Part 14 — Shipping: the `/ship` command and the packaging gates

Siegebound ships as a cooked Win64 zip anyone can run, produced by one word: `/ship` (a Claude Code command at `.claude/commands/ship.md`) driving `Tools/Packaging/ship.ps1` (about 3,400 lines of PowerShell 5.1, ASCII-only). The human's standing instruction: *"anytime we make any changes, I can say 'ship' to you and you will update the zip file and any other documentation with all the current changes to the game."* The law is `CONVENTIONS.md` `SHIP-§0..§10` and `PKG-§1..§14`; the script is the authority on the RECIPE, the command file on the PROCEDURE AND ITS REFUSALS. Copy both files and `Tools/Packaging/Fixtures/` into a new repo and adapt the project names.

### 14.1 The posture

**A failed gate STOPS the ship and SAYS SO. The command never ships a build it could not prove.** No partial ships (zip written but README stale ⇒ incomplete), no gate skipped to save time (the script has no flag for it), and on a stop the report names the gate, the evidence, what clears it, and confirms the previous zip is untouched. `BUILD SUCCESSFUL` proves only the build: the last Development cook printed it and shipped a game with an empty deck, no HUD and no hero.

Read the LAST LINE of the script's stdout, never the exit code: `SHIP RESULT: PASS | STOP at <GATE-ID> - <reason> | ADJUDICATE C3 - <capture path> | DRYRUN-OK | DRYRUN-WOULD-STOP` (exit 0 / 2 / 4 / 0 / 3). **`ADJUDICATE` is not a pass and not a ship**: the build cooked and captured and is suspended awaiting the one judgment a script cannot make.

### 14.2 The procedure

0. **Orient:** `git status --porcelain`, `git rev-parse HEAD`, `git log --oneline -5` (a dirty tree does not stop the ship, but every dirty path is listed); stop on an unresolved merge/rebase; confirm no compile or cook is live; **every `UnrealEditor.exe` must be down** (the Live Coding mutex is per engine binary; a `-game` instance holds it too). Run `Tools\stop_editor.ps1 -RequireAllDown` (Part 4.6): exit 3 names what survives, and a `PLAY` instance is the human's: name it in Blockers and wait. Read `packagedZIPofGame/README.md` for the last shipped commit: that is this ship's diff base.
1. **Dry run** if anything about the machine changed: `ship.ps1 -DryRun` runs Phase A for real and prints the whole plan (every resolved path, the exact Build.bat, suite, and UAT lines with every `-COOKDIR`, the boot-verify drive plan, zip name, retention, commit plan) and nothing else. A dry-run pass is not evidence about phases C–F.
2. **First invocation** (`ship.ps1`, Shipping by default), each gate a STOP:
   - **A** pre-flight: paths resolve · no mid-operation · quiet module · `A4-FENCE` (the staging dir is outside the work tree or matched by a live `.gitignore` rule, measured this run) · ≥4 GB disk · `A6-EVIDENCE-ROUTE` (an installed Launcher engine makes Shipping log-silent permanently, so the Pixel route is auto-selected and announced) · `A7` the cook recipe still matches the tree (`-COOKDIR`s exist, both maps exist, `GameDefaultMap` points at the boot map) · `A8-DESKTOP` (a positive lock-screen detector: `OpenInputDesktop` and `SetCursorPos` both lie on a locked machine; the honest test is the class of the root window under the click point).
   - **B** `B1-COMPILE` (Development editor target, verdict parsed from `Result:`; names Smart App Control as machine state) · `B2-SUITE` (the automation suite, 0 failures, via `Tools/run_suite_bounded.ps1`).
   - **C** `C1-COOK` / `C2-UAT-LOG` (UAT `BuildCookRun`, Win64, Shipping, the maps allowlist plus the `-COOKDIR` list; verdict from UAT's own lines) · `C2-COOK-BP-ERRORS` (any `LogBlueprint: Error` ⇒ stop) · `C2-STAGE-PRESENT` (staged binary names resolved from the manifests: `<Project>.exe` under Development, `<Project>-Win64-Shipping.exe` under Shipping) · stage hygiene (UAT does not clean the stage; 714 MB of stale Development binaries once survived a Shipping cook; delete every game `.exe`/`.pdb` in no manifest, assert exactly one runnable game exe plus the root shim; never delete the `.ucas`/`.utoc`, which are in no manifest by design) · `C3-BOOT-ARENA` (launch the shim with NO arguments like a player's double-click, click `Play` in the game's own menu with a topmost-without-activation rig whose abort predicate is "the game owns the pixel under the cursor", wait for the level travel, capture) · `C3-BOOT-TITLE` (the running game resolved by path, never by name) · `C3-CAPTURE` (a mechanical pre-filter that may only FAIL: exists, decodes, not blank, click delivered) · `C4-NO-MODELS`.
   - Under the Pixel route the run ends at **`ADJUDICATE C3`**. A Shipping first invocation takes over the screen for a couple of minutes; a Shipping `/ship` is schedulable, not unattended.
3. **Pixel adjudication:** `Read` the capture and judge it against the printed `PKG-§6a` bar: `arena` (terrain, castle, a match in progress; a menu is never a pass) · `deck` (real cards with art and costs, not six blank slots) · `hud` (gold, stance, card-bar chrome) · `hero` (spawned and visible). Write `<staging>\.ship\ship-adjudication.json` with `verdict` `PASS` or `FAIL` only, four affirmative observations of ≥24 characters and 4 words each, bound to the capture sha256, HEAD, configuration, and the staged exe's size and timestamp. Ambiguity is a FAIL. There is no `-PixelAdjudicated` flag and adding one is an automatic QA FAIL. The human may discharge this himself (double-click the shim, click Play, report the four criteria), and his eye wins. Optionally the `playtest-verifier` may add evidence beside the suspension, never the adjudication.
4. **Render the README** from the tracked source `Docs/Packaging/README-source.md` into `packagedZIPofGame/README.md`, resolving the nine `{{SHIP:*}}` placeholders from this run's measurements (`ZIP_NAME`, `DATE`, `CONFIG`, `SIZE`, `HEAD`, `DIFF_BASE`, `VERIFIED` with the verdict and its four observations, `CHANGED_SINCE` in player-facing language, `CLOUD_SYNC`). A surviving placeholder is a STOP. The README states what changed, what was verified and by which instrument, the exact click target, what is NOT in this build, the last shipped commit, and that there is no input-injection lane so no gameplay claim.
5. **Second invocation** with `-CommitPaths <explicit paths>`: Phase A re-runs; B and C are reused only if the state file proves a byte-identical build input (same HEAD, same build-relevant tree, configuration, recipe, staged exe), otherwise a full re-cook. `C3-VERDICT-BINDING` re-measures the adjudication record; a `PASS` resumes to **D** (`D0-README` names this zip · `D1-ZIP` with a ZIP64-capable .NET writer, `Compress-Archive` is banned · `D2-ZIP-READBACK` verifies entry count, click target, binaries, pak/ucas/utoc, README at the root, zero `.gguf` · size sanity · `D3-PRUNE` keeps N=2 per configuration and never prunes another configuration's zip) and **F** (`F1-COMMIT-PATHS` exist, tracked, not ignored, not under the staging dir · `F2-INDEX-CLEAN` · `git add -- <one path>` each, then `git commit -F`). The consumed record is archived, never deleted, and the state advances `boot: ADJUDICATE → PASS`.
6. **Report:** zip path and size, HEAD and diff base, what changed, what was verified and by which instrument, the pixel verdict in full, how the capture was reached, stage hygiene, what was NOT verified, docs touched and deliberately not touched, the commit hash, and **not pushed**.

### 14.3 Laws that transfer

- The `-COOKDIR` list is not optional in any configuration: soft references are house style and the cooker cannot see the content graph; a `-map`-only cook produces a beautiful, unplayable menu.
- Shipping is not Development with a flag: logging is compiled out and cannot be turned on from an ini on an installed engine (`bUseLoggingInShipping` is a UBT `TargetRules` property); a Shipping binary ignores the map argument and `-ExecCmds`; automation tests do not exist in it; the `.pdb` changes.
- Never name a staged binary or a game process from the project name; names are configuration-dependent and `Get-Process -Name` applies no wildcard.
- Git posture: never push; never stage the build or the zip (`packagedZIPofGame/` is root-ignored and the fence is measured each run); never amend. Which docs a ship updates is ruled: the README (mandatory), `CLAUDE.md` once at setup, this setup guide only when the ship changed something it asserts, never the GDD or the pipeline law.
- `Tools/**/*.ps1` and `Tools/**/*.py` are CODE: QA-gated, with fixtures (`Tools/Packaging/Fixtures/*.log`, `Tools/SuiteRunnerFixtures/*.log`) kept byte-exact by a `-text` attribute.
- The bounded suite runner `Tools/run_suite_bounded.ps1` runs every automation-suite execution under a timeout and reports `N / M` with N > 0; a runner that ran zero tests looks exactly like green. The Aura plugin's unauthenticated indexing call reds one random test ~20 s after boot, so Aura is kept out of the `-nullrhi` suite lane.

### 14.4 Verify

`ship.ps1 -DryRun` prints `DRYRUN-OK` with the full plan; a real `Development` ship reaches `ADJUDICATE C3` with a capture you can `Read`; after a `PASS` record and a rendered README, the resume produces a zip under `packagedZIPofGame/`, `git status` shows no build output, and the commit carries only the explicit paths.

---

## Part 15 — Fresh-machine order and the full verification checklist

### 15.1 The order that avoids every stall seen so far

1. **[You]** Windows prep: Smart App Control off (if enforced); Norton HTTPS exclusions (Part 1.2); `git config --global core.longpaths true`.
2. **[You]** Install Git, Git LFS (`git lfs install`), Node, Claude Code, Visual Studio C++ workload, Epic Launcher + UE 5.8, Blender 5.1, uv, Python 3.14, ffmpeg (`--source winget`).
3. **[You]** Set User environment variables: `UV_SYSTEM_CERTS=true`, `HF_TOKEN`, `MESHY_TOKEN`. Accept the FLUX.1-dev license. Open a new terminal.
4. **[You]** Create the wrapper folder and the UE 5.8 C++ project inside it (Parts 2–3). Close the editor.
5. **[Claude]** Lay down `.gitattributes`, both `.gitignore`s, `git init`, remote. First commit.
6. **[You]** Enable plugins (Unreal MCP, Editor Toolset, Terminal; later Aura). Editor Preferences: MCP auto-start on port 8000; Terminal startup commands pointing at the **project** folder; Revision Control provider None. Run `ModelContextProtocol.GenerateClientConfig ClaudeCode`.
7. **[Claude]** Scaffold the Claude layer (the lifecycle scripts, the archive and split tools, and `sync_mirrors.ps1` come with `Tools/`; `.claude/pipeline/law/` starts empty and `archive/` is created by the first archive pass): `CLAUDE.md`, the seven `.claude/agents/*.md`, `.claude/pipeline/` (TASKBOARD with the template, CONVENTIONS seeded with the naming tables, empty `handoffs/`, `qa/`, `footage/`, `fab/`, `playtest-evidence/`), `.claude/settings.json` + the two hooks, `.claude/commands/ship.md`, and **the allow-list in `.claude/settings.local.json` before the first agent chain**. Add `bRemoteExecution=True` to `DefaultEngine.ini`. Copy `Tools/` (bridge, reimport scripts, VideoReview, Packaging, Verify recipes, aura_sync, ArtPipeline).
8. **[Claude]** Blender server into `.mcp.json`; `uv sync` in `Tools/ArtPipeline`; probes.
9. **[You]** Slack workspace + connector; **[Claude]** `SLACK.md` with the standing threads and their `thread_ts`.
10. **[You]** Supabase account + connector (only if needed); **[Claude]** project, migration, advisors; **[You]** the gitignored ini.
11. **[You]** Aura account, training toggle, engine install, enable plugin, green indicator, Index Settings; Add to Editor → Claude Code. **[Claude]** move the two servers into `.mcp.json`, remove the user-scope copy, census the tool names, enumerate the grants, write `Docs/AuraIndexIgnore.txt` + `Docs/AuraProjectMemory.md`, run `Tools/aura_sync.ps1`, Delete Previous Index + Sync Files.
12. Run the checklist below top to bottom. Then write `Docs/GDD.md` from `Docs/GDD-TEMPLATE.md` and say **"Read the GDD and build milestone 1 only."** From then on: `Tools/sync_mirrors.ps1` at every session start, `Tools/archive_board.py --apply --rows` at every milestone checkpoint, `Tools/split_conventions.py --reindex` after any law edit.

### 15.2 The verification checklist

| # | Probe | Expect |
|---|-------|--------|
| 1 | `git lfs status`; `git lfs ls-files \| Select-Object -First 5` | LFS active; binary assets listed as pointers |
| 2 | `git check-ignore -v Models/x.gguf`; `git check-ignore -v testvideo/x.mp4`; `git check-ignore -v <Project>/.claude/settings.local.json` | all three ignored |
| 3 | `Build.bat` with the editor closed | output contains `Result: Succeeded` (never trust the exit code) |
| 4 | Editor open → `claude` in the project folder → *"What actors do I have selected?"* | Unreal MCP answers with editor context |
| 5 | `/mcp` | `unreal-mcp`, `blender`, `unreal_inspector`, `unreal_editor` all connected (Blender and the editor up) |
| 6 | Blender open → ask Claude for `get_scene_info` | scene summary via the bridge on 9876 |
| 7 | `Tools\ArtPipeline> uv run trellis_generate.py --check` | exit 0, tokenless |
| 8 | `uv run concept_generate.py --check`; `uv run meshy_generate.py --check`; `uv run test_meshy_multiimage.py` | exit 0 each (Meshy needs the key) |
| 9 | `C:\Python314\python.exe Tools\VideoReview\extract_frames.py --check` | ffmpeg (and Pillow) found |
| 10 | `C:\Python314\python.exe Tools\fetch_llm_model.py --check` (if the LLM lane is used) | exit 0 |
| 11 | Orchestrator posts and reads back a line in the Slack Planning thread | connector live; `thread_ts` registry recorded |
| 12 | Supabase MCP `list_projects` (if used) | the game's project visible; advisors clean after the migration; the real ini never in `git status` |
| 13 | *"Show me the task board status"* | the orchestrator answers from `TASKBOARD.md` |
| 14 | Try to write a file containing a fake `hf_…` token; try to edit a file under `Saved/` | both refused by the hooks |
| 15 | Aura toolbar green → *"Tell me about this project"* | names this game's classes, not the template |
| 16 | `Tools\aura_sync.ps1` then `git status` | both files copied; working tree clean |
| 17 | `playtest-verifier` on a known-good task | `Verdict: VERIFIED`, an evidence path that exists, quoted widget values, a `.h264` the orchestrator remuxed to `.mp4` |
| 18 | Same on a deliberately broken branch (prove the break by hash, not size) | `VERIFY-FAILED` with the failing observation |
| 19 | `Tools\stop_editor.ps1` then `Tools\launch_editor.ps1` | the stop prints the census and `hash MATCH`; the launch prints `LAUNCH_EDITOR: PID=… SECONDS=…`; no `-game` instance touched |
| 19b | `python Tools/archive_board.py` and `python Tools/split_conventions.py` (dry runs) | both print a plan and write nothing |
| 20 | `/ship Development` (Part 14) | every gate reports; the cook stops at its human adjudication, the zip lands in `packagedZIPofGame/`, `git status` shows no build output |

### 15.3 Things this guide could not verify from the record (check on the new machine)

- The exact Norton menu path for HTTPS-scan exclusions, and whether `NODE_EXTRA_CA_CERTS` or `NODE_USE_SYSTEM_CA=1` replaces the Terminal panel's `NODE_TLS_REJECT_UNAUTHORIZED=0`.
- Current claude.ai connector flows for Slack and Supabase (the UI moves).
- The uv winget package id (install from astral.sh if unsure).
- Whether Aura's Filesystem Sandbox adds a second `.uproject` plugin entry, and the Sandbox's actual state on the original machine.
- Credit cost per Aura verification run (never visible in a tool reply).
- Whether a `permissions.deny` rule honours an MCP tool name.
- The Terminal panel's startup `cd` on the original machine pointed at the git root rather than the project; the external-terminal launch from the project folder is the proven route.
- Aura's version and tool census are point-in-time readings of a self-updating plugin; re-read `Aura.uplugin` and re-census after every update.

---

## Appendix B — The permission allow-list, verbatim (`.claude/settings.local.json` as installed on the original machine, 2026-10-04)

Merge into `permissions.allow`; never replace the array. Keep this file gitignored. There is deliberately no raw `Stop-Process` rule: the editor lifecycle is granted only as `Tools/stop_editor.ps1` and `Tools/launch_editor.ps1` (Part 4.6). The four `grep`/`awk` board-reading one-offs are harmless accretions.

```json
{
  "permissions": {
    "allow": [
      "Bash(grep -oE '^\\\\s*-?\\\\s*\\\\*?\\\\*?status\\\\*?\\\\*?:\\\\s*`?[a-z-]+' TASKBOARD.md -i)",
      "Bash(grep -oE '[a-z-]+$')",
      "Bash(grep -nE '^#{2,4} .*TASK-3[0-9][0-9]' TASKBOARD.md22)",
      "Bash(awk '/^#### TASK-\\(30[0-9]|31[0-9]|32[0-9]|33[0-9]\\)/{t=$0; getline; if\\($0 ~ /assignee/\\){a=$0; getline} print t; print \"   \" $0; print \"\"}' TASKBOARD.md)",
      "mcp__claude_ai_Slack__slack_read_channel",
      "PowerShell($p = Get-Process UnrealEditor -ErrorAction SilentlyContinue; if \\($p\\) { \"EDITOR RUNNING \\(pid $\\($p.Id\\)\\)\" } else { \"editor not running\" })",
      "mcp__unreal-mcp__call_tool",
      "Bash(\"C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe\" *)",
      "PowerShell(& \"C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe\" *)",
      "PowerShell(& \"C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe\" *)",
      "Agent",
      "PowerShell(Get-Process*)",
      "PowerShell(Get-FileHash*)",
      "PowerShell(Test-Path*)",
      "PowerShell(Get-ChildItem*)",
      "PowerShell(Get-Content*)",
      "PowerShell(Select-String*)",
      "PowerShell(git status*)",
      "PowerShell(git log*)",
      "PowerShell(git diff*)",
      "PowerShell(git show*)",
      "PowerShell(git check-ignore*)",
      "PowerShell(git ls-files*)",
      "PowerShell(git rev-parse*)",
      "PowerShell(git rev-list*)",
      "PowerShell(git cat-file*)",
      "Bash(git status*)",
      "Bash(git log*)",
      "Bash(git diff*)",
      "Bash(git show*)",
      "Bash(git check-ignore*)",
      "Bash(git ls-files*)",
      "Bash(git rev-parse*)",
      "Bash(git rev-list*)",
      "Bash(git cat-file*)",
      "Bash(\"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat\"*)",
      "PowerShell(& \"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/Build.bat\"*)",
      "PowerShell(& \"C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Build\\BatchFiles\\Build.bat\"*)",
      "Bash(\"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/RunUAT.bat\"*)",
      "PowerShell(& \"C:/Program Files/Epic Games/UE_5.8/Engine/Build/BatchFiles/RunUAT.bat\"*)",
      "PowerShell(& \"C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Build\\BatchFiles\\RunUAT.bat\"*)",
      "PowerShell(C:\\Python314\\python.exe \"C:\\GitProjects\\GitHub\\GitClaudeUnrealTesting\\GitClaudeUnrealTest\\Tools\\*)",
      "Bash(C:/Python314/python.exe \"C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Tools/*)",
      "mcp__unreal_inspector__execute_unreal_python_readonly",
      "mcp__unreal_inspector__fetch_animation_skill",
      "mcp__unreal_inspector__fetch_curve_best_practices",
      "mcp__unreal_inspector__fetch_eqs_best_practices",
      "mcp__unreal_inspector__fetch_gas_best_practices",
      "mcp__unreal_inspector__fetch_level_design_skill",
      "mcp__unreal_inspector__fetch_performance_best_practices",
      "mcp__unreal_inspector__fetch_python_best_practices",
      "mcp__unreal_inspector__fetch_timeline_best_practices",
      "mcp__unreal_inspector__fetch_ui_best_practices",
      "mcp__unreal_inspector__fetch_understandings",
      "mcp__unreal_inspector__get_asset_graph",
      "mcp__unreal_inspector__get_asset_meta",
      "mcp__unreal_inspector__get_asset_structs",
      "mcp__unreal_inspector__get_attribute_set",
      "mcp__unreal_inspector__get_available_actors_in_level",
      "mcp__unreal_inspector__get_blueprint_material_properties",
      "mcp__unreal_inspector__get_blueprint_properties_specifiers",
      "mcp__unreal_inspector__get_code_examples",
      "mcp__unreal_inspector__get_enums",
      "mcp__unreal_inspector__get_gameplay_tags",
      "mcp__unreal_inspector__get_headless_status",
      "mcp__unreal_inspector__get_text_file_contents",
      "mcp__unreal_inspector__get_unreal_context",
      "mcp__unreal_inspector__get_unreal_output_logs",
      "mcp__unreal_inspector__grep",
      "mcp__unreal_inspector__import_ActorComponentsAndSubobjects_understanding",
      "mcp__unreal_inspector__import_AssetCreation_understanding",
      "mcp__unreal_inspector__import_AssetRegistry_understanding",
      "mcp__unreal_inspector__import_AssetType_Blueprint_understanding",
      "mcp__unreal_inspector__import_AssetType_DataTable_understanding",
      "mcp__unreal_inspector__import_AssetType_GameplayEffect_understanding",
      "mcp__unreal_inspector__import_AssetType_Level_understanding",
      "mcp__unreal_inspector__import_AssetType_NiagaraSystem_understanding",
      "mcp__unreal_inspector__import_AssetType_UserWidget_understanding",
      "mcp__unreal_inspector__import_AssetValidation_understanding",
      "mcp__unreal_inspector__import_Color_understanding",
      "mcp__unreal_inspector__import_CurveAsset_understanding",
      "mcp__unreal_inspector__import_FileSystem_understanding",
      "mcp__unreal_inspector__import_IncludeOrImportModules_understanding",
      "mcp__unreal_inspector__import_Logs_understanding",
      "mcp__unreal_inspector__import_PropertyModification_understanding",
      "mcp__unreal_inspector__import_Subsystems_understanding",
      "mcp__unreal_inspector__query_unreal_project_assets",
      "mcp__unreal_inspector__quicksearch",
      "mcp__unreal_inspector__read_datatable_keys",
      "mcp__unreal_inspector__read_datatable_values",
      "mcp__unreal_inspector__review_blueprint",
      "mcp__unreal_inspector__search_geometry_scripts",
      "mcp__unreal_editor__attach_pie_frames",
      "mcp__unreal_editor__capture_pie_frame",
      "mcp__unreal_editor__get_actor_by_name_in_pie",
      "mcp__unreal_editor__get_actor_property_in_pie",
      "mcp__unreal_editor__get_input_mapping_context_keys",
      "mcp__unreal_editor__get_player_transform",
      "mcp__unreal_editor__get_screenshot_of_objects_for_verification",
      "mcp__unreal_editor__get_widget_property_in_pie",
      "mcp__unreal_editor__inject_input_action",
      "mcp__unreal_editor__is_pie_active",
      "mcp__unreal_editor__load_level",
      "mcp__unreal_editor__record_burst",
      "mcp__unreal_editor__run_verification_sequence",
      "mcp__unreal_editor__set_player_transform",
      "mcp__unreal_editor__simulate_button_press",
      "mcp__unreal_editor__simulate_key_press",
      "mcp__unreal_editor__simulate_left_stick",
      "mcp__unreal_editor__simulate_right_stick",
      "mcp__unreal_editor__start_pie",
      "mcp__unreal_editor__start_pie_recording",
      "mcp__unreal_editor__start_state_recording",
      "mcp__unreal_editor__stop_pie",
      "mcp__unreal_editor__stop_pie_recording",
      "mcp__unreal_editor__stop_state_recording",
      "mcp__unreal_editor__survey_pie_scene",
      "mcp__unreal_editor__take_editor_screenshot",
      "mcp__unreal_editor__ui_perform",
      "mcp__unreal_editor__ui_snapshot",
      "mcp__unreal_editor__ui_wait_for",
      "mcp__unreal_editor__verification_agent",
      "mcp__unreal_editor__wait_pie_frames",
      "mcp__unreal_editor__wait_pie_seconds",
      "PowerShell(Get-CimInstance Win32_Process*)",
      "PowerShell(powershell -NoProfile -File Tools\\stop_editor.ps1*)",
      "PowerShell(powershell -NoProfile -File Tools/stop_editor.ps1*)",
      "PowerShell(& \"C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Tools/stop_editor.ps1\"*)",
      "PowerShell(& \"C:\\GitProjects\\GitHub\\GitClaudeUnrealTesting\\GitClaudeUnrealTest\\Tools\\stop_editor.ps1\"*)",
      "Bash(powershell -NoProfile -File Tools/stop_editor.ps1*)",
      "PowerShell(powershell -NoProfile -File Tools\\launch_editor.ps1*)",
      "PowerShell(powershell -NoProfile -File Tools/launch_editor.ps1*)",
      "PowerShell(& \"C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Tools/launch_editor.ps1\"*)",
      "PowerShell(& \"C:\\GitProjects\\GitHub\\GitClaudeUnrealTesting\\GitClaudeUnrealTest\\Tools\\launch_editor.ps1\"*)",
      "Bash(powershell -NoProfile -File Tools/launch_editor.ps1*)",
      "PowerShell(powershell -NoProfile -File Tools\\sync_mirrors.ps1*)",
      "Bash(powershell -NoProfile -File Tools/sync_mirrors.ps1*)",
      "PowerShell(powershell -NoProfile -File Tools\\aura_sync.ps1*)",
      "Bash(powershell -NoProfile -File Tools/aura_sync.ps1*)",
      "Bash(C:/Python314/python.exe Tools/archive_board.py*)",
      "Bash(C:/Python314/python.exe Tools/split_conventions.py*)"
    ]
  },
  "enabledMcpjsonServers": [
    "unreal-mcp",
    "blender",
    "unreal_inspector",
    "unreal_editor"
  ]
}
```

---

## Appendix C — The two PreToolUse hook scripts, verbatim

Both live in `.claude/hooks/` and are wired by the committed `.claude/settings.json` (Part 5.4). They are POSIX shell run through Git Bash; no `jq` is required.

### C.1 `.claude/hooks/guard-secrets.sh`

```bash
#!/usr/bin/env bash
# PreToolUse guard (Edit|Write): deny writes whose content contains token-like secrets.
# Scans the raw hook-input JSON directly so it needs no JSON parser (no jq on this machine).
input=$(cat)

deny() {
  printf '{"hookSpecificOutput":{"hookEventName":"PreToolUse","permissionDecision":"deny","permissionDecisionReason":"Secret guard: %s. Use a placeholder or a non-committed local config instead of a real credential."}}\n' "$1"
  exit 0
}

# High-confidence token formats (AWS, GitHub, Slack, Anthropic/OpenAI, Google, JWT, PEM keys,
# Meshy msy_ — observed prefix, TASK-198)
if printf '%s' "$input" | grep -qE 'AKIA[0-9A-Z]{16}|gh[oprsu]_[A-Za-z0-9]{36,}|github_pat_[A-Za-z0-9_]{22,}|hf_[A-Za-z0-9]{20,}|msy_[A-Za-z0-9]{20,}|xox[abprs]-[0-9A-Za-z-]{10,}|sk-(ant-|proj-)?[A-Za-z0-9_-]{24,}|AIza[0-9A-Za-z_-]{35}|eyJ[A-Za-z0-9_-]{8,}\.eyJ|-----BEGIN [A-Z ]*PRIVATE KEY-----'; then
  deny "content matches a known API-token format"
fi

# Credential-keyword assignments (SecurityToken=..., ApiKey: "...", etc.).
# Value must be 16+ chars AND contain a digit, so identifier-only lines and
# placeholders like YOUR_PASSWORD_HERE or CreateDefaultSubobject don't trip it.
kv=$(printf '%s' "$input" | grep -oiE '(securitytoken|secret|api_?key|access_?token|auth_?token|passwd|password|credential)[a-z_]*[[:space:]]*[=:][[:space:]]*\\?"?[A-Za-z0-9+/=_-]{16,}')
if [ -n "$kv" ] && printf '%s' "$kv" | grep -q '[0-9]'; then
  deny "content assigns a credential-like value (e.g. SecurityToken=..., ApiKey=...)"
fi

exit 0
```

### C.2 `.claude/hooks/guard-generated-dirs.sh`

```bash
#!/usr/bin/env bash
# PreToolUse guard (Edit|Write): deny edits under UE-generated directories.
# Reading those dirs (e.g. Saved/Logs for QA) is unaffected; only Edit/Write is blocked.
input=$(cat)

fp=$(printf '%s' "$input" | grep -o '"file_path"[[:space:]]*:[[:space:]]*"[^"]*"' | head -1)

# Match Binaries/Intermediate/Saved/DerivedDataCache as a full path segment,
# with either separator style; leading '"' covers relative paths.
if printf '%s' "$fp" | grep -qE '["/\\](Binaries|Intermediate|Saved|DerivedDataCache)[/\\]'; then
  printf '{"hookSpecificOutput":{"hookEventName":"PreToolUse","permissionDecision":"deny","permissionDecisionReason":"%s"}}\n' \
    "Blocked: this path is inside a UE-generated directory (Binaries/Intermediate/Saved/DerivedDataCache). These are build outputs - never hand-edit them; rebuild or let the editor regenerate them instead."
  exit 0
fi

exit 0
```

### C.3 `.mcp.json` as it stands (all four servers)

```json
{
	"mcpServers":
	{
		"unreal-mcp":
		{
			"type": "http",
			"url": "http://127.0.0.1:8000/mcp"
		},
		"blender":
		{
			"command": "C:/Program Files/Blender Foundation/Blender 5.1/5.1/python/bin/python.exe",
			"args": [
				"-u",
				"-X",
				"utf8",
				"C:/GitProjects/GitHub/GitClaudeUnrealTesting/GitClaudeUnrealTest/Tools/blender_mcp_bridge.py"
			],
			"env":
			{
				"PYTHONUTF8": "1",
				"PYTHONIOENCODING": "utf-8"
			}
		},
		"unreal_inspector":
		{
			"type": "stdio",
			"command": "C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Marketplace/Aura/PortablePython/Windows/python.exe",
			"args": [
				"C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Marketplace/Aura/MCP/unreal_inspector.py"
			]
		},
		"unreal_editor":
		{
			"type": "stdio",
			"command": "C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Marketplace/Aura/PortablePython/Windows/python.exe",
			"args": [
				"C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Marketplace/Aura/MCP/unreal_editor.py"
			]
		}
	}
}
```

---

## Appendix D — House laws worth carrying to any new game (one line each)

- Files are the contract; Slack and chat are visibility. No post is authorization.
- Every feature request goes to the manager first; footage review is the one direct dispatch.
- Nothing commits without a PASS QA report; max 3 QA loops (QA, verify, or build failures all count), then escalate to the human.
- Nothing with a runtime acceptance criterion commits without a VERIFIED report; UNOBSERVABLE and MEASURED are recorded on the row, never read as a pass.
- Never push unless the human asks; he may self-commit and push milestones, so check `HEAD` and `origin` first.
- Commit by explicit pathspec; verify the COMMIT (`git show --stat HEAD`), never the index; the editor's Git plugin stages files nobody named.
- Parse build output for `Result:`; exit codes lie. Compile with the editor CLOSED; import with it OPEN and MCP up; never Live Coding.
- Close and relaunch the editor only through `Tools/stop_editor.ps1` and `Tools/launch_editor.ps1`: they classify every `UnrealEditor.exe` by command line, refuse a `-game` instance (the human's), kill by PID and prove the never-save hash. A permission file must grant the safe tool, never the raw cmdlet the law bans.
- A Blueprint compile does not dirty the package; a tool-side save returns true and writes nothing; the hash must change, and a compiled-Blueprint deliverable carries a human keystroke.
- Verify an LFS asset by oid vs sha256, never by size; `git diff` cannot see a line-ending-only change.
- The git root is one level up; a mis-anchored pathspec answers with silence, and silence is not "nothing to commit".
- Secrets are env-only or gitignored-ini-only; scripts redact; agents check existence, not value; the hook refuses token-shaped writes.
- Quota or credit exhaustion is an expected pause, not a failure: record the reset time and stop.
- Large regenerable artifacts (weights, cooked builds, caches, videos) are gitignored, never LFS'd.
- Nothing is imported unseen; nothing unverifiable is claimed; report outages instead of faking results.
- Same-path overwrite always; never delete-and-recreate an asset.
- Numbers over adjectives in every spec; acceptance criteria per mechanic, written as properties, never indices.
- Declare a verification ceiling at boarding, never discover it at 5b; split the criterion at the ceiling and spec the human's one-sentence hand check in advance with its false positives named.
- A negative without a discriminating control is `unobs`, never `measured`; a verifier that cannot fail is not a gate.
- A silence is either a zero or a void: check the log category's verbosity and the build configuration before scoring an absence.
- Dispatches point at the board row; a `status:`-line pointer is resolved against the target row's title.
- A permission asked for three times is a standing grant waiting to be requested: ask for the grant, never infer one.
- Ceilings are candidates for removal: enumerate the verbs of the tools you already hold before proposing a new one.
- Cite laws by namespace tag (`VER-§3 cl. 6`), never a bare section number; a `file:line` inside a law is a dated annotation. Law lives in `law/<NAMESPACE>.md` behind a generated index; amend a clause by rewriting it with one dated changelog line, never by striking it through forever.
- Archive the board at every milestone checkpoint; a `TASK-###` missing from the live board is looked up in `archive/` before anyone concludes it does not exist; ids never restart.
- A commit needs no board row of its own; the integrator records the hash on every row it ships.
- Give the verifier a dev-only switch for every clock in the game (the opponent, timers, auto-saves) as a reflected function it can call.

## Appendix E — Glossary of CONVENTIONS namespaces and recurring terms

| Tag / term | Meaning |
|---|---|
| `SC-§` | standing clauses: process and epistemic laws (measurement vs claim, git hazards, board hygiene); `SC-§102` the git root is one level up · `SC-§118` the `-game` session law · `SC-§125` the Blueprint-save / hash gate · `SC-§71b` a text reviewer accepts hashes as declared |
| `VER-§` | the Aura verification lane (`§0` purpose · `§1` report shape · `§2` serialization · `§3` announce-and-report, cl. 6 the standing PIE grant · `§4` evidence promotion · `§5` UNOBSERVABLE · `§6` the gate is binding · `§7` tool-grant law · `§8` ceilings declared at boarding · `§9` build configuration before an absence · `§10` board tokens · `§11` focus-ring capture · `§12` instrument register · `§13` speed law) |
| `FR-§` | footage review lane (`§0` rulings · `§1` naming · `§2` tool law · `§3` two-pass doctrine · `§4` reporting · `§5` limitations · `§6` evidence host row) |
| `PKG-§` / `SHIP-§` | the shipping package shape and the `/ship` procedure (Part 14) |
| `ACC-§` | accounts and Supabase (`§11` key law and config home · `§12` migrations and RLS) |
| `TL-§` | art-tooling hazard laws (UCX hulls, commandlet limits, suite totals, tracked tooling) |
| `KBD-§` | keyboard layout / positional input law (no letters in menu bindings; author mappings in the editor) |
| `DECK-§`, `UNCAP-§`, `CARDBAR-§`, `HELP-§`, `GFX-§`, `WM-§`, `WR-§`, `TOWER-§`, `STACK-§`, `CONTACT-§`, `NAV-§`, `AS-§`, `FOG-§`, `WITCH-§`, `GHOST-§`, `MARK-§`, `RECALL-§`, `HIGH-§`, `FIELD-§`, `CHAR-§`, `VIS-§`, `ROT-§`, `FT-§`, `SIE-§` | Siegebound feature laws; the pattern (one namespace per feature wave, dated, cited by tag) is what transfers |
| `TASK-###` | a board row; the join key across handoffs, QA, verify reports, Slack, and commit messages |
| `VID-###` | a footage diagnosis report |
| `FAB-###` | a marketplace request |
| `RCP-*` | a verifier recipe (measured input sequence) |
| 5a / 5b / 5c | compile and relaunch · runtime verify · assemble and commit (routing rule 5) |
| `built` | compiled and relaunched, not yet committed |
| `qa-passed` | a text-level verdict; hashes and runtime are accepted-as-declared until the build-master measures |
| `VERIFIED` / `VERIFY-FAILED` / `UNOBSERVABLE` / `MEASURED` | the four verify verdicts (Part 12.6) |
| handoff | `handoffs/TASK-###-<role>.md`, the downstream completion note |
| promoted evidence | the only frames that enter git, under `playtest-evidence/<date>/` |
| the never-save law | agents never save the level under test; close by terminate when only discardable assets are dirty |
| editor bounce | close → compile → relaunch → wait for MCP |
| controlled negative | a probe that fired, with a named control that discriminated, where "it did not move" is the answer |
| the one-click | Aura's "Add to Editor → Claude Code", which writes user-scope config the house rule then moves into the project |
