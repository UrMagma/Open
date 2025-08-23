#include "PlayerManager.h"
#include "UObject.h"
#include "Definitions.h"
#include <random>
#include <algorithm>
#include <fstream>
#include <sstream>

PlayerManager& PlayerManager::Get() {
    static PlayerManager Instance;
    return Instance;
}

// Player lifecycle management
void PlayerManager::OnPlayerJoin(AFortPlayerControllerAthena* Player) {
    if (!Player) return;
    
    std::lock_guard<std::mutex> Lock(PlayerMutex);
    
    LOG_INFO("Player joining: " + Player->GetName());
    
    // Initialize player data
    PlayerStates[Player] = EPlayerState::Connecting;
    PlayerStats[Player] = FPlayerStats();
    
    // Assign team if not already assigned
    if (PlayerTeams.find(Player) == PlayerTeams.end()) {
        PlayerTeams[Player] = AssignTeam();
    }
    
    FirePlayerJoinCallbacks(Player);
    
    LOG_INFO("Player " + Player->GetName() + " assigned to team " + std::to_string(PlayerTeams[Player]));
}

void PlayerManager::OnPlayerLeave(AFortPlayerControllerAthena* Player) {
    if (!Player) return;
    
    std::lock_guard<std::mutex> Lock(PlayerMutex);
    
    LOG_INFO("Player leaving: " + Player->GetName());
    
    // Stop spectating
    StopSpectating(Player);
    
    // Remove from spectator targets
    for (auto& pair : SpectatorTargets) {
        if (pair.second == Player) {
            pair.second = nullptr;
        }
    }
    
    // Set state and fire callbacks
    PlayerStates[Player] = EPlayerState::Disconnected;
    FirePlayerLeaveCallbacks(Player);
    
    // Clean up after delay to allow for reconnection
    // In real implementation, you'd use a timer system
}

void PlayerManager::OnPlayerReady(AFortPlayerControllerAthena* Player) {
    if (!Player) return;
    
    std::lock_guard<std::mutex> Lock(PlayerMutex);
    SetPlayerState(Player, EPlayerState::InLobby);
    
    LOG_INFO("Player ready: " + Player->GetName());
}

bool PlayerManager::SpawnPlayer(AFortPlayerControllerAthena* Player, const FSpawnInfo& SpawnInfo) {
    if (!Player) return false;
    
    std::lock_guard<std::mutex> Lock(PlayerMutex);
    
    FSpawnInfo actualSpawn = SpawnInfo;
    
    // Get spawn location if not provided
    if (actualSpawn.Location.Size() == 0.0f) {
        actualSpawn = GetSpawnLocation(GetPlayerTeam(Player));
    }
    
    // Validate spawn location
    if (!IsValidSpawnLocation(actualSpawn.Location)) {
        LOG_WARN("Invalid spawn location for player: " + Player->GetName());
        return false;
    }
    
    // Spawn the player
    TeleportPlayer(Player, actualSpawn.Location, actualSpawn.Rotation);
    SetPlayerState(Player, EPlayerState::Playing);
    
    // Reset stats if needed
    auto& stats = GetPlayerStats(Player);
    stats.TimeAlive = 0.0f;
    
    FirePlayerSpawnCallbacks(Player, actualSpawn);
    
    LOG_INFO("Player spawned: " + Player->GetName() + 
             " at (" + std::to_string(actualSpawn.Location.X) + 
             ", " + std::to_string(actualSpawn.Location.Y) + 
             ", " + std::to_string(actualSpawn.Location.Z) + ")");
    
    return true;
}

void PlayerManager::EliminatePlayer(AFortPlayerControllerAthena* Player, EEliminationReason Reason, 
                                   AFortPlayerControllerAthena* Eliminator, const FString& WeaponName) {
    if (!Player) return;
    
    std::lock_guard<std::mutex> Lock(PlayerMutex);
    
    // Create elimination info
    FEliminationInfo elimInfo;
    elimInfo.Eliminated = Player;
    elimInfo.Eliminator = Eliminator;
    elimInfo.Reason = Reason;
    elimInfo.WeaponName = WeaponName;
    
    if (Player->Character) {
        // elimInfo.Location = Player->Character->GetActorLocation();
        elimInfo.Location = FVector(); // Placeholder
    }
    
    // Update stats
    auto& eliminatedStats = GetPlayerStats(Player);
    eliminatedStats.Deaths++;
    
    if (Eliminator && Eliminator != Player) {
        auto& eliminatorStats = GetPlayerStats(Eliminator);
        eliminatorStats.Kills++;
        
        // Calculate distance for stats
        // elimInfo.Distance = (elimInfo.Location - Eliminator->Character->GetActorLocation()).Size();
        elimInfo.Distance = 100.0f; // Placeholder
    }
    
    // Set player state
    SetPlayerState(Player, EPlayerState::Eliminated);
    
    // Add to history
    EliminationHistory.push_back(elimInfo);
    
    // Start spectating
    if (Settings.bAllowSpectating) {
        StartSpectating(Player, Eliminator);
    }
    
    FirePlayerEliminateCallbacks(elimInfo);
    
    LOG_INFO("Player eliminated: " + Player->GetName() + 
             " by " + (Eliminator ? Eliminator->GetName() : "Unknown"));
}

FSpawnInfo PlayerManager::GetSpawnLocation(int32_t TeamId, bool bAvoidEnemies) {
    std::lock_guard<std::mutex> Lock(PlayerMutex);
    
    if (SpawnPoints.empty()) {
        InitializeDefaultSpawnPoints();
    }
    
    // Filter spawn points by team if needed
    std::vector<FSpawnInfo> validSpawns;
    for (const auto& spawn : SpawnPoints) {
        if (TeamId == -1 || spawn.TeamId == -1 || spawn.TeamId == TeamId) {
            if (IsValidSpawnLocation(spawn.Location)) {
                validSpawns.push_back(spawn);
            }
        }
    }
    
    if (validSpawns.empty()) {
        // Return default spawn
        FSpawnInfo defaultSpawn;
        defaultSpawn.Location = FVector(0, 0, 1000); // Safe height
        return defaultSpawn;
    }
    
    // Random selection for now
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, validSpawns.size() - 1);
    
    return validSpawns[dis(gen)];
}

void PlayerManager::SetPlayerTeam(AFortPlayerControllerAthena* Player, int32_t TeamId) {
    if (!Player) return;
    
    std::lock_guard<std::mutex> Lock(PlayerMutex);
    PlayerTeams[Player] = TeamId;
    
    LOG_INFO("Player " + Player->GetName() + " assigned to team " + std::to_string(TeamId));
}

std::vector<AFortPlayerControllerAthena*> PlayerManager::GetAllPlayers() const {
    std::lock_guard<std::mutex> Lock(PlayerMutex);
    
    std::vector<AFortPlayerControllerAthena*> players;
    for (const auto& pair : PlayerStates) {
        if (pair.second != EPlayerState::Disconnected) {
            players.push_back(pair.first);
        }
    }
    return players;
}

void PlayerManager::InitializeDefaultSpawnPoints() {
    // Add some default spawn points in a circle
    const float radius = 5000.0f;
    const int32_t numSpawns = 50;
    
    for (int32_t i = 0; i < numSpawns; ++i) {
        float angle = (2.0f * 3.14159f * i) / numSpawns;
        FVector location;
        location.X = radius * cos(angle);
        location.Y = radius * sin(angle);
        location.Z = 1000.0f; // Safe height
        
        AddSpawnPoint(location);
    }
    
    LOG_INFO("Initialized " + std::to_string(numSpawns) + " default spawn points");
}

void PlayerManager::Update(float DeltaTime) {
    std::lock_guard<std::mutex> Lock(PlayerMutex);
    
    // Process respawn queue
    ProcessRespawnQueue();
    
    // Update player time alive
    for (auto& pair : PlayerStats) {
        if (IsPlayerAlive(pair.first)) {
            pair.second.TimeAlive += DeltaTime;
        }
    }
    
    // Clean up disconnected players periodically
    static float cleanupTimer = 0.0f;
    cleanupTimer += DeltaTime;
    if (cleanupTimer >= 30.0f) { // Clean up every 30 seconds
        CleanupDisconnectedPlayers();
        cleanupTimer = 0.0f;
    }
}

// Helper method implementations
void PlayerManager::ProcessRespawnQueue() {
    // Process respawn queue (simplified implementation)
    while (!RespawnQueue.empty()) {
        auto& respawn = RespawnQueue.front();
        
        // Check if respawn delay has passed
        auto now = std::chrono::steady_clock::now();
        if (now >= (MatchStartTime + std::chrono::seconds(static_cast<long>(respawn.second)))) {
            SpawnPlayer(respawn.first);
            RespawnQueue.pop();
        } else {
            break; // Wait for this one to be ready
        }
    }
}

void PlayerManager::CleanupDisconnectedPlayers() {
    // Clean up players who have been disconnected for too long
    for (auto it = PlayerStates.begin(); it != PlayerStates.end();) {
        if (it->second == EPlayerState::Disconnected) {
            // Remove player data after 5 minutes
            PlayerStats.erase(it->first);
            PlayerTeams.erase(it->first);
            SpectatorTargets.erase(it->first);
            it = PlayerStates.erase(it);
        } else {
            ++it;
        }
    }
}

// Additional missing implementations
void PlayerManager::TeleportPlayer(AFortPlayerControllerAthena* Player, const FVector& Location, const FRotator& Rotation) {
    if (!Player) return;
    
    LOG_INFO("Teleporting player " + Player->GetName() + " to (" + 
             std::to_string(Location.X) + ", " + std::to_string(Location.Y) + ", " + std::to_string(Location.Z) + ")");
    
    // In real implementation, this would actually move the player
    // For now, just log the action
}

void PlayerManager::AddSpawnPoint(const FVector& Location, const FRotator& Rotation, int32_t TeamId) {
    std::lock_guard<std::mutex> Lock(PlayerMutex);
    
    FSpawnInfo spawnInfo;
    spawnInfo.Location = Location;
    spawnInfo.Rotation = Rotation;
    spawnInfo.TeamId = TeamId;
    
    SpawnPoints.push_back(spawnInfo);
    
    LOG_INFO("Added spawn point at (" + std::to_string(Location.X) + ", " + 
             std::to_string(Location.Y) + ", " + std::to_string(Location.Z) + ")");
}

void PlayerManager::ClearSpawnPoints() {
    std::lock_guard<std::mutex> Lock(PlayerMutex);
    SpawnPoints.clear();
    LOG_INFO("Cleared all spawn points");
}

int32_t PlayerManager::AssignTeam() {
    // Simple team assignment - assign to smallest team
    std::unordered_map<int32_t, int32_t> teamCounts;
    
    for (const auto& pair : PlayerTeams) {
        teamCounts[pair.second]++;
    }
    
    // Find team with fewest players
    int32_t bestTeam = 0;
    int32_t minCount = INT_MAX;
    
    for (int32_t team = 0; team < 4; ++team) { // Support up to 4 teams
        int32_t count = teamCounts[team];
        if (count < minCount) {
            minCount = count;
            bestTeam = team;
        }
    }
    
    return bestTeam;
}

bool PlayerManager::IsValidSpawnLocation(const FVector& Location, float Radius) {
    // Basic spawn location validation
    
    // Check if location is not too close to other players
    for (const auto& pair : PlayerStates) {
        if (IsPlayerAlive(pair.first)) {
            // In real implementation, would check actual player location
            // For now, assume locations are valid
        }
    }
    
    // Check if location is within map bounds
    float maxDistance = 50000.0f; // 500m from center
    if (Location.X * Location.X + Location.Y * Location.Y > maxDistance * maxDistance) {
        return false;
    }
    
    return true;
}

// State and team management helpers
EPlayerState PlayerManager::GetPlayerState(AFortPlayerControllerAthena* Player) const {
    if (!Player) return EPlayerState::None;
    
    std::lock_guard<std::mutex> Lock(PlayerMutex);
    
    auto it = PlayerStates.find(Player);
    return it != PlayerStates.end() ? it->second : EPlayerState::None;
}

bool PlayerManager::IsPlayerAlive(AFortPlayerControllerAthena* Player) const {
    EPlayerState state = GetPlayerState(Player);
    return state == EPlayerState::Playing || state == EPlayerState::InLobby;
}

bool PlayerManager::IsPlayerEliminated(AFortPlayerControllerAthena* Player) const {
    return GetPlayerState(Player) == EPlayerState::Eliminated;
}

bool PlayerManager::IsPlayerSpectating(AFortPlayerControllerAthena* Player) const {
    return GetPlayerState(Player) == EPlayerState::Spectating;
}

int32_t PlayerManager::GetPlayerTeam(AFortPlayerControllerAthena* Player) const {
    if (!Player) return -1;
    
    std::lock_guard<std::mutex> Lock(PlayerMutex);
    
    auto it = PlayerTeams.find(Player);
    return it != PlayerTeams.end() ? it->second : -1;
}

FPlayerStats& PlayerManager::GetPlayerStats(AFortPlayerControllerAthena* Player) {
    std::lock_guard<std::mutex> Lock(PlayerMutex);
    return PlayerStats[Player]; // Creates if doesn't exist
}

// Callback management
void PlayerManager::FirePlayerJoinCallbacks(AFortPlayerControllerAthena* Player) {
    for (const auto& pair : JoinCallbacks) {
        try {
            pair.second(Player);
        } catch (...) {
            LOG_ERROR("Exception in PlayerJoin callback: " + pair.first);
        }
    }
}

void PlayerManager::FirePlayerLeaveCallbacks(AFortPlayerControllerAthena* Player) {
    for (const auto& pair : LeaveCallbacks) {
        try {
            pair.second(Player);
        } catch (...) {
            LOG_ERROR("Exception in PlayerLeave callback: " + pair.first);
        }
    }
}

void PlayerManager::FirePlayerEliminateCallbacks(const FEliminationInfo& Info) {
    for (const auto& pair : EliminateCallbacks) {
        try {
            pair.second(Info);
        } catch (...) {
            LOG_ERROR("Exception in PlayerEliminate callback: " + pair.first);
        }
    }
}

void PlayerManager::FirePlayerSpawnCallbacks(AFortPlayerControllerAthena* Player, const FSpawnInfo& SpawnInfo) {
    for (const auto& pair : SpawnCallbacks) {
        try {
            pair.second(Player, SpawnInfo);
        } catch (...) {
            LOG_ERROR("Exception in PlayerSpawn callback: " + pair.first);
        }
    }
}

// Callback registration
void PlayerManager::RegisterPlayerJoinCallback(const std::string& Name, PlayerJoinCallback Callback) {
    std::lock_guard<std::mutex> Lock(PlayerMutex);
    JoinCallbacks[Name] = Callback;
}

void PlayerManager::RegisterPlayerLeaveCallback(const std::string& Name, PlayerLeaveCallback Callback) {
    std::lock_guard<std::mutex> Lock(PlayerMutex);
    LeaveCallbacks[Name] = Callback;
}

void PlayerManager::RegisterPlayerEliminateCallback(const std::string& Name, PlayerEliminateCallback Callback) {
    std::lock_guard<std::mutex> Lock(PlayerMutex);
    EliminateCallbacks[Name] = Callback;
}

void PlayerManager::RegisterPlayerSpawnCallback(const std::string& Name, PlayerSpawnCallback Callback) {
    std::lock_guard<std::mutex> Lock(PlayerMutex);
    SpawnCallbacks[Name] = Callback;
}

void PlayerManager::UnregisterCallback(const std::string& Name) {
    std::lock_guard<std::mutex> Lock(PlayerMutex);
    JoinCallbacks.erase(Name);
    LeaveCallbacks.erase(Name);
    EliminateCallbacks.erase(Name);
    SpawnCallbacks.erase(Name);
}

// Additional helper methods
std::vector<AFortPlayerControllerAthena*> PlayerManager::GetAlivePlayers() const {
    std::lock_guard<std::mutex> Lock(PlayerMutex);
    
    std::vector<AFortPlayerControllerAthena*> alivePlayers;
    for (const auto& pair : PlayerStates) {
        if (IsPlayerAlive(pair.first)) {
            alivePlayers.push_back(pair.first);
        }
    }
    return alivePlayers;
}

uint32_t PlayerManager::GetPlayerCount() const {
    std::lock_guard<std::mutex> Lock(PlayerMutex);
    return static_cast<uint32_t>(PlayerStates.size());
}

uint32_t PlayerManager::GetAlivePlayerCount() const {
    return static_cast<uint32_t>(GetAlivePlayers().size());
}

// Spectator system stubs
void PlayerManager::StartSpectating(AFortPlayerControllerAthena* Spectator, AFortPlayerControllerAthena* Target) {
    if (!Spectator) return;
    
    std::lock_guard<std::mutex> Lock(PlayerMutex);
    
    SetPlayerState(Spectator, EPlayerState::Spectating);
    SpectatorTargets[Spectator] = Target;
    
    LOG_INFO("Player " + Spectator->GetName() + " started spectating " + 
             (Target ? Target->GetName() : "no target"));
}

void PlayerManager::StopSpectating(AFortPlayerControllerAthena* Spectator) {
    if (!Spectator) return;
    
    std::lock_guard<std::mutex> Lock(PlayerMutex);
    
    SpectatorTargets.erase(Spectator);
    
    LOG_INFO("Player " + Spectator->GetName() + " stopped spectating");
}