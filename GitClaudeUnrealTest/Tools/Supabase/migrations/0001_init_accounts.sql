-- ============================================================================
-- 0001_init_accounts.sql
-- Siegebound — ACCOUNTS Phase 2: cloud account schema (TASK-639, batch
-- TASK-639..652).
--
-- PURPOSE
--   Creates the three per-user sync tables (profiles / decks / settings) that
--   mirror the Phase-1 local save model (USiegeAccountSaveGame profile
--   identity, USiegeDeckSaveGame named decks, USiegeSettingsSaveGame settings)
--   as jsonb-payload projections, enables row-level security with a full
--   per-operation owner policy matrix keyed to auth.uid(), and installs the
--   server-side updated_at sync clock.
--
-- LAW
--   CONVENTIONS ACC-§12 (schema + RLS law — this file is its column-for-column
--   implementation) · ACC-§11 (client trust model: publishable key + user JWT
--   + RLS is the entire client surface; RLS is the security boundary) ·
--   ACC-§13 (sync law: updated_at is SERVER time, the A3 last-write-wins
--   clock).
--
-- SECURITY MODEL
--   * RLS is ENABLED on all three tables. One policy per operation per table,
--     named <table>_<op>_own, each bound to auth.uid() (= id on profiles,
--     = user_id on decks/settings) in the clause each operation requires:
--     select/delete -> USING · insert -> WITH CHECK · update -> both.
--   * Every policy is scoped TO authenticated. The anon role holds no policy
--     on any table here, and auth.uid() is NULL for anonymous requests anyway
--     — anon reaches zero rows by two independent fences.
--   * No policy references any privileged role (ACC-§12: a policy naming a
--     role that bypasses RLS is a design smell and a QA FAIL).
--
-- DECLARED DEVIATION (SC-§15 — stated here and in the TASK-639 handoff)
--   ACC-§12's letter names "a before update trigger per table". The triggers
--   below fire BEFORE INSERT OR UPDATE instead: an INSERT that supplies
--   updated_at explicitly bypasses the column default, which would violate
--   the same section's invariant ("the client never writes updated_at") and
--   TASK-640's named criterion ("no default-bypass hole"). Firing the same
--   touch function at insert time closes that hole with no new objects.
--
-- RE-RUN POSTURE
--   Idempotent by construction: create table IF NOT EXISTS · create OR
--   REPLACE function · drop trigger / drop policy IF EXISTS before each
--   create · enable row level security is naturally re-runnable. Safe to
--   apply repeatedly to a database whose accounts schema was created by THIS
--   file. NOT safe as a reconciler: IF NOT EXISTS skips an existing table
--   wholesale, so column drift introduced outside this file is neither
--   detected nor repaired — any schema change goes in a new numbered
--   migration (ACC-§12: no ad-hoc DDL), never in edits to an applied one.
--
-- NOTES
--   * gen_random_uuid() is core PostgreSQL (13+): no extension required.
--   * This file is authored (TASK-639) and QA-reviewed (TASK-640) BEFORE it
--     is applied verbatim by the MCP migration tool (TASK-642). Nothing here
--     runs at authoring time.
-- ============================================================================

-- ─── Tables (ACC-§12, column-for-column) ────────────────────────────────────

-- One row per cloud account: the cloud mirror of the Phase-1 profile
-- identity. id == auth.users.id (the GoTrue account, ACC-§10 A2 email login);
-- display_name mirrors FSiegeProfileInfo::DisplayName (the in-game handle).
-- The Phase-1 local credential material (CredentialSaltHex /
-- CredentialHashHex) has NO column here BY LAW (P2-R6): it never leaves the
-- machine.
create table if not exists public.profiles (
    id           uuid primary key references auth.users (id) on delete cascade,
    display_name text not null,
    updated_at   timestamptz not null default now()
);

-- One row per (user, named deck): the cloud mirror of one FDeckList entry in
-- USiegeDeckSaveGame::SavedDecks, serialized to jsonb by
-- FSiegeCloudSync::MakeDeckRowJson (ACC-§15). unique (user_id, deck_name) is
-- the ACC-§13 per-deck sync granularity AND the upsert conflict target
-- (Prefer: resolution=merge-duplicates); its backing index also covers
-- per-user deck lookups via the user_id prefix.
create table if not exists public.decks (
    id         uuid primary key default gen_random_uuid(),
    user_id    uuid not null references auth.users (id) on delete cascade,
    deck_name  text not null,
    payload    jsonb not null,
    updated_at timestamptz not null default now(),
    unique (user_id, deck_name)
);

-- ONE settings row per user (user_id IS the primary key — ACC-§13
-- granularity): the cloud mirror of USiegeSettingsSaveGame, serialized to
-- jsonb by FSiegeCloudSync::MakeSettingsRowJson (ACC-§15).
create table if not exists public.settings (
    user_id    uuid primary key references auth.users (id) on delete cascade,
    payload    jsonb not null,
    updated_at timestamptz not null default now()
);

-- ─── updated_at: the server-time sync clock (ACC-§12 · ACC-§13 A3) ──────────

-- Stamps server time on every write, overriding anything the client sent.
-- Runs with the caller's rights (invoker); the empty search_path pin leaves
-- only pg_catalog resolvable inside the body — no schema-shadowing risk.
create or replace function public.touch_updated_at()
returns trigger
language plpgsql
set search_path = ''
as $$
begin
    new.updated_at := now();
    return new;
end;
$$;

drop trigger if exists profiles_touch_updated_at on public.profiles;
create trigger profiles_touch_updated_at
    before insert or update on public.profiles
    for each row execute function public.touch_updated_at();

drop trigger if exists decks_touch_updated_at on public.decks;
create trigger decks_touch_updated_at
    before insert or update on public.decks
    for each row execute function public.touch_updated_at();

drop trigger if exists settings_touch_updated_at on public.settings;
create trigger settings_touch_updated_at
    before insert or update on public.settings
    for each row execute function public.touch_updated_at();

-- ─── Row-level security (ACC-§12: RLS on ALL three, no exceptions) ──────────

alter table public.profiles enable row level security;
alter table public.decks    enable row level security;
alter table public.settings enable row level security;

-- profiles — owner column is id (auth.uid() = id, ACC-§12)

drop policy if exists profiles_select_own on public.profiles;
create policy profiles_select_own on public.profiles
    for select to authenticated
    using (auth.uid() = id);

drop policy if exists profiles_insert_own on public.profiles;
create policy profiles_insert_own on public.profiles
    for insert to authenticated
    with check (auth.uid() = id);

drop policy if exists profiles_update_own on public.profiles;
create policy profiles_update_own on public.profiles
    for update to authenticated
    using (auth.uid() = id)
    with check (auth.uid() = id);

drop policy if exists profiles_delete_own on public.profiles;
create policy profiles_delete_own on public.profiles
    for delete to authenticated
    using (auth.uid() = id);

-- decks — owner column is user_id

drop policy if exists decks_select_own on public.decks;
create policy decks_select_own on public.decks
    for select to authenticated
    using (auth.uid() = user_id);

drop policy if exists decks_insert_own on public.decks;
create policy decks_insert_own on public.decks
    for insert to authenticated
    with check (auth.uid() = user_id);

drop policy if exists decks_update_own on public.decks;
create policy decks_update_own on public.decks
    for update to authenticated
    using (auth.uid() = user_id)
    with check (auth.uid() = user_id);

drop policy if exists decks_delete_own on public.decks;
create policy decks_delete_own on public.decks
    for delete to authenticated
    using (auth.uid() = user_id);

-- settings — owner column is user_id

drop policy if exists settings_select_own on public.settings;
create policy settings_select_own on public.settings
    for select to authenticated
    using (auth.uid() = user_id);

drop policy if exists settings_insert_own on public.settings;
create policy settings_insert_own on public.settings
    for insert to authenticated
    with check (auth.uid() = user_id);

drop policy if exists settings_update_own on public.settings;
create policy settings_update_own on public.settings
    for update to authenticated
    using (auth.uid() = user_id)
    with check (auth.uid() = user_id);

drop policy if exists settings_delete_own on public.settings;
create policy settings_delete_own on public.settings
    for delete to authenticated
    using (auth.uid() = user_id);

-- ============================================================================
-- End of 0001_init_accounts.sql — 3 tables · 1 function · 3 triggers ·
-- 12 policies. Applied by TASK-642 only, verbatim from the QA-passed file.
-- ============================================================================
