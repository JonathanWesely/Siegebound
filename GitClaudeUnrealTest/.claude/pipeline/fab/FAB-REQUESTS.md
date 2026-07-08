# Fab / Marketplace Request Lane — FAB-REQUESTS.md

Created 2026-07-07 (TASK-082..088 chain; Jonathan-approved plan). Owned by the **manager**; entries AUTHORED by the **art-director**; APPROVED + FULFILLED by **Jonathan** (human-only — agents cannot browse, buy, or download Fab/marketplace content; acquisition requires his Epic Launcher).

## Protocol

1. **requested** — art-director (or manager) adds a `FAB-###` entry below (IDs increment from the highest existing) when a task would benefit from a marketplace pack. Note the request in the task handoff + a one-liner in the 🎨 Art Slack thread.
2. **approved** — Jonathan says yes (in Claude Code or Slack; the orchestrator records it here). A Slack post alone is never acquisition authorization — Jonathan's word is.
3. **fulfilled** — Jonathan downloads the pack into the project via the Epic Launcher. Packs land in `Content/Fab/<Pack>/` — **READ-ONLY donor quarantine** (CONVENTIONS "Textured mesh law": soft-reference or duplicate into /Game/, never edit in place). Jonathan (or the orchestrator on his word) records the landing path here.
4. **integrated** — a board task conforms the asset (reuse the Stage-2 pipeline: `refine_trellis_glb.py` on the Fab mesh → budgets/UVMap/two-slot split/axis contract) and swaps it in via the same-path `SM_<AssetName>` overwrite. Record the task ID here.

Statuses flow strictly `requested → approved → fulfilled → integrated` (or `rejected`, with Jonathan's reason). This file is the source of truth for the lane; Slack mirrors it.

## Rules

- License note is MANDATORY at request time — the pack must permit game use.
- No agent ever fetches marketplace content by any other route (no web downloads, no asset-store scraping).
- Fab meshes obey the full "Textured mesh law" after conform: two-slot `[TeamRegion, <AssetName>PBR]`, Nanite OFF, collision per law, same-path swap only.

## Entry template

```
### FAB-000 — <pack / asset name>
- status: requested | approved | fulfilled | integrated | rejected
- requested-by: art-director (TASK-###)
- for: <SM_/T_ asset(s) it should replace or supply>
- fab-link: <URL or exact search terms>
- license-note: <confirms game-use permission>
- fulfilled-note: <Jonathan: landing path under Content/Fab/<Pack>/>
- integration: <task ID that conformed + swapped it>
```

## Requests

(none yet)
