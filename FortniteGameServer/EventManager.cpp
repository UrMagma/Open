#include "EventManager.h"
#include "PlayerManager.h"
#include "SafezoneManager.h"
#include "UObject.h"
#include "Definitions.h"
#include <random>
#include <algorithm>

EventManager& EventManager::Get() {
    static EventManager Instance;
    return Instance;
}

void EventManager::Update(float DeltaTime) {
    if (!Settings.bEventsEnabled) return;
    
    std::lock_guard<std::mutex> Lock(EventMutex);
    
    auto now = std::chrono::steady_clock::now();
    
    // Process scheduled events
    ProcessScheduledEvents();
    
    // Update active events
    UpdateActiveEvents(DeltaTime);
    
    // Update supply drops
    UpdateSupplyDrops(DeltaTime);
    
    // Cleanup expired events periodically
    auto timeSinceCleanup = std::chrono::duration<float>(now - LastCleanup).count();
    if (timeSinceCleanup >= Settings.EventCleanupInterval) {
        CleanupExpiredEvents();
        LastCleanup = now;
    }
    
    LastUpdate = now;
}

void EventManager::InitializeDefaultEvents() {
    std::lock_guard<std::mutex> Lock(EventMutex);
    
    // Clear existing templates
    EventTemplates.clear();
    
    // Supply Drop Event Template
    FEventTemplate supplyDropTemplate;
    supplyDropTemplate.Name = FString("SupplyDrop");
    supplyDropTemplate.Type = EEventType::SupplyDrop;
    supplyDropTemplate.MinInterval = 180.0f;
    supplyDropTemplate.MaxInterval = 300.0f;
    supplyDropTemplate.Duration = 60.0f;
    supplyDropTemplate.Priority = EEventPriority::High;
    supplyDropTemplate.MinPlayers = 10;
    EventTemplates[supplyDropTemplate.Name] = supplyDropTemplate;
    
    // Random Loot Spawn Template
    FEventTemplate lootSpawnTemplate;
    lootSpawnTemplate.Name = FString("RandomLoot");
    lootSpawnTemplate.Type = EEventType::LootSpawn;
    lootSpawnTemplate.MinInterval = 60.0f;
    lootSpawnTemplate.MaxInterval = 120.0f;
    lootSpawnTemplate.Duration = 0.0f; // Instant
    lootSpawnTemplate.Priority = EEventPriority::Normal;
    lootSpawnTemplate.MinPlayers = 5;
    EventTemplates[lootSpawnTemplate.Name] = lootSpawnTemplate;
    
    // Schedule recurring events
    ScheduleRecurringEvent(supplyDropTemplate, 240.0f);
    ScheduleRecurringEvent(lootSpawnTemplate, 90.0f);
    
    LOG_INFO("Initialized default events: " + std::to_string(EventTemplates.size()) + " templates");
}

uint32_t EventManager::CreateEvent(const FGameEvent& Event) {
    std::lock_guard<std::mutex> Lock(EventMutex);
    
    uint32_t eventId = GenerateEventId();
    FGameEvent newEvent = Event;
    newEvent.Id = eventId;
    
    ActiveEvents[eventId] = newEvent;
    
    LOG_INFO("Created event: " + Event.EventName.ToString() + " (ID: " + std::to_string(eventId) + ")");
    
    return eventId;
}

bool EventManager::StartEvent(uint32_t EventId) {
    std::lock_guard<std::mutex> Lock(EventMutex);
    
    auto it = ActiveEvents.find(EventId);
    if (it == ActiveEvents.end()) return false;
    
    auto& event = it->second;
    event.bActive = true;
    event.StartTime = std::chrono::steady_clock::now();
    
    if (event.Duration > 0) {
        event.EndTime = event.StartTime + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::duration<float>(event.Duration));
    }
    
    FireEventStartedCallbacks(event);
    BroadcastEventNotification(event);
    
    LOG_INFO("Started event: " + event.EventName.ToString() + " (ID: " + std::to_string(EventId) + ")");
    
    return true;
}

bool EventManager::EndEvent(uint32_t EventId, bool bCompleted) {
    std::lock_guard<std::mutex> Lock(EventMutex);
    
    auto it = ActiveEvents.find(EventId);
    if (it == ActiveEvents.end()) return false;
    
    auto& event = it->second;
    event.bActive = false;
    event.bCompleted = bCompleted;
    
    FireEventEndedCallbacks(event, bCompleted);
    
    LOG_INFO("Ended event: " + event.EventName.ToString() + " (ID: " + std::to_string(EventId) + 
             ", Completed: " + (bCompleted ? "Yes" : "No") + ")");
    
    // Remove from active events
    ActiveEvents.erase(it);
    
    return true;
}

void EventManager::ScheduleEvent(const FEventTemplate& Template, float DelaySeconds) {
    std::lock_guard<std::mutex> Lock(EventMutex);
    
    FScheduledEvent scheduled;
    scheduled.Template = Template;
    scheduled.TriggerTime = std::chrono::steady_clock::now() + 
        std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::duration<float>(DelaySeconds));
    scheduled.bRecurring = false;
    
    ScheduledEvents.push_back(scheduled);
    
    LOG_INFO("Scheduled event: " + Template.Name.ToString() + " in " + std::to_string(DelaySeconds) + " seconds");
}

void EventManager::ScheduleRecurringEvent(const FEventTemplate& Template, float IntervalSeconds) {
    std::lock_guard<std::mutex> Lock(EventMutex);
    
    FScheduledEvent scheduled;
    scheduled.Template = Template;
    scheduled.TriggerTime = std::chrono::steady_clock::now() + 
        std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::duration<float>(IntervalSeconds));
    scheduled.bRecurring = true;
    scheduled.Interval = IntervalSeconds;
    
    ScheduledEvents.push_back(scheduled);
    
    LOG_INFO("Scheduled recurring event: " + Template.Name.ToString() + 
             " every " + std::to_string(IntervalSeconds) + " seconds");
}

uint32_t EventManager::SpawnSupplyDrop(const FVector& SpawnLocation, const FVector& LandingLocation) {
    std::lock_guard<std::mutex> Lock(EventMutex);
    
    FSupplyDrop supplyDrop;
    supplyDrop.Id = GenerateSupplyDropId();
    supplyDrop.SpawnLocation = SpawnLocation;
    supplyDrop.LandingLocation = LandingLocation;
    
    // Calculate fall time based on distance
    FVector diff = SpawnLocation - LandingLocation;
    float fallDistance = std::abs(diff.Z);
    supplyDrop.FallTime = fallDistance / supplyDrop.FallSpeed;
    
    // Generate loot
    supplyDrop.LootTable = GenerateRandomLoot(supplyDrop.LootCount, ELootRarity::Rare, ELootRarity::Legendary);
    
    SupplyDrops[supplyDrop.Id] = supplyDrop;
    
    LOG_INFO("Spawned supply drop (ID: " + std::to_string(supplyDrop.Id) + ") - " +
             "Fall time: " + std::to_string(supplyDrop.FallTime) + "s");
    
    return supplyDrop.Id;
}

uint32_t EventManager::SpawnRandomSupplyDrop() {
    FVector spawnLocation = GetRandomMapLocation();
    spawnLocation.Z = Settings.SupplyDropHeight;
    
    FVector landingLocation = GetRandomSafeLocation();
    
    return SpawnSupplyDrop(spawnLocation, landingLocation);
}

void EventManager::SpawnLootAtLocation(const FVector& Location, const std::vector<FString>& LootTable, 
                                      int32_t Count, ELootRarity MinRarity) {
    std::lock_guard<std::mutex> Lock(EventMutex);
    
    FLootSpawn lootSpawn;
    lootSpawn.Location = Location;
    lootSpawn.Location.Z += Settings.DefaultLootSpawnHeight;
    lootSpawn.LootTable = LootTable;
    lootSpawn.ItemCount = Count;
    lootSpawn.MinRarity = MinRarity;
    
    ActiveLootSpawns.push_back(lootSpawn);
    
    FireLootSpawnedCallbacks(lootSpawn);
    
    LOG_INFO("Spawned " + std::to_string(Count) + " loot items at (" + 
             std::to_string(Location.X) + ", " + std::to_string(Location.Y) + ", " + std::to_string(Location.Z) + ")");
}

void EventManager::SpawnRandomLoot(const FVector& Location, int32_t Count) {
    auto randomLoot = GenerateRandomLoot(Count, ELootRarity::Common, ELootRarity::Epic);
    SpawnLootAtLocation(Location, randomLoot, Count);
}

std::vector<FGameEvent*> EventManager::GetActiveEvents() const {
    std::lock_guard<std::mutex> Lock(EventMutex);
    
    std::vector<FGameEvent*> events;
    for (auto& pair : ActiveEvents) {
        if (pair.second.bActive) {
            events.push_back(const_cast<FGameEvent*>(&pair.second));
        }
    }
    
    return events;
}

uint32_t EventManager::GetActiveEventCount() const {
    return static_cast<uint32_t>(GetActiveEvents().size());
}

// Internal methods
void EventManager::ProcessScheduledEvents() {
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = ScheduledEvents.begin(); it != ScheduledEvents.end();) {
        if (now >= it->TriggerTime) {
            // Check if conditions are met
            auto& playerMgr = PlayerManager::Get();
            uint32_t playerCount = playerMgr.GetPlayerCount();
            
            if (playerCount >= it->Template.MinPlayers && playerCount <= it->Template.MaxPlayers) {
                // Create and start event based on template
                FGameEvent event;
                event.EventName = it->Template.Name;
                event.Type = it->Template.Type;
                event.Priority = it->Template.Priority;
                event.Duration = it->Template.Duration;
                event.Location = GetRandomSafeLocation();
                
                uint32_t eventId = CreateEvent(event);
                StartEvent(eventId);
            }
            
            // Handle recurring events
            if (it->bRecurring) {
                it->TriggerTime = now + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                    std::chrono::duration<float>(it->Interval));
                ++it;
            } else {
                it = ScheduledEvents.erase(it);
            }
        } else {
            ++it;
        }
    }
}

void EventManager::UpdateActiveEvents(float DeltaTime) {
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = ActiveEvents.begin(); it != ActiveEvents.end();) {
        auto& event = it->second;
        
        if (event.bActive && event.Duration > 0) {
            if (now >= event.EndTime) {
                EndEvent(event.Id, true);
                it = ActiveEvents.begin(); // Restart iteration since EndEvent modifies the map
                continue;
            }
        }
        
        ++it;
    }
}

void EventManager::UpdateSupplyDrops(float DeltaTime) {
    auto now = std::chrono::steady_clock::now();
    
    for (auto& pair : SupplyDrops) {
        auto& supplyDrop = pair.second;
        
        if (!supplyDrop.bHasLanded) {
            auto timeSinceSpawn = std::chrono::duration<float>(now - supplyDrop.SpawnTime).count();
            
            if (timeSinceSpawn >= supplyDrop.FallTime) {
                supplyDrop.bHasLanded = true;
                
                // Spawn loot at landing location
                SpawnLootAtLocation(supplyDrop.LandingLocation, supplyDrop.LootTable, supplyDrop.LootCount, supplyDrop.GuaranteedRarity);
                
                FireSupplyDropLandedCallbacks(supplyDrop);
                
                LOG_INFO("Supply drop " + std::to_string(supplyDrop.Id) + " landed");
            }
        }
    }
}

void EventManager::CleanupExpiredEvents() {
    // Remove old supply drops
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = SupplyDrops.begin(); it != SupplyDrops.end();) {
        auto timeSinceSpawn = std::chrono::duration<float>(now - it->second.SpawnTime).count();
        
        if (it->second.bHasLanded && timeSinceSpawn > 600.0f) { // 10 minutes
            it = SupplyDrops.erase(it);
        } else {
            ++it;
        }
    }
    
    // Clean up old loot spawns
    ActiveLootSpawns.erase(
        std::remove_if(ActiveLootSpawns.begin(), ActiveLootSpawns.end(),
            [](const FLootSpawn& spawn) {
                // Remove loot spawns after some time (simplified)
                return false; // Keep all for now
            }),
        ActiveLootSpawns.end());
}

uint32_t EventManager::GenerateEventId() {
    return NextEventId++;
}

uint32_t EventManager::GenerateSupplyDropId() {
    return NextSupplyDropId++;
}

std::vector<FString> EventManager::GenerateRandomLoot(int32_t Count, ELootRarity MinRarity, ELootRarity MaxRarity) {
    std::vector<FString> loot;
    
    std::vector<FString> lootPool = {
        FString("weapon_assault_rifle_common"),
        FString("weapon_shotgun_uncommon"),
        FString("weapon_sniper_rare"),
        FString("consumable_shield_potion"),
        FString("consumable_health_kit"),
        FString("ammo_light"),
        FString("ammo_medium"),
        FString("ammo_heavy")
    };
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, lootPool.size() - 1);
    
    for (int32_t i = 0; i < Count; ++i) {
        loot.push_back(lootPool[dis(gen)]);
    }
    
    return loot;
}

FVector EventManager::GetRandomMapLocation() const {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-25000.0f, 25000.0f);
    
    return FVector(dis(gen), dis(gen), 1000.0f);
}

FVector EventManager::GetRandomSafeLocation() const {
    // Get a location that's not in the storm
    auto& safezoneMgr = SafezoneManager::Get();
    auto safezoneInfo = safezoneMgr.GetSafezoneInfo();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> angleDis(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> radiusDis(0.0f, safezoneInfo.CurrentRadius * 0.8f);
    
    float angle = angleDis(gen);
    float radius = radiusDis(gen);
    
    FVector location;
    location.X = safezoneInfo.Center.X + radius * std::cos(angle);
    location.Y = safezoneInfo.Center.Y + radius * std::sin(angle);
    location.Z = 1000.0f;
    
    return location;
}

bool EventManager::IsLocationSafe(const FVector& Location) const {
    auto& safezoneMgr = SafezoneManager::Get();
    return safezoneMgr.GetSafezoneInfo().IsPlayerInSafezone(Location);
}

void EventManager::BroadcastEventNotification(const FGameEvent& Event) {
    if (!Settings.bAnnounceEvents) return;
    
    LOG_INFO("Event Started: " + Event.EventName.ToString() + " - " + Event.Description.ToString());
}

// Callback management
void EventManager::RegisterEventStartedCallback(const std::string& Name, EventStartedCallback Callback) {
    std::lock_guard<std::mutex> Lock(EventMutex);
    StartedCallbacks[Name] = Callback;
}

void EventManager::RegisterEventEndedCallback(const std::string& Name, EventEndedCallback Callback) {
    std::lock_guard<std::mutex> Lock(EventMutex);
    EndedCallbacks[Name] = Callback;
}

void EventManager::RegisterSupplyDropLandedCallback(const std::string& Name, SupplyDropLandedCallback Callback) {
    std::lock_guard<std::mutex> Lock(EventMutex);
    SupplyDropCallbacks[Name] = Callback;
}

void EventManager::RegisterLootSpawnedCallback(const std::string& Name, LootSpawnedCallback Callback) {
    std::lock_guard<std::mutex> Lock(EventMutex);
    LootSpawnCallbacks[Name] = Callback;
}

void EventManager::UnregisterCallback(const std::string& Name) {
    std::lock_guard<std::mutex> Lock(EventMutex);
    StartedCallbacks.erase(Name);
    EndedCallbacks.erase(Name);
    SupplyDropCallbacks.erase(Name);
    LootSpawnCallbacks.erase(Name);
}

void EventManager::FireEventStartedCallbacks(const FGameEvent& Event) {
    for (const auto& pair : StartedCallbacks) {
        try {
            pair.second(Event);
        } catch (...) {
            LOG_ERROR("Exception in EventStarted callback: " + pair.first);
        }
    }
}

void EventManager::FireEventEndedCallbacks(const FGameEvent& Event, bool bCompleted) {
    for (const auto& pair : EndedCallbacks) {
        try {
            pair.second(Event, bCompleted);
        } catch (...) {
            LOG_ERROR("Exception in EventEnded callback: " + pair.first);
        }
    }
}

void EventManager::FireSupplyDropLandedCallbacks(const FSupplyDrop& SupplyDrop) {
    for (const auto& pair : SupplyDropCallbacks) {
        try {
            pair.second(SupplyDrop);
        } catch (...) {
            LOG_ERROR("Exception in SupplyDropLanded callback: " + pair.first);
        }
    }
}

void EventManager::FireLootSpawnedCallbacks(const FLootSpawn& LootSpawn) {
    for (const auto& pair : LootSpawnCallbacks) {
        try {
            pair.second(LootSpawn);
        } catch (...) {
            LOG_ERROR("Exception in LootSpawned callback: " + pair.first);
        }
    }
}