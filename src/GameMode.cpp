#include "GameMode.h"
#include <iostream>
#include <random>
#include <cmath>
#include <thread>
#include <chrono>

GameMode::GameMode()
    : m_currentPhase(GamePhase::WaitingToStart)
    , m_matchTime(0.0f)
    , m_phaseTimer(0.0f)
    , m_currentStormPhase(0)
    , m_stormRadius(0.0f)
    , m_stormCenter(SDK::FVector())
    , m_stormActive(false)
    , m_aircraftActive(false)
    , m_aircraftSpeed(2000.0f)
    , m_aircraftPosition(SDK::FVector())
    , m_aircraftDirection(SDK::FVector())
    , m_matchActive(false)
    , m_matchEnded(false)
    , m_playersAlive(0)
    , m_teamsAlive(0)
{
}

GameMode::~GameMode()
{
}

bool GameMode::Initialize(const GameSettings& settings)
{
    std::cout << "[GameMode] Initializing game mode..." << std::endl;
    
    m_settings = settings;
    
    // Initialize storm phases
    InitializeStorm();
    
    // Initialize aircraft
    InitializeAircraft();
    
    // Reset match state
    m_matchTime = 0.0f;
    m_phaseTimer = 0.0f;
    m_currentPhase = GamePhase::WaitingToStart;
    m_matchActive = false;
    m_matchEnded = false;
    m_playersAlive = 0;
    m_teamsAlive = 0;
    
    std::cout << "[GameMode] Game mode initialized successfully" << std::endl;
    std::cout << "[GameMode] Mode: " << (int)m_settings.mode << ", Max Players: " << m_settings.maxPlayers << std::endl;
    
    return true;
}

void GameMode::StartMatch()
{
    if (m_matchActive)
    {
        std::cout << "[GameMode] Match is already active" << std::endl;
        return;
    }
    
    std::cout << "[GameMode] Starting new match..." << std::endl;
    
    // Reset match state
    m_matchTime = 0.0f;
    m_phaseTimer = 0.0f;
    m_matchActive = true;
    m_matchEnded = false;
    
    // Start in warm-up phase
    TransitionToPhase(GamePhase::WarmUp);
    
    std::cout << "[GameMode] Match started!" << std::endl;
}

void GameMode::EndMatch()
{
    if (!m_matchActive || m_matchEnded)
    {
        return;
    }
    
    std::cout << "[GameMode] Ending match..." << std::endl;
    
    m_matchActive = false;
    m_matchEnded = true;
    m_stormActive = false;
    m_aircraftActive = false;
    
    TransitionToPhase(GamePhase::EndGame);
    
    std::cout << "[GameMode] Match ended after " << m_matchTime << " seconds" << std::endl;
}

void GameMode::RestartMatch()
{
    std::cout << "[GameMode] Restarting match..." << std::endl;
    
    EndMatch();
    
    // Wait a moment then start new match
    std::this_thread::sleep_for(std::chrono::seconds(2));
    StartMatch();
}

void GameMode::Tick(float deltaTime)
{
    if (!m_matchActive || m_matchEnded)
    {
        return;
    }
    
    // Update match time
    m_matchTime += deltaTime;
    m_phaseTimer += deltaTime;
    
    // Update storm
    if (m_settings.stormEnabled)
    {
        UpdateStorm(deltaTime);
    }
    
    // Update aircraft
    if (m_aircraftActive)
    {
        UpdateAircraft(deltaTime);
    }
    
    // Update phase timer
    UpdatePhaseTimer(deltaTime);
    
    // Process game logic
    ProcessGameLogic();
    
    // Update statistics
    UpdateMatchStatistics();
    
    // Check victory conditions
    if (CheckVictoryConditions())
    {
        EndMatch();
    }
}

int GameMode::GetPlayersAlive() const
{
    return m_playersAlive;
}

int GameMode::GetTeamsAlive() const
{
    return m_teamsAlive;
}

void GameMode::InitializeStorm()
{
    std::cout << "[GameMode] Initializing storm system..." << std::endl;
    
    // Clear existing phases
    m_stormPhases.clear();
    
    // Create default storm phases for Battle Royale
    m_stormPhases.push_back({ 240.0f, 1.0f, 8000.0f, SDK::FVector(0, 0, 0) });
    m_stormPhases.push_back({ 180.0f, 2.0f, 5000.0f, SDK::FVector(0, 0, 0) });
    m_stormPhases.push_back({ 120.0f, 5.0f, 3000.0f, SDK::FVector(0, 0, 0) });
    m_stormPhases.push_back({ 90.0f, 8.0f, 1500.0f, SDK::FVector(0, 0, 0) });
    m_stormPhases.push_back({ 60.0f, 10.0f, 500.0f, SDK::FVector(0, 0, 0) });
    
    // Initialize storm state
    m_currentStormPhase = 0;
    m_stormRadius = 15000.0f; // Start with large radius
    m_stormCenter = SDK::FVector(0, 0, 0); // Center of map
    m_stormActive = false;
    
    std::cout << "[GameMode] Storm system initialized with " << m_stormPhases.size() << " phases" << std::endl;
}

void GameMode::UpdateStorm(float deltaTime)
{
    if (!m_stormActive || m_currentStormPhase >= m_stormPhases.size())
    {
        return;
    }
    
    const auto& currentPhase = m_stormPhases[m_currentStormPhase];
    
    // Gradually shrink storm radius
    float shrinkRate = (m_stormRadius - currentPhase.shrinkRadius) / currentPhase.duration;
    m_stormRadius -= shrinkRate * deltaTime;
    
    if (m_stormRadius <= currentPhase.shrinkRadius)
    {
        m_stormRadius = currentPhase.shrinkRadius;
    }
    
    // TODO: Apply storm damage to players outside the safe zone
    // This would require access to player positions and health systems
}

void GameMode::StartNextStormPhase()
{
    if (m_currentStormPhase >= m_stormPhases.size())
    {
        std::cout << "[GameMode] All storm phases completed" << std::endl;
        return;
    }
    
    std::cout << "[GameMode] Starting storm phase " << (m_currentStormPhase + 1) << std::endl;
    
    const auto& phase = m_stormPhases[m_currentStormPhase];
    
    // Generate new storm center (for more interesting gameplay)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-500.0f, 500.0f);
    
    m_stormCenter.X += dis(gen);
    m_stormCenter.Y += dis(gen);
    
    m_stormActive = true;
    m_currentStormPhase++;
    
    std::cout << "[GameMode] Storm phase " << m_currentStormPhase << " active. Radius: " << phase.shrinkRadius 
              << ", Damage: " << phase.damagePerSecond << "/sec" << std::endl;
}

void GameMode::OnPlayerJoined(FortPlayer* player)
{
    if (!player)
    {
        return;
    }
    
    std::cout << "[GameMode] Player joined: " << player->GetPlayerName() << std::endl;
    
    m_playersAlive++;
    
    // Assign to team if team-based mode
    if (m_settings.mode != GameModeType::Solo)
    {
        // Team assignment logic would go here
        m_teamsAlive++;
    }
    else
    {
        m_teamsAlive = m_playersAlive; // In solo, each player is their own team
    }
    
    std::cout << "[GameMode] Players alive: " << m_playersAlive << ", Teams alive: " << m_teamsAlive << std::endl;
}

void GameMode::OnPlayerLeft(FortPlayer* player)
{
    if (!player)
    {
        return;
    }
    
    std::cout << "[GameMode] Player left: " << player->GetPlayerName() << std::endl;
    
    if (m_playersAlive > 0)
    {
        m_playersAlive--;
    }
    
    // Update team count logic would go here based on team composition
    if (m_settings.mode == GameModeType::Solo && m_teamsAlive > 0)
    {
        m_teamsAlive--;
    }
}

void GameMode::OnPlayerEliminated(FortPlayer* player, FortPlayer* eliminator)
{
    if (!player)
    {
        return;
    }
    
    std::string eliminatorName = eliminator ? eliminator->GetPlayerName() : "Unknown";
    std::cout << "[GameMode] Player eliminated: " << player->GetPlayerName() 
              << " by " << eliminatorName << std::endl;
    
    if (m_playersAlive > 0)
    {
        m_playersAlive--;
    }
    
    // Update placement
    player->GetStats().placement = m_playersAlive + 1;
    
    // Award kill to eliminator
    if (eliminator)
    {
        eliminator->AddKill(player);
    }
    
    // Check if team is eliminated
    // This would require more complex team tracking
    if (m_settings.mode == GameModeType::Solo && m_teamsAlive > 0)
    {
        m_teamsAlive--;
    }
    
    std::cout << "[GameMode] Players remaining: " << m_playersAlive << ", Teams remaining: " << m_teamsAlive << std::endl;
}

void GameMode::OnTeamEliminated(int teamId)
{
    std::cout << "[GameMode] Team " << teamId << " eliminated" << std::endl;
    
    if (m_teamsAlive > 0)
    {
        m_teamsAlive--;
    }
}

void GameMode::OnBuildingPlaced(class ABuildingSMActor* building, FortPlayer* player)
{
    if (!player || !building)
    {
        return;
    }
    
    // Update player stats
    player->OnBuildingPlaced(building);
    
    // TODO: Implement building validation, resource deduction, etc.
}

void GameMode::OnBuildingDestroyed(class ABuildingSMActor* building, FortPlayer* destroyer)
{
    if (!building)
    {
        return;
    }
    
    if (destroyer)
    {
        destroyer->OnBuildingDestroyed(building, true);
    }
    
    // TODO: Award materials, update stats, etc.
}

void GameMode::SpawnLoot()
{
    std::cout << "[GameMode] Spawning loot..." << std::endl;
    
    if (m_settings.weaponDropsEnabled)
    {
        SpawnWeapons();
    }
    
    SpawnConsumables();
    SpawnMaterials();
    
    std::cout << "[GameMode] Loot spawn complete" << std::endl;
}

void GameMode::SpawnWeapons()
{
    // TODO: Implement weapon spawning using SDK weapon classes
    std::cout << "[GameMode] Spawning weapons (placeholder)" << std::endl;
}

void GameMode::SpawnConsumables()
{
    // TODO: Implement consumable spawning
    std::cout << "[GameMode] Spawning consumables (placeholder)" << std::endl;
}

void GameMode::SpawnMaterials()
{
    // TODO: Implement material spawning
    std::cout << "[GameMode] Spawning materials (placeholder)" << std::endl;
}

void GameMode::InitializeAircraft()
{
    std::cout << "[GameMode] Initializing aircraft..." << std::endl;
    
    // Set initial aircraft position and direction
    m_aircraftPosition = SDK::FVector(-10000.0f, 0.0f, 5000.0f); // Start off the map
    m_aircraftDirection = SDK::FVector(1.0f, 0.0f, 0.0f); // Flying east
    m_aircraftSpeed = 2000.0f; // Units per second
    
    std::cout << "[GameMode] Aircraft initialized" << std::endl;
}

void GameMode::UpdateAircraft(float deltaTime)
{
    if (!m_aircraftActive)
    {
        return;
    }
    
    // Move aircraft
    m_aircraftPosition.X += m_aircraftDirection.X * m_aircraftSpeed * deltaTime;
    m_aircraftPosition.Y += m_aircraftDirection.Y * m_aircraftSpeed * deltaTime;
    
    // Check if aircraft has crossed the map
    if (m_aircraftPosition.X > 10000.0f) // Arbitrary map boundary
    {
        m_aircraftActive = false;
        std::cout << "[GameMode] Aircraft has left the map area" << std::endl;
        
        // Transition to next phase
        TransitionToPhase(GamePhase::SafeZones);
    }
}

bool GameMode::CanPlayersJump() const
{
    return m_aircraftActive && m_currentPhase == GamePhase::Aircraft;
}

bool GameMode::CheckVictoryConditions()
{
    // Check if match should end
    if (m_teamsAlive <= 1)
    {
        std::cout << "[GameMode] Victory condition met! Teams remaining: " << m_teamsAlive << std::endl;
        return true;
    }
    
    // Check if storm has fully closed
    if (m_stormActive && m_stormRadius <= 0.0f)
    {
        std::cout << "[GameMode] Storm has fully closed!" << std::endl;
        return true;
    }
    
    return false;
}

void GameMode::DeclareWinner(FortPlayer* winner)
{
    if (!winner)
    {
        std::cout << "[GameMode] Match ended with no winner" << std::endl;
        return;
    }
    
    std::cout << "[GameMode] Victory! Winner: " << winner->GetPlayerName() << std::endl;
    
    // Update winner stats
    winner->GetStats().placement = 1;
    
    // TODO: Broadcast victory message to all players
    // TODO: Award victory rewards
}

void GameMode::DeclareWinningTeam(int teamId)
{
    std::cout << "[GameMode] Victory! Winning team: " << teamId << std::endl;
    
    // TODO: Update team stats and broadcast victory
}

void GameMode::TransitionToPhase(GamePhase newPhase)
{
    if (m_currentPhase == newPhase)
    {
        return;
    }
    
    std::cout << "[GameMode] Transitioning from phase " << (int)m_currentPhase 
              << " to phase " << (int)newPhase << std::endl;
    
    // Handle phase exit logic
    switch (m_currentPhase)
    {
        case GamePhase::WaitingToStart:
            break;
        case GamePhase::WarmUp:
            break;
        case GamePhase::Aircraft:
            m_aircraftActive = false;
            break;
        case GamePhase::SafeZones:
            break;
        case GamePhase::EndGame:
            break;
    }
    
    // Update phase
    m_currentPhase = newPhase;
    m_phaseTimer = 0.0f;
    
    // Handle phase entry logic
    switch (newPhase)
    {
        case GamePhase::WaitingToStart:
            break;
        case GamePhase::WarmUp:
            SpawnLoot();
            break;
        case GamePhase::Aircraft:
            m_aircraftActive = true;
            break;
        case GamePhase::SafeZones:
            StartNextStormPhase();
            break;
        case GamePhase::EndGame:
            break;
    }
    
    std::cout << "[GameMode] Phase transition complete" << std::endl;
}

void GameMode::UpdatePhaseTimer(float deltaTime)
{
    // Handle phase transitions based on timers
    switch (m_currentPhase)
    {
        case GamePhase::WaitingToStart:
            // Transition when enough players join
            if (m_playersAlive >= 2) // Minimum for testing
            {
                TransitionToPhase(GamePhase::WarmUp);
            }
            break;
            
        case GamePhase::WarmUp:
            // Fixed warm-up time
            if (m_phaseTimer >= 30.0f)
            {
                TransitionToPhase(GamePhase::Aircraft);
            }
            break;
            
        case GamePhase::Aircraft:
            // Aircraft phase duration
            if (m_phaseTimer >= 60.0f)
            {
                TransitionToPhase(GamePhase::SafeZones);
            }
            break;
            
        case GamePhase::SafeZones:
            // Check if current storm phase is complete
            if (m_currentStormPhase < m_stormPhases.size())
            {
                const auto& phase = m_stormPhases[m_currentStormPhase - 1];
                if (m_phaseTimer >= phase.duration)
                {
                    StartNextStormPhase();
                    m_phaseTimer = 0.0f;
                }
            }
            break;
            
        case GamePhase::EndGame:
            // End game cleanup
            break;
    }
}

void GameMode::ProcessGameLogic()
{
    // Handle phase-specific logic
    switch (m_currentPhase)
    {
        case GamePhase::WaitingToStart:
            HandleWaitingToStart();
            break;
        case GamePhase::WarmUp:
            HandleWarmUp();
            break;
        case GamePhase::Aircraft:
            HandleAircraft();
            break;
        case GamePhase::SafeZones:
            HandleSafeZones();
            break;
        case GamePhase::EndGame:
            HandleEndGame();
            break;
    }
}

void GameMode::UpdateMatchStatistics()
{
    // Update match statistics
    // This would be expanded to track various gameplay metrics
}

void GameMode::HandleWaitingToStart()
{
    // Wait for players to join
}

void GameMode::HandleWarmUp()
{
    // Warm-up phase logic
}

void GameMode::HandleAircraft()
{
    // Aircraft phase logic
}

void GameMode::HandleSafeZones()
{
    // Safe zones/storm phase logic
}

void GameMode::HandleEndGame()
{
    // End game logic
}
