#pragma once

#include "../FortniteClasses.h"
#include "../Logic/Teams.h"
#include "../Logic/Inventory.h"
#include "../Logic/Abilities.h"

// Interface for game modes
class IGameModeBase {
public:
    virtual ~IGameModeBase() = default;
    virtual void OnPlayerJoined(AFortPlayerControllerAthena* Controller) = 0;
    virtual void OnPlayerKilled(AFortPlayerControllerAthena* Controller) = 0;
};

// Abstract base class for all game modes
class AbstractGameModeBase : protected IGameModeBase {
public:
    AbstractGameModeBase(const std::string& BasePlaylist, bool bRespawnEnabled = false, 
                        int32_t MaxTeamSize = 1, bool bRegenEnabled = false, bool bRejoinEnabled = false);
    
    virtual ~AbstractGameModeBase();
    
    // Game mode properties
    bool isRespawnEnabled() const { return m_bRespawnEnabled; }
    bool isRegenEnabled() const { return m_bRegenEnabled; }
    bool isRejoinEnabled() const { return m_bRejoinEnabled; }
    int32_t getMaxTeamSize() const { return m_MaxTeamSize; }
    
    // Player management
    void LoadJoiningPlayer(AFortPlayerControllerAthena* Controller);
    void LoadKilledPlayer(AFortPlayerControllerAthena* Controller, FVector SpawnLocation = FVector(500, 500, 500));
    
    // Abstract functions to be implemented by derived classes
    virtual void OnPlayerJoined(AFortPlayerControllerAthena* Controller) override = 0;
    virtual void OnPlayerKilled(AFortPlayerControllerAthena* Controller) override;
    
    // Loadout system
    virtual PlayerLoadout& GetPlaylistLoadout();
    
    // Player initialization
    void InitPawn(AFortPlayerControllerAthena* PlayerController, 
                 FVector Location = FVector(1250, 1818, 3284), 
                 FQuat Rotation = FQuat(), 
                 bool bResetCharacterParts = false);
    
    // Team management
    std::unique_ptr<PlayerTeams> GetTeams() const { return std::move(m_Teams); }
    
protected:
    // Game mode configuration
    UFortPlaylistAthena* m_BasePlaylist;
    bool m_bRespawnEnabled;
    bool m_bRegenEnabled;
    bool m_bRejoinEnabled;
    int32_t m_MaxTeamSize;
    
    // Health settings
    float m_MaxHealth = 100.0f;
    float m_MaxShield = 100.0f;
    
    // Team management
    std::unique_ptr<PlayerTeams> m_Teams;
    
    // Helper functions
    FTransform GetPlayerStart(AFortPlayerControllerAthena* Controller);
    void SetupPlayerCosmetics(AFortPlayerControllerAthena* Controller);
    void SetupPlayerAbilities(AFortPlayerPawnAthena* Pawn);
    void SetupPlayerInventory(AFortPlayerControllerAthena* Controller);
    
    // Spawn helpers
    template<typename T>
    T* SpawnActor(const FVector& Location, AFortPlayerControllerAthena* Owner = nullptr, const FRotator& Rotation = FRotator()) {
        return Spawners::SpawnActor<T>(Location, Owner, Rotation);
    }
};

// Spawner utility namespace
namespace Spawners {
    template<typename T>
    T* SpawnActor(const FVector& Location, AFortPlayerControllerAthena* Owner = nullptr, const FRotator& Rotation = FRotator()) {
        auto World = GetWorld();
        if (!World) return nullptr;
        
        // This would use UE4's SpawnActor functionality
        // For now, we'll create a placeholder implementation
        AActor* NewActor = nullptr;
        if (World->SpawnActor(T::StaticClass(), Location, Rotation, NewActor)) {
            if (Owner) {
                NewActor->Owner = Owner;
                NewActor->OnRep_Owner();
            }
            return static_cast<T*>(NewActor);
        }
        
        return nullptr;
    }
}
