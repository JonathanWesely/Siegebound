# TASK-300 — Wizard concept art (W-UNIT-1) — art-director handoff

**Status:** DONE (concept generated + archived). No editor, no MCP, no Git touched.
**Date:** 2026-07-26

## Assets produced
| File | Purpose | Git |
|---|---|---|
| `Tools/ArtPipeline/Inbox/Wizard.png` | Meshy image-to-3D pickup (Stage-1 input for TASK-301) | gitignored (`Inbox/*`) — local pickup, not committed |
| `Content/RawAssets/Concepts/Wizard.png` | Archived acceptance concept (M7.5 color-fidelity bar) | untracked `??` — build-master commits with the Wizard batch |

Both files are BYTE-IDENTICAL (sha256 `63214fd228e0e65bb2535fcc05bc605f6cc9831d04e4394f01929e018f7724c4`, 757,592 bytes, 1024×1024 PNG). The SAME final image is in both paths, per the naming block.

## How it was generated (reproducible)
- Tool: `Tools/ArtPipeline/concept_generate.py` (Stage 0), model `black-forest-labs/FLUX.1-dev`, provider `auto`, 1024×1024, 40 steps, guidance 3.5.
- Command: `uv run concept_generate.py Wizard` from `Tools/ArtPipeline/`.
- Prompt entry ADDED to `Tools/ArtPipeline/concept_prompts.json` under CardID `Wizard`, **seed 71017** (next in the 71001–71016 roster sequence; deterministically re-rollable via `--force --seed N`). concept_prompts.json is a committed file — this new entry ships with the batch.
- Subject prompt (the shared PROMPT_SUFFIX for framing/background/silhouette is auto-appended by the tool):
  > a menacing robed medieval battle-mage fire sorcerer, a hooded bearded wizard casting an offensive fire spell, conjuring a bright glowing orange fireball cradled in one upturned open palm and gripping a gnarled wooden staff topped with a burning ember crystal in the other hand, arms held out to the sides in a spellcasting stance, no bow and no sword; muted charcoal-grey and deep ember-brown wool robes with scorched singed hems and a hood, a wide brown leather belt with spell pouches and a bronze clasp, one small neutral cream sash stole draped over the shoulders left uncolored as a team accent, drifting glowing orange embers and warm fiery light as the one warm color key radiating from the fireball and the staff crystal; rendered as a polished stylized low-poly game character in the Warcraft Rumble and Fortnite quality tier, chunky exaggerated proportions about 2.5 to 3 heads tall, oversized hands and staff, beveled simplified cloth folds, hand-painted-style textures with soft gradient shading and a crisp rim light, bold readable silhouette

## Style-consistency rationale
- Matched the CURRENT canonical roster house style defined in `concept_prompts.json` `_doc.art_direction`: polished stylized low-poly, Warcraft Rumble / Fortnite tier, team-agnostic muted palette. Mirrored the **Cleric** entry's grammar (the Wizard's direct sibling: robed hooded caster with a staff) and re-themed holy/healing → offensive fire/arcane. This keeps rendering style, proportions, palette approach, and background treatment consistent with the units the pipeline actually renders.
- NOTE for whoever compares against the older `Archer.png`/`Footman.png`: those are the pre-tool Jul-7 pilot renders (photorealistic). The live tool + the 16-entry prompt library are the stylized-low-poly house style; the Wizard is authored to that library, which is the correct consistency target for the rendered roster.
- **Team-agnostic (verified visually):** robe base is muted charcoal/ember-brown; the neutral **cream hood-mantle / shoulder stole is the intended TeamRegion** (recolors Blue/Red at runtime). No baked blue or red on the character. The warm-orange fireball + ember-orb glow is the ability color key (precedented by Sapper's orange fuse spark + GoldNode's warm-gold emissive) and is distinct from the pure team-red (1.0,0.1,0.05) — it will NOT be confused for a team color.

## Meshy-readiness (for TASK-301 image-to-3D)
Verified on the render: single centered subject, full body head-to-feet with no limb cropping, upright near-front stance, BOTH arms separated from the torso (left palm out, right hand on a vertical staff → clean image-to-3D depth), plain neutral grey background, even lighting, no motion blur, feet planted. This is a clean Meshy input.
- Minor: a small floating fireball sits just left of the open palm. It reads as attached to the hand; if Meshy's cutout produces a stray blob, reroll Stage-1 seed or mask it — same class of harmless emissive as the Cleric staff-glow. Not expected to be a problem.
- TeamRegion split for Stage 2: assign slot-0 `TeamRegion` to the cream hood-mantle/stole cloth; everything else (dark robe, leather, staff, boots) → `WizardPBR`.

## OPERATIONAL NOTE — Norton TLS interception (applies to TASK-301 Meshy gen + TASK-303 card-art gen)
First generation attempt failed 3× with `[SSL: CERTIFICATE_VERIFY_FAILED] unable to get local issuer certificate`. Cause: `provider=auto` routed FLUX.1-dev to a third-party inference host NOT covered by Norton's `huggingface.co`/`*.hf.space` exclusions; the venv's runtime Python uses certifi (no Norton MITM root). `UV_SYSTEM_CERTS=true` only covers uv's own downloads, not runtime SSL, and `truststore`/`pip_system_certs` are not installed.
- **Fix used (documented machine-notes `SSL_CERT_FILE` fallback):** built a combined CA bundle = certifi + all Windows Root/CA store certs → `Tools/ArtPipeline/Cache/_certs/win-ca-bundle.pem` (gitignored), then re-ran with `SSL_CERT_FILE` / `REQUESTS_CA_BUNDLE` / `CURL_CA_BUNDLE` all pointed at it. Succeeded in 4.2 s on attempt 1.
- The bundle is reusable and left in place (Cache is gitignored/disposable). Rebuild it if a Windows rollback wipes the root store. HF_TOKEN was never read, printed, or passed on argv — env-only throughout.

## Downstream
- Unblocks **TASK-301** (`SM_Wizard` Meshy image-to-3D from `Inbox/Wizard.png`) and **TASK-303** (`T_CardArt_Wizard` — reuse this concept for the 512×512 card face).
