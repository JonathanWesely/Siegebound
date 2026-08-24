// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Siegebound/SiegeCloudClient.h"

class UGameInstance;

/**
 *  THE EXPLICIT THREE-TRIGGER SYNC ENGINE (batch ACCOUNTS PHASE 2, TASK-645;
 *  CONVENTIONS ACC-§13 — the sync law — and the ACC-§15 block-2 pinned
 *  signature registry, reproduced below character-for-character).
 *
 *  Plain C++, NO UObject, by law (ACC-§14): the pure seams (ShouldPullRow /
 *  MakeDeckRowJson / MakeSettingsRowJson) are offline unit-testable with zero
 *  network (TASK-647's Siegebound.Cloud.* suite pins them). The instance
 *  carries NO state — every in-flight sync lives in a self-contained shared
 *  context released by the HTTP delegates, so the engine object's lifetime is
 *  irrelevant to a round trip already on the wire.
 *
 *  ⭐ THE SYNC SURFACE IS EXPLICIT — EXACTLY THREE TRIGGERS (ACC-§13):
 *    (1) PullAll — the cloud-login pull: rows with updated_at > LastSyncUtc
 *        land in the ACTIVE profile's LOCAL save slots.
 *    (2) PushAll — the first-link upload (A4): the active profile's decks +
 *        settings push up once.
 *    (3) SyncNow — manual button: pull-newer -> push-all -> LastSyncUtc =
 *        server now.
 *  ⛔ NO per-save hooks (P2-R5): sync reads and writes SAVE SLOTS through the
 *  existing save classes — never widget, controller, or subsystem live state.
 *  DeckBuilderWidget.cpp / SiegePlayerController.cpp / SiegeSettingsSubsystem.*
 *  are untouched by this entire batch. Payloads are jsonb PROJECTIONS of the
 *  existing save classes (ACC-§3 survives; no save class is rewritten).
 *
 *  ⛔ GUEST NEVER SYNCS (ACC-§13/ACC-§1): every trigger no-ops (one
 *  LogSiegeCloud line + OnDone(false, reason)) unless a logged-in,
 *  CLOUD-LINKED profile is active AND the cloud client is configured AND
 *  authenticated. Cloud gates NOTHING: every failure path leaves local state
 *  byte-identical and reports through the delegate for CloudStatusText.
 *
 *  ⛔ NOTHING BLOCKS ON HTTP: all I/O goes through USiegeCloudClient's
 *  delegate-async surface; the pipeline is a chain of continuations holding a
 *  TWeakObjectPtr to the GameInstance, re-resolving subsystems null-safe at
 *  every step (a dead world drops the chain silently).
 *
 *  THE HONEST A3 SHAPE (ACC-§13, recorded — ⛔ not bugs): last-write-wins on
 *  the SERVER-side updated_at clock (the ACC-§12 before-insert-or-update
 *  trigger — this client NEVER writes updated_at; qa/TASK-640's ruling means
 *  it could not forward-date the clock even by accident). A row edited on two
 *  devices between syncs resolves to the most recent WRITER, not a field
 *  merge; deck DELETION does not propagate (no tombstones); pulled decks merge
 *  by name (overwrite-on-collision, the M6 idiom) and never remove a local
 *  deck; the local ActiveDeckName is per-device and is PRESERVED by every pull
 *  (qa/TASK-640's recorded Phase-3 candidate — no cloud column exists for it).
 *
 *  ⚖️ SC-§15 DECLARED DEVIATIONS (mechanisms named; full table in
 *  handoffs/TASK-645-programmer.md):
 *    (D1) The ACC-§15 registry pins SetLastSyncUtc but NO getter, and the
 *         account registry slot has a single-reader law — so the engine keeps
 *         a SESSION-SCOPED LastSyncUtc mirror (file-static, game thread),
 *         seeded only by its own completed cycles. Consequences, both
 *         LOCAL-FIRST-SAFE: PullAll with no session baseline pulls ALL rows
 *         (trigger 1's own fresh-link semantics); SyncNow with no session
 *         baseline SKIPS its pull phase (conservative — an unfiltered pull
 *         could stomp local edits with older cloud rows before push-all
 *         re-uploaded the stomped values). A one-line LastSyncUtc getter on
 *         USiegeAccountSubsystem collapses both branches to the law's letter.
 *    (D2) LastSyncUtc is stamped after ANY fully-successful cycle, not only
 *         trigger 3 — keeps ShouldPullRow monotone so a login-pull's rows
 *         cannot re-stomp later local edits at the next SyncNow.
 *    (D3) "Server now" without a clock endpoint: after a push phase, one
 *         read-back GET of the just-written settings row's updated_at (true
 *         server time); after a pure pull, the max updated_at observed;
 *         empty-cloud pull falls back to FDateTime::UtcNow() (skew posture:
 *         clock-ahead under-pulls recoverably, clock-behind over-pulls
 *         idempotently).
 *    (D4) Deck upserts pass Table = "decks?on_conflict=user_id,deck_name" so
 *         PostgREST's merge-duplicates targets the ACC-§12 unique
 *         (user_id, deck_name) constraint instead of the uuid PK (a bare
 *         "decks" POST would 409 on every re-push of an existing deck).
 *
 *  M8 DECLARATION (batch header, verbatim): Adds no replicated property, no
 *  new replicated class, no new relevancy tier, no RPC. All cloud traffic is
 *  client-local HTTPS from USiegeCloudClient (a UGameInstanceSubsystem);
 *  nothing crosses the UE networking layer. Does NOT consume the M8 Phase-1
 *  checkpoint gate; does NOT substitute for Jonathan's owed feedback items.
 */
class FSiegeCloudSync {                       // plain C++, no UObject — offline unit-testable
public:

	/**
	 *  PURE (pinned by TASK-647's boundary matrix): true iff the cloud row is
	 *  STRICTLY newer than the profile's last sync point. Equal timestamps do
	 *  NOT pull (the row was already landed by the cycle that stamped the
	 *  baseline); an unset baseline (default-constructed FDateTime, ticks 0)
	 *  makes every real cloud timestamp pull — the fresh-link semantics of
	 *  ACC-§13 trigger 1.
	 */
	static bool    ShouldPullRow(const FDateTime& CloudUpdatedUtc, const FDateTime& ProfileLastSyncUtc); // pure — pinned by tests

	/**
	 *  One ACC-§12-shaped decks row as condensed JSON: keys "user_id",
	 *  "deck_name", "payload" — EXACTLY the client-writable columns.
	 *  ⛔ "updated_at" is NEVER present (server-owned, the A3 clock); "id" is
	 *  never present (gen_random_uuid + the on_conflict merge own identity).
	 */
	static FString MakeDeckRowJson(const FString& CloudUserId, const FString& DeckName, const TSharedRef<FJsonObject>& Payload);

	/**
	 *  The ONE ACC-§12-shaped settings row as condensed JSON: keys "user_id",
	 *  "payload". ⛔ "updated_at" is NEVER present (server-owned).
	 */
	static FString MakeSettingsRowJson(const FString& CloudUserId, const TSharedRef<FJsonObject>& Payload);

	void PullAll(UGameInstance& GameInstance, FSiegeCloudResult OnDone);    // cloud-login lane (ACC-§13 trigger 1)
	void PushAll(UGameInstance& GameInstance, FSiegeCloudResult OnDone);    // first-link upload (ACC-§13 trigger 2, A4)
	void SyncNow(UGameInstance& GameInstance, FSiegeCloudResult OnDone);    // pull-newer -> push-all -> LastSyncUtc = server now (trigger 3)
};
