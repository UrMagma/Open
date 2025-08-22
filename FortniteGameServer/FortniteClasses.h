#pragma once

#include "Engine.h"
#include "UObject.h"

// Forward declarations
class UFortAbilitySet;
class UFortPlaylistAthena;
class UFortItemDefinition;
class UFortWeaponItemDefinition;
class UFortHeroType;
class UCustomCharacterPart;
class UAbilitySystemComponent;
class UFortHealthSet;

// Enums matching Fortnite's actual enums
enum class EAthenaGamePhase : uint8_t {
    None = 0,
    Setup = 1,
    Warmup = 2,
    Aircraft = 3,
    SafeZone = 4,
    EndGame = 5,
    Count = 6
};

enum class EDeathCause : uint8_t {
    Unspecified = 0,
    Shotgun = 1,
    Rifle = 2,
    SMG = 3,
    Pistol = 4,
    Sniper = 5,
    Minigun = 6,
    RocketLauncher = 7,
    GrenadeLauncher = 8,
    Grenade = 9,
    Bow = 10,
    Trap = 11,
    Melee = 12,
    FallDamage = 13,
    OutsideSafeZone = 14,
    MAX = 15
};

enum class EFortCustomPartType : uint8_t {
    Head = 0,
    Body = 1,
    Hat = 2,
    Backpack = 3,
    Charm = 4,
    Face = 5,
    NumTypes = 6
};

enum class EFortQuickBars : uint8_t {
    Primary = 0,
    Secondary = 1,
    MAX = 2
};

enum class EMovementMode : uint8_t {
    MOVE_None = 0,
    MOVE_Walking = 1,
    MOVE_NavWalking = 2,
    MOVE_Falling = 3,
    MOVE_Swimming = 4,
    MOVE_Flying = 5,
    MOVE_Custom = 6,
    MOVE_MAX = 7
};

// Structs
struct FFortPlayerDeathReport {
    struct {
        TArray<struct FGameplayTag> GameplayTags;
    } Tags;
    
    AActor* DamageCauser;
    APlayerController* KillerPlayerController;
    float Distance;
    bool bWasDBNO;
    // More fields would be here in the real struct
};

struct FGameplayTag {
    FName TagName;
    
    std::string ToString() const {
        return TagName.ToString();
    }
};

struct FGameplayTagContainer {
    TArray<FGameplayTag> GameplayTags;
};

struct FPlayerStats {
    uint32_t Kills = 0;
    uint32_t Deaths = 0;
    uint32_t MatchesPlayed = 0;
    uint32_t Wins = 0;
    uint64_t DamageDealt = 0;
    uint64_t DamageTaken = 0;
    uint32_t StructuresBuilt = 0;
    uint32_t StructuresDestroyed = 0;
};

struct FMatchResult {
    uint32_t PlayerId;
    uint8_t Placement;
    uint32_t Kills;
    uint32_t DamageDealt;
    uint32_t StructuresBuilt;
    float SurvivalTime;
};

struct FDateTime {
    int64_t Ticks;
    
    static FDateTime Now() {
        FDateTime Result;
        Result.Ticks = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        return Result;
    }
};

// Fortnite-specific Actor classes
class AFortPlayerPawnAthena : public APawn {
public:
    UAbilitySystemComponent* AbilitySystemComponent;
    UFortHealthSet* HealthSet;
    class UCharacterMovementComponent* CharacterMovement;
    
    // Health and shields
    float CurrentShield;
    float MaxShield;
    
    // Set max shield
    virtual void SetMaxShield(float NewMaxShield) override {
        MaxShield = NewMaxShield;
        if (HealthSet) {
            // Set on health set as well
            static auto MaxShieldOffset = HealthSet->GetOffset("MaxShield");
            if (MaxShieldOffset != -1) {
                HealthSet->Get<float>(MaxShieldOffset) = NewMaxShield;
            }
        }
    }
    
    static UClass* StaticClass();
};

class AFortPlayerControllerAthena : public APlayerController {
public:
    AFortPlayerPawnAthena* Character;
    class AFortPlayerStateAthena* FortPlayerState;
    
    // Inventory and quickbars
    int32_t OverriddenBackpackSize;
    
    // Connection state
    uint8_t bHasClientFinishedLoading : 1;
    uint8_t bHasServerFinishedLoading : 1;
    uint8_t bHasInitiallySpawned : 1;
    
    // Quickbar management
    void ActivateSlot(EFortQuickBars QuickBarType, int32_t Slot, int32_t SecondarySlot, bool bShouldActivateItem);
    
    // Replication callbacks  
    void OnRep_bHasServerFinishedLoading();
    
    static UClass* StaticClass();
};

class AFortPlayerStateAthena : public class APlayerState {
public:
    UFortHeroType* HeroType;
    TArray<UCustomCharacterPart*> CharacterParts;
    
    // Player state flags
    uint8_t bHasFinishedLoading : 1;
    uint8_t bHasStartedPlaying : 1;
    
    // Replication callbacks
    void OnRep_HeroType();
    void OnRep_CharacterParts();
    void OnRep_bHasStartedPlaying();
    
    static UClass* StaticClass();
};

class AFortGameModeAthena : public AGameModeBase {
public:
    UFortPlaylistAthena* Playlist;
    
    // Match configuration
    uint8_t bDisableGCOnServerDuringMatch : 1;
    uint8_t bAllowSpectateAfterDeath : 1;
    uint8_t bEnableReplicationGraph : 1;
    
    // Ready to start match
    virtual void ReadyToStartMatch();
    
    static UClass* StaticClass();
};

class AAthena_GameState_C : public AGameStateBase {
public:
    UFortPlaylistAthena* CurrentPlaylistData;
    FName CurrentPlaylistId;
    EAthenaGamePhase GamePhase;
    
    // Aircraft settings
    uint8_t bGameModeWillSkipAircraft : 1;
    float AircraftStartTime;
    float WarmupCountdownEndTime;
    
    // Replication callbacks
    void OnRep_GamePhase(EAthenaGamePhase PreviousGamePhase);
    void OnRep_CurrentPlaylistId();
    void OnRep_CurrentPlaylistData();
    
    static UClass* StaticClass();
};

// Building system
class ABuildingActor : public AActor {
public:
    // Building properties
    uint8_t Team;
    uint8_t TeamIndex;
    
    uint8_t bIsPlayerBuildable : 1;
    uint8_t bDestroyed : 1;
    
    // Building functions
    void InitializeBuildingActor(APlayerController* Controller, ABuildingActor* BuildingOwner, 
                                bool bUsePlayerBuildAnimations, ABuildingActor* ReplacedBuilding = nullptr);
    
    bool IsDestroyed() const;
    void SilentDie();
    
    float GetMaxHealth();
    float GetHealthPercent();
    float GetHealth();
    
    void SetTeam(uint8_t InTeam);
    bool IsPlayerBuildable() const;
    
    // Damage handling
    virtual void OnDamageServer(float Damage, FGameplayTagContainer DamageTags, FVector Momentum, 
                               void* HitInfo, APlayerController* InstigatedBy, AActor* DamageCauser, void* EffectContext);
    
    static UClass* StaticClass();
};

// Utility classes
class UFortKismetLibrary : public UObject {
public:
    static FName STATIC_Conv_StringToName(const FString& InString);
    static UClass* StaticClass();
};

class UKismetSystemLibrary : public UObject {
public:
    static void STATIC_ExecuteConsoleCommand(UWorld* WorldContext, const FString& Command, APlayerController* Player);
    static UClass* StaticClass();
};

class UGameplayStatics : public UObject {
public:
    static void LoadStreamLevel(UWorld* WorldContext, FName LevelName, bool bMakeVisibleAfterLoad, 
                               bool bShouldBlockOnLoad, struct FLatentActionInfo LatentInfo);
    static void UnloadStreamLevel(UWorld* WorldContext, FName LevelName, struct FLatentActionInfo LatentInfo, bool bShouldBlockOnUnload);
    static UClass* StaticClass();
};

// Global accessors for Fortnite-specific objects
AFortGameModeAthena* GetFortGameMode();
AAthena_GameState_C* GetAthenaGameState();
AFortPlayerControllerAthena* GetFortPlayerController();

// Utility functions
UFortAbilitySet* GetPlayerAbilitySet();
UFortItemDefinition* FindWID(const std::string& WeaponName);
UCustomCharacterPart* FindCharacterPart(const std::string& PartName);

// Helper functions for object finding with Fortnite-specific patterns
template<typename T>
T* FindFortObject(const std::string& ObjectPath) {
    return UObject::FindObject<T>(ObjectPath);
}

template<typename T>
T* LoadFortObject(const std::string& ObjectPath) {
    return UObject::LoadObject<T>(ObjectPath);
}
