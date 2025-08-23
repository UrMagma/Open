#pragma once

#include "ForwardDeclarations.h"
#include "Definitions.h"
#include <chrono>
#include <vector>
#include <functional>
#include <mutex>

// Forward declarations
class AFortPlayerControllerAthena;

/**
 * SafezoneManager - Dynamic storm/safezone system inspired by Project Reboot
 * 
 * Features:
 * - Multi-phase storm shrinking
 * - Player damage outside safezone
 * - Dynamic storm movement
 * - Customizable storm phases
 * - Warning systems and notifications
 * - Visual effects coordination
 */

enum class EStormPhase : uint8_t {
    PreStorm = 0,     // Before first storm
    Shrinking = 1,    // Storm is shrinking
    Waiting = 2,      // Waiting between phases
    Moving = 3,       // Storm center is moving
    FinalPhase = 4,   // Final small circle
    Ended = 5         // Storm has ended
};

struct FStormPhaseData {
    float WaitTime = 60.0f;          // Time before shrinking starts
    float ShrinkTime = 180.0f;       // Time for storm to shrink
    float DamagePerSecond = 1.0f;    // Damage dealt outside safezone
    float NewRadius = 5000.0f;       // Target radius for this phase
    FVector NewCenter = FVector();   // Target center (if moving)
    bool bMoveCenter = false;        // Whether storm center moves
    
    FString PhaseName = FString("Storm Phase");
    FString WarningMessage = FString("The storm is approaching!");
    
    FStormPhaseData() = default;
    FStormPhaseData(float waitTime, float shrinkTime, float damage, float radius, const FString& name = FString())
        : WaitTime(waitTime), ShrinkTime(shrinkTime), DamagePerSecond(damage), NewRadius(radius), PhaseName(name) {}
};

struct FSafezoneInfo {
    FVector Center = FVector();
    float CurrentRadius = 10000.0f;
    float TargetRadius = 5000.0f;
    FVector TargetCenter = FVector();
    
    EStormPhase CurrentPhase = EStormPhase::PreStorm;
    int32_t PhaseIndex = 0;
    
    std::chrono::steady_clock::time_point PhaseStartTime;
    std::chrono::steady_clock::time_point ShrinkStartTime;
    
    float GetShrinkProgress() const {
        if (CurrentPhase != EStormPhase::Shrinking && CurrentPhase != EStormPhase::Moving) {
            return 0.0f;
        }
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - ShrinkStartTime).count();
        return std::min(1.0f, elapsed / 180.0f); // Default shrink time
    }
    
    bool IsPlayerInSafezone(const FVector& PlayerLocation) const {
        float distance = (PlayerLocation - Center).Size();
        return distance <= CurrentRadius;
    }
    
    float GetDistanceFromEdge(const FVector& PlayerLocation) const {
        float distance = (PlayerLocation - Center).Size();
        return distance - CurrentRadius;
    }
};

class SafezoneManager {
public:
    static SafezoneManager& Get();
    
    // Storm control
    void StartStorm();
    void StopStorm();
    void PauseStorm();
    void ResumeStorm();
    void SkipToNextPhase();
    void SkipToPhase(int32_t PhaseIndex);
    
    // Phase management
    void SetStormPhases(const std::vector<FStormPhaseData>& Phases);
    void AddStormPhase(const FStormPhaseData& Phase);
    void ClearStormPhases();
    std::vector<FStormPhaseData> GetDefaultStormPhases();
    
    // Safezone queries
    const FSafezoneInfo& GetSafezoneInfo() const { return SafezoneInfo; }
    FVector GetSafezoneCenter() const { return SafezoneInfo.Center; }
    float GetSafezoneRadius() const { return SafezoneInfo.CurrentRadius; }
    EStormPhase GetCurrentPhase() const { return SafezoneInfo.CurrentPhase; }
    int32_t GetCurrentPhaseIndex() const { return SafezoneInfo.PhaseIndex; }
    
    // Player queries
    bool IsPlayerInSafezone(AFortPlayerControllerAthena* Player) const;
    float GetPlayerDistanceFromEdge(AFortPlayerControllerAthena* Player) const;
    std::vector<AFortPlayerControllerAthena*> GetPlayersInStorm() const;
    std::vector<AFortPlayerControllerAthena*> GetPlayersInSafezone() const;
    uint32_t GetPlayersInStormCount() const;
    uint32_t GetPlayersInSafezoneCount() const;
    
    // Damage system
    void DamagePlayersInStorm();
    void SetStormDamage(float DamagePerSecond);
    float GetCurrentStormDamage() const;
    bool ShouldDamagePlayer(AFortPlayerControllerAthena* Player) const;
    
    // Notifications and warnings
    void SendStormWarning(float SecondsUntilShrink);
    void SendPhaseNotification(const FStormPhaseData& Phase);
    void BroadcastStormUpdate();
    void NotifyPlayersInStorm();
    
    // Custom safezone
    void SetCustomSafezone(const FVector& Center, float Radius);
    void MoveSafezoneTo(const FVector& NewCenter, float Duration = 60.0f);
    void ShrinkSafezoneTo(float NewRadius, float Duration = 180.0f);
    void SetSafezoneInstant(const FVector& Center, float Radius);
    
    // Time management
    float GetTimeUntilNextPhase() const;
    float GetTimeUntilShrink() const;
    float GetPhaseProgress() const;
    float GetShrinkProgress() const;
    
    // Events and callbacks
    using StormPhaseCallback = std::function<void(EStormPhase, int32_t)>;
    using PlayerEnteredStormCallback = std::function<void(AFortPlayerControllerAthena*)>;
    using PlayerExitedStormCallback = std::function<void(AFortPlayerControllerAthena*)>;
    using StormDamageCallback = std::function<void(AFortPlayerControllerAthena*, float)>;
    
    void RegisterStormPhaseCallback(const std::string& Name, StormPhaseCallback Callback);
    void RegisterPlayerEnteredStormCallback(const std::string& Name, PlayerEnteredStormCallback Callback);
    void RegisterPlayerExitedStormCallback(const std::string& Name, PlayerExitedStormCallback Callback);
    void RegisterStormDamageCallback(const std::string& Name, StormDamageCallback Callback);
    void UnregisterCallback(const std::string& Name);
    
    // Configuration
    struct Config {
        bool bEnabled = true;
        bool bDamageInStorm = true;
        bool bShowWarnings = true;
        float DamageTickRate = 1.0f;        // How often to damage players (seconds)
        float WarningTime = 30.0f;          // Warn players X seconds before shrink
        bool bAutoStart = true;             // Start storm automatically when match begins
        float PreStormDelay = 60.0f;        // Delay before first storm phase
        bool bVisualEffects = true;         // Enable visual effects
        float MaxStormDamage = 10.0f;       // Cap on storm damage per tick
        bool bScaleDamageWithPhase = true;  // Increase damage each phase
    } Settings;
    
    // System management
    void Update(float DeltaTime);
    void Reset();
    bool IsStormActive() const;
    bool IsStormPaused() const;
    
    // Statistics and monitoring
    struct StormStats {
        uint32_t TotalPlayersKilledByStorm = 0;
        uint32_t CurrentPlayersInStorm = 0;
        float TotalStormDamageDealt = 0.0f;
        std::chrono::steady_clock::time_point StormStartTime;
        float CurrentStormDuration = 0.0f;
    } Stats;
    
    const StormStats& GetStormStats() const { return Stats; }
    void ResetStormStats();
    
    // Advanced features
    void CreateMovingStorm(const FVector& StartCenter, const FVector& EndCenter, float Duration);
    void CreatePredictedSafezone(int32_t PhasesAhead = 1);
    void ShowSafezonePreview(AFortPlayerControllerAthena* Player, int32_t PhaseIndex);
    
    // Utilities
    FVector GetOptimalPositionInSafezone(const FVector& CurrentPosition) const;
    float GetTimeToReachSafezone(const FVector& StartPosition, float MovementSpeed = 600.0f) const;
    bool CanPlayerReachSafezone(AFortPlayerControllerAthena* Player) const;
    
    // Debugging
    void DumpStormInfo() const;
    void GenerateStormReport(const std::string& FilePath = "") const;
    void DebugDrawSafezone(bool bEnabled = true);
    
private:
    SafezoneManager() = default;
    ~SafezoneManager() = default;
    
    SafezoneManager(const SafezoneManager&) = delete;
    SafezoneManager& operator=(const SafezoneManager&) = delete;
    
    // Core data
    FSafezoneInfo SafezoneInfo;
    std::vector<FStormPhaseData> StormPhases;
    
    // State management
    bool bStormActive = false;
    bool bStormPaused = false;
    std::chrono::steady_clock::time_point LastUpdateTime;
    std::chrono::steady_clock::time_point LastDamageTime;
    
    // Player tracking
    std::unordered_set<AFortPlayerControllerAthena*> PlayersInStorm;
    std::unordered_set<AFortPlayerControllerAthena*> PlayersWarnedAboutStorm;
    
    // Callbacks
    std::unordered_map<std::string, StormPhaseCallback> PhaseCallbacks;
    std::unordered_map<std::string, PlayerEnteredStormCallback> EnteredStormCallbacks;
    std::unordered_map<std::string, PlayerExitedStormCallback> ExitedStormCallbacks;
    std::unordered_map<std::string, StormDamageCallback> DamageCallbacks;
    
    // Thread safety
    mutable std::mutex SafezoneMutex;
    
    // Internal helpers
    void UpdateStormPhase(float DeltaTime);
    void ProcessShrinking(float DeltaTime);
    void ProcessMoving(float DeltaTime);
    void StartNextPhase();
    void UpdatePlayerStormStatus();
    
    void FireStormPhaseCallbacks(EStormPhase Phase, int32_t PhaseIndex);
    void FirePlayerEnteredStormCallbacks(AFortPlayerControllerAthena* Player);
    void FirePlayerExitedStormCallbacks(AFortPlayerControllerAthena* Player);
    void FireStormDamageCallbacks(AFortPlayerControllerAthena* Player, float Damage);
    
    // Default configurations
    void InitializeDefaultPhases();
    FVector CalculateOptimalStormCenter(int32_t PhaseIndex);
    
    // Interpolation helpers
    FVector LerpVector(const FVector& A, const FVector& B, float Alpha);
    float LerpFloat(float A, float B, float Alpha);
};