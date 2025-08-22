#pragma once

#include "Native.h"
#include "FortniteClasses.h"
#include "GameModes/GameModes.h"
#include <memory>

// Game state tracking
extern bool bStartedBus;

namespace Game {
    
    // Current game mode - can be changed to different game modes
    using CurrentGameMode = GameModeSolos;
    
    extern std::unique_ptr<CurrentGameMode> Mode;
    
    /**
     * Start the game server - switches to Athena level and initializes
     */
    void Start();
    
    /**
     * Called when ready to start match - initializes game state and mode
     */
    void OnReadyToStartMatch();
    
    /**
     * Get death cause from damage report for proper kill feed
     */
    EDeathCause GetDeathCause(const FFortPlayerDeathReport& DeathReport);
    
    /**
     * Initialize the server with proper game state setup
     */
    bool Initialize();
    
    /**
     * Shutdown the game server
     */
    void Shutdown();
    
    /**
     * Update game logic - called periodically
     */
    void Tick(float DeltaTime);
    
    /**
     * Handle player joining the game
     */
    void OnPlayerJoined(AFortPlayerControllerAthena* Controller);
    
    /**
     * Handle player leaving the game
     */
    void OnPlayerLeft(AFortPlayerControllerAthena* Controller);
    
    /**
     * Handle player death/elimination
     */
    void OnPlayerKilled(AFortPlayerControllerAthena* Controller, const FFortPlayerDeathReport& DeathReport);
    
    /**
     * Start the battle bus phase
     */
    void StartBattleBus();
    
    /**
     * Start the safe zone phase
     */
    void StartSafeZone();
    
    /**
     * End the current match
     */
    void EndMatch();
    
    /**
     * Get current player count
     */
    int32_t GetPlayerCount();
    
    /**
     * Get game phase
     */
    EAthenaGamePhase GetGamePhase();
    
    /**
     * Set game phase
     */
    void SetGamePhase(EAthenaGamePhase NewPhase);
    
    // Game configuration
    namespace Config {
        // Match settings
        extern float WarmupTime;
        extern float AircraftTime; 
        extern float SafeZoneTime;
        extern int32_t MaxPlayers;
        
        // Gameplay settings
        extern bool bBuildingEnabled;
        extern bool bInfiniteAmmo;
        extern bool bInfiniteMaterials;
        extern bool bRespawnEnabled;
        
        // Storm settings
        extern float StormDamagePerSecond;
        extern int32_t NumStormPhases;
        
        // Initialize default config values
        void Initialize();
    }
    
    // Event system
    namespace Events {
        /**
         * Broadcast game phase change to all players
         */
        void BroadcastGamePhaseChange(EAthenaGamePhase NewPhase);
        
        /**
         * Broadcast player elimination to all players
         */
        void BroadcastPlayerElimination(AFortPlayerControllerAthena* Victim, 
                                       AFortPlayerControllerAthena* Killer, 
                                       EDeathCause DeathCause);
        
        /**
         * Broadcast match end results
         */
        void BroadcastMatchEnd(const TArray<FMatchResult>& Results);
        
        /**
         * Send notification to all players
         */
        void BroadcastNotification(const FString& Title, const FString& Message);
    }
    
    // Statistics tracking
    namespace Stats {
        struct MatchStats {
            int32_t TotalPlayers = 0;
            int32_t TotalEliminations = 0;
            int32_t TotalBuilds = 0;
            float MatchDuration = 0.0f;
            FDateTime MatchStartTime;
            FDateTime MatchEndTime;
        };
        
        extern MatchStats CurrentMatch;
        
        /**
         * Record player elimination
         */
        void RecordElimination(AFortPlayerControllerAthena* Killer, AFortPlayerControllerAthena* Victim);
        
        /**
         * Record building placed
         */
        void RecordBuildPlaced(AFortPlayerControllerAthena* Builder);
        
        /**
         * Get player statistics
         */
        FPlayerStats GetPlayerStats(AFortPlayerControllerAthena* Player);
        
        /**
         * Reset match statistics
         */
        void ResetMatchStats();
    }
}
