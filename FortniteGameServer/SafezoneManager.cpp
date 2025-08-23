#include "SafezoneManager.h"
#include "PlayerManager.h"
#include "UObject.h"
#include "Definitions.h"
#include <random>
#include <algorithm>

SafezoneManager& SafezoneManager::Get() {
    static SafezoneManager Instance;
    return Instance;
}

void SafezoneManager::Update(float DeltaTime) {
    if (!Settings.bEnabled || !bStormActive || bStormPaused) return;
    
    std::lock_guard<std::mutex> Lock(SafezoneMutex);
    
    auto now = std::chrono::steady_clock::now();
    
    // Update storm phase timing
    UpdateStormPhase(DeltaTime);
    
    // Damage players in storm
    auto timeSinceLastDamage = std::chrono::duration<float>(now - LastDamageTime).count();
    if (timeSinceLastDamage >= Settings.DamageTickRate) {
        DamagePlayersInStorm();
        LastDamageTime = now;
    }
    
    // Update player storm status
    UpdatePlayerStormStatus();
    
    LastUpdateTime = now;
}

void SafezoneManager::StartStorm() {
    if (bStormActive) return;
    
    std::lock_guard<std::mutex> Lock(SafezoneMutex);
    
    if (StormPhases.empty()) {
        InitializeDefaultPhases();
    }
    
    bStormActive = true;
    bStormPaused = false;
    
    SafezoneInfo.CurrentPhase = EStormPhase::PreStorm;
    SafezoneInfo.PhaseIndex = 0;
    SafezoneInfo.PhaseStartTime = std::chrono::steady_clock::now();
    
    Stats.StormStartTime = SafezoneInfo.PhaseStartTime;
    Stats.CurrentStormDuration = 0.0f;
    
    LOG_INFO("Storm started with " + std::to_string(StormPhases.size()) + " phases");
    
    FireStormPhaseCallbacks(SafezoneInfo.CurrentPhase, SafezoneInfo.PhaseIndex);
}

void SafezoneManager::StopStorm() {
    if (!bStormActive) return;
    
    std::lock_guard<std::mutex> Lock(SafezoneMutex);
    
    bStormActive = false;
    bStormPaused = false;
    
    SafezoneInfo.CurrentPhase = EStormPhase::Ended;
    
    LOG_INFO("Storm stopped");
    
    FireStormPhaseCallbacks(SafezoneInfo.CurrentPhase, SafezoneInfo.PhaseIndex);
}

void SafezoneManager::SkipToNextPhase() {
    if (!bStormActive || SafezoneInfo.CurrentPhase == EStormPhase::Ended) return;
    
    std::lock_guard<std::mutex> Lock(SafezoneMutex);
    
    StartNextPhase();
    
    LOG_INFO("Skipped to storm phase " + std::to_string(SafezoneInfo.PhaseIndex));
}

void SafezoneManager::SetStormPhases(const std::vector<FStormPhaseData>& Phases) {
    std::lock_guard<std::mutex> Lock(SafezoneMutex);
    StormPhases = Phases;
    LOG_INFO("Updated storm phases, now has " + std::to_string(StormPhases.size()) + " phases");
}

bool SafezoneManager::IsPlayerInSafezone(AFortPlayerControllerAthena* Player) const {
    if (!Player) return true;
    
    // Get player location (simplified)
    FVector playerLocation = FVector(); // In real implementation, get actual player location
    
    return SafezoneInfo.IsPlayerInSafezone(playerLocation);
}

std::vector<AFortPlayerControllerAthena*> SafezoneManager::GetPlayersInStorm() const {
    std::lock_guard<std::mutex> Lock(SafezoneMutex);
    
    std::vector<AFortPlayerControllerAthena*> playersInStorm;
    
    auto& playerMgr = PlayerManager::Get();
    auto allPlayers = playerMgr.GetAllPlayers();
    
    for (auto* player : allPlayers) {
        if (!IsPlayerInSafezone(player)) {
            playersInStorm.push_back(player);
        }
    }
    
    return playersInStorm;
}

std::vector<AFortPlayerControllerAthena*> SafezoneManager::GetPlayersInSafezone() const {
    std::lock_guard<std::mutex> Lock(SafezoneMutex);
    
    std::vector<AFortPlayerControllerAthena*> playersInSafezone;
    
    auto& playerMgr = PlayerManager::Get();
    auto allPlayers = playerMgr.GetAllPlayers();
    
    for (auto* player : allPlayers) {
        if (IsPlayerInSafezone(player)) {
            playersInSafezone.push_back(player);
        }
    }
    
    return playersInSafezone;
}

void SafezoneManager::DamagePlayersInStorm() {
    if (!Settings.bDamageInStorm) return;
    
    auto playersInStorm = GetPlayersInStorm();
    float damageAmount = GetCurrentStormDamage();
    
    for (auto* player : playersInStorm) {
        if (ShouldDamagePlayer(player)) {
            // Apply storm damage (simplified)
            Stats.TotalStormDamageDealt += damageAmount;
            
            // Check if player is eliminated by storm
            // In real implementation, this would interface with health system
            
            FireStormDamageCallbacks(player, damageAmount);
        }
    }
    
    Stats.CurrentPlayersInStorm = static_cast<uint32_t>(playersInStorm.size());
}

float SafezoneManager::GetCurrentStormDamage() const {
    if (SafezoneInfo.PhaseIndex < StormPhases.size()) {
        float damage = StormPhases[SafezoneInfo.PhaseIndex].DamagePerSecond;
        
        if (Settings.bScaleDamageWithPhase) {
            damage *= (1.0f + SafezoneInfo.PhaseIndex * 0.5f); // Increase damage each phase
        }
        
        return std::min(damage, Settings.MaxStormDamage);
    }
    
    return 1.0f; // Default damage
}

bool SafezoneManager::ShouldDamagePlayer(AFortPlayerControllerAthena* Player) const {
    if (!Player) return false;
    
    auto& playerMgr = PlayerManager::Get();
    return playerMgr.IsPlayerAlive(Player);
}

void SafezoneManager::UpdateStormPhase(float DeltaTime) {
    if (SafezoneInfo.PhaseIndex >= StormPhases.size()) {
        if (SafezoneInfo.CurrentPhase != EStormPhase::Ended) {
            SafezoneInfo.CurrentPhase = EStormPhase::Ended;
            FireStormPhaseCallbacks(SafezoneInfo.CurrentPhase, SafezoneInfo.PhaseIndex);
        }
        return;
    }
    
    const auto& currentPhase = StormPhases[SafezoneInfo.PhaseIndex];
    auto now = std::chrono::steady_clock::now();
    auto phaseElapsed = std::chrono::duration<float>(now - SafezoneInfo.PhaseStartTime).count();
    
    switch (SafezoneInfo.CurrentPhase) {
        case EStormPhase::PreStorm:
        case EStormPhase::Waiting:
            if (phaseElapsed >= currentPhase.WaitTime) {
                SafezoneInfo.CurrentPhase = EStormPhase::Shrinking;
                SafezoneInfo.ShrinkStartTime = now;
                SafezoneInfo.TargetRadius = currentPhase.NewRadius;
                if (currentPhase.bMoveCenter) {
                    SafezoneInfo.TargetCenter = currentPhase.NewCenter;
                }
                
                LOG_INFO("Storm phase " + std::to_string(SafezoneInfo.PhaseIndex) + " beginning shrink");
            }
            break;
            
        case EStormPhase::Shrinking:
            ProcessShrinking(DeltaTime);
            
            auto shrinkElapsed = std::chrono::duration<float>(now - SafezoneInfo.ShrinkStartTime).count();
            if (shrinkElapsed >= currentPhase.ShrinkTime) {
                StartNextPhase();
            }
            break;
    }
}

void SafezoneManager::ProcessShrinking(float DeltaTime) {
    if (SafezoneInfo.PhaseIndex >= StormPhases.size()) return;
    
    const auto& currentPhase = StormPhases[SafezoneInfo.PhaseIndex];
    float progress = SafezoneInfo.GetShrinkProgress();
    
    // Interpolate radius
    SafezoneInfo.CurrentRadius = SafezoneInfo.CurrentRadius + 
        (SafezoneInfo.TargetRadius - SafezoneInfo.CurrentRadius) * progress;
    
    // Interpolate center if moving
    if (currentPhase.bMoveCenter) {
        SafezoneInfo.Center = LerpVector(SafezoneInfo.Center, SafezoneInfo.TargetCenter, progress);
    }
}

void SafezoneManager::StartNextPhase() {
    SafezoneInfo.PhaseIndex++;
    SafezoneInfo.PhaseStartTime = std::chrono::steady_clock::now();
    
    if (SafezoneInfo.PhaseIndex < StormPhases.size()) {
        SafezoneInfo.CurrentPhase = EStormPhase::Waiting;
        
        const auto& nextPhase = StormPhases[SafezoneInfo.PhaseIndex];
        SendPhaseNotification(nextPhase);
        
        LOG_INFO("Started storm phase " + std::to_string(SafezoneInfo.PhaseIndex));
    } else {
        SafezoneInfo.CurrentPhase = EStormPhase::FinalPhase;
        LOG_INFO("Storm reached final phase");
    }
    
    FireStormPhaseCallbacks(SafezoneInfo.CurrentPhase, SafezoneInfo.PhaseIndex);
}

void SafezoneManager::UpdatePlayerStormStatus() {
    auto& playerMgr = PlayerManager::Get();
    auto allPlayers = playerMgr.GetAllPlayers();
    
    for (auto* player : allPlayers) {
        bool wasInStorm = PlayersInStorm.find(player) != PlayersInStorm.end();
        bool isInStorm = !IsPlayerInSafezone(player);
        
        if (!wasInStorm && isInStorm) {
            PlayersInStorm.insert(player);
            FirePlayerEnteredStormCallbacks(player);
        } else if (wasInStorm && !isInStorm) {
            PlayersInStorm.erase(player);
            FirePlayerExitedStormCallbacks(player);
        }
    }
}

void SafezoneManager::SendPhaseNotification(const FStormPhaseData& Phase) {
    if (!Settings.bShowWarnings) return;
    
    // Send notification to all players about upcoming phase
    LOG_INFO("Storm notification: " + Phase.PhaseName.ToString() + " - " + Phase.WarningMessage.ToString());
}

void SafezoneManager::InitializeDefaultPhases() {
    StormPhases.clear();
    
    // Phase 1: Large initial shrink
    StormPhases.push_back(FStormPhaseData(120.0f, 240.0f, 1.0f, 8000.0f, FString("Phase 1")));
    
    // Phase 2: Moderate shrink
    StormPhases.push_back(FStormPhaseData(90.0f, 180.0f, 2.0f, 5000.0f, FString("Phase 2")));
    
    // Phase 3: Faster shrink
    StormPhases.push_back(FStormPhaseData(60.0f, 120.0f, 3.0f, 2500.0f, FString("Phase 3")));
    
    // Phase 4: Small circle
    StormPhases.push_back(FStormPhaseData(45.0f, 90.0f, 5.0f, 1000.0f, FString("Phase 4")));
    
    // Phase 5: Final circle
    StormPhases.push_back(FStormPhaseData(30.0f, 60.0f, 10.0f, 300.0f, FString("Final Phase")));
    
    LOG_INFO("Initialized " + std::to_string(StormPhases.size()) + " default storm phases");
}

FVector SafezoneManager::LerpVector(const FVector& A, const FVector& B, float Alpha) {
    return FVector(
        A.X + (B.X - A.X) * Alpha,
        A.Y + (B.Y - A.Y) * Alpha,
        A.Z + (B.Z - A.Z) * Alpha
    );
}

float SafezoneManager::LerpFloat(float A, float B, float Alpha) {
    return A + (B - A) * Alpha;
}

// Callback management
void SafezoneManager::RegisterStormPhaseCallback(const std::string& Name, StormPhaseCallback Callback) {
    std::lock_guard<std::mutex> Lock(SafezoneMutex);
    PhaseCallbacks[Name] = Callback;
}

void SafezoneManager::RegisterPlayerEnteredStormCallback(const std::string& Name, PlayerEnteredStormCallback Callback) {
    std::lock_guard<std::mutex> Lock(SafezoneMutex);
    EnteredStormCallbacks[Name] = Callback;
}

void SafezoneManager::RegisterPlayerExitedStormCallback(const std::string& Name, PlayerExitedStormCallback Callback) {
    std::lock_guard<std::mutex> Lock(SafezoneMutex);
    ExitedStormCallbacks[Name] = Callback;
}

void SafezoneManager::RegisterStormDamageCallback(const std::string& Name, StormDamageCallback Callback) {
    std::lock_guard<std::mutex> Lock(SafezoneMutex);
    DamageCallbacks[Name] = Callback;
}

void SafezoneManager::UnregisterCallback(const std::string& Name) {
    std::lock_guard<std::mutex> Lock(SafezoneMutex);
    PhaseCallbacks.erase(Name);
    EnteredStormCallbacks.erase(Name);
    ExitedStormCallbacks.erase(Name);
    DamageCallbacks.erase(Name);
}

void SafezoneManager::FireStormPhaseCallbacks(EStormPhase Phase, int32_t PhaseIndex) {
    for (const auto& pair : PhaseCallbacks) {
        try {
            pair.second(Phase, PhaseIndex);
        } catch (...) {
            LOG_ERROR("Exception in StormPhase callback: " + pair.first);
        }
    }
}

void SafezoneManager::FirePlayerEnteredStormCallbacks(AFortPlayerControllerAthena* Player) {
    for (const auto& pair : EnteredStormCallbacks) {
        try {
            pair.second(Player);
        } catch (...) {
            LOG_ERROR("Exception in PlayerEnteredStorm callback: " + pair.first);
        }
    }
}

void SafezoneManager::FirePlayerExitedStormCallbacks(AFortPlayerControllerAthena* Player) {
    for (const auto& pair : ExitedStormCallbacks) {
        try {
            pair.second(Player);
        } catch (...) {
            LOG_ERROR("Exception in PlayerExitedStorm callback: " + pair.first);
        }
    }
}

void SafezoneManager::FireStormDamageCallbacks(AFortPlayerControllerAthena* Player, float Damage) {
    for (const auto& pair : DamageCallbacks) {
        try {
            pair.second(Player, Damage);
        } catch (...) {
            LOG_ERROR("Exception in StormDamage callback: " + pair.first);
        }
    }
}