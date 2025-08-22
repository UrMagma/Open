#pragma once

#include "Definitions.h"
#include "Patterns.h"
#include "PatternScanner.h"
#include "Logger.h"

// Global variables
extern uintptr_t Imagebase;
extern bool bTraveled;
extern bool bPlayButton;

// Global function pointers
extern void* (*ProcessEvent)(UObject* Object, UFunction* Function, void* Params);
extern void* (*FMemory_Malloc)(size_t Size, uint32_t Alignment);
extern void* (*FMemory_Realloc)(void* Ptr, size_t NewSize, uint32_t Alignment);
extern void (*FMemory_Free)(void* Ptr);
extern void (*FNameToString)(FName* Name, FString* OutString);

// Forward declarations for global accessors
inline UWorld* GetWorld();
inline UEngine* GetEngine();
inline APlayerController* GetPlayerController();

namespace Native {
    
    // Actor namespace
    namespace Actor {
        extern int64_t (*GetNetMode)(int64_t* Actor);
    }
    
    // PlayerController namespace
    namespace PlayerController {
        extern void (*GetPlayerViewPoint)(APlayerController* PC, FVector* OutLocation, FRotator* OutRotation);
    }
    
    // LocalPlayer namespace
    namespace LocalPlayer {
        extern bool (*SpawnPlayActor)(class ULocalPlayer* Player, const FString& URL, FString& OutError, UWorld* World);
    }
    
    // Garbage Collection namespace
    namespace GC {
        extern int64_t (*CollectGarbage)(int64_t Flags);
    }
    
    // AbilitySystemComponent namespace
    namespace AbilitySystemComponent {
        extern void* (*GiveAbility)(class UAbilitySystemComponent* ASC, void* OutHandle, void* InSpec);
        extern bool (*InternalTryActivateAbility)(class UAbilitySystemComponent* ASC, void* Handle, 
                                                 void* PredictionKey, UObject** OutInstancedAbility, 
                                                 void* OnGameplayAbilityEndedDelegate, void* TriggerEventData);
        extern void (*MarkAbilitySpecDirty)(class UAbilitySystemComponent* ASC, void* Spec);
    }
    
    // NetDriver namespace
    namespace NetDriver {
        extern void (*TickFlush)(class UNetDriver* NetDriver, float DeltaSeconds);
        extern bool (*IsLevelInitializedForActor)(class UNetDriver* NetDriver, AActor* Actor, class UNetConnection* Connection);
        extern bool (*InitListen)(UObject* Driver, void* InNotify, void* LocalURL, bool bReuseAddressAndPort, FString& Error);
    }
    
    // ReplicationDriver namespace
    namespace ReplicationDriver {
        extern void (*ServerReplicateActors)(class UReplicationDriver* ReplicationDriver);
    }
    
    // NetConnection namespace
    namespace NetConnection {
        extern void (*ReceiveFString)(void* Bunch, FString& Str);
        extern void (*ReceiveUniqueIdRepl)(void* Bunch, void* UniqueId);
        extern FString (*LowLevelGetRemoteAddress)(class UNetConnection* Connection, bool bAppendPort);
    }
    
    // OnlineSession namespace
    namespace OnlineSession {
        extern uint8_t (*KickPlayer)(class AGameSession* GameSession, APlayerController* PC, FString Reason);
    }
    
    // OnlineBeacon namespace
    namespace OnlineBeacon {
        extern void (*PauseBeaconRequests)(class AOnlineBeacon* Beacon, bool bPause);
        extern uint8_t (*NotifyAcceptingConnection)(class AOnlineBeacon* Beacon);
    }
    
    // OnlineBeaconHost namespace
    namespace OnlineBeaconHost {
        extern bool (*InitHost)(class AOnlineBeaconHost* Beacon);
        extern void (*NotifyControlMessage)(class AOnlineBeaconHost* Beacon, class UNetConnection* Connection, 
                                          uint8_t MessageType, void* Bunch);
    }
    
    // World namespace
    namespace World {
        extern void (*RemoveNetworkActor)(UWorld* World, AActor* Actor);
        extern void (*WelcomePlayer)(UWorld* World, class UNetConnection* Connection);
        extern void (*NotifyControlMessage)(UWorld* World, class UNetConnection* Connection, uint8_t MessageType, void* Bunch);
        extern APlayerController* (*SpawnPlayActor)(UWorld* World, class UPlayer* NewPlayer, ENetRole RemoteRole, 
                                                   void* URL, void* UniqueId, FString& Error, uint8_t NetPlayerIndex);
        extern uint8_t (*NotifyAcceptingConnection)(UWorld* World);
    }
    
    // Engine namespace
    namespace Engine {
        extern void* (*SeamlessTravelHandlerForWorld)(class UEngine* Engine, UWorld* World);
    }
    
    // GameViewportClient namespace
    namespace GameViewportClient {
        extern void (*PostRender)(class UGameViewportClient* GameViewport, class UCanvas* Canvas);
    }
    
    // Function to initialize all native function pointers
    bool InitializeAll();
    
    // Helper functions
    template<typename T>
    inline bool InitializeFunction(const std::string& PatternName, const std::string& Pattern, 
                                  T*& FunctionPtr, bool bRelative = false, int32_t Offset = 0) {
        if (!Utils::GetFunctionFromPattern(Pattern, FunctionPtr, bRelative, Offset)) {
            LOG_ERROR("Failed to initialize function: {}", PatternName);
            return false;
        }
        LOG_DEBUG("Initialized {}: 0x{:X}", PatternName, reinterpret_cast<uintptr_t>(FunctionPtr));
        return true;
    }
}

// Global accessor implementations
inline UEngine* GetEngine() {
    static UEngine* Engine = UObject::FindObject<UEngine>("FortEngine_");
    
    if (!Engine) {
        // Try to find with different indices
        for (int64_t i = 2147482000; i < 2147482000 + 1000; ++i) {
            Engine = UObject::FindObject<UEngine>("/Engine/Transient.FortEngine_" + std::to_string(i));
            if (Engine) break;
        }
    }
    
    return Engine;
}

inline UWorld* GetWorld() {
    auto Engine = GetEngine();
    if (!Engine) return nullptr;
    
    // Get GameViewport from Engine
    static auto GameViewportOffset = Engine->GetOffset("GameViewport");
    auto GameViewport = Engine->Get<UObject*>(GameViewportOffset);
    
    if (!GameViewport) return nullptr;
    
    // Get World from GameViewport
    static auto WorldOffset = GameViewport->GetOffset("World");
    return GameViewport->Get<UWorld*>(WorldOffset);
}

inline APlayerController* GetPlayerController() {
    auto Engine = GetEngine();
    if (!Engine) return nullptr;
    
    static auto GameInstanceOffset = Engine->GetOffset("GameInstance");
    auto GameInstance = Engine->Get<UObject*>(GameInstanceOffset);
    
    if (!GameInstance) return nullptr;
    
    static auto LocalPlayersOffset = GameInstance->GetOffset("LocalPlayers");
    auto& LocalPlayers = GameInstance->Get<TArray<UObject*>>(LocalPlayersOffset);
    
    if (LocalPlayers.Num() == 0) return nullptr;
    
    auto LocalPlayer = LocalPlayers[0];
    static auto PlayerControllerOffset = LocalPlayer->GetOffset("PlayerController");
    
    return LocalPlayer->Get<APlayerController*>(PlayerControllerOffset);
}
