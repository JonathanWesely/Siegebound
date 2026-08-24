# QA Report — TASK-640 — SQL/RLS review of `Tools/Supabase/migrations/0001_init_accounts.sql` (covers TASK-639)
Verdict: **PASS**

Reviewer: qa-reviewer · 2026-08-23 · file-only review (no cloud access, no edits — QA no-edit law).
Reviewed against: CONVENTIONS `ACC-§10..§15` (esp. §11, §12, §13) · the TASK-640 ledger criteria (TASKBOARD line 9196) · the REAL Phase-1 field set read first-hand from `SiegeAccountSaveGame.h` / `SiegeDeckSaveGame.h` / `SiegeSettingsSaveGame.h` / `DeckTypes.h` (SC-§34 — recomputed, not trusted from the handoff).

**Blockers: 0 · Warns: 0 · Nits: 3**

## (a) Schema completeness vs ACC-§12 + real Phase-1 fields — PASS
- All three tables are **column-for-column verbatim** against ACC-§12's table law: `profiles(id uuid pk → auth.users cascade, display_name text not null, updated_at timestamptz not null default now())` · `decks(id uuid pk default gen_random_uuid(), user_id → auth.users cascade, deck_name text not null, payload jsonb not null, updated_at, unique(user_id, deck_name))` · `settings(user_id uuid pk → auth.users cascade, payload jsonb not null, updated_at)`. No extra tables, no extra columns, no extensions (`gen_random_uuid()` is core PG13+ — true).
- Phase-1 field enumeration, recomputed from the headers: `FSiegeProfileInfo` = ProfileId, DisplayName, CredentialSaltHex, CredentialHashHex, CreatedUtc, LastLoginUtc; registry = Profiles[], ActiveProfileId; `USiegeDeckSaveGame` = SavedDecks (TArray<FDeckList>, key `FDeckList::DeckName` — confirmed at `DeckTypes.h:47`), ActiveDeckName; `USiegeSettingsSaveGame` = bAssistantConfirmBeforeExecute. Mapping holds: DisplayName→`display_name`; cloud identity = `auth.users.id` (NOT ProfileId — correct, the local FGuid stays local per ACC-§14/§15); DeckName→`deck_name`; deck/settings bodies→jsonb projections (ACC-§13, save classes untouched); LastSyncUtc correctly a LOCAL save field, not a column; CreatedUtc/LastLoginUtc correctly absent (local bookkeeping, not in the law).
- **P2-R6 verified: `CredentialSaltHex`/`CredentialHashHex` have NO column.** They appear only in a comment (SQL lines 62–64) stating their deliberate absence — documentation of the honest-credential law, not an upload path. Compliant.

## (b) RLS matrix — PASS
- `enable row level security` on all three tables (lines 128–130). **12 policies, named `<table>_<op>_own` exactly**; every `drop policy if exists` / `create policy` pair verified line-by-line (134–199).
- Owner binding law-verbatim: `auth.uid() = id` on profiles, `auth.uid() = user_id` on decks/settings, in every clause.
- USING vs WITH CHECK correct per operation on all 12: select→`using` only · insert→`with check` only · update→both · delete→`using` only. (This is also the Postgres-validity shape — `with check` on select/delete and `using` on insert are syntax errors; none present.)
- **Anon reach = zero rows by two independent fences:** every policy is `to authenticated` (anon holds no policy anywhere → RLS default-deny) AND `auth.uid()` is NULL for anon. No table without policies; no policy references `service_role` or any privileged role (the ACC-§12 design-smell FAIL — absent).
- No grant/revoke surgery — correct per ACC-§11 ("RLS is the security boundary"); Supabase's default table grants to anon are inert against zero policies.

## (c) The three SC-§15 declared deviations — RULED
1. **Trigger timing `before insert or update` vs the law's literal `before update` — RULING: THE INVARIANT BINDS. Deviation ACCEPTED.** ACC-§12's own invariant ("⛔ The client never writes `updated_at`") and TASK-640's ledger criterion ("no default-bypass hole") outrank the letter's under-specification. The hole is not theoretical: the client's primary write verb is PostgREST upsert (`UpsertRow`, ACC-§15) = `INSERT ... ON CONFLICT DO UPDATE` — with a before-UPDATE-only trigger, the INSERT branch accepts a client-supplied `updated_at` (a column default cannot stop an explicit value), letting a malicious/buggy client forward-date rows and permanently win the A3 last-write-wins clock across devices. That is a sync-integrity hole with a genuine adversarial angle. `before insert or update` closes both branches with zero new objects. Letter-conformance here would REQUIRE an explicit waiver of that hole; I decline to waive it. **Carry for manager: amend ACC-§12's letter with a dated note recording this ruling** so migration 0002+ inherits the correct wording.
2. **`to authenticated` scoping — verified strictly-tightening, ACCEPTED.** Default policy scope is PUBLIC; restricting to `authenticated` can only shrink the allowed set, never grow it. The game always acts under a user JWT (authenticated role), so no legitimate path is lost; `service_role`/owner bypass RLS regardless, so nothing else changes.
3. **`set search_path = ''` on `touch_updated_at()` — verified strictly-tightening, ACCEPTED.** The body touches only `new.updated_at` and `now()` (pg_catalog, always resolvable under an empty path); no unqualified schema references exist to break. Removes schema-shadowing risk and pre-quiets the `function_search_path_mutable` SECURITY advisor for TASK-642. Function is SECURITY INVOKER (default) — no definer escalation.
- (The 4th item, re-run scaffolding, is not a deviation: the TASK-639 spec itself mandates `if not exists`/`or replace` posture.)

## (d) Idempotence · injection · secrets — PASS
- Re-run posture verified construct-by-construct: `create table if not exists` ×3 · `create or replace function` (identity-preserving; dependent triggers survive) · `drop trigger if exists`+`create` ×3 · `drop policy if exists`+`create` ×12 (Postgres has no `create policy if not exists` — drop+create is the correct idiom) · `enable row level security` is a re-runnable no-op. The header honestly states the limit: idempotent against its own schema, NOT a drift reconciler; changes go in new numbered migrations. Matches ACC-§12's no-ad-hoc-DDL law.
- Injection/escalation surface: pure static DDL — no dynamic SQL, no EXECUTE, no SECURITY DEFINER, no grants. None.
- **Secret material — recomputed first-hand:** `Tools/Supabase/` contains exactly ONE file (this migration). Case-insensitive grep for `service_role` / `sb_secret` / `eyJ` / `secret` / `apikey` / `password` / `token` over the folder = **ZERO hits** (a broader net than the handoff's own, also zero). P2-R2 satisfied.

## (e) The advisor concern — ON THE RECORD
Bare `auth.uid()` in policies may trip Supabase's `auth_rls_initplan` advisor suggesting `(select auth.uid())`. That advisor is a **PERFORMANCE** lint (per-row re-evaluation), **not a SECURITY finding** — it has zero security semantics; the expressions are behaviorally identical. TASK-642's gate is zero **security** findings on these tables, and this file should be clean there: RLS enabled everywhere, function search_path pinned, no auth.users-exposing view, one permissive policy per role+action (no `multiple_permissive_policies`). **Ruling: a performance-category initplan lint does NOT block TASK-642; any SECURITY-category finding still blocks absolutely (never waived in-task, per ACC-§12).** At one-user-per-row cardinality the perf cost is negligible; law-verbatim `auth.uid() = user_id` stands.

## Findings
- [NIT] `0001_init_accounts.sql` — `USiegeDeckSaveGame::ActiveDeckName` has no cloud representation, so "which deck is active" will not sync across devices. This CONFORMS to ACC-§12/§13 (the programmer correctly did not improvise a column — the fence makes that a manager finding, and per-device active-deck is a defensible default). Recorded for manager as a possible Phase-3 line, not a defect.
- [NIT] `display_name text not null` carries no server-side length bound (P1 enforces 3–24 chars client-side only) and both `payload jsonb` columns are unbounded. Law-verbatim, so compliant; a hostile client with a valid JWT could store oversized junk in its OWN rows only (RLS confines the blast radius). Phase-3 hardening candidate (`check` constraints), never an in-flight edit.
- [NIT] Cosmetic: `auth.users (id)` spacing vs the law's `auth.users(id)` — semantically identical, no action.

## Notes for build-master (TASK-642 apply)
1. Apply **verbatim from this QA-passed file** — any needed edit routes back through TASK-639/640, never in-flight. Paste the tool echo; confirm applied-text == file-text.
2. Re-verify live: 3 tables present with RLS enabled · all 12 `<table>_<op>_own` policies listed · **security advisors = ZERO findings on these tables (the gate)**.
3. Expected/waived: a **performance**-category `auth_rls_initplan` lint on the 12 policies MAY appear — it does not block (ruling in (e) above). `function_search_path_mutable` should NOT appear (pinned); if it does, that IS a security-category finding — block and escalate.
4. The FK to `auth.users` requires the migration role's privileges — the MCP migration tool runs as postgres, which suffices; a permission error there is an apply-surface issue, not a file defect.
5. Re-run of this file against its own schema is safe (verified above); do not use it to reconcile drift.

## Notes for manager
- Record the deviation-1 ruling as a dated ACC-§12 amendment ("before insert or update" is the binding wording; rationale: the upsert INSERT-branch default-bypass hole).
- The two NIT observations (ActiveDeckName non-sync; length/size bounds) are Phase-3 candidates, no action owed this batch.
