# TASK-639 — [ACC-P2-1] Handoff — gameplay-programmer — 2026-08-23

**Status: ready-for-qa** (orchestrator flips the board; TASK-640 is the gate — `qa/TASK-640.md`)

## What was made

One new file, one new folder, nothing else touched:

- `Tools/Supabase/migrations/0001_init_accounts.sql` — the ACCOUNTS Phase 2 cloud schema migration, authored column-for-column from CONVENTIONS `ACC-§12`, RLS matrix per `ACC-§11`/`ACC-§12`, server-time `updated_at` clock per `ACC-§13` (A3).

Inventory: **3 tables · 1 trigger function · 3 triggers · 12 RLS policies.** The file is authored only — nothing was applied (TASK-642 owns apply, gated on TASK-640 PASS). No `Source/` file was written or modified (QUIET-MODULE holds; the Phase-1 reads below were read-only). No editor, no MCP, no git writes, no Supabase calls.

## Schema table map — Phase-1 provenance per column

Phase-1 sources read (read-only): `SiegeAccountSaveGame.h`, `SiegeDeckSaveGame.h`, `SiegeSettingsSaveGame.h`.

### `public.profiles` — one row per cloud account
| Column | Type / constraint | Phase-1 provenance |
|---|---|---|
| `id` | `uuid primary key references auth.users(id) on delete cascade` | The CLOUD identity (GoTrue `auth.users.id`) — becomes `FSiegeProfileInfo::CloudUserId` (ACC-§15 block 3, TASK-644). NOT `ProfileId` — the local FGuid stays local; the cloud key is the auth account. |
| `display_name` | `text not null` | `FSiegeProfileInfo::DisplayName` (the in-game handle; login identity is the EMAIL per A2/ACC-§10 and lives in `auth.users`, not here). |
| `updated_at` | `timestamptz not null default now()` | No Phase-1 field — the ACC-§13 A3 sync clock, server-written only. |

**Deliberately absent (P2-R6):** `CredentialSaltHex` / `CredentialHashHex` have no column. The local convenience hash never uploads; there is nowhere for it to land, by construction. Also absent: `CreatedUtc`/`LastLoginUtc` (local-only bookkeeping, not in the ACC-§12 law) and `LastSyncUtc` (a LOCAL save field per ACC-§15, not a cloud column).

### `public.decks` — one row per (user, named deck)
| Column | Type / constraint | Phase-1 provenance |
|---|---|---|
| `id` | `uuid primary key default gen_random_uuid()` | Row identity only — no Phase-1 counterpart (decks are name-keyed locally). |
| `user_id` | `uuid not null references auth.users(id) on delete cascade` | Row owner = the linked cloud account (`CloudUserId`). |
| `deck_name` | `text not null` | `FDeckList::DeckName` (the key of `USiegeDeckSaveGame::SavedDecks`, overwrite-on-collision — mirrored by the unique constraint + upsert). |
| `payload` | `jsonb not null` | jsonb PROJECTION of one `FDeckList` (cards etc.), produced by `FSiegeCloudSync::MakeDeckRowJson` (ACC-§15, TASK-645). ACC-§13: the save classes are NOT rewritten — projection, not schema. |
| `updated_at` | `timestamptz not null default now()` | Server sync clock (A3). |
| `unique (user_id, deck_name)` | constraint | ACC-§13 per-deck granularity AND the PostgREST `merge-duplicates` upsert conflict target; its index covers per-user lookups via the `user_id` prefix (no extra index added). |

### `public.settings` — ONE row per user
| Column | Type / constraint | Phase-1 provenance |
|---|---|---|
| `user_id` | `uuid primary key references auth.users(id) on delete cascade` | Row owner; PRIMARY KEY = the one-row-per-user law (ACC-§13). |
| `payload` | `jsonb not null` | jsonb projection of `USiegeSettingsSaveGame` (today: `bAssistantConfirmBeforeExecute`; tagged-property-style tolerance to future fields lives in the jsonb shape), via `FSiegeCloudSync::MakeSettingsRowJson`. |
| `updated_at` | `timestamptz not null default now()` | Server sync clock (A3). |

## RLS policy matrix

RLS **enabled on all three tables**. 12 policies, named `<table>_<op>_own` exactly. Owner binding: `auth.uid() = id` on `profiles`, `auth.uid() = user_id` on `decks`/`settings` (law-verbatim expression). Clause-per-operation:

| Policy | Op | `using` | `with check` |
|---|---|---|---|
| `profiles_select_own` | select | ✔ `auth.uid() = id` | — (n/a for select) |
| `profiles_insert_own` | insert | — (n/a for insert) | ✔ `auth.uid() = id` |
| `profiles_update_own` | update | ✔ | ✔ |
| `profiles_delete_own` | delete | ✔ | — (n/a for delete) |
| `decks_select_own` | select | ✔ `auth.uid() = user_id` | — |
| `decks_insert_own` | insert | — | ✔ `auth.uid() = user_id` |
| `decks_update_own` | update | ✔ | ✔ |
| `decks_delete_own` | delete | ✔ | — |
| `settings_select_own` | select | ✔ `auth.uid() = user_id` | — |
| `settings_insert_own` | insert | — | ✔ `auth.uid() = user_id` |
| `settings_update_own` | update | ✔ | ✔ |
| `settings_delete_own` | delete | ✔ | — |

**Anon fence (double):** every policy is scoped `to authenticated`, so the anon role holds NO policy on any table (RLS denies by default), and `auth.uid()` is NULL for anon regardless — zero rows reachable by two independent mechanisms. No policy references any privileged role.

**Self-checks run:** secret-material grep over `Tools/Supabase/` (`service_role` / `sb_secret` / `eyJ`) = **ZERO hits** (P2-R2). No P1 credential field name appears in the SQL outside the comment stating its deliberate absence.

## Declared deviations + additions beyond the law's letter (SC-§15 — none silent)

1. **Trigger timing: `before insert or update`, not `before update` (DEVIATION, with cause).** ACC-§12's letter says "a `before update` trigger per table", but its own invariant ("the client never writes `updated_at`") and TASK-640's named criterion ("no default-bypass hole") are violated by an INSERT that supplies `updated_at` explicitly — a column default cannot stop that. The same `public.touch_updated_at()` firing at insert time closes the hole with no new objects. Also declared in the SQL header. If QA rules the letter binding, the fix is deleting two words per trigger — but then the insert-time hole must be waived on the record.
2. **Policies scoped `to authenticated` (ADDITION).** The law pins name + binding expression but not the role scope; default scope would be PUBLIC. Scoping to `authenticated` removes anon from the policy surface entirely — a strictly tighter fence, consistent with ACC-§11.
3. **`set search_path = ''` on `touch_updated_at()` (ADDITION).** Search-path hardening (also keeps Supabase's function-search-path security advisor quiet for TASK-642). Body only touches `new.updated_at`/`now()` — pg_catalog resolves regardless.
4. **Re-run scaffolding** (`if not exists` / `or replace` / `drop … if exists` before each trigger/policy create) — sanctioned by the spec's own re-run-posture requirement. Posture, verbatim from the header: idempotent against a schema THIS file created; NOT a reconciler — pre-existing column drift is skipped by `if not exists`, and any schema change goes in a new numbered migration, never edits to an applied one.

**Deliberately NOT added (so QA doesn't hunt for them):** no grant/revoke surgery on Supabase's default privileges (RLS is the ruled boundary; grant edits would be an improvisation beyond ACC-§12), no extra indexes (every owner-lookup path is covered by pk/unique backing indexes), no extensions (`gen_random_uuid()` is core PG13+), no extra tables/columns (none were needed — no finding to send back to manager).

## For TASK-640's reviewer to scrutinize

- Column-for-column conformance against ACC-§12 (the table defs are meant to be verbatim-equivalent; order and spelling per the law).
- The clause-per-operation mapping above — Postgres rejects `with check` on select/delete and `using` on insert, so the matrix shape is also a syntax-validity claim.
- Deviation #1 (trigger timing) — the one place the letter and the invariant conflict; rule it.
- Idempotence claim: `drop policy if exists` + `create policy` pairs (Postgres has no `create policy if not exists`) — confirm every create is preceded by its drop.
- Known, accepted lint: Supabase's PERFORMANCE advisor may flag bare `auth.uid()` in policies (initplan re-evaluation) and suggest `(select auth.uid())`. The law's expression is `auth.uid() = user_id` verbatim and the TASK-642 gate is zero RLS SECURITY findings, so law-verbatim was chosen. Flag if you rule otherwise.

## M8 declaration (batch-level, verbatim)

**Adds no replicated property, no new replicated class, no new relevancy tier, no RPC.** All cloud traffic is client-local HTTPS from `USiegeCloudClient` (a `UGameInstanceSubsystem`); nothing crosses the UE networking layer. Does NOT consume the M8 Phase-1 checkpoint gate; does NOT substitute for Jonathan's owed feedback items.

(This task adds SQL only; no C++ was written.)
