#pragma once

#include "SDK.hpp"
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

struct PlayerStats
{
    int kills = 0;
    int assists = 0;
    int damage = 0;
    int revives = 0;
    int materialsGathered = 0;
    int structuresBuilt = 0;
    int structuresDestroyed = 0;
    float distanceTraveled = 0.0f;
    float timeAlive = 0.0f;
    int placement = 0;
};

struct PlayerInventory
{
    struct InventoryItem
    {
        SDK::UFortItemDefinition* definition = nullptr;
        int quantity = 0;
        int durability = 100;
    };
    
    std::vector<InventoryItem> items;
    std::vector<InventoryItem> weapons;
    std::vector<InventoryItem> consumables;
    
    int wood = 0;
    int stone = 0;
    int metal = 0;
};

enum class PlayerState : uint8_t
{
    Disconnected,
    Loading,
    Lobby,
    WaitingToStart,
    InAircraft,
    Playing,
    Spectating,
    Eliminated
};

class FortPlayer
{
public:
    FortPlayer(const std::string& playerId, const std::string& playerName);
    ~FortPlayer();

    // Basic info
    const std::string& GetPlayerId() const { return m_playerId; }
    const std::string& GetPlayerName() const { return m_playerName; }
    int GetTeamId() const { return m_teamId; }
    PlayerState GetState() const { return m_state; }

    // Game objects
    SDK::AFortPlayerController* GetController() const { return m_controller; }
    SDK::AFortPlayerPawn* GetPawn() const { return m_pawn; }
    void SetController(SDK::AFortPlayerController* controller) { m_controller = controller; }
    void SetPawn(SDK::AFortPlayerPawn* pawn) { m_pawn = pawn; }

    // State management
    void SetState(PlayerState newState) { m_state = newState; }
    void SetTeamId(int teamId) { m_teamId = teamId; }
    void SetPosition(const SDK::FVector& position) { m_position = position; }
    void SetHealth(float health, float shield);

    // Stats and inventory
    PlayerStats& GetStats() { return m_stats; }
    const PlayerStats& GetStats() const { return m_stats; }
    PlayerInventory& GetInventory() { return m_inventory; }
    const PlayerInventory& GetInventory() const { return m_inventory; }

    // Game actions
    void SpawnPlayer(const SDK::FVector& spawnLocation);
    void EliminatePlayer(FortPlayer* eliminator = nullptr);
    void RespawnPlayer();
    void AddKill(FortPlayer* victim);
    void AddDamage(float damage);
    void UpdatePosition(const SDK::FVector& newPosition);

    // Building
    void OnBuildingPlaced(SDK::ABuildingSMActor* building);
    void OnBuildingDestroyed(SDK::ABuildingSMActor* building, bool wasDestroyer);

    // Items and inventory
    bool AddItem(SDK::UFortItemDefinition* itemDef, int quantity);
    bool RemoveItem(SDK::UFortItemDefinition* itemDef, int quantity);
    bool HasItem(SDK::UFortItemDefinition* itemDef, int minQuantity = 1) const;
    void AddMaterials(int wood, int stone, int metal);
    bool UseMaterials(int wood, int stone, int metal);

private:
    // Identity
    std::string m_playerId;
    std::string m_playerName;
    int m_teamId;
    PlayerState m_state;

    // SDK objects
    SDK::AFortPlayerController* m_controller;
    SDK::AFortPlayerPawn* m_pawn;

    // Game state
    SDK::FVector m_position;
    float m_health;
    float m_shield;
    float m_maxHealth;
    float m_maxShield;

    // Game data
    PlayerStats m_stats;
    PlayerInventory m_inventory;

    // Timing
    float m_joinTime;
    float m_lastUpdateTime;
};

class PlayerManager
{
public:
    PlayerManager();
    ~PlayerManager();

    // Player management
    FortPlayer* AddPlayer(const std::string& playerId, const std::string& playerName);
    void RemovePlayer(const std::string& playerId);
    FortPlayer* GetPlayer(const std::string& playerId) const;
    FortPlayer* GetPlayerByController(SDK::AFortPlayerController* controller) const;

    // Team management
    void AssignPlayerToTeam(const std::string& playerId, int teamId);
    void CreateTeam(int teamId, int maxSize = 4);
    void DisbandTeam(int teamId);
    std::vector<FortPlayer*> GetTeamMembers(int teamId) const;
    int GetTeamCount() const;
    int GetAliveTeamCount() const;

    // Player queries
    std::vector<FortPlayer*> GetAllPlayers() const;
    std::vector<FortPlayer*> GetAlivePlayers() const;
    std::vector<FortPlayer*> GetPlayersInState(PlayerState state) const;
    int GetPlayerCount() const { return static_cast<int>(m_players.size()); }
    int GetAlivePlayerCount() const;

    // Player actions
    void SpawnAllPlayers(const SDK::FVector& spawnCenter, float spawnRadius = 1000.0f);
    void EliminatePlayer(const std::string& playerId, const std::string& eliminatorId = "");
    void UpdatePlayerStats();

    // Communication
    void BroadcastMessage(const std::string& message);
    void SendMessageToPlayer(const std::string& playerId, const std::string& message);
    void SendMessageToTeam(int teamId, const std::string& message);

    // Events
    void OnPlayerConnected(const std::string& playerId, const std::string& playerName);
    void OnPlayerDisconnected(const std::string& playerId);
    void OnPlayerKilled(const std::string& victimId, const std::string& killerId);
    void OnPlayerDamaged(const std::string& victimId, const std::string& attackerId, float damage);

    // Cleanup
    void ClearAllPlayers();
    void ResetPlayerStats();

private:
    std::unordered_map<std::string, std::unique_ptr<FortPlayer>> m_players;
    std::unordered_map<int, std::vector<std::string>> m_teams;
    int m_nextTeamId;

    // Helper methods
    int AssignAvailableTeam();
    void UpdateTeamStatus(int teamId);
    void NotifyTeamElimination(int teamId);
    SDK::FVector GenerateSpawnLocation(const SDK::FVector& center, float radius);
};
