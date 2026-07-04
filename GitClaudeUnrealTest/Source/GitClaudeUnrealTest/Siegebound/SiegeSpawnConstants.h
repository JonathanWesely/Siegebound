#pragma once

// Shared spawn-placement constants used by BOTH ASiegePlayerController (TASK-030 placement) and
// ASiegeBotController (TASK-046 bot placement). Single source of truth — previously each file
// defined these in its own file-scope anonymous namespace, which collided under the unity build.
namespace SiegeSpawn
{
    inline constexpr float SpawnGroundClearance = 2.f;      // lift the spawn above ground so the capsule doesn't clip
    inline constexpr float DefaultCapsuleHalfHeight = 88.f; // UE default character capsule half-height fallback
}
