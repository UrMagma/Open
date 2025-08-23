#pragma once

#include "ForwardDeclarations.h"
#include "Definitions.h"
#include <chrono>
#include <random>
#include <functional>
#include <mutex>

// Forward declarations
class AFortPlayerControllerAthena;
class ObjectManager;
class PlayerManager;
class InventoryManager;
class MatchManager;
enum class EMatchState : uint8_t;

/**
 * EventManager - Comprehensive game events system
 * 
 * Features:
 * - Loot spawning and management
 * - Supply drop system
 * - Special events and activities
 * - Dynamic event scheduling
 * - Event rewards and progression
 * - Custom event creation
 */

enum class EEventType : uint8_t {
    None = 0,
    LootSpawn = 1,           // Spawn loot items
    SupplyDrop = 2,          // Supply drop event
    SpecialLocation = 3,     // Special location activation
    PlayerChallenge = 4,     // Player-specific challenge
    GlobalEvent = 5,         // Server-wide event
    TimedEvent = 6,          // Time-based event
    EnvironmentalEvent = 7,  // Environmental changes
    Custom = 8               // Custom scripted event
};

enum class EEventPriority : uint8_t {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

enum class ELootRarity : uint8_t {
    Common = 0,     // Gray
    Uncommon = 1,   // Green
    Rare = 2,       // Blue  
    Epic = 3,       // Purple
    Legendary = 4,  // Gold
    Mythic = 5      // Orange/Red
};

struct FLootSpawn {
    FVector Location = FVector();
    std::vector<FString> LootTable;
    ELootRarity MinRarity = ELootRarity::Common;
    ELootRarity MaxRarity = ELootRarity::Legendary;
    int32_t ItemCount = 1;
    float SpawnRadius = 100.0f;
    bool bRespawnAfterLooted = false;
    float RespawnTime = 300.0f;
    
    FLootSpawn() = default;
    FLootSpawn(const FVector& loc, int32_t count = 1) 
        : Location(loc), ItemCount(count) {}
};

struct FSupplyDrop {
    uint32_t Id = 0;
    FVector SpawnLocation = FVector();
    FVector LandingLocation = FVector();
    float FallSpeed = 500.0f;
    float FallTime = 0.0f;
    
    std::vector<FString> LootTable;
    int32_t LootCount = 5;
    ELootRarity GuaranteedRarity = ELootRarity::Epic;
    
    bool bHasLanded = false;
    bool bIsLooted = false;
    std::chrono::steady_clock::time_point SpawnTime;
    AFortPlayerControllerAthena* FirstOpener = nullptr;
    
    FSupplyDrop() {
        SpawnTime = std::chrono::steady_clock::now();
    }
};

struct FGameEvent {
    uint32_t Id = 0;
    FString EventName;
    FString Description;
    EEventType Type = EEventType::None;
    EEventPriority Priority = EEventPriority::Normal;
    
    std::chrono::steady_clock::time_point StartTime;
    std::chrono::steady_clock::time_point EndTime;
    float Duration = 0.0f;
    bool bActive = false;
    bool bCompleted = false;
    
    // Event data
    FVector Location = FVector();
    float Radius = 1000.0f;
    std::unordered_map<FString, FString> Parameters;
    
    // Participants
    std::vector<AFortPlayerControllerAthena*> Participants;
    std::unordered_map<AFortPlayerControllerAthena*, int32_t> PlayerScores;
    
    // Rewards
    std::vector<FString> RewardItems;
    int32_t ExperienceReward = 0;
    
    FGameEvent() {
        StartTime = std::chrono::steady_clock::now();
    }
    
    bool IsActive() const {
        auto now = std::chrono::steady_clock::now();
        return bActive && now >= StartTime && (Duration <= 0 || now <= EndTime);
    }
    
    float GetTimeRemaining() const {
        if (Duration <= 0) return -1.0f;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<float>(now - StartTime).count();
        return std::max(0.0f, Duration - elapsed);
    }
};

struct FEventTemplate {
    FString Name;
    EEventType Type;
    float MinInterval = 60.0f;
    float MaxInterval = 300.0f;
    float Duration = 120.0f;
    EEventPriority Priority = EEventPriority::Normal;
    
    // Spawn conditions
    int32_t MinPlayers = 1;
    int32_t MaxPlayers = 100;
    bool bRequireMatchActive = true;
    std::vector<EMatchState> ValidMatchStates;
    
    // Custom setup function
    std::function<FGameEvent()> SetupFunction;
    std::function<void(FGameEvent&)> UpdateFunction;
    std::function<void(const FGameEvent&)> CompleteFunction;
    
    FEventTemplate() = default;
    FEventTemplate(const FString& name, EEventType type) 
        : Name(name), Type(type) {}
};

class EventManager {
public:
    static EventManager& Get();
    
    // Core event management
    uint32_t CreateEvent(const FGameEvent& Event);
    bool StartEvent(uint32_t EventId);
    bool EndEvent(uint32_t EventId, bool bCompleted = true);
    bool CancelEvent(uint32_t EventId);
    FGameEvent* GetEvent(uint32_t EventId);
    
    // Event scheduling
    void ScheduleEvent(const FEventTemplate& Template, float DelaySeconds = 0.0f);
    void ScheduleRecurringEvent(const FEventTemplate& Template, float IntervalSeconds);
    void CancelScheduledEvents(const FString& EventName = FString());
    
    // Loot system
    void SpawnLoot(const FLootSpawn& LootSpawn);
    void SpawnLootAtLocation(const FVector& Location, const std::vector<FString>& LootTable, 
                            int32_t Count = 1, ELootRarity MinRarity = ELootRarity::Common);
    void SpawnRandomLoot(const FVector& Location, int32_t Count = 1);
    void ClearLootAtLocation(const FVector& Location, float Radius = 500.0f);
    
    // Supply drop system
    uint32_t SpawnSupplyDrop(const FVector& SpawnLocation, const FVector& LandingLocation);
    uint32_t SpawnRandomSupplyDrop();
    void UpdateSupplyDrops(float DeltaTime);
    std::vector<FSupplyDrop*> GetActiveSupplyDrops();
    
    // Event templates
    void RegisterEventTemplate(const FEventTemplate& Template);
    void UnregisterEventTemplate(const FString& Name);
    FEventTemplate* GetEventTemplate(const FString& Name);
    std::vector<FString> GetAvailableEventTemplates() const;
    
    // Player participation
    void AddPlayerToEvent(uint32_t EventId, AFortPlayerControllerAthena* Player);
    void RemovePlayerFromEvent(uint32_t EventId, AFortPlayerControllerAthena* Player);
    bool IsPlayerInEvent(uint32_t EventId, AFortPlayerControllerAthena* Player) const;
    std::vector<FGameEvent*> GetPlayerEvents(AFortPlayerControllerAthena* Player);
    
    // Event queries
    std::vector<FGameEvent*> GetActiveEvents() const;
    std::vector<FGameEvent*> GetEventsInRadius(const FVector& Location, float Radius) const;
    std::vector<FGameEvent*> GetEventsByType(EEventType Type) const;
    uint32_t GetActiveEventCount() const;
    
    // Rewards and progression
    void GiveEventRewards(uint32_t EventId, AFortPlayerControllerAthena* Player);
    void SetEventScore(uint32_t EventId, AFortPlayerControllerAthena* Player, int32_t Score);
    int32_t GetEventScore(uint32_t EventId, AFortPlayerControllerAthena* Player) const;
    std::vector<std::pair<AFortPlayerControllerAthena*, int32_t>> GetEventLeaderboard(uint32_t EventId);
    
    // System management
    void Update(float DeltaTime);
    void CleanupExpiredEvents();
    void PauseAllEvents();
    void ResumeAllEvents();
    void ClearAllEvents();
    
    // Configuration
    struct Config {
        bool bEventsEnabled = true;
        float EventUpdateRate = 1.0f;
        uint32_t MaxConcurrentEvents = 10;
        float DefaultLootSpawnHeight = 100.0f;
        float SupplyDropHeight = 2000.0f;
        bool bAnnounceEvents = true;
        float EventCleanupInterval = 60.0f;
    } Settings;
    
    // Events and callbacks
    using EventStartedCallback = std::function<void(const FGameEvent&)>;
    using EventEndedCallback = std::function<void(const FGameEvent&, bool)>;
    using PlayerJoinedEventCallback = std::function<void(uint32_t, AFortPlayerControllerAthena*)>;
    using SupplyDropLandedCallback = std::function<void(const FSupplyDrop&)>;
    using LootSpawnedCallback = std::function<void(const FLootSpawn&)>;
    
    void RegisterEventStartedCallback(const std::string& Name, EventStartedCallback Callback);
    void RegisterEventEndedCallback(const std::string& Name, EventEndedCallback Callback);
    void RegisterPlayerJoinedEventCallback(const std::string& Name, PlayerJoinedEventCallback Callback);
    void RegisterSupplyDropLandedCallback(const std::string& Name, SupplyDropLandedCallback Callback);
    void RegisterLootSpawnedCallback(const std::string& Name, LootSpawnedCallback Callback);
    void UnregisterCallback(const std::string& Name);
    
    // Predefined events
    void InitializeDefaultEvents();
    FGameEvent CreateSupplyDropEvent(const FVector& Location);
    FGameEvent CreateLootGoblinEvent(const FVector& Location);
    FGameEvent CreateKingOfTheHillEvent(const FVector& Location);
    FGameEvent CreateTreasureHuntEvent();
    
    // Utilities
    FVector GetRandomMapLocation() const;
    FVector GetRandomSafeLocation() const;
    bool IsLocationSafe(const FVector& Location) const;
    void BroadcastEventNotification(const FGameEvent& Event);
    
    // Debugging
    void DumpEventInfo() const;
    void GenerateEventReport(const std::string& FilePath = "") const;
    void DebugSpawnTestLoot();
    void DebugSpawnTestSupplyDrop();
    
private:
    EventManager() = default;
    ~EventManager() = default;
    
    EventManager(const EventManager&) = delete;
    EventManager& operator=(const EventManager&) = delete;
    
    // Data structures
    std::unordered_map<uint32_t, FGameEvent> ActiveEvents;
    std::unordered_map<FString, FEventTemplate> EventTemplates;
    std::unordered_map<uint32_t, FSupplyDrop> SupplyDrops;
    std::vector<FLootSpawn> ActiveLootSpawns;
    
    // Scheduling
    struct FScheduledEvent {
        FEventTemplate Template;
        std::chrono::steady_clock::time_point TriggerTime;
        bool bRecurring = false;
        float Interval = 0.0f;
    };
    std::vector<FScheduledEvent> ScheduledEvents;
    
    // ID generation
    uint32_t NextEventId = 1;
    uint32_t NextSupplyDropId = 1;
    
    // Timing
    std::chrono::steady_clock::time_point LastUpdate;
    std::chrono::steady_clock::time_point LastCleanup;
    
    // Random generation
    std::random_device RandomDevice;
    std::mt19937 RandomGenerator;
    
    // Callbacks
    std::unordered_map<std::string, EventStartedCallback> StartedCallbacks;
    std::unordered_map<std::string, EventEndedCallback> EndedCallbacks;
    std::unordered_map<std::string, PlayerJoinedEventCallback> JoinedEventCallbacks;
    std::unordered_map<std::string, SupplyDropLandedCallback> SupplyDropCallbacks;
    std::unordered_map<std::string, LootSpawnedCallback> LootSpawnCallbacks;
    
    // Thread safety
    mutable std::mutex EventMutex;
    
    // Internal helpers
    uint32_t GenerateEventId();
    uint32_t GenerateSupplyDropId();
    void ProcessScheduledEvents();
    void UpdateActiveEvents(float DeltaTime);
    void CheckEventConditions();
    
    // Loot generation
    std::vector<FString> GenerateRandomLoot(int32_t Count, ELootRarity MinRarity, ELootRarity MaxRarity);
    ELootRarity RollLootRarity() const;
    FString SelectRandomItem(const std::vector<FString>& LootTable) const;
    
    // Location utilities
    bool IsValidEventLocation(const FVector& Location, float MinRadius = 500.0f) const;
    std::vector<FVector> GetPossibleEventLocations() const;
    
    // Callback firing
    void FireEventStartedCallbacks(const FGameEvent& Event);
    void FireEventEndedCallbacks(const FGameEvent& Event, bool bCompleted);
    void FirePlayerJoinedEventCallbacks(uint32_t EventId, AFortPlayerControllerAthena* Player);
    void FireSupplyDropLandedCallbacks(const FSupplyDrop& SupplyDrop);
    void FireLootSpawnedCallbacks(const FLootSpawn& LootSpawn);
};