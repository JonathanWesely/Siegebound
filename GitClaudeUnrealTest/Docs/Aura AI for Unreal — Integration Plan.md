---
title: Aura AI for Unreal — Integration Plan
date: 2026-09-14
tags:
  - unreal-engine
  - claude
  - agents
  - aura
  - playtesting
  - setup-guide
aliases:
  - Aura AI
  - Aura Integration
  - Aura for Unreal
---

# Aura AI for Unreal — Integration Plan

What Aura is, where it actually improves the [[UE5 Agent Team System]], where it does **not** beat what is already built, and the exact setup to bolt it onto `GitClaudeUnrealTesting` without breaking the house laws in [[GitClaudeUnrealsetupdirections]].

> [!info] Status
> **Researched 2026-09-14** against the live repo (`CLAUDE.md`, `.claude/agents/*`, `.mcp.json`, `settings.local.json`, `Tools/`, the task board at TASK-1213) and Aura's official docs at the **Aura 1.0** release (2026-09-09). Nothing is installed yet. Every price and feature below has a source in [[#Sources]]; anything I could not confirm is marked ⚠️ unverified. The 8.6 MB `TASKBOARD.md` and 2.9 MB `CONVENTIONS.md` were sampled (headers, milestone lines, status counts), not read end to end — a fresh session should grep them for `PIE`, `playtest`, `Automation` and `SiegeCheatManager` before writing `VER-§`, because there are 162 / 93 / 52 / 11 existing mentions to stay consistent with.
> **Next step:** §6 is the handoff for a fresh Claude Code session — it contains the kickoff prompt to paste.

> [!abstract] TL;DR
> **Yes, worth it — for one reason above all: it closes the runtime-verification hole.** Today `qa-passed` means "the text says what it should" (the board's own words), nothing can simulate player input, nothing can see the viewport, and `/ship` suspends at `ADJUDICATE C3` waiting for a human to look at a screenshot. Aura's **Verification Agent** launches PIE, simulates input, takes screenshots, reads live actor/UMG state, and hands back a recorded video — and it exposes all of that to Claude Code as MCP tools. That is a new lane the current pipeline structurally cannot have.
>
> Everything else Aura does is **additive but not a replacement**: keep TRELLIS.2 + Meshy + headless Blender for production meshes (Aura's model gen is a Tripo-backed prototyping tool, ≤20k tris, "may not be production-ready"), keep Epic's built-in MCP (Aura can call it too), keep the footage lane (human playtests stay ground truth).
>
> **Plan:** free 2-week trial → measure credit burn on 3 real verification cases → then **Pro ($40/mo)**, not Indie, if the pipeline is going to run verification on every code task. Bridge it into Claude Code as a new `playtest-verifier` agent behind a new `verified` gate.

---

## 1. What Aura is (facts, not marketing)

| | |
|---|---|
| **Product** | Aura — "the agentic AI for game developers on Unreal Engine and Unity", [tryaura.dev](https://www.tryaura.dev/) |
| **Company** | Ramen (formerly Ramen VR; SF, YC S19, makers of *Zenith*). ~$40M raised. Acquired Coplay (Unity MCP) Mar 2026 and now owns the `flopperam/unreal-engine-mcp` OSS repo |
| **Current version** | **Aura 1.0**, released 2026-09-09 (numbering jumped from "Aura 15" in June — 1.0 is the "out of beta" marker, not a first release) |
| **UE support** | **Windows: UE 5.4–5.8. macOS: 5.7–5.8.** Launcher builds only; source-built engines need Enterprise. ✅ Our UE 5.8 launcher install is the best-supported target — several features (Sandbox, calling Epic's built-in MCP) are 5.8-only |
| **Where it installs** | **Engine level** (one install per `UE_5.x`), then enabled per project in Edit → Plugins. Ships as a plugin + an `Aura.exe` tray/standalone app talking to the plugin over **local port 41200** |
| **Models** | Hosted — "Built on Claude by Anthropic"; premium tiers list Fable, Opus, Sol, Grok, Kimi. **No bring-your-own-API-key** is documented. "Auto Mode" = free unlimited model, but ⚠️ *unlimited Auto requires training-on-your-conversations to be ON* (toggle in privacy settings) |
| **UX** | Its own chat (standalone Electron window or docked in-editor panel), Ask / Plan / Agent modes, ~4 parallel threads. **Plus** an "IDE / Claude Code (Alpha)" bridge that exposes Aura's tools as two stdio MCP servers |
| **Closed source** | No public repo for Aura itself. GamesBeat reports a Fab-store SKU with C++ source for custom engine builds |

### The two MCP servers (this is the part that matters for the agent team)

From the [IDE/Claude Code doc](https://www.tryaura.dev/documentation/aura-ide-mcp/), verbatim:

- **`unreal_inspector`** (read-only) — "Inspects project state without changing it: asset metadata and graphs, blueprint review, asset and code search, logs, read-only Python queries, and engine lifecycle (launch/recompile/shutdown), plus planning, skill references, and image/3D-model generation."
- **`unreal_editor`** (mutating) — "Makes changes in the editor: Blueprint and C++ authoring, materials, Niagara VFX, behavior trees, data tables, UMG widgets, Enhanced Input, and Unreal Python — **plus Play-In-Editor control and verification (input simulation, screenshots, profiling)**."

Supported clients: **Claude Code**, Cursor, Visual Studio, Rider (VS Code manual only). A `launch_unreal_project` tool can start the editor if it is not open.

> [!tip] Not either/or with Epic's MCP
> Same settings page, two buttons pointing opposite ways: **Add to Editor** exposes Aura's tools to Claude Code; **Set up Unreal MCP** "connects Aura *to* the MCP server built into Unreal Engine itself, so Aura can call the engine's own MCP tools alongside its own. This is UE 5.8 only." Our existing `unreal-mcp` on `127.0.0.1:8000` stays exactly as it is.

---

## 2. Where it fits — gap by gap against the current setup

| Area | What the pipeline does today | What Aura adds | Verdict |
|---|---|---|---|
| **Playtesting / runtime verification** | `qa-reviewer` holds no Bash and no MCP — the board's own header says a `qa-passed` is "a TEXT-LEVEL VERIFICATION". Setup doc §4.4: "**No input injection:** MCP has no lane to simulate player input, so 'PIE gameplay matrix' claims can't be made by agents." Runtime truth comes from your playtests + the footage lane, and `/ship` stops at `ADJUDICATE C3` for a human to look at one screenshot | **Verification Agent**: "launches Play-In-Editor, exercises the feature you asked for, reads the live gameplay state, and reports back with evidence, including recorded video of the run." Reads actors (spawned/where/exists), GAS attributes, **UMG widgets + values + visibility**, multi-client behaviour. 1.0: "under a minute per test case, 3 test cases verified in parallel." Exposed to Claude Code via `unreal_editor` | ⭐⭐⭐ **The reason to buy.** Fills the one gap every law on the board keeps apologising for |
| **Viewport viewing** | Epic's built-in toolsets (SceneTools / ActorTools / MaterialInstanceTools / ObjectTools) — no screenshot or viewport capture. Agents "see" the game only through your Game Bar clips → ffmpeg frames | `unreal_editor` verification includes **screenshots**; Aura also sees your current Content Browser / level selection and accepts pasted images | ⭐⭐ Real, but scoped to Aura's tools — it is not a general "look at the editor" camera for every agent |
| **3D modeling / mesh gen** | TRELLIS.2 (HF) + Meshy + headless Blender refine (remesh → UV → bake D/N/ORM → same-path FBX overwrite) tuned to the GDD §6 "premium stylized" bar; `rig_character.py` / `retarget_meshy_to_siegebiped.py` for the M7 rig workstream | Text/image → mesh via **Tripo**; default ≤20k tris (V3 ~50k); lands in `/Game/AuraModelGen/`; rigged SkeletalMesh from prompt; docs say "Assets may not be production-ready." Also: IK rigs / retargeters / batch anim conversion, **Material Agent** (procedural materials with custom HLSL nodes), **Niagara VFX agent**, image gen with in-painting | ⭐ for meshes (blockouts/placeholders only — and by the Meshy invariant, anything it makes must re-enter through Stage 2 before touching `Content/`). ⭐⭐ for **materials / Niagara / retargeting**, which the art-director currently does by hand-written `bpy` + MCP |
| **Blueprint / UMG** | Mostly C++ (Siegebound is a C++ codebase with ~45 automation test files); Blueprint via Epic MCP tools | Dedicated Blueprint agent ("Telos"): create/edit, Timeline nodes, static-analysis review for "typos, hanging nodes, logic errors"; ~3 min per Blueprint in Auto mode; UMG widgets; Enhanced Input; data tables (`DT_Cards` reimports are a recurring board task) | ⭐⭐ Useful for the `WBP_` widgets and the `BP_` subclasses; not transformative for the C++ core |
| **C++** | gameplay-programmer writes, qa-reviewer reads, build-master compiles with `Build.bat` and parses `Result:` | Coding Agent creates/edits/compiles C++ and "will trigger Live Coding to compile and hot-reload as needed" | ⚠️ Conflicts with house law — Live Coding's mutex is exactly what produces the "exit-code lie" (§2.2). Keep compilation with build-master; use Aura's C++ agent in **Ask/Plan** mode or with compile off |
| **Level design** | Battlefield is procedural C++ scatter (`BattlefieldScatter.cpp`, M6.5 laws); Fab packs via the human-fulfilled `FAB-REQUESTS.md` lane | Blockouts from natural language (Dynamic Mesh Actors, real cm), landscape heightmaps, PCG graphs (5.5+), batch property edits / renames / unused-asset finds via Unreal Python ("review scripts before running them"); **Fab store asset search** | ⭐ Nice for greybox iteration; the arena is already built |
| **Unattended runs / editor hygiene** | `bRemoteExecution=True` + graceful `quit_editor()`, Escape-to-cancel Save Content modals, `Stop-Process -Name UnrealEditor*` allow-rule, overnight runs (`OVERNIGHT-AUTH`, `MORNING-BRIEF` files exist) | **Automatic crash recovery** (Windows, standalone app only): diagnoses the crash report, incremental build if binaries are stale, relaunches "full editor or headless". **Headless mode**: standalone app auto-launches a lightweight background UE for the chosen `.uproject` | ⭐⭐ Directly relevant to overnight milestone runs — but it only recovers *Aura's* sessions, not a Claude Code chain that wedged the editor |
| **Safety net for asset edits** | "Nothing is imported unseen"; QA cannot inspect a `.uasset`; git is the only undo | **Filesystem Sandbox** (experimental, **5.8-only**): stages all asset changes in `Intermediate/Sandboxes/AuraSandbox`, Accept/Reject before anything touches real files. **C++ is not sandboxed**; per-file reject "may sometimes fail on assets with dependency chains" | ⭐⭐ Matches the house posture exactly; use it for every Aura asset mutation |
| **Skills / conventions** | `CLAUDE.md` + `CONVENTIONS.md` (2.9 MB of case law) + Obsidian skills | `Saved/.Aura/Skills/<name>/SKILL.md` — "A SKILL.md you write for Aura works in Claude Code, and a skill you already have for Claude Code works in Aura — same format, no conversion." `Saved/.Aura/project_memory.txt` for always-on context | ⭐⭐ Free interop — but note `Saved/` is gitignored, so both are machine-local (see §4 step 2.4) |
| **Cost model** | Claude subscription + HF PRO (~$9) + Meshy credits | Per-seat sub + premium credit; specialized agents (Verification, Blueprint) burn more than chat; "Unused credit does not roll over"; MCP usage from Claude Code is "charged to your Aura subscription, not your IDE's AI subscription" | See [[#5. Pricing and which tier]] |

### What Aura does **not** change

- **Production meshes stay on TRELLIS/Meshy/Blender.** Same-path overwrite, `T_/MI_/SM_` naming, Nanite-off, two-slot `[TeamRegion, <Asset>PBR]` — none of that exists in Aura's `/Game/AuraModelGen/` output. Treat Aura mesh gen as a blockout tool feeding the Inbox, never `Content/`.
- **Epic's MCP stays the actor/scene surface** for gameplay-programmer, art-director and build-master. Aura is a second server, not a swap.
- **Human playtests + the footage lane stay ground truth.** Aura verification is evidence *before* your playtest, not instead of it (`FR-§` remains).
- **Git stays with build-master, never-push law intact.** Aura 15 added shell commands "behind a consent prompt" (Perforce, Git, builds) — do not grant Aura git in Agent mode.

---

## 3. Cautions before paying

> [!warning] Read these once — they shape the architecture in §4
> 1. **The Claude Code bridge is labelled Alpha.** Aura's own words: "Less seamless than using Aura directly… you need to know which tools to invoke", "Integrations are only set up with one project at a time", "Existing MCP Configurations including Auto-generated comments… can break the one-click install", removal is manual.
> 2. **Aura's docs say the Claude Code config lives at `~/.claude/mcp.json` (user-level)** and the snippet uses a `"servers"` key (VS Code style). Our whole setup keeps servers in the project `.mcp.json` (`"mcpServers"` key) + `enabledMcpjsonServers`. ⚠️ Verify what the one-click actually writes and where Claude Code actually reads it (`/mcp` in the session shows connected servers) — then move the block into the project file so the repo stays the source of truth (§4 step 3.2).
> 3. **Privacy trade.** "Unlimited usage via Auto Mode requires training to be on." Training can be turned off in privacy settings (off by default only for Enterprise). Decide before indexing the GDD and the Siegebound source. Turning it off means Auto Mode is rate-limited, not unlimited.
> 4. **Indexing cap.** Semantic index covers projects <30,000 files by default and "the editor may slow down when Aura is indexing large files such as art assets." With the Fab packs (`MedievalCastle…`, `Megaplant_Library`, `Realistic_*`, `Tree_Pack_1`…) we are almost certainly over that — write `INDEX_IGNORE.txt` before the first index (§4 step 2.3).
> 5. **Two mutating agents on one editor is the same hazard as two Claude clients on Blender 9876.** Aura's Verification Agent *takes over PIE*. It must be serialized like the quiet-module build gate, and it violates the "when the human is present, closing/driving the editor is his call" law unless you are told first. Rule: Aura verification runs only when explicitly dispatched, one at a time, and the dispatch prompt says the editor will be driven.
> 6. **Reception is mixed and young.** Forum beta threads: installer shipped 5.3 DLLs while offering multiple engine versions (Mar 2026), "barely in Alpha state" (Apr 2026), Blueprint-editing complaints (Jan). StraySpark's benchmark (a competitor, self-declared bias) rated Aura "highest quality output for complex, Unreal-specific tasks" but "sometimes overconfident — generates code that looks correct but has subtle issues" and "vendor lock-in risk." → Aura output still goes through `qa-reviewer`; it never bypasses the QA gate.
> 7. **Crash recovery is Windows + standalone-app only**, and does nothing if a debugger is attached. Fine for us; just do not expect it to rescue a Claude Code–driven session.
> 8. **The Fab "$150 upfront, lifetime MCP usage + one-year subscription" SKU is single-sourced (GamesBeat).** I could not find the Fab listing itself. ⚠️ Verify on Fab before choosing it over the monthly tiers — if real, it is the best deal for a pipeline whose main use is MCP.

---

## 4. Setup — step by step

Same convention as [[GitClaudeUnrealsetupdirections]]: **[You]** = human clicks/accounts, **[Claude]** = a Claude Code session in the project root. Secrets law unchanged: no keys in files, chat or handoffs — Aura authenticates through its own account login, so there is no API key to store at all.

### Phase 0 — Trial and measure (before any money)

1. **[You]** Create an account at [tryaura.dev/about](https://www.tryaura.dev/about/). The **2-week trial is free, no card, unlimited Auto Mode + $10 premium credit.**
2. **[You]** Decide the training toggle (privacy settings) *before* the first index — see caution 3.
3. Pick **three verification cases from work that is already `qa-passed` on the board** (36 tasks are sitting there) where you know the true runtime answer — e.g. one ladder-climb rule (`TOWER-§8`), one fog behaviour (`SiegeFogVolume`), one deck-builder flow (`DECK-§`). Run each through Aura verification (Phase 2, step 2.6) and record: credit consumed, wall time, whether the verdict matched your playtest. **That number decides the tier.**

### Phase 1 — Install (engine level)

1. **[You]** Close the editor. From the Aura dashboard download the Windows installer; "Run the installer and select your Unreal Engine version" → **5.8**. It installs into the engine, so it becomes available to every 5.8 project (re-run the installer on an engine upgrade).
2. **[You]** Open `GitClaudeUnrealTest.uproject` → Edit → Plugins → search **Aura** → Enable → restart once. Expect the editor to add an `Aura` entry to the `.uproject` `Plugins` array — that is a committed file; let build-master commit it under a task like any config change.
3. **[You]** Click the new **Aura** toolbar button. Standalone: green 🟢 indicator top-left; docked: hover the 🟢 top-right. Test with *"Tell me about this project."* If it cannot connect: confirm `Aura.exe` is in the tray, that nothing else holds **port 41200**, and that the VPN/AV is not blocking loopback (same class of problem as the HF/Supabase TLS exclusions in THE LIST).
4. **[You]** Optional: Editor Preferences → **Aura Plugin Settings** → uncheck **Open Aura in Electron Window** to dock it. For long unattended verification runs prefer the standalone app (crash recovery only works there).

### Phase 2 — Configure for this project

1. **[You]** Aura Settings → **MCP Configuration → Set up Unreal MCP** — lets Aura call Epic's built-in tools alongside its own (5.8 only). Our `unreal-mcp` server config in `.mcp.json` is untouched.
2. **[You]** Enable the **Filesystem Sandbox** (experimental, 5.8; the toggle lives in Aura's settings — verify its exact location on first run) so every Aura asset mutation stages in `Intermediate/Sandboxes/AuraSandbox` for Accept/Reject. Remember it does **not** cover C++.
3. **[Claude]** Write `INDEX_IGNORE.txt` (project root — verify the exact location in Aura's Project Understanding doc on first run) excluding: `Content/Fab/`, `Content/MedievalCastleEnvironmentAndSiegeWeaponProps/`, `Content/Megaplant_Library/`, `Content/Realistic_*`, `Content/Tree_Pack_1/`, `Content/sA_*`, `Content/Prickly_Knight/`, `Content/RawAssets/`, `Models/`, `Intermediate/`, `Saved/`, `Binaries/`, `DerivedDataCache/`, `Tools/ArtPipeline/.venv/`, `Tools/ArtPipeline/Cache/`, `.claude/pipeline/playtest-evidence/`, and `packagedZIPofGame/`. Keep `Source/`, `Content/Blueprints|UI|Data|Maps|Materials|VFX`, `Docs/GDD.md`, `.claude/pipeline/CONVENTIONS.md`. Commit it (it is project config, not a secret).
4. **[Claude]** Seed `Saved/.Aura/project_memory.txt` with a **short** digest: the team table from `CLAUDE.md`, the asset-prefix + texture-suffix tables from `CONVENTIONS.md`, the build command, and the two laws Aura is most likely to violate (never Live-Coding-compile; never write `Content/` from model gen). ⚠️ `Saved/` is gitignored, so this file is machine-local — add a line to [[GitClaudeUnrealsetupdirections]] Chapter 11 saying "regenerate from `CLAUDE.md`" and keep the canonical copy at `Docs/AuraProjectMemory.md` with a copy step.
5. **[Claude]** Copy the skills you want shared into `Saved/.Aura/Skills/<name>/SKILL.md` (same gitignore caveat — canonical copies live in `.claude/skills/`, mirror them with a one-line PowerShell copy in the session-start checklist).
6. **[You]** First verification, interactively, in Aura's own chat: *"Verify that a Footman played from the hand walks to the enemy castle and attacks it; report actor positions and the castle health-bar widget value before and after."* Watch what it does to PIE, read the video it attaches, and note credit burn. This is the Phase 0 measurement.

### Phase 3 — Bridge into the Claude Code agent team

1. **[You]** Aura Settings → **MCP Configuration → Add to Editor → Claude Code** → fully restart Claude Code.
2. **[Claude]** Run `/mcp` in the session and confirm `unreal_inspector` and `unreal_editor` are connected. Then find what the one-click wrote (Aura docs say `~/.claude/mcp.json`; Claude Code may or may not read that path) and **move the two server blocks into the project `.mcp.json`** under `"mcpServers"` next to `unreal-mcp` and `blender`, converting the doc's `"servers"` shape:

   ```json
   "unreal_inspector": {
     "command": "<path Aura installed>/python.exe",
     "args": ["<path Aura installed>/Aura/MCP/unreal_inspector.py"]
   },
   "unreal_editor": {
     "command": "<path Aura installed>/python.exe",
     "args": ["<path Aura installed>/Aura/MCP/unreal_editor.py"]
   }
   ```

   Use the exact `python.exe` the installer wrote (not `C:\Python314` and not the uv 3.12 venv — Aura's scripts pin their own deps). Then add both names to `enabledMcpjsonServers` in `.claude/settings.local.json` and delete the user-level copy so there is one source of truth. Restart Claude Code again (it does not hot-reload `.mcp.json`).
3. **[Claude]** Permissions (Appendix D law — merge, never replace). Read-only tools can be allowed wholesale; mutating ones stay enumerated:

   ```jsonc
   "mcp__unreal_inspector__*",                 // read-only: safe to allow entirely
   "mcp__unreal_editor__<pie/verify tool>",    // enumerate the PIE + screenshot + verification tools by their real names after /mcp lists them
   // ⛔ do NOT allow the C++ authoring / compile / shell tools of unreal_editor — compilation stays with build-master
   ```

4. **[Claude]** New agent `.claude/agents/playtest-verifier.md`:

   ```markdown
   ---
   name: playtest-verifier
   description: Runs Aura's Play-In-Editor verification against a task's acceptance criteria and writes an evidence-backed runtime report. Use when a code task is qa-passed and its spec names a runtime-observable acceptance criterion. Never edits code or assets, never compiles, never runs Git.
   tools: Read, Grep, Glob, Write, Edit, mcp__unreal_inspector__*, <the enumerated unreal_editor PIE/verify/screenshot tools>, mcp__claude_ai_Slack__slack_send_message, mcp__claude_ai_Slack__slack_read_thread
   ---
   ```

   Body, in the house voice: reads the task's `acceptance:` lines from TASKBOARD + the programmer handoff; runs ONE verification at a time; every claim carries a screenshot path / video path / a quoted actor or widget value ("the castle health widget read 87 after the third hit at t=0:41", never "damage works"); writes `.claude/pipeline/qa/TASK-###-verify.md` with `Verdict: VERIFIED | VERIFY-FAILED | UNOBSERVABLE`; flips only its own row's `status:` (`verified` / `verify-failed`); posts once in the 🧪 Dev & QA thread prefixed `🎮 VERIFIER:`. `UNOBSERVABLE` is the honest answer for pure-data or editor-only tasks — Aura's own docs: "Changes with no runtime signal… can't be meaningfully verified."
5. **[Claude]** `CLAUDE.md` routing edits. ⚠️ **Verification runs AFTER the compile, never before** — `qa-reviewer` reviews code pre-compile, so the running editor does not yet contain the change; PIE can only test binaries that exist. Build-master's single "compile + assemble + commit" step therefore splits in two around the verifier:
   - **Step 5a** — `qa-passed` → `build-master` compiles (`Result: Succeeded` law) and, for C++ changes, relaunches the editor on the new binaries (the graceful-quit lane from §4.3 of the setup doc; never Live Coding). Status → `built`. No commit yet.
   - **Step 5b** — if the spec has a runtime acceptance criterion → invoke `playtest-verifier`. `verify-failed` routes back to `gameplay-programmer` and counts as a QA loop (same max-3-then-escalate). Blueprint/asset-only tasks skip 5a and go straight here.
   - **Step 5c** — `verified` (or `UNOBSERVABLE`, or no runtime criterion) → `build-master` assembles and commits as today.
   **Hard gate amendment:** *"Nothing with a runtime acceptance criterion is committed without a `VERIFIED` report; `UNOBSERVABLE` is recorded on the row, not treated as a pass."*
   **Why not just give `qa-reviewer` the PIE tools:** it sits before the compile; `unreal_editor` bundles C++ authoring, Live Coding compile and shell tools with the PIE tools, so granting it would break QA's read-only-by-design posture; and keeping "the text says what it should" (QA) and "the engine did what it should" (verifier) as two verdicts keeps both honest. Also add to the team table: `playtest-verifier | Runs Aura PIE verification, writes runtime evidence | Editing code/art, compiling, Git`. Give `qa-reviewer` **only** `mcp__unreal_inspector__*` — it stays read-only by design but can now actually look at a Blueprint graph and the editor log instead of accepting them "as declared" (`SC-§71b`).
6. **[Claude]** `CONVENTIONS.md`: new namespace **`VER-§`** (verification lane) — report shape, the serialization rule (one Aura verification at a time; never concurrent with a build-master assemble or an art-director import), the "Jonathan present ⇒ announce before driving PIE" rule, evidence promotion into `playtest-evidence/<date>/` reusing the `FR-§1` naming.
7. **[Claude]** `/ship` (`SHIP-§8`): Aura verification can *pre-screen* `ADJUDICATE C3` — run the main-menu → match → HUD-present check and attach its screenshot to the suspension — but **the human adjudication stays**. The law that a script cannot make that judgment was written after a build shipped with "an empty deck, no HUD and no hero" (`PKG-§5a`); Aura makes the evidence better, it does not remove the stop.

### Phase 4 — Pilot on real backlog

Run the three Phase-0 cases through the full chain (`qa-passed` → `build-master` compile → `playtest-verifier` → `build-master` commit). Compare each `TASK-###-verify.md` to what you saw in your own playtest. Only after three matches does the `verified` gate become binding for new tasks; until then it is advisory and says so on the row.

### Verification checklist (append to Appendix A of the setup doc)

| # | Probe | Expect |
|---|---|---|
| 12 | Aura toolbar → 🟢 → "Tell me about this project" | Project summary that names `Siegebound` classes, not the template |
| 13 | Claude Code `/mcp` | `unreal_inspector`, `unreal_editor`, `unreal-mcp`, `blender` all connected |
| 14 | `playtest-verifier` on a known-good task | `Verdict: VERIFIED`, a video/screenshot path that exists, quoted widget values |
| 15 | Same on a deliberately broken branch | `VERIFY-FAILED` with the failing observation — a verifier that cannot fail is not a gate |
| 16 | Sandbox on → Aura edits a widget → Reject | Real `Content/` file unchanged (`git status` clean) |

---

## 5. Pricing and which tier

Per-seat, with a monthly **premium credit** billed by tokens (input, output, cache read/write); specialized agents cost more than chat; unused credit does not roll over; Pro+ can set an overage cap. Source: [About / pricing](https://www.tryaura.dev/about/), [Pricing Explained](https://www.tryaura.dev/documentation/pricing-explained/).

| Tier | Price | Premium credit / mo | Includes |
|---|---|---|---|
| **Trial** | Free, 2 weeks, no card | $10 | Unlimited Auto Mode |
| **Indie** | $20 list, **$10/mo effective** (the 50% beta discount was made permanent at 1.0); $120/yr | $15 | Unlimited Auto, Unreal + Unity, **external-agent MCP usage**, 3D model + audio gen |
| **Pro** ★ | $40/mo; $360/yr | $60 | + higher rate limits, pay-as-you-go overages, Super Mode (max reasoning) |
| **Ultimate** | $200/mo; $1,800/yr | $335 | + highest rate limits |
| **Enterprise** | Contact sales | — | Source access, source-built engines, onboarding |
| ⚠️ **Fab SKU** | $150 upfront (GamesBeat) | — | "lifetime MCP usage as well as a one-year subscription" — unverified on Fab |

> [!tip] Recommendation
> **Trial → Pro.** Indie's $15 credit is priced for chat; the Verification Agent is the one thing we want and it is the expensive agent. If Phase 0 shows three verifications cost more than ~$3–4 of credit, Indie would be exhausted in a week of pipeline use and, because credit does not roll over, would effectively become "Auto Mode only". Pro's $60 + an overage cap you set is the right shape for a pipeline that runs verification per task. Check the Fab SKU first — if the lifetime-MCP claim holds, it dominates for our use.
>
> Budget line alongside the existing stack: HF PRO ~$9 + Meshy credits + Aura Pro $40.

---

## 6. Handoff — what the next Claude Code session should do

> [!todo] Paste this as the first message of a fresh `claude` session in `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest`
> *"Read `C:\GitProjects\GitHub\MyObsidianVault\JonWesOBVault\Aura AI for Unreal — Integration Plan.md` end to end. It is the plan for adding Aura AI (tryaura.dev) to this pipeline. Jonathan will do every **[You]** step himself; you own every **[Claude]** step in §4 and every item in §6 below, in order. Tell me which step you are on before you start it, and stop at each ⛔ STOP. Do not install anything, do not push, and do not change any agent's tool grants beyond what the plan names."*

Work items for that session, in order (each is a normal boarded task through `manager` — the plan is the spec, this list is the decomposition):

1. **Pre-flight (no changes yet).** Confirm the current state matches this note: `CLAUDE.md` team table has six agents; `.mcp.json` has `unreal-mcp` + `blender`; `settings.local.json` `enabledMcpjsonServers` lists both; `Docs/setupdirections.md` is the same file as the vault's `GitClaudeUnrealsetupdirections.md` (both 55,770 bytes on 2026-09-14). Report drift before proceeding. ⛔ STOP if `Aura` is already in the `.uproject` Plugins array — Jonathan may have started without you.
2. **Wait for Phase 1.** Nothing in this list runs until Jonathan says the Aura toolbar button shows 🟢 and *"Tell me about this project"* returned Siegebound classes (Phase 1 step 3). Ask for that confirmation explicitly.
3. **`INDEX_IGNORE.txt`** (Phase 2 step 3) — write it at the location Aura's Project Understanding doc specifies (verify with a WebFetch of https://www.tryaura.dev/documentation/project-understanding/ first; if the doc is ambiguous, project root). Commit under a task; it is config, not a secret.
4. **`Docs/AuraProjectMemory.md`** (canonical, committed) + a copy to `Saved/.Aura/project_memory.txt` (machine-local, gitignored). Content: the six-agent table, asset prefixes + texture suffixes from `CONVENTIONS.md`, the build command, and these three lines verbatim: *"Never compile via Live Coding — compilation belongs to build-master via Build.bat."* · *"Never write generated meshes into Content/ — they enter through Tools/ArtPipeline Stage 2."* · *"Never run Git."* Keep it under ~150 lines; Aura injects it every turn.
5. **Skills mirror.** Add one PowerShell line to the session-start checklist in `CLAUDE.md` (or a `Tools/aura_sync.ps1`) that copies `.claude/skills/*` → `Saved/.Aura/Skills/*`. Only the skills that make sense inside the editor (none of the Obsidian ones).
6. **Phase 0 measurement.** Ask Jonathan for the three verification cases he chose (§4 Phase 0 step 3) and their expected outcomes. He runs them in Aura's own chat; you record credit consumed, wall time, verdict-vs-playtest in a new file `.claude/pipeline/qa/AURA-PHASE0.md`. ⛔ STOP — the tier decision (§5) is his, made from that file.
7. **Phase 3 bridge** (only after he says Add to Editor → Claude Code is done and Claude Code was restarted): run `/mcp`, record the exact tool names under `unreal_inspector` and `unreal_editor` into `AURA-PHASE0.md`, locate the file the one-click wrote, move the two server blocks into the project `.mcp.json` (`"mcpServers"` key), add both names to `enabledMcpjsonServers`, ask Jonathan to delete the user-level copy, restart, re-run `/mcp`.
8. **Permissions** — merge into `settings.local.json` `permissions.allow`: `mcp__unreal_inspector__*` wholesale, and the **enumerated** PIE / verify / screenshot tool names from step 7. ⛔ Never `mcp__unreal_editor__*` — that would grant C++ authoring, Live Coding compile and shell tools to every agent.
9. **`.claude/agents/playtest-verifier.md`** — frontmatter per §4 Phase 3 step 4 with the real tool names; body per the draft below.
10. **`CLAUDE.md` edits** — team-table row, routing steps 5a/5b/5c (compile → verify → commit; the verifier never runs on un-compiled code), hard-gate amendment, and one line under Hard gates: *"Aura verification drives PIE; when Jonathan is present the dispatch announces it first and waits for a go."*
11. **`CONVENTIONS.md` → `VER-§`** — the verification lane namespace (report shape, one-at-a-time serialization with build-master assemble and art-director import, evidence naming reusing `FR-§1`, `UNOBSERVABLE` semantics, "advisory until three matches" pilot clause).
12. **`qa-reviewer.md`** — add `mcp__unreal_inspector__*` to its `tools:` line and one paragraph saying it may now *inspect* Blueprint graphs and the editor log to check a declaration, and that its no-mutation posture is unchanged.
13. **`Docs/setupdirections.md` + vault `GitClaudeUnrealsetupdirections.md`** — new **Chapter 11 — Aura** (install, port 41200, Set up Unreal MCP, Sandbox, `INDEX_IGNORE`, project memory regeneration, the two MCP servers, allow-list), THE LIST row 17 (Aura account, paid), Appendix A rows 12–16 from this note, Appendix B entries for the ⚠️ unverified items in §3 and §7. Keep both copies byte-identical (they are today).
14. **`/ship` (`SHIP-§8`)** — add the optional Aura pre-screen of `ADJUDICATE C3` described in §4 Phase 3 step 7. The human adjudication stays; do not touch `ship.ps1`'s verdict lines.
15. **Phase 4 pilot** — run the three Phase-0 tasks through `qa-passed → build-master compile → playtest-verifier → build-master commit`. Report the three `TASK-###-verify.md` files against Jonathan's playtest notes. ⛔ STOP — he decides whether the `verified` gate becomes binding.
16. **Vault sync** — update [[UE5 Agent Team System]] (tool-layer table gets an Aura row; roster gets `playtest-verifier`) and this note's Status callout with what was actually installed, the real tool names, the tier chosen, and the credit-per-verification figure.

### Draft body for `playtest-verifier.md`

```markdown
You are the Playtest Verifier for GitClaudeUnrealTest (UE 5.8). You turn a task's
acceptance criteria into a Play-In-Editor run through Aura's verification tools and
write down what the engine actually did. You VERIFY ONLY: you never edit code or
assets, never compile, never run Git, never touch the Blender or unreal-mcp servers.

## Inputs
- Dispatch prompt: the TASK-###, the acceptance lines quoted from TASKBOARD.md, the
  programmer handoff path, and whether Jonathan is present (if yes, the orchestrator
  has already announced that PIE will be driven — do not start until told "go").
- Read-only context: TASKBOARD.md (your row only), CONVENTIONS.md VER-§, the handoff.

## How you work
1. ONE verification at a time. Never run while build-master is assembling or
   art-director is importing (VER-§ serialization) — if the board shows either
   `integrating`, stop and report.
2. Confirm the editor is up via unreal_inspector before any PIE call, AND that the
   task's row reads `built` (C++) or `qa-passed` (Blueprint/asset-only) — you only
   ever test binaries that exist. If Aura is not connected, report the outage —
   never fake a run.
3. Map each acceptance line to an observable: an actor that must exist/move, a widget
   value, a log line, a screenshot. If a line has no runtime signal, mark it
   UNOBSERVABLE and say why — do not invent a proxy.
4. Run the verification. Budget: ≤ 3 attempts per task; each attempt's evidence is
   kept even if a later one is cleaner.
5. Pixel-proof doctrine (FR-§ applies): report OBSERVATIONS with evidence paths and
   quoted values — "castle health widget read 87 after the third Footman hit at
   t=0:41 (screenshot …)" — never conclusions. Mechanism lines are HYPOTHESIS.
6. Promote the proving screenshots/video into
   `.claude/pipeline/playtest-evidence/<date>/` using the FR-§1 naming with a `VER`
   prefix; everything else stays in Saved/ (gitignored).

## Output — `.claude/pipeline/qa/TASK-###-verify.md`
# Verification — TASK-###
Verdict: VERIFIED | VERIFY-FAILED | UNOBSERVABLE
Editor/Aura state: <connected, map, PIE mode, attempts used>
## Acceptance lines → observations
| # | acceptance line | observable chosen | observed (quoted values, evidence path) | pass/fail/unobs |
## Evidence (promoted)
## Hypotheses (not verdicts)
## Not examined / limitations this run

Then flip ONLY your own row's `status:` to `verified` or `verify-failed`
(UNOBSERVABLE leaves the status at `qa-passed` and appends `verify: unobservable`).
Post once in the Dev & QA standing thread prefixed `🎮 VERIFIER:` + status emoji +
TASK-### (thread_ts in SLACK.md; return the text for proxy if the tools are absent).
```

---

## 7. Open questions to settle on first run

- [ ] Exact tool names under `mcp__unreal_editor__*` (needed for the enumerated allow-list and the agent `tools:` line) — from `/mcp` after Phase 3.
- [ ] Where the one-click actually wrote the Claude Code config, and whether Claude Code reads `~/.claude/mcp.json` at all.
- [ ] Whether Aura's PIE verification can drive **our** Enhanced Input mappings (positional-layout keyboard law, `KBD-§`) or only default bindings.
- [ ] Credit per verification run (Phase 0 measurement) → tier decision.
- [ ] Does enabling the plugin modify `.uproject`, and does the Sandbox plugin add a second entry?
- [ ] Fab listing: does the $150 lifetime-MCP SKU exist, and does it include the standalone app + crash recovery?
- [ ] Can Aura's verification observe our C++ `USiegeAssistantSnapshot` / cheat-manager state directly, or only actors + UMG? (Docs list actors, GAS, UMG, multiplayer — we do not use GAS.)

---

## Related Notes

- [[UE5 Agent Team System]]
- [[GitClaudeUnrealsetupdirections]]
- [[Claude + Git Setup in Unreal Engine 5]]
- [[Siegebound - Game Concept]]
- [[UE5 AI Art Pipeline]]
- [[Blender MCP — official addon vs uvx client]]
- [[Agent Team Communication Best Practices]]

## Sources

**Aura official**
- [tryaura.dev](https://www.tryaura.dev/) · [About + pricing + FAQ](https://www.tryaura.dev/about/) · [Docs index](https://www.tryaura.dev/documentation/)
- [Quick Start](https://www.tryaura.dev/documentation/quick-start/) · [IDE / Claude Code (Alpha)](https://www.tryaura.dev/documentation/aura-ide-mcp/) · [Verification Agent](https://www.tryaura.dev/documentation/verification-agent/)
- [Blueprints](https://www.tryaura.dev/documentation/blueprints/) · [Coding Agent C++](https://www.tryaura.dev/documentation/coding-agent-cpp/) · [Level Design Agent](https://www.tryaura.dev/documentation/level-design-agent/) · [Editor Agent](https://www.tryaura.dev/documentation/editor-agent/) · [Art Tooling](https://www.tryaura.dev/documentation/art-tooling/)
- [Filesystem Sandbox](https://www.tryaura.dev/documentation/filesystem-sandbox/) · [Crash Recovery](https://www.tryaura.dev/documentation/crash-recovery/) · [Advanced Settings](https://www.tryaura.dev/documentation/advanced-settings/) · [Aura Skills](https://www.tryaura.dev/documentation/aura-skills/) · [Project Understanding](https://www.tryaura.dev/documentation/project-understanding/) · [Giving Aura Context](https://www.tryaura.dev/documentation/giving-aura-context/) · [Connection Issues](https://www.tryaura.dev/documentation/connection-issues/) · [Pricing Explained](https://www.tryaura.dev/documentation/pricing-explained/)
- [Aura 1.0 launch post](https://www.tryaura.dev/updates/aura-1-0-launches-today) · [Aura 15 post](https://www.tryaura.dev/updates/introducing-aura-15) · [Updates](https://www.tryaura.dev/updates/)

**Press / third party**
- [GamesBeat — Aura 1.0 interview (Fab $150 SKU, tiers, speed claims)](https://gamesbeat.com/ramen-launches-aura-1-0-to-push-agentic-ai-in-game-development/) · [BusinessWire — Aura 1.0](https://www.businesswire.com/news/home/20260909618328/en/Aura-1.0-Launches-Today-Pushing-the-Frontier-of-Agentic-AI-in-Game-Development) · [BusinessWire — Aura 15](https://www.businesswire.com/news/home/20260626815100/en/Aura-15.0-Releases-with-New-Features-and-Unlimited-Usage-for-Unreal-Engine-and-Unity) · [BusinessWire — Coplay acquisition](https://www.businesswire.com/news/home/20260316440483/en/GDC-Ramen-Acquisition-of-Coplay-Brings-Together-the-Best-In-Class-Multi-agent-AI-Assistants-for-Unreal-Engine-and-Unity-Under-One-Roof) · [PRNewswire — Jan 2026 launch](https://www.prnewswire.com/news-releases/aura-ai-assistant-for-unreal-engine-launches-vr-studio-ships-game-in-half-the-time-with-new-agent-capabilities-302651608.html)
- [Epic — Unreal MCP in Unreal Editor (5.8 toolsets, transports, limits)](https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor?lang=en-US)
- [Unreal forums — Aura launch thread (beta reception)](https://forums.unrealengine.com/t/aura-ai-agent-for-unreal-editor/2689209) · [Unreal forums — Aura 15 thread](https://forums.unrealengine.com/t/aura-now-unlimited-unreal-5-8-sandbox-integration-mcp-claude-skills/2731687)
- [StraySpark — Aura vs MCP (competitor benchmark, declared bias)](https://www.strayspark.studio/blog/aura-vs-mcp-ai-assistants-unreal-engine-2026) · [StraySpark — 5.7 assistant vs MCP vs Aura](https://www.strayspark.studio/blog/ue-57-ai-assistant-vs-mcp-vs-aura-comparison)
- [flopperam/unreal-engine-mcp (now owned by Aura)](https://github.com/flopperam/unreal-engine-mcp)
