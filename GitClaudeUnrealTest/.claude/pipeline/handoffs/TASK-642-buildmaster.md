# TASK-642 — [ACC-P2-4] APPLY THE MIGRATION + THE ADVISORS GATE — build-master handoff

- **Date:** 2026-08-23 · **Status:** done — **THE GATE PASSES: security advisors = ZERO findings**
- **Access surface (P2-R3):** every figure below names the orchestrator-held Supabase MCP tool that produced it. No agent holds standalone Supabase credentials.
- ⛔ Fences honored: no `Source/` write · no editor/engine MCP · no git write (this file rides TASK-650's commit) · no secrets anywhere · the paused `sebxviekcrnnhifdmfdl` never touched.

## 1. Pre-apply project check (`get_project`)

`cjgqqeogsynrphowdcdp` (`siegebound`, org `pbgbbgneacgeznvgaiid`, us-west-1) → `ACTIVE_HEALTHY`, Postgres 17.6.1, `ga` channel. Matches the TASK-641 handoff.

## 2. The apply + verbatim proof (`apply_migration` · `list_migrations` · `execute_sql` · Bash md5)

| fact | value | tool |
|---|---|---|
| Apply result | `{"success":true}` — first attempt, no retries, no edits | `apply_migration` |
| Ledger entry | version **`20260823174712`** · name **`0001_init_accounts`** (only migration in the ledger) | `list_migrations` |
| Applied-text == file-text | **PROVEN byte-for-byte:** md5 of the ledger-stored statement (`supabase_migrations.schema_migrations.statements`, stmt_count 1) = `c8802bd85290a0cbb645cc02a0bb2163` == md5 of `Tools/Supabase/migrations/0001_init_accounts.sql` (9180 bytes; CRLF-normalized hash identical → file is LF). Ledger `length()` reads 9000 = characters, not bytes (UTF-8 punctuation) — same bytes, same hash. | `execute_sql` + Bash `md5sum` |

## 3. Live re-verification (recomputed from the database, not trusted from the file)

### 3.1 Tables (`list_tables` verbose + `pg_class` recompute) — 3/3, RLS on all

| table | RLS | PK | FK → auth.users(id) | columns (law-verbatim) | extra |
|---|---|---|---|---|---|
| `public.profiles` | ✅ enabled | `id` | `profiles_id_fkey` (cascade) | id uuid · display_name text NN · updated_at timestamptz NN default now() | — |
| `public.decks` | ✅ enabled | `id` (default gen_random_uuid()) | `decks_user_id_fkey` (cascade) | + user_id uuid NN · deck_name text NN · payload jsonb NN · updated_at | `decks_user_id_deck_name_key` **UNIQUE (user_id, deck_name)** — the ACC-§13 upsert conflict target, confirmed in `pg_constraint` |
| `public.settings` | ✅ enabled | `user_id` | `settings_user_id_fkey` (cascade) | user_id uuid · payload jsonb NN · updated_at | — |

**Exactly 3 relations in `public`** (`pg_class` relkind='r' → profiles, decks, settings, all `relrowsecurity=true`). No extra tables, no extra columns.

### 3.2 Policies (`pg_policies`) — 12/12, all `{authenticated}`, correct clause shape

| table | select | insert | update | delete |
|---|---|---|---|---|
| profiles | `profiles_select_own` USING `(auth.uid() = id)` | `profiles_insert_own` WITH CHECK `(auth.uid() = id)` | `profiles_update_own` USING+WITH CHECK `(auth.uid() = id)` | `profiles_delete_own` USING `(auth.uid() = id)` |
| decks | `decks_select_own` USING `(auth.uid() = user_id)` | `decks_insert_own` WITH CHECK `(auth.uid() = user_id)` | `decks_update_own` USING+WITH CHECK `(auth.uid() = user_id)` | `decks_delete_own` USING `(auth.uid() = user_id)` |
| settings | `settings_select_own` USING `(auth.uid() = user_id)` | `settings_insert_own` WITH CHECK `(auth.uid() = user_id)` | `settings_update_own` USING+WITH CHECK `(auth.uid() = user_id)` | `settings_delete_own` USING `(auth.uid() = user_id)` |

Every policy `roles = {authenticated}`; select/delete carry USING only, insert WITH CHECK only, update both — the exact ACC-§12 matrix. Anon holds zero policies (default-deny).

### 3.3 Trigger + function (`pg_trigger` / `pg_proc`)

- 3/3 triggers, each `BEFORE INSERT OR UPDATE ... FOR EACH ROW EXECUTE FUNCTION touch_updated_at()`: `profiles_touch_updated_at` · `decks_touch_updated_at` · `settings_touch_updated_at` (the QA-ruled deviation-1 wording, live).
- `public.touch_updated_at`: `prosecdef=false` (SECURITY INVOKER) · `proconfig = [search_path=""]` — **the search_path pin is live.**

## 4. THE GATE — advisors (`get_advisors`)

- **SECURITY: `{"lints":[]}` — ZERO findings. GATE PASSED.** In particular `function_search_path_mutable` did NOT appear (pin proven in §3.3) and no RLS finding exists on any table. (TASK-641 baselined the empty project at zero, so zero-after-apply attributes cleanly to this migration.)
- **PERFORMANCE (recorded, not gating — QA-640 ruling (e)):** exactly **12 × `auth_rls_initplan` WARN**, one per policy (suggests `(select auth.uid())`; [remediation](https://supabase.com/docs/guides/database/database-linter?lint=0003_auth_rls_initplan)). Pre-ruled PERFORMANCE-category, behaviorally identical, negligible at one-user-per-row cardinality; law-verbatim `auth.uid()` stands. **Nothing else** — no `multiple_permissive_policies`, no unindexed-FK lint (the unique index covers `decks.user_id`; profiles/settings FKs are their PKs).

## 5. What TASK-650/651's live smoke inherits

- Schema is LIVE and empty (0 rows everywhere): project `cjgqqeogsynrphowdcdp`, ledger `20260823174712_0001_init_accounts`.
- Re-run of 0001 against this schema is safe (idempotent posture, QA-verified) but is NOT a drift reconciler — changes go in `0002_...` per ACC-§12.
- ⚠️ **Outstanding hand-step (TASK-641 §5.2, unchanged):** email "Confirm email" → OFF is a Jonathan dashboard step; until flipped, live-smoke signups need email confirmation before login works. Not a schema issue.
- The client contract the smoke exercises: anon key + user JWT only (`Config/SiegeCloudDev.ini`, gitignored) · upsert conflict target `(user_id, deck_name)` with `Prefer: resolution=merge-duplicates` · `updated_at` is server-stamped on BOTH insert and update — the client's value is always overwritten (A3 clock is trustworthy).
- Expected advisor state at any later re-check: security 0 · performance 12 × `auth_rls_initplan` (waived on the record). Any NEW security finding after later migrations attributes to those migrations.

## 6. For the manager (carried from QA-640, still owed)

- Amend ACC-§12's letter with the dated deviation-1 ruling: `before insert or update` is the binding trigger wording (upsert INSERT-branch default-bypass hole). Now also the LIVE wording (§3.3).
