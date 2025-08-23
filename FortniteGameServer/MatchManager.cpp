#include "MatchManager.h"
#include "PlayerManager.h"
#include "SafezoneManager.h"
#include "UObject.h"
#include "Definitions.h"
#include <algorithm>

MatchManager& MatchManager::Get() {
    static MatchManager Instance;
    return Instance;
}

void MatchManager::Update(float DeltaTime) {
    std::lock_guard<std::mutex> Lock(MatchMutex);
    
    if (!IsMatchActive()) return;
    
    auto now = std::chrono::steady_clock::now();
    float stateTime = std::chrono::duration<float>(now - MatchInfo.StateStartTime).count();
    MatchInfo.CurrentStateTime = stateTime;
    
    // Update based on current state
    switch (MatchInfo.State) {
        case EMatchState::Lobby:
            UpdateLobby(DeltaTime);
            break;
        case EMatchState::WaitingToStart:
            UpdateWaitingToStart(DeltaTime);
            break;
        case EMatchState::Warmup:
            UpdateWarmup(DeltaTime);
            break;
        case EMatchState::InProgress:
            UpdateInProgress(DeltaTime);
            break;
        case EMatchState::Ending:
            UpdateEnding(DeltaTime);
            break;
    }
    
    LastUpdateTime = now;
}

void MatchManager::StartMatch(const FMatchSettings& Settings) {
    std::lock_guard<std::mutex> Lock(MatchMutex);
    
    if (IsMatchActive()) {
        LOG_WARN("Cannot start match - match already active");
        return;
    }
    
    MatchInfo = FMatchInfo();
    MatchInfo.Settings = Settings;
    MatchInfo.MatchStartTime = std::chrono::steady_clock::now();
    
    SetMatchState(EMatchState::Lobby);
    
    LOG_INFO("Match started with settings: Max Players=" + std::to_string(Settings.MaxPlayers) + 
             ", Teams=" + (Settings.bTeamsEnabled ? "Enabled" : "Disabled"));
    
    FireMatchStartedCallbacks(Settings);
}

void MatchManager::EndMatch(AFortPlayerControllerAthena* Winner, int32_t WinningTeam) {
    std::lock_guard<std::mutex> Lock(MatchMutex);
    
    if (!IsMatchActive()) return;
    
    MatchInfo.Winner = Winner;
    MatchInfo.WinningTeam = WinningTeam;
    
    SetMatchState(EMatchState::Ending);
    
    // Generate final results
    GenerateMatchResults();
    
    if (Winner) {
        LOG_INFO("Match ended - Winner: " + Winner->GetName());
    } else if (WinningTeam >= 0) {
        LOG_INFO("Match ended - Winning Team: " + std::to_string(WinningTeam));
    } else {
        LOG_INFO("Match ended - No winner");
    }
    
    FireMatchEndedCallbacks(Winner, MatchInfo.Results);
}

void MatchManager::SetMatchState(EMatchState NewState) {
    if (MatchInfo.State == NewState) return;
    
    EMatchState oldState = MatchInfo.State;
    MatchInfo.State = NewState;
    MatchInfo.StateStartTime = std::chrono::steady_clock::now();
    MatchInfo.CurrentStateTime = 0.0f;
    
    // State-specific initialization
    switch (NewState) {
        case EMatchState::Lobby:
            StartLobby();
            break;
        case EMatchState::WaitingToStart:
            StartWaitingToStart();
            break;
        case EMatchState::Warmup:
            StartWarmup();
            break;
        case EMatchState::InProgress:
            StartGameplay();
            break;
        case EMatchState::Ending:
            StartEnding();
            break;
    }
    
    BroadcastMatchState();
    FireMatchStateChangedCallbacks(oldState, NewState);
    
    LOG_INFO("Match state changed: " + std::to_string(static_cast<int>(oldState)) + 
             " -> " + std::to_string(static_cast<int>(NewState)));
}

void MatchManager::PlayerJoinLobby(AFortPlayerControllerAthena* Player) {
    if (!Player) return;
    
    std::lock_guard<std::mutex> Lock(MatchMutex);
    
    MatchInfo.CurrentPlayerCount++;
    
    LOG_INFO("Player joined lobby: " + Player->GetName() + 
             " (" + std::to_string(MatchInfo.CurrentPlayerCount) + "/" + 
             std::to_string(MatchInfo.Settings.MaxPlayers) + ")");
    
    // Check if we can start
    if (CanStartMatch() && MatchInfo.State == EMatchState::Lobby) {
        SetMatchState(EMatchState::WaitingToStart);
    }
}

void MatchManager::PlayerLeaveLobby(AFortPlayerControllerAthena* Player) {
    if (!Player) return;
    
    std::lock_guard<std::mutex> Lock(MatchMutex);
    
    MatchInfo.CurrentPlayerCount = std::max(0u, MatchInfo.CurrentPlayerCount - 1);
    ReadyPlayers.erase(Player);
    
    LOG_INFO("Player left lobby: " + Player->GetName() + 
             " (" + std::to_string(MatchInfo.CurrentPlayerCount) + "/" + 
             std::to_string(MatchInfo.Settings.MaxPlayers) + ")");
}

void MatchManager::PlayerReadyUp(AFortPlayerControllerAthena* Player, bool bReady) {
    if (!Player) return;
    
    std::lock_guard<std::mutex> Lock(MatchMutex);
    
    if (bReady) {
        ReadyPlayers.insert(Player);
    } else {
        ReadyPlayers.erase(Player);
    }
    
    MatchInfo.PlayersReady = static_cast<uint32_t>(ReadyPlayers.size());
    
    LOG_INFO("Player " + Player->GetName() + (bReady ? " ready" : " not ready") + 
             " (" + std::to_string(MatchInfo.PlayersReady) + "/" + 
             std::to_string(MatchInfo.CurrentPlayerCount) + " ready)");
}

bool MatchManager::CanStartMatch() const {
    return MatchInfo.CurrentPlayerCount >= MatchInfo.Settings.MinPlayersToStart &&
           MatchInfo.PlayersReady >= MatchInfo.Settings.MinPlayersToStart;
}

void MatchManager::CheckVictoryConditions() {
    if (MatchInfo.State != EMatchState::InProgress) return;
    
    AFortPlayerControllerAthena* winner = nullptr;
    int32_t winningTeam = -1;
    
    if (IsVictoryAchieved(winner, winningTeam)) {
        EndMatch(winner, winningTeam);
    }
}

bool MatchManager::IsVictoryAchieved(AFortPlayerControllerAthena*& OutWinner, int32_t& OutWinningTeam) {
    auto& playerMgr = PlayerManager::Get();
    
    switch (MatchInfo.Settings.VictoryCondition) {
        case EVictoryCondition::LastPlayerStanding:
            return CheckLastPlayerStanding(OutWinner);
            
        case EVictoryCondition::LastTeamStanding:
            return CheckLastTeamStanding(OutWinner, OutWinningTeam);
            
        case EVictoryCondition::TimeLimit:
            return CheckTimeLimit();
            
        default:
            return false;
    }
}

void MatchManager::OnPlayerEliminated(AFortPlayerControllerAthena* Player) {
    if (!Player) return;
    
    std::lock_guard<std::mutex> Lock(MatchMutex);
    
    // Update player placement
    uint32_t placement = CalculateNextPlacement();
    PlayerPlacements[Player] = placement;
    
    FirePlayerEliminatedCallbacks(Player, placement);
    
    // Check for victory conditions
    CheckVictoryConditions();
    
    LOG_INFO("Player eliminated: " + Player->GetName() + " (Placement: " + std::to_string(placement) + ")");
}

void MatchManager::GenerateMatchResults() {
    MatchInfo.Results.clear();
    
    auto& playerMgr = PlayerManager::Get();
    auto allPlayers = playerMgr.GetAllPlayers();
    
    for (auto* player : allPlayers) {
        FMatchResult result(player, GetPlayerPlacement(player));
        
        // Fill in stats
        auto stats = playerMgr.GetPlayerStats(player);
        result.Kills = stats.Kills;
        result.Deaths = stats.Deaths;
        result.Assists = stats.Assists;
        result.DamageDealt = stats.DamageDealt;
        result.TimeAlive = stats.TimeAlive;
        result.StructuresBuilt = stats.StructuresBuilt;
        
        result.bWon = (player == MatchInfo.Winner);
        result.bEliminated = (result.Placement > 1);
        
        MatchInfo.Results.push_back(result);
    }
    
    // Sort by placement
    SortResultsByPlacement();
}

uint32_t MatchManager::GetPlayerPlacement(AFortPlayerControllerAthena* Player) const {
    auto it = PlayerPlacements.find(Player);
    if (it != PlayerPlacements.end()) {
        return it->second;
    }
    
    // If not eliminated, they're still alive so placement is 1
    auto& playerMgr = PlayerManager::Get();
    if (playerMgr.IsPlayerAlive(Player)) {
        return 1;
    }
    
    return MatchInfo.CurrentPlayerCount; // Default placement
}

uint32_t MatchManager::GetAlivePlayers() const {
    auto& playerMgr = PlayerManager::Get();
    return playerMgr.GetAlivePlayerCount();
}

// Internal update methods
void MatchManager::UpdateLobby(float DeltaTime) {
    // Check for lobby timeout
    if (MatchInfo.CurrentStateTime >= MatchInfo.Settings.LobbyTimeLimit) {
        if (MatchInfo.CurrentPlayerCount >= MatchInfo.Settings.MinPlayersToStart) {
            SetMatchState(EMatchState::WaitingToStart);
        } else {
            LOG_WARN("Lobby timeout with insufficient players");
        }
    }
}

void MatchManager::UpdateWaitingToStart(float DeltaTime) {
    // Automatic progression after brief wait
    if (MatchInfo.CurrentStateTime >= 5.0f) {
        SetMatchState(EMatchState::Warmup);
    }
}

void MatchManager::UpdateWarmup(float DeltaTime) {
    if (MatchInfo.CurrentStateTime >= MatchInfo.Settings.WarmupTime) {
        SetMatchState(EMatchState::InProgress);
    }
    
    // Broadcast countdown
    float remaining = MatchInfo.Settings.WarmupTime - MatchInfo.CurrentStateTime;
    if (remaining > 0 && static_cast<int>(remaining) != static_cast<int>(remaining + DeltaTime)) {
        BroadcastCountdown(static_cast<int32_t>(remaining));
    }
}

void MatchManager::UpdateInProgress(float DeltaTime) {
    // Check victory conditions periodically
    static float victoryCheckTimer = 0.0f;
    victoryCheckTimer += DeltaTime;
    
    if (victoryCheckTimer >= 1.0f) {
        CheckVictoryConditions();
        victoryCheckTimer = 0.0f;
    }
    
    // Check time limit
    if (MatchInfo.Settings.MatchTimeLimit > 0 && 
        MatchInfo.CurrentStateTime >= MatchInfo.Settings.MatchTimeLimit) {
        EndMatch(nullptr, -1); // Time limit reached
    }
}

void MatchManager::UpdateEnding(float DeltaTime) {
    if (MatchInfo.CurrentStateTime >= MatchInfo.Settings.EndgameTime) {
        SetMatchState(EMatchState::Ended);
    }
}

// State initialization methods
void MatchManager::StartLobby() {
    MatchInfo.StartingPlayerCount = 0;
    MatchInfo.CurrentPlayerCount = 0;
    MatchInfo.PlayersReady = 0;
    ReadyPlayers.clear();
    PlayerPlacements.clear();
}

void MatchManager::StartWaitingToStart() {
    MatchInfo.StartingPlayerCount = MatchInfo.CurrentPlayerCount;
}

void MatchManager::StartWarmup() {
    // Prepare all systems for gameplay
    BroadcastMatchState();
}

void MatchManager::StartGameplay() {
    // Start storm if enabled
    if (MatchInfo.Settings.bStormEnabled) {
        SafezoneManager::Get().StartStorm();
    }
    
    // Spawn all players
    auto& playerMgr = PlayerManager::Get();
    auto allPlayers = playerMgr.GetAllPlayers();
    
    for (auto* player : allPlayers) {
        playerMgr.SpawnPlayer(player);
    }
}

void MatchManager::StartEnding() {
    // Stop storm
    SafezoneManager::Get().StopStorm();
    
    // Broadcast victory
    BroadcastVictory(MatchInfo.Winner, MatchInfo.WinningTeam);
}

// Victory condition checking
bool MatchManager::CheckLastPlayerStanding(AFortPlayerControllerAthena*& OutWinner) {
    auto& playerMgr = PlayerManager::Get();
    auto alivePlayers = playerMgr.GetAlivePlayers();
    
    if (alivePlayers.size() <= 1) {
        OutWinner = alivePlayers.empty() ? nullptr : alivePlayers[0];
        return true;
    }
    
    return false;
}

bool MatchManager::CheckLastTeamStanding(AFortPlayerControllerAthena*& OutWinner, int32_t& OutWinningTeam) {
    // Simplified team checking
    auto& playerMgr = PlayerManager::Get();
    auto alivePlayers = playerMgr.GetAlivePlayers();
    
    if (alivePlayers.size() <= 1) {
        OutWinner = alivePlayers.empty() ? nullptr : alivePlayers[0];
        OutWinningTeam = OutWinner ? playerMgr.GetPlayerTeam(OutWinner) : -1;
        return true;
    }
    
    return false;
}

bool MatchManager::CheckTimeLimit() {
    return MatchInfo.Settings.MatchTimeLimit > 0 && 
           MatchInfo.CurrentStateTime >= MatchInfo.Settings.MatchTimeLimit;
}

// Utility methods
uint32_t MatchManager::CalculateNextPlacement() {
    auto& playerMgr = PlayerManager::Get();
    return playerMgr.GetAlivePlayerCount() + 1;
}

void MatchManager::SortResultsByPlacement() {
    std::sort(MatchInfo.Results.begin(), MatchInfo.Results.end(),
        [](const FMatchResult& a, const FMatchResult& b) {
            return a.Placement < b.Placement;
        });
}

// Notification methods
void MatchManager::BroadcastMatchState() {
    LOG_INFO("Broadcasting match state: " + std::to_string(static_cast<int>(MatchInfo.State)));
}

void MatchManager::BroadcastCountdown(int32_t Seconds) {
    LOG_INFO("Countdown: " + std::to_string(Seconds) + " seconds");
}

void MatchManager::BroadcastVictory(AFortPlayerControllerAthena* Winner, int32_t WinningTeam) {
    if (Winner) {
        LOG_INFO("Victory: " + Winner->GetName() + " wins!");
    } else if (WinningTeam >= 0) {
        LOG_INFO("Victory: Team " + std::to_string(WinningTeam) + " wins!");
    } else {
        LOG_INFO("Match ended with no winner");
    }
}

// Callback management
void MatchManager::RegisterMatchStateChangedCallback(const std::string& Name, MatchStateChangedCallback Callback) {
    std::lock_guard<std::mutex> Lock(MatchMutex);
    StateChangedCallbacks[Name] = Callback;
}

void MatchManager::RegisterMatchStartedCallback(const std::string& Name, MatchStartedCallback Callback) {
    std::lock_guard<std::mutex> Lock(MatchMutex);
    MatchStartedCallbacks[Name] = Callback;
}

void MatchManager::RegisterMatchEndedCallback(const std::string& Name, MatchEndedCallback Callback) {
    std::lock_guard<std::mutex> Lock(MatchMutex);
    MatchEndedCallbacks[Name] = Callback;
}

void MatchManager::RegisterPlayerEliminatedCallback(const std::string& Name, PlayerEliminatedCallback Callback) {
    std::lock_guard<std::mutex> Lock(MatchMutex);
    PlayerEliminatedCallbacks[Name] = Callback;
}

void MatchManager::UnregisterCallback(const std::string& Name) {
    std::lock_guard<std::mutex> Lock(MatchMutex);
    StateChangedCallbacks.erase(Name);
    MatchStartedCallbacks.erase(Name);
    MatchEndedCallbacks.erase(Name);
    PlayerEliminatedCallbacks.erase(Name);
}

void MatchManager::FireMatchStateChangedCallbacks(EMatchState OldState, EMatchState NewState) {
    for (const auto& pair : StateChangedCallbacks) {
        try {
            pair.second(OldState, NewState);
        } catch (...) {
            LOG_ERROR("Exception in MatchStateChanged callback: " + pair.first);
        }
    }
}

void MatchManager::FireMatchStartedCallbacks(const FMatchSettings& Settings) {
    for (const auto& pair : MatchStartedCallbacks) {
        try {
            pair.second(Settings);
        } catch (...) {
            LOG_ERROR("Exception in MatchStarted callback: " + pair.first);
        }
    }
}

void MatchManager::FireMatchEndedCallbacks(AFortPlayerControllerAthena* Winner, const std::vector<FMatchResult>& Results) {
    for (const auto& pair : MatchEndedCallbacks) {
        try {
            pair.second(Winner, Results);
        } catch (...) {
            LOG_ERROR("Exception in MatchEnded callback: " + pair.first);
        }
    }
}

void MatchManager::FirePlayerEliminatedCallbacks(AFortPlayerControllerAthena* Player, uint32_t Placement) {
    for (const auto& pair : PlayerEliminatedCallbacks) {
        try {
            pair.second(Player, Placement);
        } catch (...) {
            LOG_ERROR("Exception in PlayerEliminated callback: " + pair.first);
        }
    }
}