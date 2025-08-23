#pragma once

#include "ForwardDeclarations.h"
#include "Definitions.h"
#include <unordered_map>
#include <queue>
#include <functional>
#include <chrono>
#include <mutex>

// Forward declaration for Fortnite classes
class AFortPlayerControllerAthena;
class AFortPlayerPawnAthena;

/**
 * PlayerManager - Advanced player management system inspired by Project Reboot
 * 
 * This system handles:
 * - Player spawning and respawning
 * - Elimination and death handling
 * - Player statistics tracking
 * - Team management
 * - Spectating system
 * - Connection management
 */

enum class EPlayerState : uint8_t {
    None = 0,
    Connecting = 1,
    Loading = 2,
    InLobby = 3,
    WarmingUp = 4,
    Playing = 5,
    Eliminated = 6,
    Spectating = 7,
    Disconnected = 8
};

enum class EEliminationReason : uint8_t {
    None = 0,
    PlayerKill = 1,
    FallDamage = 2,
    StormDamage = 3,
    Suicide = 4,
    Disconnect = 5,
    OutOfBounds = 6,
    VehicleDestruction = 7
};

struct FPlayerStats {
    uint32_t Kills = 0;
    uint32_t Deaths = 0;
    uint32_t Assists = 0;
    uint32_t Revives = 0;
    uint32_t DamageDealt = 0;
    uint32_t DamageTaken = 0;
    uint32_t StructuresBuilt = 0;
    uint32_t StructuresDestroyed = 0;
    uint32_t MaterialsGathered = 0;
    uint32_t ItemsLooted = 0;
    float DistanceTraveled = 0.0f;
    float TimeAlive = 0.0f;
    
    void Reset() {
        *this = FPlayerStats{};
    }
    
    uint32_t GetScore() const {
        return Kills * 10 + Assists * 5 + Revives * 2;
    }
};

struct FEliminationInfo {
    AFortPlayerControllerAthena* Eliminator = nullptr;
    AFortPlayerControllerAthena* Eliminated = nullptr;
    EEliminationReason Reason = EEliminationReason::None;
    FVector Location = FVector();
    float Distance = 0.0f;
    FString WeaponName = FString();
    bool bWasHeadshot = false;
    bool bWasKnockdown = false;
    std::chrono::steady_clock::time_point Timestamp;
    
    FEliminationInfo() {
        Timestamp = std::chrono::steady_clock::now();
    }
};

struct FSpawnInfo {
    FVector Location = FVector();
    FRotator Rotation = FRotator();
    bool bSafeSpawn = true;
    float SafeRadius = 1000.0f;
    int32_t TeamId = -1;
};

class PlayerManager {
public:
    static PlayerManager& Get();
    
    // Player lifecycle management
    void OnPlayerJoin(AFortPlayerControllerAthena* Player);
    void OnPlayerLeave(AFortPlayerControllerAthena* Player);
    void OnPlayerReady(AFortPlayerControllerAthena* Player);
    void OnPlayerDisconnect(AFortPlayerControllerAthena* Player);
    
    // Spawning system
    bool SpawnPlayer(AFortPlayerControllerAthena* Player, const FSpawnInfo& SpawnInfo = FSpawnInfo());
    bool RespawnPlayer(AFortPlayerControllerAthena* Player, float DelaySeconds = 0.0f);
    FSpawnInfo GetSpawnLocation(int32_t TeamId = -1, bool bAvoidEnemies = true);
    void AddSpawnPoint(const FVector& Location, const FRotator& Rotation = FRotator(), int32_t TeamId = -1);
    void ClearSpawnPoints();
    
    // Elimination system
    void EliminatePlayer(AFortPlayerControllerAthena* Player, EEliminationReason Reason, 
                        AFortPlayerControllerAthena* Eliminator = nullptr, const FString& WeaponName = FString());
    void KnockdownPlayer(AFortPlayerControllerAthena* Player, AFortPlayerControllerAthena* Attacker = nullptr);
    void RevivePlayer(AFortPlayerControllerAthena* Player, AFortPlayerControllerAthena* Reviver = nullptr);
    void FinishPlayer(AFortPlayerControllerAthena* Player, AFortPlayerControllerAthena* Finisher = nullptr);
    
    // Team management
    void SetPlayerTeam(AFortPlayerControllerAthena* Player, int32_t TeamId);
    int32_t GetPlayerTeam(AFortPlayerControllerAthena* Player) const;
    std::vector<AFortPlayerControllerAthena*> GetTeammates(AFortPlayerControllerAthena* Player);
    std::vector<AFortPlayerControllerAthena*> GetEnemies(AFortPlayerControllerAthena* Player);
    bool AreTeammates(AFortPlayerControllerAthena* Player1, AFortPlayerControllerAthena* Player2);
    
    // Spectating system
    void StartSpectating(AFortPlayerControllerAthena* Spectator, AFortPlayerControllerAthena* Target = nullptr);
    void StopSpectating(AFortPlayerControllerAthena* Spectator);
    void SetSpectatorTarget(AFortPlayerControllerAthena* Spectator, AFortPlayerControllerAthena* Target);
    AFortPlayerControllerAthena* GetSpectatorTarget(AFortPlayerControllerAthena* Spectator) const;
    std::vector<AFortPlayerControllerAthena*> GetSpectators(AFortPlayerControllerAthena* Player) const;
    
    // Player state management
    void SetPlayerState(AFortPlayerControllerAthena* Player, EPlayerState State);
    EPlayerState GetPlayerState(AFortPlayerControllerAthena* Player) const;
    bool IsPlayerAlive(AFortPlayerControllerAthena* Player) const;
    bool IsPlayerEliminated(AFortPlayerControllerAthena* Player) const;
    bool IsPlayerSpectating(AFortPlayerControllerAthena* Player) const;
    
    // Statistics tracking
    FPlayerStats& GetPlayerStats(AFortPlayerControllerAthena* Player);
    void UpdatePlayerStats(AFortPlayerControllerAthena* Player, const std::function<void(FPlayerStats&)>& UpdateFunc);
    void ResetPlayerStats(AFortPlayerControllerAthena* Player);
    void ResetAllStats();
    
    // Player queries
    std::vector<AFortPlayerControllerAthena*> GetAllPlayers() const;
    std::vector<AFortPlayerControllerAthena*> GetAlivePlayers() const;
    std::vector<AFortPlayerControllerAthena*> GetEliminatedPlayers() const;
    std::vector<AFortPlayerControllerAthena*> GetPlayersInRadius(const FVector& Center, float Radius) const;
    AFortPlayerControllerAthena* FindPlayerByName(const std::string& Name) const;
    AFortPlayerControllerAthena* FindPlayerById(int32_t PlayerId) const;
    
    // Match statistics
    uint32_t GetPlayerCount() const;
    uint32_t GetAlivePlayerCount() const;
    uint32_t GetEliminatedPlayerCount() const;
    uint32_t GetTeamCount() const;
    uint32_t GetPlayersInTeam(int32_t TeamId) const;
    
    // Events and callbacks
    using PlayerJoinCallback = std::function<void(AFortPlayerControllerAthena*)>;
    using PlayerLeaveCallback = std::function<void(AFortPlayerControllerAthena*)>;
    using PlayerEliminateCallback = std::function<void(const FEliminationInfo&)>;
    using PlayerSpawnCallback = std::function<void(AFortPlayerControllerAthena*, const FSpawnInfo&)>;
    
    void RegisterPlayerJoinCallback(const std::string& Name, PlayerJoinCallback Callback);
    void RegisterPlayerLeaveCallback(const std::string& Name, PlayerLeaveCallback Callback);
    void RegisterPlayerEliminateCallback(const std::string& Name, PlayerEliminateCallback Callback);
    void RegisterPlayerSpawnCallback(const std::string& Name, PlayerSpawnCallback Callback);
    void UnregisterCallback(const std::string& Name);
    
    // Utilities
    void BroadcastToAll(const FString& Message);
    void BroadcastToTeam(int32_t TeamId, const FString& Message);
    void BroadcastToPlayer(AFortPlayerControllerAthena* Player, const FString& Message);
    void TeleportPlayer(AFortPlayerControllerAthena* Player, const FVector& Location, const FRotator& Rotation = FRotator());
    void HealPlayer(AFortPlayerControllerAthena* Player, float HealthAmount, float ShieldAmount = 0.0f);
    void DamagePlayer(AFortPlayerControllerAthena* Player, float Damage, AFortPlayerControllerAthena* Attacker = nullptr);
    
    // Configuration
    struct Config {
        float RespawnDelay = 5.0f;
        float KnockdownTime = 60.0f;
        bool bAllowSpectating = true;
        bool bAllowRespawn = false;
        bool bFriendlyFire = false;
        uint32_t MaxPlayers = 100;
        uint32_t MaxTeamSize = 4;
        float SpawnProtectionTime = 3.0f;
        float OutOfBoundsTime = 10.0f;
    } Settings;
    
    // System management
    void Update(float DeltaTime);
    
    // Advanced features
    void StartMatch();
    void EndMatch();
    void PauseMatch();
    void ResumeMatch();
    bool IsMatchActive() const;
    
    // Debugging and monitoring
    void DumpPlayerInfo() const;
    void GenerateMatchReport(const std::string& FilePath = "") const;
    std::vector<FEliminationInfo> GetEliminationHistory() const;
    
private:
    PlayerManager() = default;
    ~PlayerManager() = default;
    
    PlayerManager(const PlayerManager&) = delete;
    PlayerManager& operator=(const PlayerManager&) = delete;
    
    // Internal data structures
    std::unordered_map<AFortPlayerControllerAthena*, EPlayerState> PlayerStates;
    std::unordered_map<AFortPlayerControllerAthena*, FPlayerStats> PlayerStats;
    std::unordered_map<AFortPlayerControllerAthena*, int32_t> PlayerTeams;
    std::unordered_map<AFortPlayerControllerAthena*, AFortPlayerControllerAthena*> SpectatorTargets;
    std::unordered_map<AFortPlayerControllerAthena*, std::vector<AFortPlayerControllerAthena*>> PlayerSpectators;
    
    std::vector<FSpawnInfo> SpawnPoints;
    std::vector<FEliminationInfo> EliminationHistory;
    std::queue<std::pair<AFortPlayerControllerAthena*, float>> RespawnQueue;
    
    // Callbacks
    std::unordered_map<std::string, PlayerJoinCallback> JoinCallbacks;
    std::unordered_map<std::string, PlayerLeaveCallback> LeaveCallbacks;
    std::unordered_map<std::string, PlayerEliminateCallback> EliminateCallbacks;
    std::unordered_map<std::string, PlayerSpawnCallback> SpawnCallbacks;
    
    // Match state
    bool bMatchActive = false;
    bool bMatchPaused = false;
    std::chrono::steady_clock::time_point MatchStartTime;
    
    // Thread safety
    mutable std::mutex PlayerMutex;
    
    // Internal helpers
    void FirePlayerJoinCallbacks(AFortPlayerControllerAthena* Player);
    void FirePlayerLeaveCallbacks(AFortPlayerControllerAthena* Player);
    void FirePlayerEliminateCallbacks(const FEliminationInfo& Info);
    void FirePlayerSpawnCallbacks(AFortPlayerControllerAthena* Player, const FSpawnInfo& SpawnInfo);
    
    int32_t AssignTeam();
    bool IsValidSpawnLocation(const FVector& Location, float Radius = 500.0f);
    void ProcessRespawnQueue();
    void CleanupDisconnectedPlayers();
    
    // Default spawn locations (can be overridden)
    void InitializeDefaultSpawnPoints();
};