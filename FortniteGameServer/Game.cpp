#include "Game.h"
#include "GameModes/GameModeSolos.h"

// Game state tracking
bool bStartedBus = false;

namespace Game {
    
    std::unique_ptr<CurrentGameMode> Mode;
    
    void Start() {
        LOG_INFO("Starting Fortnite game server...");
        
        // Switch to Athena level - this is the core of making it work like PR3/Raider
        auto PlayerController = GetPlayerController();
        if (PlayerController) {
            PlayerController->SwitchLevel(L"Athena_Terrain?game=/Game/Athena/Athena_GameMode.Athena_GameMode_C");
            bTraveled = true;
            LOG_INFO("Switched to Athena level successfully");
        } else {
            LOG_ERROR("Failed to get player controller for level switch");
        }
    }
    
    void OnReadyToStartMatch() {
        LOG_INFO("Initializing match for the server!");
        
        auto World = GetWorld();
        if (!World) {
            LOG_ERROR("Failed to get world for match initialization");
            return;
        }
        
        auto GameState = static_cast<AAthena_GameState_C*>(World->GameState);
        auto GameMode = static_cast<AFortGameModeAthena*>(World->AuthorityGameMode);
        
        if (!GameState || !GameMode) {
            LOG_ERROR("Failed to get GameState or GameMode");
            return;
        }
        
        // Set up game state - following PR3 pattern exactly
        GameState->bGameModeWillSkipAircraft = true;
        GameState->AircraftStartTime = 9999.9f;
        GameState->WarmupCountdownEndTime = 99999.9f;
        
        GameState->GamePhase = EAthenaGamePhase::Warmup;
        GameState->OnRep_GamePhase(EAthenaGamePhase::None);
        
        // Configure game mode
        GameMode->bDisableGCOnServerDuringMatch = true;
        GameMode->bAllowSpectateAfterDeath = true;
        GameMode->bEnableReplicationGraph = true;
        
        // Set match state to InProgress
        auto InProgress = UFortKismetLibrary::STATIC_Conv_StringToName(FString(L"InProgress"));
        GameMode->MatchState = InProgress;
        GameMode->K2_OnSetMatchState(InProgress);
        
        // Create game mode instance
        Mode = std::make_unique<CurrentGameMode>();
        
        GameMode->MinRespawnDelay = 5.0f;
        GameMode->StartPlay();
        
        GameState->bReplicatedHasBegunPlay = true;
        GameState->OnRep_ReplicatedHasBegunPlay();
        GameMode->StartMatch();
        
        LOG_INFO("Match initialized successfully!");
    }
    
    EDeathCause GetDeathCause(const FFortPlayerDeathReport& DeathReport) {
        static std::map<std::string, EDeathCause> DeathCauses = {
            { "weapon.ranged.shotgun", EDeathCause::Shotgun },
            { "weapon.ranged.assault", EDeathCause::Rifle },
            { "Gameplay.Damage.Environment.Falling", EDeathCause::FallDamage },
            { "weapon.ranged.sniper", EDeathCause::Sniper },
            { "Weapon.Ranged.SMG", EDeathCause::SMG },
            { "weapon.ranged.heavy.rocket_launcher", EDeathCause::RocketLauncher },
            { "weapon.ranged.heavy.grenade_launcher", EDeathCause::GrenadeLauncher },
            { "Weapon.ranged.heavy.grenade", EDeathCause::Grenade },
            { "Weapon.Ranged.Heavy.Minigun", EDeathCause::Minigun },
            { "Weapon.Ranged.Crossbow", EDeathCause::Bow },
            { "trap.floor", EDeathCause::Trap },
            { "weapon.ranged.pistol", EDeathCause::Pistol },
            { "Gameplay.Damage.OutsideSafeZone", EDeathCause::OutsideSafeZone },
            { "Weapon.Melee.Impact.Pickaxe", EDeathCause::Melee }
        };
        
        for (int i = 0; i < DeathReport.Tags.GameplayTags.Num(); i++) {
            auto TagName = DeathReport.Tags.GameplayTags[i].ToString();
            
            for (auto& [Key, Value] : DeathCauses) {
                if (TagName == Key) {
                    return Value;
                }
            }
        }
        
        return EDeathCause::Unspecified;
    }
    
    bool Initialize() {
        LOG_INFO("Initializing game systems...");
        
        Config::Initialize();
        Stats::ResetMatchStats();
        
        LOG_INFO("Game systems initialized");
        return true;
    }
    
    void Shutdown() {
        LOG_INFO("Shutting down game systems...");
        
        if (Mode) {
            Mode.reset();
        }
        
        LOG_INFO("Game systems shut down");
    }
    
    // Game configuration
    namespace Config {
        // Match settings
        float WarmupTime = 10.0f;
        float AircraftTime = 120.0f;
        float SafeZoneTime = 1500.0f;
        int32_t MaxPlayers = 100;
        
        // Gameplay settings
        bool bBuildingEnabled = true;
        bool bInfiniteAmmo = false;
        bool bInfiniteMaterials = false;
        bool bRespawnEnabled = false;
        
        // Storm settings
        float StormDamagePerSecond = 1.0f;
        int32_t NumStormPhases = 9;
        
        void Initialize() {
            LOG_INFO("Game configuration initialized");
            LOG_INFO("Max Players: {}", MaxPlayers);
            LOG_INFO("Building Enabled: {}", bBuildingEnabled);
            LOG_INFO("Respawn Enabled: {}", bRespawnEnabled);
        }
    }
    
    // Statistics tracking
    namespace Stats {
        MatchStats CurrentMatch;
        
        void ResetMatchStats() {
            CurrentMatch = MatchStats();
            CurrentMatch.MatchStartTime = FDateTime::Now();
        }
    }
    
    // Event system
    namespace Events {
        void BroadcastGamePhaseChange(EAthenaGamePhase NewPhase) {
            LOG_INFO("Broadcasting game phase change: {}", static_cast<int>(NewPhase));
            // Would send to all connected players
        }
    }
}
