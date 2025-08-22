#pragma once

#include "SDK.hpp"
#include <vector>
#include <memory>

enum class GamePhase : uint8_t
{
    WaitingToStart,
    WarmUp,
    Aircraft,
    SafeZones,
    EndGame
};

enum class GameModeType : uint8_t
{
    Solo,
    Duo,
    Squad
};

struct GameSettings
{
    GameModeType mode = GameModeType::Solo;
    int maxPlayers = 100;
    int maxTeams = 100;
    int playersPerTeam = 1;
    bool buildingEnabled = true;
    bool stormEnabled = true;
    float stormPhaseTime = 300.0f; // 5 minutes per phase
    bool weaponDropsEnabled = true;
    bool materialHarvestingEnabled = true;
};

class GameMode
{
public:
    GameMode();
    ~GameMode();

    // Game lifecycle
    bool Initialize(const GameSettings& settings);
    void StartMatch();
    void EndMatch();
    void RestartMatch();
    void Tick(float deltaTime);

    // Game state
    GamePhase GetCurrentPhase() const { return m_currentPhase; }
    GameModeType GetGameType() const { return m_settings.mode; }
    float GetMatchTime() const { return m_matchTime; }
    int GetPlayersAlive() const;
    int GetTeamsAlive() const;

    // Storm system
    void InitializeStorm();
    void UpdateStorm(float deltaTime);
    void StartNextStormPhase();

    // Player/Team management
    void OnPlayerJoined(class FortPlayer* player);
    void OnPlayerLeft(class FortPlayer* player);
    void OnPlayerEliminated(class FortPlayer* player, class FortPlayer* eliminator);
    void OnTeamEliminated(int teamId);

    // Building system
    void OnBuildingPlaced(class ABuildingSMActor* building, class FortPlayer* player);
    void OnBuildingDestroyed(class ABuildingSMActor* building, class FortPlayer* destroyer);

    // Weapon/Item spawning
    void SpawnLoot();
    void SpawnWeapons();
    void SpawnConsumables();
    void SpawnMaterials();

    // Aircraft system
    void InitializeAircraft();
    void UpdateAircraft(float deltaTime);
    bool CanPlayersJump() const;

    // Victory condition checks
    bool CheckVictoryConditions();
    void DeclareWinner(class FortPlayer* winner);
    void DeclareWinningTeam(int teamId);

private:
    GameSettings m_settings;
    GamePhase m_currentPhase;
    
    // Timing
    float m_matchTime;
    float m_phaseTimer;
    
    // Storm system
    struct StormPhase
    {
        float duration;
        float damagePerSecond;
        float shrinkRadius;
        SDK::FVector center;
    };
    
    std::vector<StormPhase> m_stormPhases;
    int m_currentStormPhase;
    float m_stormRadius;
    SDK::FVector m_stormCenter;
    bool m_stormActive;

    // Aircraft
    bool m_aircraftActive;
    float m_aircraftSpeed;
    SDK::FVector m_aircraftPosition;
    SDK::FVector m_aircraftDirection;
    
    // Match state
    bool m_matchActive;
    bool m_matchEnded;
    int m_playersAlive;
    int m_teamsAlive;
    
    // Internal methods
    void TransitionToPhase(GamePhase newPhase);
    void UpdatePhaseTimer(float deltaTime);
    void ProcessGameLogic();
    void UpdateMatchStatistics();
    
    // Phase handlers
    void HandleWaitingToStart();
    void HandleWarmUp();
    void HandleAircraft();
    void HandleSafeZones();
    void HandleEndGame();
};
