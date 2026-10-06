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

What Aura is, where it actually improves the [[UE5 Agent Team System]], where it does **not** beat what is already built, and the exact setup to bolt it onto `GitClaudeUnrealTesting` without breaking the house laws in [[GameDevSetup]] (formerly `GitClaudeUnrealsetupdirections`, retired 2026-10-03).

> [!info] Status — as researched (historical)
> **Researched 2026-09-14** against the live repo (`CLAUDE.md`, `.claude/agents/*`, `.mcp.json`, `settings.local.json`, `Tools/`, the task board at TASK-1213) and Aura's official docs at the **Aura 1.0** release (2026-09-09). ~~Nothing is installed yet.~~ → **superseded: installed 2026-09-13; see the *Installed* callout below.** Every price and feature below has a source in [[#Sources]]; anything I could not confirm is marked ⚠️ unverified. The 8.6 MB `TASKBOARD.md` and 2.9 MB `CONVENTIONS.md` were sampled (headers, milestone lines, status counts), not read end to end — a fresh session should grep them for `PIE`, `playtest`, `Automation` and `SiegeCheatManager` before writing `VER-§`, because there are 162 / 93 / 52 / 11 existing mentions to stay consistent with.
> ~~**Next step:** §6 is the handoff for a fresh Claude Code session — it contains the kickoff prompt to paste.~~ → **SPENT** — §6 ran as `TASK-1213`..`TASK-1273`; the outcome is the callout below.

> [!tip] Current state — 2026-10-03 (consolidation pass; the live procedure is now `Docs/GameDevSetup.md` **Part 12**, vault twin [[GameDevSetup]])
> This plan is the **research and decision record** (written 2026-09-14, amended in place through 2026-09-21). It is kept as history; nothing below is the live procedure any more. What changed after its last amendment, dated:
>
> | Date | Change |
> |---|---|
> | 2026-09-22 | Plugin 1.0.5 → **1.0.6** (`Version 74`); re-census of both servers: 62 inspector + 114 editor names, **zero** added/removed/renamed; the `ui_perform` click ceiling re-measured and it HELD (`TASK-1390`, `MEASURED`) |
> | 2026-09-22 | A Unit card was **played from hand** inside one batched `run_verification_sequence` (`SetMouseLocation` aim + `simulate_key_press "LeftMouseButton"`), twice, with omitted-set controls (`TASK-1391`) — the "the fix is a tool, not a row" claim was struck; the verb was already granted and unenumerated |
> | 2026-09-23 | The `call_actor_function` grant question was put to Jonathan and **declined**: declared per row, never granted (`VER-§7` cl. 2) |
> | 2026-09-24 | `VER-§9` (a log-line absence needs the build configuration named), `VER-§10` (a 🚧 flips the row to `blocked`; `MEASURED` flips to `verified` with the word in the status line), `VER-§11` (focus-ring pixel recipe), `VER-§12` (the instrument register), `PKG-§14` (cook-recipe clause) |
> | 2026-09-25 | MENU-NAV-PARITY shipped: the whole main menu is keyboard/gamepad/`IA_Menu*`-navigable; 8 of 10 screens `VERIFIED`, 2 `MEASURED` as instrument limits |
> | 2026-09-26 | `VER-§13` **speed law** (batch into ONE sequence, read back and top up because rapid batches drop gestures, recipe library `Tools/Verify/recipes/RCP-*.md`); `ui_perform` **`double_click` fires `UButton.OnClicked`** (the UMG blind spot narrowed); verifier model pinned `claude-opus-5-5[1m]` / `effort: medium` (dispatch without a `model` parameter); **every PIE run records** and the orchestrator remuxes `recording.h264` → `.mp4` with `ffmpeg -framerate 30 -i recording.h264 -c copy recording.mp4`; the keyboard set-active door (`IA_MenuSecondary`) + held-key repeat filter |
> | 2026-09-27 | `ui_perform` **clears Slate keyboard focus** — the call, not the step (`VER-§12` cl. 7f); `simulate_key_press` RMB measured negative; the report records what the dispatch said about the PIE announcement |
> | 2026-09-28/29 | `VER-§12` cl. 7g: a Blueprint in `BS_ERROR` anywhere in editor memory raises a blocking modal at PIE start (the archery-pack demo trio was deleted, `ab57522`); Jonathan's Down+Enter sitting landed `MEASURED` (real keys route through Slate's own focus navigation, not the `IA_Menu*` handlers); help-page numbers now read from their owner classes, `VERIFIED`; last Aura-lane commits `dcfadb9`, `5a2f5de` ("finally done with Aura") |
>
> **Still open on 2026-10-03:** credit per verification (visible in no tool reply; closes only by a dashboard read or an overage receipt) · `TASK-1368` (the sequence runner's step-vocabulary census with schema text) · the positional `KBD-§` key layout under injection · whether the Filesystem Sandbox was ever enabled · the Fab SKU non-finding. The design lessons for a NEW game (give every action an Input Action door, focus the button not the root, deterministic PIE start, reflected state, grep-able log lines, declare ceilings at boarding) are written up once in `Docs/GameDevSetup.md` **Part 13**.

> [!success] Installed — status as of 2026-09-21 (`TASK-1231`, amended `TASK-1371`)
> **The lane is live and the gate is binding.** Every number below is copied from `.claude/pipeline/qa/AURA-PHASE0.md` with the `MEASURED BY` label it carries there, or re-measured on disk and labelled as such — ⛔ nothing here is re-derived, and a column nobody measured reads **OWED**, never an estimate.
>
> | | |
> |---|---|
> | **Installed version** | ~~**Aura 1.0.5** — `"VersionName": "1.0.5"`, `"Version": 73`~~ → **CORRECTED 2026-09-21 (`TASK-1371`): Aura 1.0.6 — `"VersionName": "1.0.6"`, `"Version": 74`**, `"EngineVersion": "5.8.0"`, `CreatedBy: RamenVR`. RE-MEASURED BY GAMEPLAY-PROGRAMMER (on disk, `Read` of `C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Marketplace\Aura\Aura.uplugin`, 2026-09-21). The 1.0.5 / 73 reading was correct when it was taken (2026-09-20) — ⚠️ **this row is a point-in-time reading of a self-updating plugin and will go stale again without telling anyone; re-read the `.uplugin`, never cite this line as current.** §1's "Aura 1.0, released 2026-09-09" is the *release*; 1.0.6 is the point release actually on this machine |
> | **Install date** | **2026-09-13**, engine-level for UE 5.8, 🟢 toolbar, project probe passed — 🧑 MEASURED BY JONATHAN (*"I have finished steps 1-5, everything seems good"*, relayed on `TASK-1214`). He committed the `.uproject` entry himself in `0399d1c` |
> | **Training toggle** | **ON** (kept — unlimited Auto Mode) — 🧑 MEASURED BY JONATHAN (Aura chat, 2026-09-13; `AURA-PHASE0.md` §Totals) |
> | **Tier chosen** | ✅ **`Pro` — DECIDED 2026-09-21** (`TASK-1215` stage B, now `done`). 🧑 His words, verbatim, in Claude Code: *"I just upgraded to the Aura Pro subscription."* ⚠️ Labelled as an **ACCOUNT of a purchase he made**, ⛔ not as a measurement anyone can re-take on this machine. **SOURCE: `TASKBOARD.md` → `TASK-1215`'s `status:` line, copied, ⛔ not paraphrased — ⛔ no pricing page was read and no number here is re-derived** (`Read`, 2026-09-21). **Why `Pro` and not Ultimate** — kept because a bare "Pro" loses the reason: Ultimate is ~5× the price for ~5.6× the credit ⇒ **near-linear** ⇒ it buys **headroom, not efficiency**; `Pro`'s decisive property is that **overage is PURCHASABLE, and the top-up is itself the measurement**. ~~⏳ OWED — no tier decision exists … §5's *Trial → Pro* below is a recommendation, ⛔ not a decision~~ (struck 2026-09-21, `TASK-1371`; §5 below is struck to match) |
> | **Credit per verification** | ⏳ **OWED — "not visible" in every tool reply.** `AURA-PHASE0.md` §Pilot carries `not visible` in all five rows — *"absence of the field, not a zero"* — MEASURED BY THE PLAYTEST-VERIFIER (2026-09-14). The one figure he ever supplied is the **context-window** gauge *"18% used context (90k of 500k tokens)"* across his three chat probes — 🧑 MEASURED BY JONATHAN (2026-09-13) — ⚠️ a context reading, ⛔ **never** a dollar figure. ⇒ §5's "~$3–4 per three verifications" arithmetic is **still unmeasured** … 🚨 **AND THE HALF A READER WILL ELIDE, ADDED 2026-09-21 (`TASK-1371`): ⛔ A DECIDED TIER IS NOT A CHARACTERISED COST.** The tier row above is now CLOSED; **this row is still OPEN and may stay so** — the pilot could not see the field, which is an instrument ceiling, not an omission. The record for it is `qa/AURA-PHASE0.md` §Tier's `Cost characterisation` subsection — ⚠️ at this amendment's instant that subsection did **not** exist (§Tier is still the empty placeholder heading; `Read`, 2026-09-21); it is **being written under `TASK-1369`**, so ⛔ nothing here asserts its contents |
>
> **The real tool names** (⛔ the guess in §7 is now answered — MEASURED BY THE ORCHESTRATOR, `/mcp` census 2026-09-13, copied into `AURA-PHASE0.md` §MCP tool census): the servers are keyed `unreal_inspector` and `unreal_editor` in the project `.mcp.json`, so Claude Code exposes them as **`mcp__unreal_inspector__<tool>`** and **`mcp__unreal_editor__<tool>`** (server key with the underscore). Counts: **62** inspector tools · **114** editor tools, of which **32** fall in the four verification classes (PIE lifecycle 6 · verification 10 · screenshot/recording 9 · input simulation 7) and **82** outside. The `playtest-verifier` holds **49** inspector names (ruling R10 replaced the wholesale `mcp__unreal_inspector__*` with an enumerated list, excluding the 13 census-§2a non-read names — `launch_unreal_project`, `recompile_unreal_project`, `shutdown_headless`, `cancel_operation`, the seven image/mesh generators, and the two plan tools) **+ those 32** editor names. ⛔ No wildcard on `unreal_editor`, ever.
>
> **The one-click path — the plan's guess was wrong, and wrong in the way that matters.** Aura's **Add to Editor → Claude Code** wrote the two servers into **`~/.claude.json` → `mcpServers` (user scope)**, which Claude Code **does** read — ⛔ **not** `~/.claude/mcp.json`, which Claude Code does **not** read and which was measured **ABSENT**. MEASURED BY THE ORCHESTRATOR (`TASK-1216`, 2026-09-13). One source of truth now holds: all four servers live in the project `.mcp.json`, and user-scope `mcpServers` measured `[]` at census time. The paths Aura installed, verbatim: command `C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Marketplace\Aura\PortablePython\Windows\python.exe`, args `…\Aura\MCP\unreal_inspector.py` / `…\Aura\MCP\unreal_editor.py`.
>
> **The Enhanced-Input answer — ✅ yes at the ACTION layer, ⚠️ the positional `KBD-§` layout itself is still UNMEASURED.** `inject_input_action` on **our own** actions (`IA_Card1`, `IA_Move`) drove `IMC_Hero`: the card was played and the hero walked and climbed (pilot legs N2 and N4) — MEASURED BY THE PLAYTEST-VERIFIER (Aura, PIE, 2026-09-14; `AURA-PHASE0.md` §Lane facts). That is the *action* layer. Whether a raw **key** press resolves through the positional `KBD-§` layout reads **OWED** in `AURA-PHASE0.md` §Enhanced-Input finding, and the one place it was tried it failed: `simulate_key_press Tab` ×8 on `L_MainMenu` returned `binding_found: false` with `applied_mapping_contexts: []`.
>
> ⚠️ **The blind spot to design around:** any flow gated on a **UMG button click** is `UNOBSERVABLE` by construction — `ui_perform`'s three click shapes never fire `OnClicked`, and `simulate_key_press` on a menu reaches the player controller, not Slate. Both are named dead ends in `VER-§5`. Route B (`load_level L_Arena`, vs-bot) is the working substitute; a human click is the only known route into the deck builder, settings and login.
>
> 🚨⚖️⭐⭐ **THE FRAME THIS PLAN STRUCTURALLY COULD NOT HAVE HAD (added 2026-09-21, `TASK-1371`) — ⛔ THE SETUP IS DONE; THE CEILINGS ARE PERMANENT, AND THOSE ARE TWO DIFFERENT THINGS.** Phases 0–4 below all ran; ⛔ not one of them moves a wall. The measured ceilings, short — the law is `CONVENTIONS.md` **`VER-§8`** and it is ⛔ deliberately **not** restated here:
>
> - **(i) INPUT INJECTS BENEATH SLATE — PERMANENTLY.** `simulate_key_press` is delivered to the player controller / Enhanced Input and ⛔ **never to Slate** — MEASURED **TWICE**, on two separate PIE sessions, with a **discriminating control pair** both times (`VER-§8` cl. 1: `Tab`, Slate's own key, moved focus not at all while `Down` → `IA_MenuDown` did). ⇒ any flow gated on a **UMG button click** or a **Slate keypress** is `UNOBSERVABLE` **BY CONSTRUCTION**, and `VER-§8` cl. 2 puts that on the **ROW at boarding**, ⛔ never on the verifier at 5b. The blind-spot paragraph above states the same fact; what it lacked are the word **PERMANENT** and the law pointer.
> - **(ii) THE CONFIRM CLICK IS A RAW KEY POLL on an unreflected function** ⇒ ⚖️ ***a reproducible aim point is ⛔ NOT a played card*** (`VER-§8` cl. 7 — ⛔ **wall (a) STANDS UNRELAXED**; wall (b) FELL, and ⛔ nobody may cite the fallen half against the standing one).
> - **(iii) THE AIM POINT DOES ⛔ NOT LATCH ACROSS MCP ROUND TRIPS** — ≈**648 uu** of scatter between two identical `SetMouseLocation(300,420)` calls at an unmoved camera, against **≤1.2026 uu** when the same set→read repeats are **BATCHED inside ONE `run_verification_sequence`** (`VER-§8` cl. 7). ⇒ the recipe is: **batch the set and every dependent read into one sequence.** ⛔ **WHY batching removes the scatter is NOT MEASURED — no mechanism is written here** (`SC-§101`).
> - **(iv) CREDIT APPEARS IN ⛔ NO TOOL REPLY** ⇒ the $ figure is a 🧑 human dashboard read, and *"not visible"* is the **ABSENCE OF A FIELD**, ⛔ never a zero.
> - **(v) GAMEPAD: GRANTED, BUT IT DOES ⛔ NOT REACH OUR BINDINGS** — ⛔ *"ungranted"* would be the wrong word: the stick/button tools **are** in the grant. `simulate_right_stick` produced **NO CHANGE** in `control_rotation` (`{pitch:317.5,yaw:265}` → unchanged), the tool's own reply reading **`"binding_found": false`** and *"delivered to the player controller, but NOTHING BINDS IT"* — while the **same observable in the same posture** was moved by the probes either side of it ⇒ a **real negative**, ⛔ not a dead observable. MEASURED BY THE PLAYTEST-VERIFIER, `qa/TASK-1348-verify.md` probe **P3**, 2026-09-20 (`Read`, 2026-09-21).
> - **(vi) MENU NAVIGATION (`L_MainMenu`) IS OUT OF THE RIG'S SCOPE** — it is ceiling (i) wearing a feature name, and it is recorded as **declared-out with its reason** and carried to 🧑 him as an **ASK**, ⛔ not as a silent omission (`TASKBOARD.md`'s two `(C5) MENU NAVIGATION` entries + `qa/TASK-1348-verify.md`'s out-of-scope row, which corroborates in passing that `ui_snapshot` could not even resolve a widget root). ⚠️ **UNVERIFIED and therefore NOT claimed here: 🧑 his own confirming sentence.** The verify report attributes the exclusion to *"his ruling (C5)"*; the board calls it *"an ASK FOR him"*. Those are **different claims** and this note takes the narrower one (`SC-§101`).
>
> 🧑 **HIS RULING — `verified` is BINDING.** Ruled **2026-09-14** (orchestrator relay of his word in Claude Code: *"binding"*), recorded on `TASK-1230`'s status line and written into law by `TASK-1273` (`VER-§6` cl. 5), commit `bbee7d9`. ⇒ a **`VERIFY-FAILED` blocks the commit** and bounces to `gameplay-programmer` as a QA loop (max 3, then escalate to Jonathan); **`UNOBSERVABLE` never blocks** — it routes to commit exactly as a row with no runtime criterion does, and is recorded on the row, not treated as a pass.
>
> **What the pilot cost to learn (`TASK-1230`, five legs, 2026-09-14):** 2 full `VERIFIED` · 1 `VERIFIED partial 1/3` · 1 `UNOBSERVABLE` (the UMG blind spot) · 1 deliberate `VERIFY-FAILED` exhibit, which caught a **data-only** break — `DT_Cards` Fog cost 50→5000 — **0.81 s** after the press, naming the refusing value. A break invisible to compile, to the suite and to text QA. That exhibit is why the gate is worth its loop.

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

> [!danger] 🚨 CORRECTED 2026-09-21 (`TASK-1371`) — ⛔ "(read-only)" IS THE SERVER'S **NAME**, ⛔ NOT ITS **CONTENTS**, AND IT IS REFUTED AS A GRANT BASIS BY RULING **R10**
> Aura's own sentence quoted above already names `engine lifecycle (launch/recompile/shutdown)` and `image/3D-model generation` — neither is a read. The `/mcp` census **MEASURED 13 non-read tools inside `unreal_inspector`**, including **`recompile_unreal_project`** (⛔ a COMPILE PATH), `launch_unreal_project`, `shutdown_headless`, `cancel_operation`, seven credit-spending image/mesh generators and two plan tools.
> ⇒ ⛔ **NO agent holds `mcp__unreal_inspector__*`, ever.** The grant is the **ENUMERATED read set (49 names, census §2 minus §2a)**, one name per entry, in `permissions.allow` and in the `qa-reviewer` + `playtest-verifier` `tools:` lines alike. Law: `CONVENTIONS.md` **`VER-§7` cl. 3**; procedure: `Docs/setupdirections.md` **§11.5**. ⛔ The 49 names are **not** restated in this file — `handoffs/AURA-MCP-CENSUS.md` is their only legal source.
> ⚠️ Every "allow it wholesale" instruction in §4 and §6 below is struck for this reason. **Source:** `CONVENTIONS.md` `VER-§7` cl. 3 (`Read`, 2026-09-21) + the R10 entry on `CONVENTIONS.md:48`.

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
> 2. **Aura's docs say the Claude Code config lives at `~/.claude/mcp.json` (user-level)** and the snippet uses a `"servers"` key (VS Code style). Our whole setup keeps servers in the project `.mcp.json` (`"mcpServers"` key) + `enabledMcpjsonServers`. ~~⚠️ Verify what the one-click actually writes and where Claude Code actually reads it~~ 🚨 **ANSWERED — see the *Installed* callout above, which carries the measurement and its label; ⛔ it is not re-argued here.** In one line: the one-click wrote **`~/.claude.json` → `mcpServers`** (user scope, which Claude Code *does* read); **`~/.claude/mcp.json` is ABSENT** — re-measured on disk 2026-09-21 (`TASK-1371`) — and is not read at all. Still true and still the point: move the block into the project `.mcp.json` so the repo stays the source of truth (§4 step 3.2).
> 3. **Privacy trade.** "Unlimited usage via Auto Mode requires training to be on." Training can be turned off in privacy settings (off by default only for Enterprise). Decide before indexing the GDD and the Siegebound source. Turning it off means Auto Mode is rate-limited, not unlimited.
> 4. **Indexing cap.** ~~Semantic index covers projects <30,000 files by default and "the editor may slow down when Aura is indexing large files such as art assets." With the Fab packs (`MedievalCastle…`, `Megaplant_Library`, `Realistic_*`, `Tree_Pack_1`…) we are almost certainly over that~~ — 🚨 **AMENDED 2026-09-21 (`TASK-1371`) FROM RULING R17: THIS FRAMING IS MATERIALLY MISLEADING, BECAUSE THE "INDEX" IS NOT A CRAWL.** What Aura answers from is a **CURATED DOCUMENT**: `Saved/.Aura/project_memory.txt` is a **byte-identical copy** of the hand-authored, git-tracked `Docs/AuraProjectMemory.md` (61 lines each, sha256 `4bb799687cbc5c8c…f514297`), copied by `Tools/aura_sync.ps1` — whose only writes are `New-Item` and `Copy-Item`. ⇒ the index is bounded by **one small file a person maintains line by line**, not by a summarisation pass over the tree, and it **cannot acquire a token nobody typed into that file**. Measured at source 2026-09-17 (`TASK-1283`, R17) and recorded in `CONVENTIONS.md` `ACC-§11`'s 2026-09-17 amendment + `Docs/setupdirections.md` §11.3. 🚨⛔ **AND THE OTHER HALF, WHICH THIS CORRECTION MUST NOT RELAX: *THE INDEX IS CLEAN; THE DISK IS NOT.*** Aura's **file tools read the disk on demand** regardless of any ignore list — so `INDEX_IGNORE.txt` (§4 step 2.3) shapes the **INDEX** and **nothing else**. ⛔ It is not a read fence, and no line anywhere may call a listed file "excluded" or "never indexed".
> 5. **Two mutating agents on one editor is the same hazard as two Claude clients on Blender 9876.** Aura's Verification Agent *takes over PIE*. It must be serialized like the quiet-module build gate, and it violates the "when the human is present, closing/driving the editor is his call" law unless you are told first. Rule: Aura verification runs only when explicitly dispatched, one at a time, and the dispatch prompt says the editor will be driven. 🚨 **AMENDED 2026-09-21 (`TASK-1371`), AND THE PRECISION IS THE WHOLE POINT — *"the wait is gone"* and *"the announcement is gone"* are DIFFERENT SENTENCES AND ONLY ONE IS TRUE:** the **BLOCKING WAIT for his "go" is DISCHARGED STANDING** since 2026-09-20, on 🧑 his own sentence (`CONVENTIONS.md` `VER-§3` cl. 6, which struck cl. 2's wait in place rather than deleting it). ⛔ **The ANNOUNCE-AND-REPORT SURVIVES IN FULL** — the dispatch still says the editor will be driven, and the run still reports. ⛔ Serialization (one at a time, never concurrent with a build-master assemble or an art-director import) is untouched, and if the editor is already in PIE when the verifier looks, that is **his** session: report and wait, never stop it.
> 6. **Reception is mixed and young.** Forum beta threads: installer shipped 5.3 DLLs while offering multiple engine versions (Mar 2026), "barely in Alpha state" (Apr 2026), Blueprint-editing complaints (Jan). StraySpark's benchmark (a competitor, self-declared bias) rated Aura "highest quality output for complex, Unreal-specific tasks" but "sometimes overconfident — generates code that looks correct but has subtle issues" and "vendor lock-in risk." → Aura output still goes through `qa-reviewer`; it never bypasses the QA gate.
> 7. **Crash recovery is Windows + standalone-app only**, and does nothing if a debugger is attached. Fine for us; just do not expect it to rescue a Claude Code–driven session.
> 8. **The Fab "$150 upfront, lifetime MCP usage + one-year subscription" SKU is single-sourced (GamesBeat).** I could not find the Fab listing itself. ~~⚠️ Verify on Fab before choosing it over the monthly tiers — if real, it is the best deal for a pipeline whose main use is MCP.~~ 🚨 **RECORDED 2026-09-21 (`TASK-1371`) AS ⛔ UNCONFIRMED / POSSIBLY NEVER EXISTED AS RECORDED. PROVENANCE OF THE NON-FINDING, so it can be re-checked rather than re-believed: the ORCHESTRATOR read Aura's own *Pricing Explained* and *About* pages on 2026-09-21 and found NO mention of it** (relayed onto `TASK-1215`'s `status:` line, which is the source this note copies; ⛔ no agent re-visited a pricing page for this amendment). ⇒ the tier was chosen **without** it and stage B's *"the Fab SKU if it exists"* clause is **ANSWERED: NOT FOUND.** 🚨⛔ **AND SAID ONCE SO IT IS NEVER RE-DERIVED AS A MISSED OPPORTUNITY: an ABSENCE OF EVIDENCE IS NOT EVIDENCE OF ABSENCE AND IS NOT A REFUTATION** (`SC-§39`) — this is *"nobody has found it"*, ⛔ not *"it does not exist"*, and ⛔ not *"a better deal was forgone"*.
> 9. 🚨 **ON-DISK CREDENTIAL EXPOSURE — ACCEPTED WITH CONDITIONS, NOT ABSENT (added 2026-09-21, `TASK-1371`, ruling R15).** Aura's file tools read this machine's disk on demand, so the gitignored `Config/SiegeCloudDev.ini` is reachable by them — and under a training-ON toggle (caution 3) what is in that file is what may enter a chat turn. 🧑 He **ACCEPTED** that exposure, and the acceptance carries **conditions that stand unrelaxed**. ⛔ **The law is not restated here — its home is the law:** `CONVENTIONS.md` **`ACC-§11`** (the contents rule, the standing census, the R15/R17 amendments) and `Docs/setupdirections.md` **§11.3**. Read them before putting anything new in that file.

---

## 4. Setup — step by step

Same convention as [[GameDevSetup]]: **[You]** = human clicks/accounts, **[Claude]** = a Claude Code session in the project root. Secrets law unchanged: no keys in files, chat or handoffs — Aura authenticates through its own account login, so there is no API key to store at all.

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
3. **[Claude]** Write `INDEX_IGNORE.txt` ~~(project root — verify the exact location in Aura's Project Understanding doc on first run)~~ 🚨 **CORRECTED 2026-09-21 (`TASK-1371`): it lives at `Saved/.Aura/INDEX_IGNORE.txt`, ⛔ NOT the project root — and because `Saved/` is gitignored, the file you actually EDIT is the canonical, committed `Docs/AuraIndexIgnore.txt`, which `Tools/aura_sync.ps1` copies into place** (measured 2026-09-21 by `Read` of `Tools/aura_sync.ps1`, whose pair table at `:68` names exactly that source→destination pair; corroborated at `CONVENTIONS.md:43` and `Docs/setupdirections.md` §11.3). ⛔ The `Saved/` copy is machine-local and is **never staged**. ⚠️ And per caution 4 above: this list shapes the **INDEX** and nothing else — ⛔ it is **not** a read fence. Excluding: `Content/Fab/`, `Content/MedievalCastleEnvironmentAndSiegeWeaponProps/`, `Content/Megaplant_Library/`, `Content/Realistic_*`, `Content/Tree_Pack_1/`, `Content/sA_*`, `Content/Prickly_Knight/`, `Content/RawAssets/`, `Models/`, `Intermediate/`, `Saved/`, `Binaries/`, `DerivedDataCache/`, `Tools/ArtPipeline/.venv/`, `Tools/ArtPipeline/Cache/`, `.claude/pipeline/playtest-evidence/`, and `packagedZIPofGame/`. Keep `Source/`, `Content/Blueprints|UI|Data|Maps|Materials|VFX`, `Docs/GDD.md`, `.claude/pipeline/CONVENTIONS.md`. Commit it (it is project config, not a secret).
4. **[Claude]** Seed `Saved/.Aura/project_memory.txt` with a **short** digest: the team table from `CLAUDE.md`, the asset-prefix + texture-suffix tables from `CONVENTIONS.md`, the build command, and the two laws Aura is most likely to violate (never Live-Coding-compile; never write `Content/` from model gen). ⚠️ `Saved/` is gitignored, so this file is machine-local — add a line to [[GitClaudeUnrealsetupdirections]] Chapter 11 saying ~~"regenerate from `CLAUDE.md`"~~ and keep the canonical copy at `Docs/AuraProjectMemory.md` with a copy step. 🚨 **AS BUILT, 2026-09-21 (`TASK-1371`): `Docs/AuraProjectMemory.md` is HAND-AUTHORED and `Tools/aura_sync.ps1` COPIES it VERBATIM — ⛔ nothing regenerates it from `CLAUDE.md` or `CONVENTIONS.md`, and neither file is a source** (measured: the script's only writes are `New-Item` and `Copy-Item`; source and destination are both 61 lines, sha256 `4bb799687cbc5c8c…f514297`). ⛔ That is load-bearing, not trivia: it is exactly why caution 4's index is bounded by one small tracked file.
5. ~~**[Claude]** Copy the skills you want shared into `Saved/.Aura/Skills/<name>/SKILL.md` (same gitignore caveat — canonical copies live in `.claude/skills/`, mirror them with a one-line PowerShell copy in the session-start checklist).~~ 🚨 **STRUCK 2026-09-21 (`TASK-1371`) — THE SKILLS MIRROR WAS DROPPED AND IT IS CORRECTLY DROPPED: there is nothing to mirror.** Measured 2026-09-21: `.claude/skills/` **does not exist in this repo** (0 files). `Tools/aura_sync.ps1`'s own header says so in its own words — it *"mirrors nothing else - the integration plan's skills-mirror step was dropped"* (`:36`) — and `Docs/setupdirections.md` §11.3 already records the step as dropped. ⛔ Do not re-add it on the strength of this plan's original text.
6. **[You]** First verification, interactively, in Aura's own chat: *"Verify that a Footman played from the hand walks to the enemy castle and attacks it; report actor positions and the castle health-bar widget value before and after."* Watch what it does to PIE, read the video it attaches, and note credit burn. This is the Phase 0 measurement.

### Phase 3 — Bridge into the Claude Code agent team

1. **[You]** Aura Settings → **MCP Configuration → Add to Editor → Claude Code** → fully restart Claude Code.
2. **[Claude]** Run `/mcp` in the session and confirm `unreal_inspector` and `unreal_editor` are connected. Then find what the one-click wrote ~~(Aura docs say `~/.claude/mcp.json`; Claude Code may or may not read that path)~~ 🚨 **ANSWERED 2026-09-21 (`TASK-1371`) — see the *Installed* callout, which carries the measurement; ⛔ not re-argued here: it wrote `~/.claude.json` → `mcpServers`, and `~/.claude/mcp.json` is ABSENT and is not read** — and **move the two server blocks into the project `.mcp.json`** under `"mcpServers"` next to `unreal-mcp` and `blender`, converting the doc's `"servers"` shape:

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
3. **[Claude]** Permissions (Appendix D law — merge, never replace). ~~Read-only tools can be allowed wholesale; mutating ones stay enumerated:~~ 🚨 **STRUCK 2026-09-21 (`TASK-1371`) — REFUTED BY RULING R10 (`CONVENTIONS.md` `VER-§7` cl. 3). ⛔ NEITHER server is granted wholesale; BOTH are enumerated.** A reader who pasted the struck line below handed **`recompile_unreal_project`** — and twelve other non-read tools — **to every agent in the pipeline.**

   ```jsonc
   // ~~"mcp__unreal_inspector__*",             // read-only: safe to allow entirely~~
   // 🚨 STRUCK 2026-09-21 (TASK-1371), ruling R10 — the server is NOT read-only (13 non-read tools,
   //    incl. a compile path). Kept struck, never deleted, so a reader who remembers this line
   //    finds its correction here instead of re-deriving the grant.
   "mcp__unreal_inspector__<enumerated read name>",  // 49 names, ONE PER ENTRY = census §2 minus §2a
   "mcp__unreal_editor__<pie/verify name>",          // 32 names, enumerated (PIE 6 · verify 10 · screenshot 9 · input 7)
   // ⛔ NEVER a wildcard on EITHER server (VER-§7 cl. 1 + cl. 3); ⛔ never the C++ authoring /
   //    compile / shell tools of unreal_editor — compilation stays with build-master.
   // ⛔ The names live in handoffs/AURA-MCP-CENSUS.md; a name absent from the current census
   //    is not grantable. This file does not list them.
   ```

4. **[Claude]** New agent `.claude/agents/playtest-verifier.md`:

   ```markdown
   ---
   name: playtest-verifier
   description: Runs Aura's Play-In-Editor verification against a task's acceptance criteria and writes an evidence-backed runtime report. Use when a code task is qa-passed and its spec names a runtime-observable acceptance criterion. Never edits code or assets, never compiles, never runs Git.
   tools: Read, Grep, Glob, Write, Edit, <the 49 enumerated unreal_inspector read names>, <the 32 enumerated unreal_editor PIE/verify/screenshot/input names>, mcp__claude_ai_Slack__slack_send_message, mcp__claude_ai_Slack__slack_read_thread
   # ~~mcp__unreal_inspector__*~~ 🚨 STRUCK 2026-09-21 (TASK-1371), ruling R10 / VER-§7 cl. 3:
   # the inspector is NOT read-only and is NEVER granted wholesale. Names from AURA-MCP-CENSUS.md.
   ---
   ```

   Body, in the house voice: reads the task's `acceptance:` lines from TASKBOARD + the programmer handoff; runs ONE verification at a time; every claim carries a screenshot path / video path / a quoted actor or widget value ("the castle health widget read 87 after the third hit at t=0:41", never "damage works"); writes `.claude/pipeline/qa/TASK-###-verify.md` with `Verdict: VERIFIED | VERIFY-FAILED | UNOBSERVABLE` 🚨 **| `MEASURED` — a REAL FOURTH TOKEN, minted 2026-09-20, added here 2026-09-21 (`TASK-1371`); `VER-§1` cl. 1, and ⛔ it is worthless without its CELL RULE at `VER-§1` cl. 3a, which is ⛔ not restated here**; flips only its own row's `status:` (`verified` / `verify-failed`); posts once in the ~~🧪~~ **⚙️ Dev & QA** standing thread (`thread_ts` **`1783116269.740549`**, channel `C0BF0QZP3CN`) prefixed `🎮 VERIFIER:` — emoji corrected and the `thread_ts` carried 2026-09-21 (`TASK-1371`), source: `SLACK.md`'s registry table (`Read`, 2026-09-21), which lists the verifier on that row by name. ⛔ Never top-level. `UNOBSERVABLE` is the honest answer for pure-data or editor-only tasks — Aura's own docs: "Changes with no runtime signal… can't be meaningfully verified."
5. **[Claude]** `CLAUDE.md` routing edits. ⚠️ **Verification runs AFTER the compile, never before** — `qa-reviewer` reviews code pre-compile, so the running editor does not yet contain the change; PIE can only test binaries that exist. Build-master's single "compile + assemble + commit" step therefore splits in two around the verifier:
   - **Step 5a** — `qa-passed` → `build-master` compiles (`Result: Succeeded` law) and, for C++ changes, relaunches the editor on the new binaries (the graceful-quit lane from §4.3 of the setup doc; never Live Coding). Status → `built`. No commit yet.
   - **Step 5b** — if the spec has a runtime acceptance criterion → invoke `playtest-verifier`. `verify-failed` routes back to `gameplay-programmer` and counts as a QA loop (same max-3-then-escalate). Blueprint/asset-only tasks skip 5a and go straight here.
   - **Step 5c** — `verified` (or `UNOBSERVABLE`, or no runtime criterion) → `build-master` assembles and commits as today.
   **Hard gate amendment:** *"Nothing with a runtime acceptance criterion is committed without a `VERIFIED` report; `UNOBSERVABLE` is recorded on the row, not treated as a pass."*
   **Why not just give `qa-reviewer` the PIE tools:** it sits before the compile; `unreal_editor` bundles C++ authoring, Live Coding compile and shell tools with the PIE tools, so granting it would break QA's read-only-by-design posture; and keeping "the text says what it should" (QA) and "the engine did what it should" (verifier) as two verdicts keeps both honest. Also add to the team table: `playtest-verifier | Runs Aura PIE verification, writes runtime evidence | Editing code/art, compiling, Git`. Give `qa-reviewer` ~~**only** `mcp__unreal_inspector__*`~~ 🚨 **CORRECTED 2026-09-21 (`TASK-1371`), ruling R10 / `VER-§7` cl. 3: the same ENUMERATED 49 read names the verifier holds — ⛔ never the wildcard, which would hand QA a compile path and break the very read-only posture this sentence is arguing for** — it stays read-only by design but can now actually look at a Blueprint graph and the editor log instead of accepting them "as declared" (`SC-§71b`).
6. **[Claude]** `CONVENTIONS.md`: new namespace **`VER-§`** (verification lane) — report shape, the serialization rule (one Aura verification at a time; never concurrent with a build-master assemble or an art-director import), the "Jonathan present ⇒ announce before driving PIE" rule, evidence promotion into `playtest-evidence/<date>/` reusing the `FR-§1` naming.
7. **[Claude]** `/ship` (`SHIP-§8`): Aura verification can *pre-screen* `ADJUDICATE C3` — run the main-menu → match → HUD-present check and attach its screenshot to the suspension — but **the human adjudication stays**. The law that a script cannot make that judgment was written after a build shipped with "an empty deck, no HUD and no hero" (`PKG-§5a`); Aura makes the evidence better, it does not remove the stop.

### Phase 4 — Pilot on real backlog

Run the three Phase-0 cases through the full chain (`qa-passed` → `build-master` compile → `playtest-verifier` → `build-master` commit). Compare each `TASK-###-verify.md` to what you saw in your own playtest. ~~Only after three matches does the `verified` gate become binding for new tasks; until then it is advisory and says so on the row.~~ 🚨 **DISCHARGED 2026-09-14 — STRUCK HERE 2026-09-21 (`TASK-1371`).** 🧑 He ruled *"binding"* on the five-leg pilot as it actually stood (two distinct behaviours matched, not three), and the "three" is **superseded by his ruling, ⛔ not left standing as a pending condition**: `CONVENTIONS.md` **`VER-§6` cl. 5**, commit `bbee7d9`. From that date the verdict line carries **no** advisory suffix, a `VERIFY-FAILED` **blocks** the commit host, and `UNOBSERVABLE` never blocks. The *Installed* callout above already recorded the ruling; this section did not, which is why it is struck here rather than quietly rewritten.

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
| ⚠️ **Fab SKU** | $150 upfront (GamesBeat) | — | "lifetime MCP usage as well as a one-year subscription" — ~~unverified on Fab~~ 🚨 **UNCONFIRMED / POSSIBLY NEVER EXISTED AS RECORDED** (2026-09-21, `TASK-1371`): not mentioned on Aura's own *Pricing Explained* or *About* pages at the orchestrator's 2026-09-21 reading. ⛔ Record it as a non-finding with its provenance — ⛔ **never as a missed option** (`SC-§39`; caution 8) |

> [!tip] Recommendation — ✅ **SPENT: THE DECISION WAS TAKEN 2026-09-21 AND IT IS `Pro`** (`TASK-1215` stage B; the *Installed* callout carries 🧑 his verbatim sentence and the reasoning). Everything below is the **recommendation as first written**, kept because it records what was argued *before* the decision; ⛔ it is no longer an open question, and ⛔ its *"~$3–4 per three verifications"* arithmetic is **still unmeasured** — a decided tier is ⛔ not a characterised cost.
> **Trial → Pro.** Indie's $15 credit is priced for chat; the Verification Agent is the one thing we want and it is the expensive agent. If Phase 0 shows three verifications cost more than ~$3–4 of credit, Indie would be exhausted in a week of pipeline use and, because credit does not roll over, would effectively become "Auto Mode only". Pro's $60 + an overage cap you set is the right shape for a pipeline that runs verification per task. Check the Fab SKU first — if the lifetime-MCP claim holds, it dominates for our use.
>
> Budget line alongside the existing stack: HF PRO ~$9 + Meshy credits + Aura Pro $40.

---

## 6. Handoff — what the next Claude Code session should do

> [!done] ⛔ **SPENT — THIS SECTION IS HISTORICAL (marked 2026-09-21, `TASK-1371`).** The 16-item list below ran as `TASK-1213`..`TASK-1273` and the outcome is the *Installed* callout at the top of this note. ⛔ **Do not execute it as a checklist**; read it as the record of what was planned. Only the items that would be **actively harmful** if pasted are struck individually below — ⛔ the rest are left exactly as written, because annotating sixteen spent items is bloat, not accuracy. The live procedure is `Docs/GameDevSetup.md` **Part 12** (it replaced `Docs/setupdirections.md` Chapter 11 on 2026-10-03).

> [!todo] Paste this as the first message of a fresh `claude` session in `C:\GitProjects\GitHub\GitClaudeUnrealTesting\GitClaudeUnrealTest`
> *"Read `C:\GitProjects\GitHub\MyObsidianVault\JonWesOBVault\Aura AI for Unreal — Integration Plan.md` end to end. It is the plan for adding Aura AI (tryaura.dev) to this pipeline. Jonathan will do every **[You]** step himself; you own every **[Claude]** step in §4 and every item in §6 below, in order. Tell me which step you are on before you start it, and stop at each ⛔ STOP. Do not install anything, do not push, and do not change any agent's tool grants beyond what the plan names."*

Work items for that session, in order (each is a normal boarded task through `manager` — the plan is the spec, this list is the decomposition):

1. **Pre-flight (no changes yet).** Confirm the current state matches this note: `CLAUDE.md` team table has six agents; `.mcp.json` has `unreal-mcp` + `blender`; `settings.local.json` `enabledMcpjsonServers` lists both; `Docs/setupdirections.md` is the same file as the vault's `GitClaudeUnrealsetupdirections.md` (both 55,770 bytes on 2026-09-14). Report drift before proceeding. ⛔ STOP if `Aura` is already in the `.uproject` Plugins array — Jonathan may have started without you.
2. **Wait for Phase 1.** Nothing in this list runs until Jonathan says the Aura toolbar button shows 🟢 and *"Tell me about this project"* returned Siegebound classes (Phase 1 step 3). Ask for that confirmation explicitly.
3. **`INDEX_IGNORE.txt`** (Phase 2 step 3) — ~~write it at the location Aura's Project Understanding doc specifies (verify with a WebFetch … first; if the doc is ambiguous, project root)~~ 🚨 **CORRECTED 2026-09-21 (`TASK-1371`): the destination is `Saved/.Aura/INDEX_IGNORE.txt`; the file you commit is `Docs/AuraIndexIgnore.txt` and `Tools/aura_sync.ps1` copies it.** Commit the canonical copy under a task; it is config, not a secret. ⛔ Never stage the `Saved/` copy.
4. **`Docs/AuraProjectMemory.md`** (canonical, committed) + a copy to `Saved/.Aura/project_memory.txt` (machine-local, gitignored). Content: the six-agent table, asset prefixes + texture suffixes from `CONVENTIONS.md`, the build command, and these three lines verbatim: *"Never compile via Live Coding — compilation belongs to build-master via Build.bat."* · *"Never write generated meshes into Content/ — they enter through Tools/ArtPipeline Stage 2."* · *"Never run Git."* Keep it under ~150 lines; Aura injects it every turn.
5. ~~**Skills mirror.** Add one PowerShell line to the session-start checklist in `CLAUDE.md` (or a `Tools/aura_sync.ps1`) that copies `.claude/skills/*` → `Saved/.Aura/Skills/*`. Only the skills that make sense inside the editor (none of the Obsidian ones).~~ 🚨 **DROPPED — STRUCK 2026-09-21 (`TASK-1371`): `.claude/skills/` does not exist in this repo (0 files, measured 2026-09-21), `aura_sync.ps1:36` records the step as dropped in its own header, and `Docs/setupdirections.md` §11.3 already says so.**
6. **Phase 0 measurement.** Ask Jonathan for the three verification cases he chose (§4 Phase 0 step 3) and their expected outcomes. He runs them in Aura's own chat; you record credit consumed, wall time, verdict-vs-playtest in a new file `.claude/pipeline/qa/AURA-PHASE0.md`. ⛔ STOP — the tier decision (§5) is his, made from that file.
7. **Phase 3 bridge** (only after he says Add to Editor → Claude Code is done and Claude Code was restarted): run `/mcp`, record the exact tool names under `unreal_inspector` and `unreal_editor` into `AURA-PHASE0.md`, locate the file the one-click wrote (🚨 **ANSWERED 2026-09-21, `TASK-1371`: `~/.claude.json` → `mcpServers`; `~/.claude/mcp.json` is ABSENT — see the *Installed* callout**), move the two server blocks into the project `.mcp.json` (`"mcpServers"` key), add both names to `enabledMcpjsonServers`, ask Jonathan to delete the user-level copy, restart, re-run `/mcp`.
8. **Permissions** — merge into `settings.local.json` `permissions.allow`: ~~`mcp__unreal_inspector__*` wholesale~~ 🚨 **STRUCK 2026-09-21 (`TASK-1371`), RULING R10 — ⛔ NEVER WHOLESALE: the inspector carries 13 non-read tools including `recompile_unreal_project`, a COMPILE PATH. The grant is the ENUMERATED 49 read names (census §2 minus §2a), one per entry** — and the **enumerated** 32 PIE / verify / screenshot / input names from step 7. ⛔ Never `mcp__unreal_editor__*` either — that would grant C++ authoring, Live Coding compile and shell tools to every agent. Law: `CONVENTIONS.md` `VER-§7` cl. 1 + cl. 3.
9. **`.claude/agents/playtest-verifier.md`** — frontmatter per §4 Phase 3 step 4 with the real tool names; body per the draft below.
10. **`CLAUDE.md` edits** — team-table row, routing steps 5a/5b/5c (compile → verify → commit; the verifier never runs on un-compiled code), hard-gate amendment, and one line under Hard gates: *"Aura verification drives PIE; when Jonathan is present the dispatch announces it first and ~~waits for a go~~ **reports it**."* 🚨 **AMENDED 2026-09-21 (`TASK-1371`): the BLOCKING WAIT is discharged standing (2026-09-20, `VER-§3` cl. 6, on 🧑 his own sentence); the ANNOUNCEMENT and the REPORT survive in full.** ⚠️ **MEASURED, AND THE INSTRUMENT NAMED:** `CLAUDE.md`'s Hard-gates section **still carries the original *"waits for a go"* sentence** — read 2026-09-21 in the copy of `CLAUDE.md` this session was given as project instructions, ⛔ not by opening the file. ⛔ That file is **not** edited by this amendment; the discrepancy is a **FINDING routed to the manager**, ⛔ never a repair taken in passing (`SC-§50`).
11. **`CONVENTIONS.md` → `VER-§`** — the verification lane namespace (report shape, one-at-a-time serialization with build-master assemble and art-director import, evidence naming reusing `FR-§1`, `UNOBSERVABLE` semantics, "advisory until three matches" pilot clause).
12. **`qa-reviewer.md`** — add ~~`mcp__unreal_inspector__*`~~ 🚨 **the ENUMERATED 49 read names (STRUCK 2026-09-21, `TASK-1371`, ruling R10 — the wildcard would hand QA a compile path)** to its `tools:` line and one paragraph saying it may now *inspect* Blueprint graphs and the editor log to check a declaration, and that its no-mutation posture is unchanged.
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
  has already announced that PIE will be driven — ~~do not start until told "go"~~
  CORRECTED 2026-09-21 (TASK-1371): the BLOCKING WAIT is discharged standing,
  2026-09-20, VER-§3 cl. 6, on his own sentence. The ANNOUNCEMENT and the REPORT
  survive; only the wait fell. Do not block a verify leg on a permission already given.
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
Verdict: VERIFIED | VERIFY-FAILED | UNOBSERVABLE | MEASURED
# MEASURED added 2026-09-21 (TASK-1371): a REAL FOURTH TOKEN, minted 2026-09-20 —
# VER-§1 cl. 1. It is worthless without its CELL RULE, VER-§1 cl. 3a (the table's
# last column then reads `measured`). Read cl. 3a; it is not restated here.
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

⚠️ **Answers live in the *Installed* callout at the top** (`TASK-1231`, 2026-09-20; amended `TASK-1371`, 2026-09-21) — the boxes below are the questions as first asked; the ticks record which are settled and the callout carries the measurement and its `MEASURED BY` label. ⛔ **A box is ticked only for what someone MEASURED; two below stay OPEN on purpose.**

- [x] Exact tool names under `mcp__unreal_editor__*` (needed for the enumerated allow-list and the agent `tools:` line) — from `/mcp` after Phase 3. → **ANSWERED** — 114 editor + 62 inspector names, census 2026-09-13.
- [x] Where the one-click actually wrote the Claude Code config, and whether Claude Code reads `~/.claude/mcp.json` at all. → **ANSWERED** — it wrote `~/.claude.json` → `mcpServers` (user scope, which Claude Code *does* read); `~/.claude/mcp.json` was ABSENT and is not read.
- [x] Whether Aura's PIE verification can drive **our** Enhanced Input mappings (positional-layout keyboard law, `KBD-§`) or only default bindings. → **PARTLY** — ✅ our *actions* via `inject_input_action` (`IMC_Hero` driven); ⏳ the positional `KBD-§` *key* layout stays OWED.
- [ ] Credit per verification run (Phase 0 measurement) → tier decision. → ⏳ **STILL OPEN, AND DELIBERATELY UNTICKED** — 🚨 **the two halves of this question separated on 2026-09-21 (`TASK-1371`): the TIER is now DECIDED (`Pro`, `TASK-1215` stage B `done`), the CREDIT PER RUN is STILL UNMEASURED and may stay so.** Credit appears in no tool reply (all five pilot legs read `not visible` — absence of the field, ⛔ not a zero), so the $ figure is a 🧑 human dashboard read. ⛔ A decided tier is **not** a characterised cost; the record is `qa/AURA-PHASE0.md` §Tier's `Cost characterisation` subsection, **being written under `TASK-1369`** (⚠️ absent at this amendment's instant, so its contents are not asserted here).
- [x] Does enabling the plugin modify `.uproject`, and does the Sandbox plugin add a second entry? → **PARTLY** — ✅ yes, one entry: `{"Name": "Aura", "Enabled": true, "SupportedTargetPlatforms": ["Win64", "Mac"]}`, 1 of the 9 plugins, committed by 🧑 Jonathan in `0399d1c`; and exactly one `Aura` folder exists under `Engine/Plugins/Marketplace/` (MEASURED BY GAMEPLAY-PROGRAMMER, 2026-09-20). ⏳ The Sandbox half is **UNMEASURED** — whether the Filesystem Sandbox was ever enabled here was not established, so no second entry is expected and none was found, but that is not the same as testing it. 🚨 **AND THE CONSEQUENCE NOBODY ASKED ABOUT WHEN THIS QUESTION WAS WRITTEN, ADDED 2026-09-21 (`TASK-1371`), RULING R11 / `PKG-§13`:** that entry carries **no `TargetAllowList`**, so UBT enables Aura's **23 dependency plugins for EVERY target** — 9 of them carrying **19 runtime-class modules** that were not in the last shipped package's 18. Aura's own two modules are `EditorNoCommandlet` and **cannot** ship (measured, `TASK-1248`); its dependencies **can**. 🧑 **HE RULED OPTION B on 2026-09-13** — *"We will also go with option B for 1257"* — ⇒ **the entry stays WITHOUT a `TargetAllowList`, and the next `/ship` NAMES the module delta plugin-by-plugin and STOPS on anything unexplained.** ⛔ No agent edits the `.uproject`. **SOURCE: `TASKBOARD.md` → `TASK-1257`'s `status:` line + `CONVENTIONS.md` `PKG-§13` (`Read`, 2026-09-21).** ⚠️ Void condition: an Aura update that rewrites the entry re-opens the clause — and the version row above shows this plugin **does** update itself.
- [ ] Fab listing: does the $150 lifetime-MCP SKU exist, and does it include the standalone app + crash recovery? → ⏳ **STILL OPEN, AND THE BOX STAYS UNTICKED ON PURPOSE.** 🚨 **RECORDED 2026-09-21 (`TASK-1371`) as UNCONFIRMED / possibly never existed as recorded: the ORCHESTRATOR read Aura's own *Pricing Explained* and *About* pages on 2026-09-21 and found no mention** (via `TASK-1215`'s `status:` line; ⛔ no page was re-read for this amendment). ⛔ **That is a NON-FINDING, not a refutation — an absence of evidence is not evidence of absence** (`SC-§39`) ⇒ it is ⛔ never to be cited as a missed option or a forgone deal. The tier was chosen without it.
- [ ] Can Aura's verification observe our C++ `USiegeAssistantSnapshot` / cheat-manager state directly, or only actors + UMG? (Docs list actors, GAS, UMG, multiplayer — we do not use GAS.)

---

## Related Notes

- [[GameDevSetup]] — the master setup document; Part 12 is the live Aura procedure, Part 13 the verifier-friendly design rules
- [[UE5 Agent Team System]]
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
