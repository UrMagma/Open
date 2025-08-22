#pragma once

#include "Native.h"
#include "Game.h"
#include <MinHook.h>

// MinHook macros for cleaner detour management
#define DETOUR_START \
    if (MH_Initialize() != MH_OK) { \
        LOG_ERROR("Failed to initialize MinHook"); \
        return false; \
    }

#define DETOUR_END \
    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) { \
        LOG_ERROR("Failed to enable hooks"); \
        return false; \
    }

#define DetourAttachE(original, detour) \
    if (MH_CreateHook(reinterpret_cast<LPVOID>(original), reinterpret_cast<LPVOID>(detour), \
                     reinterpret_cast<LPVOID*>(&original##Original)) != MH_OK) { \
        LOG_ERROR("Failed to create hook for " #original); \
        return false; \
    }

namespace Hooks {
    
    // Hook declarations matching Native function signatures
    DECLARE_FUNCTION(Native::LocalPlayer::SpawnPlayActor);
    DECLARE_FUNCTION(Native::NetDriver::TickFlush);
    DECLARE_FUNCTION(Native::PlayerController::GetPlayerViewPoint);
    DECLARE_FUNCTION(Native::World::WelcomePlayer);
    DECLARE_FUNCTION(Native::World::NotifyControlMessage);
    DECLARE_FUNCTION(Native::World::SpawnPlayActor);
    DECLARE_FUNCTION(Native::OnlineBeaconHost::NotifyControlMessage);
    DECLARE_FUNCTION(Native::OnlineSession::KickPlayer);
    DECLARE_FUNCTION(Native::GameViewportClient::PostRender);
    DECLARE_FUNCTION(Native::GC::CollectGarbage);
    DECLARE_FUNCTION(Native::Actor::GetNetMode);
    
    // Special hooks
    extern void* (*ProcessEventOriginal)(UObject* Object, UFunction* Function, void* Params);
    extern void* (*NetDebugOriginal)(UObject* Object);
    
    // Hook implementations
    bool LocalPlayerSpawnPlayActorHook(class ULocalPlayer* Player, const FString& URL, FString& OutError, UWorld* World);
    
    void TickFlushHook(class UNetDriver* NetDriver, float DeltaSeconds);
    
    void GetPlayerViewPointHook(APlayerController* PC, FVector* OutLocation, FRotator* OutRotation);
    
    void WelcomePlayerHook(UWorld* World, class UNetConnection* Connection);
    
    void World_NotifyControlMessageHook(UWorld* World, class UNetConnection* Connection, uint8_t MessageType, void* Bunch);
    
    APlayerController* SpawnPlayActorHook(UWorld* World, class UPlayer* NewPlayer, ENetRole RemoteRole, 
                                         void* URL, void* UniqueId, FString& Error, uint8_t NetPlayerIndex);
    
    void Beacon_NotifyControlMessageHook(class AOnlineBeaconHost* Beacon, class UNetConnection* Connection, 
                                        uint8_t MessageType, int64_t* Bunch);
    
    uint8_t KickPlayerHook(class AGameSession* GameSession, APlayerController* PC, FString Reason);
    
    void PostRenderHook(class UGameViewportClient* GameViewport, class UCanvas* Canvas);
    
    int64_t CollectGarbageHook(int64_t Flags);
    
    int64_t GetNetModeHook(int64_t* Actor);
    
    // Special hooks
    void* ProcessEventHook(UObject* Object, UFunction* Function, void* Params);
    void* NetDebugHook(UObject* Object);
    
    // Utility hooks that return specific values
    uint8_t Beacon_NotifyAcceptingConnectionHook(class AOnlineBeacon* Beacon);
    void* SeamlessTravelHandlerForWorldHook(class UEngine* Engine, UWorld* World);
    uint8_t World_NotifyAcceptingConnectionHook(UWorld* World);
    
    // Hook initialization functions
    bool InitializeBasicHooks();
    bool InitializeNetworkHooks();
    
    // Main initialization function
    bool InitializeAll();
}
