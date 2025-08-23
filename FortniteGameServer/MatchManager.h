#pragma once

#include "ForwardDeclarations.h"
#include "Definitions.h"
#include <chrono>
#include <functional>
#include <mutex>

// Forward declarations
class AFortPlayerControllerAthena;

/**
 * MatchManager - Complete match flow system inspired by Project Reboot
 * 
 * Features:
 * - Full match lifecycle management
 * - Lobby system with ready checks
 * - Warmup phase with countdown
 * - Gameplay phase coordination
 * - Endgame detection and handling
 * - Victory conditions and placement
 * - Match statistics and reporting
 */

enum class EMatchState : uint8_t {
    None = 0,
    Lobby = 1,           // Players joining, waiting for ready
    WaitingToStart = 2,  // Countdown to match start
    Warmup = 3,          // Pre-gameplay warmup
    InProgress = 4,      // Active gameplay
    Ending = 5,          // Victory detected, wrapping up
    Ended = 6,           // Match completed
    Aborted = 7          // Match cancelled/aborted
};

enum class EVictoryCondition : uint8_t {
    LastPlayerStanding = 0,
    LastTeamStanding = 1,
    TimeLimit = 2,
    ScoreLimit = 3,
    Elimination = 4,
    Custom = 5
};

struct FMatchSettings {
    // Basic settings
    uint32_t MaxPlayers = 100;
    uint32_t MinPlayersToStart = 2;
    uint32_t MaxTeamSize = 4;
    bool bTeamsEnabled = true;
    
    // Timing
    float LobbyTimeLimit = 300.0f;      // 5 minutes max lobby time
    float WarmupTime = 10.0f;           // Warmup/countdown duration
    float MatchTimeLimit = 1800.0f;     // 30 minutes max match time
    float EndgameTime = 15.0f;          // Time to show victory screen
    
    // Victory conditions
    EVictoryCondition VictoryCondition = EVictoryCondition::LastPlayerStanding;
    uint32_t ScoreLimit = 100;
    bool bAutoEndOnVictory = true;
    
    // Features
    bool bStormEnabled = true;
    bool bBuildingEnabled = true;
    bool bRespawnEnabled = false;
    bool bFriendlyFireEnabled = false;
    bool bEventsEnabled = true;
    
    FMatchSettings() = default;
};

struct FMatchResult {
    uint32_t Placement = 0;
    AFortPlayerControllerAthena* Player = nullptr;
    int32_t TeamId = -1;
    
    uint32_t Kills = 0;
    uint32_t Deaths = 0;
    uint32_t Assists = 0;
    uint32_t DamageDealt = 0;
    float TimeAlive = 0.0f;
    uint32_t StructuresBuilt = 0;
    
    bool bWon = false;
    bool bEliminated = true;
    
    FMatchResult() = default;
    FMatchResult(AFortPlayerControllerAthena* player, uint32_t placement) 
        : Player(player), Placement(placement) {}
};

struct FMatchInfo {
    EMatchState State = EMatchState::None;
    FMatchSettings Settings;
    
    std::chrono::steady_clock::time_point MatchStartTime;
    std::chrono::steady_clock::time_point StateStartTime;
    float CurrentStateTime = 0.0f;
    
    uint32_t CurrentPlayerCount = 0;
    uint32_t StartingPlayerCount = 0;
    uint32_t PlayersReady = 0;
    
    std::vector<FMatchResult> Results;
    AFortPlayerControllerAthena* Winner = nullptr;
    int32_t WinningTeam = -1;
    
    bool IsActive() const {
        return State == EMatchState::InProgress || 
               State == EMatchState::Warmup || 
               State == EMatchState::WaitingToStart;
    }
    
    bool HasEnded() const {
        return State == EMatchState::Ended || State == EMatchState::Aborted;
    }
    
    float GetMatchDuration() const {
        if (State == EMatchState::None) return 0.0f;
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration<float>(now - MatchStartTime).count();
    }
};

class MatchManager {
public:
    static MatchManager& Get();
    
    // Match control
    void StartMatch(const FMatchSettings& Settings = FMatchSettings());
    void EndMatch(AFortPlayerControllerAthena* Winner = nullptr, int32_t WinningTeam = -1);
    void AbortMatch(const FString& Reason = FString());
    void RestartMatch();
    void PauseMatch();
    void ResumeMatch();
    
    // State management
    void SetMatchState(EMatchState NewState);
    EMatchState GetMatchState() const { return MatchInfo.State; }
    const FMatchInfo& GetMatchInfo() const { return MatchInfo; }
    bool IsMatchActive() const { return MatchInfo.IsActive(); }
    bool IsMatchPaused() const { return bMatchPaused; }
    
    // Lobby management
    void PlayerJoinLobby(AFortPlayerControllerAthena* Player);
    void PlayerLeaveLobby(AFortPlayerControllerAthena* Player);
    void PlayerReadyUp(AFortPlayerControllerAthena* Player, bool bReady = true);
    bool IsPlayerReady(AFortPlayerControllerAthena* Player) const;
    bool CanStartMatch() const;
    void ForceStartMatch();
    
    // Victory conditions
    void CheckVictoryConditions();
    bool IsVictoryAchieved(AFortPlayerControllerAthena*& OutWinner, int32_t& OutWinningTeam);
    void SetCustomVictoryCondition(std::function<bool()> Condition);
    
    // Player tracking
    void OnPlayerEliminated(AFortPlayerControllerAthena* Player);
    void OnPlayerDisconnected(AFortPlayerControllerAthena* Player);
    void UpdatePlayerPlacements();
    uint32_t GetPlayerPlacement(AFortPlayerControllerAthena* Player) const;
    
    // Match results
    const std::vector<FMatchResult>& GetMatchResults() const { return MatchInfo.Results; }
    FMatchResult* GetPlayerResult(AFortPlayerControllerAthena* Player);
    void AddPlayerResult(const FMatchResult& Result);
    void GenerateMatchResults();
    
    // Statistics
    uint32_t GetAlivePlayers() const;
    uint32_t GetAliveTeams() const;
    uint32_t GetTotalEliminations() const;
    float GetAverageTimeAlive() const;
    AFortPlayerControllerAthena* GetTopFragger() const;
    
    // Time management
    float GetMatchTime() const;
    float GetStateTime() const;
    float GetTimeRemaining() const;
    float GetTimeInCurrentState() const;
    
    // Events and callbacks
    using MatchStateChangedCallback = std::function<void(EMatchState, EMatchState)>;
    using MatchStartedCallback = std::function<void(const FMatchSettings&)>;
    using MatchEndedCallback = std::function<void(AFortPlayerControllerAthena*, const std::vector<FMatchResult>&)>;
    using PlayerEliminatedCallback = std::function<void(AFortPlayerControllerAthena*, uint32_t)>;
    
    void RegisterMatchStateChangedCallback(const std::string& Name, MatchStateChangedCallback Callback);
    void RegisterMatchStartedCallback(const std::string& Name, MatchStartedCallback Callback);
    void RegisterMatchEndedCallback(const std::string& Name, MatchEndedCallback Callback);
    void RegisterPlayerEliminatedCallback(const std::string& Name, PlayerEliminatedCallback Callback);
    void UnregisterCallback(const std::string& Name);
    
    // Configuration presets
    void LoadSolosSettings();
    void LoadDuosSettings();
    void LoadSquadsSettings();
    void LoadTeamRumbleSettings();
    void LoadCreativeSettings();
    void ApplyCustomSettings(const FMatchSettings& Settings);
    
    // System management
    void Update(float DeltaTime);
    void Reset();
    void CleanupMatch();
    
    // Advanced features
    void EnableSpectatorMode(bool bEnabled = true);
    void SetMatchTimeLimit(float TimeLimit);
    void AddBonusTime(float BonusTime);
    void TriggerSuddenDeath();
    
    // Notifications
    void BroadcastMatchState();
    void BroadcastCountdown(int32_t Seconds);
    void BroadcastVictory(AFortPlayerControllerAthena* Winner, int32_t WinningTeam = -1);
    void BroadcastElimination(AFortPlayerControllerAthena* Player, uint32_t Placement);
    
    // Debugging and utilities
    void DumpMatchInfo() const;
    void GenerateMatchReport(const std::string& FilePath = "") const;
    void SimulateMatch(); // For testing
    
    // Custom game modes
    void RegisterGameMode(const FString& ModeName, std::function<void()> SetupFunction);
    void ActivateGameMode(const FString& ModeName);
    std::vector<FString> GetAvailableGameModes() const;
    
private:
    MatchManager() = default;
    ~MatchManager() = default;
    
    MatchManager(const MatchManager&) = delete;
    MatchManager& operator=(const MatchManager&) = delete;
    
    // Core data
    FMatchInfo MatchInfo;
    bool bMatchPaused = false;
    
    // Player tracking
    std::unordered_set<AFortPlayerControllerAthena*> ReadyPlayers;
    std::unordered_map<AFortPlayerControllerAthena*, uint32_t> PlayerPlacements;
    
    // Custom victory condition
    std::function<bool()> CustomVictoryCondition;
    
    // Callbacks
    std::unordered_map<std::string, MatchStateChangedCallback> StateChangedCallbacks;
    std::unordered_map<std::string, MatchStartedCallback> MatchStartedCallbacks;
    std::unordered_map<std::string, MatchEndedCallback> MatchEndedCallbacks;
    std::unordered_map<std::string, PlayerEliminatedCallback> PlayerEliminatedCallbacks;
    
    // Game modes
    std::unordered_map<FString, std::function<void()>> GameModes;
    
    // Timing
    std::chrono::steady_clock::time_point LastUpdateTime;
    float StateTimeAccumulator = 0.0f;
    
    // Thread safety
    mutable std::mutex MatchMutex;
    
    // Internal helpers
    void UpdateLobby(float DeltaTime);
    void UpdateWaitingToStart(float DeltaTime);
    void UpdateWarmup(float DeltaTime);
    void UpdateInProgress(float DeltaTime);
    void UpdateEnding(float DeltaTime);
    
    void StartLobby();
    void StartWaitingToStart();
    void StartWarmup();
    void StartGameplay();
    void StartEnding();
    
    void HandleStateTimeout();
    
    void FireMatchStateChangedCallbacks(EMatchState OldState, EMatchState NewState);
    void FireMatchStartedCallbacks(const FMatchSettings& Settings);
    void FireMatchEndedCallbacks(AFortPlayerControllerAthena* Winner, const std::vector<FMatchResult>& Results);
    void FirePlayerEliminatedCallbacks(AFortPlayerControllerAthena* Player, uint32_t Placement);
    
    // Victory checking
    bool CheckLastPlayerStanding(AFortPlayerControllerAthena*& OutWinner);
    bool CheckLastTeamStanding(AFortPlayerControllerAthena*& OutWinner, int32_t& OutWinningTeam);
    bool CheckTimeLimit();
    bool CheckScoreLimit(AFortPlayerControllerAthena*& OutWinner);
    
    // Utilities
    uint32_t CalculateNextPlacement();
    void SortResultsByPlacement();
    void InitializeGameModes();
};