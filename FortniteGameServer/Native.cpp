#include "Native.h"

// Global variables
uintptr_t Imagebase = 0;
bool bTraveled = false;
bool bPlayButton = false;

// Global function pointers
void* (*ProcessEvent)(UObject* Object, UFunction* Function, void* Params) = nullptr;
void* (*FMemory_Malloc)(size_t Size, uint32_t Alignment) = nullptr;
void* (*FMemory_Realloc)(void* Ptr, size_t NewSize, uint32_t Alignment) = nullptr;
void (*FMemory_Free)(void* Ptr) = nullptr;
void (*FNameToString)(FName* Name, FString* OutString) = nullptr;

// Native function pointers
namespace Native {
    
    // Actor namespace
    namespace Actor {
        int64_t (*GetNetMode)(int64_t* Actor) = nullptr;
    }
    
    // PlayerController namespace
    namespace PlayerController {
        void (*GetPlayerViewPoint)(APlayerController* PC, FVector* OutLocation, FRotator* OutRotation) = nullptr;
    }
    
    // LocalPlayer namespace
    namespace LocalPlayer {
        bool (*SpawnPlayActor)(class ULocalPlayer* Player, const FString& URL, FString& OutError, UWorld* World) = nullptr;
    }
    
    // Garbage Collection namespace
    namespace GC {
        int64_t (*CollectGarbage)(int64_t Flags) = nullptr;
    }
    
    // AbilitySystemComponent namespace
    namespace AbilitySystemComponent {
        void* (*GiveAbility)(class UAbilitySystemComponent* ASC, void* OutHandle, void* InSpec) = nullptr;
        bool (*InternalTryActivateAbility)(class UAbilitySystemComponent* ASC, void* Handle, 
                                         void* PredictionKey, UObject** OutInstancedAbility, 
                                         void* OnGameplayAbilityEndedDelegate, void* TriggerEventData) = nullptr;
        void (*MarkAbilitySpecDirty)(class UAbilitySystemComponent* ASC, void* Spec) = nullptr;
    }
    
    // NetDriver namespace
    namespace NetDriver {
        void (*TickFlush)(class UNetDriver* NetDriver, float DeltaSeconds) = nullptr;
        bool (*IsLevelInitializedForActor)(class UNetDriver* NetDriver, AActor* Actor, class UNetConnection* Connection) = nullptr;
        bool (*InitListen)(UObject* Driver, void* InNotify, void* LocalURL, bool bReuseAddressAndPort, FString& Error) = nullptr;
    }
    
    // ReplicationDriver namespace
    namespace ReplicationDriver {
        void (*ServerReplicateActors)(class UReplicationDriver* ReplicationDriver) = nullptr;
    }
    
    // NetConnection namespace
    namespace NetConnection {
        void (*ReceiveFString)(void* Bunch, FString& Str) = nullptr;
        void (*ReceiveUniqueIdRepl)(void* Bunch, void* UniqueId) = nullptr;
        FString (*LowLevelGetRemoteAddress)(class UNetConnection* Connection, bool bAppendPort) = nullptr;
    }
    
    // OnlineSession namespace
    namespace OnlineSession {
        uint8_t (*KickPlayer)(class AGameSession* GameSession, APlayerController* PC, FString Reason) = nullptr;
    }
    
    // OnlineBeacon namespace
    namespace OnlineBeacon {
        void (*PauseBeaconRequests)(class AOnlineBeacon* Beacon, bool bPause) = nullptr;
        uint8_t (*NotifyAcceptingConnection)(class AOnlineBeacon* Beacon) = nullptr;
    }
    
    // OnlineBeaconHost namespace
    namespace OnlineBeaconHost {
        bool (*InitHost)(class AOnlineBeaconHost* Beacon) = nullptr;
        void (*NotifyControlMessage)(class AOnlineBeaconHost* Beacon, class UNetConnection* Connection, 
                                  uint8_t MessageType, void* Bunch) = nullptr;
    }
    
    // World namespace
    namespace World {
        void (*RemoveNetworkActor)(UWorld* World, AActor* Actor) = nullptr;
        void (*WelcomePlayer)(UWorld* World, class UNetConnection* Connection) = nullptr;
        void (*NotifyControlMessage)(UWorld* World, class UNetConnection* Connection, uint8_t MessageType, void* Bunch) = nullptr;
        APlayerController* (*SpawnPlayActor)(UWorld* World, class UPlayer* NewPlayer, ENetRole RemoteRole, 
                                           void* URL, void* UniqueId, FString& Error, uint8_t NetPlayerIndex) = nullptr;
        uint8_t (*NotifyAcceptingConnection)(UWorld* World) = nullptr;
    }
    
    // Engine namespace
    namespace Engine {
        void* (*SeamlessTravelHandlerForWorld)(class UEngine* Engine, UWorld* World) = nullptr;
    }
    
    // GameViewportClient namespace
    namespace GameViewportClient {
        void (*PostRender)(class UGameViewportClient* GameViewport, class UCanvas* Canvas) = nullptr;
    }
    
    bool InitializeAll() {
        LOG_INFO("Initializing native functions...");
        
        Imagebase = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
        LOG_INFO("Base address: 0x{:X}", Imagebase);
        
        // Initialize core functions first
        FIND_PATTERN_RELATIVE(GObjects, Patterns::GObjects, 3);
        extern FUObjectArray* GObjects;
        GObjects = reinterpret_cast<FUObjectArray*>(GObjects_Address);
        
        if (!InitializeFunction("FMemory_Free", Patterns::FMemory_Free, FMemory_Free)) return false;
        if (!InitializeFunction("FMemory_Malloc", Patterns::FMemory_Malloc, FMemory_Malloc)) return false;
        if (!InitializeFunction("FMemory_Realloc", Patterns::FMemory_Realloc, FMemory_Realloc)) return false;
        if (!InitializeFunction("FNameToString", Patterns::FNameToString, FNameToString)) return false;
        
        // Initialize ProcessEvent from engine vtable
        auto Engine = GetEngine();
        if (Engine && Utils::IsValidReadPtr(Engine)) {
            ProcessEvent = reinterpret_cast<decltype(ProcessEvent)>(Engine->VTable[0x40]);
            LOG_INFO("ProcessEvent initialized from engine vtable: 0x{:X}", reinterpret_cast<uintptr_t>(ProcessEvent));
        } else {
            LOG_WARN("Could not get engine for ProcessEvent, will try pattern scan later");
        }
        
        // Initialize network functions
        if (!InitializeFunction("NetDriver::TickFlush", Patterns::NetDriver_TickFlush, NetDriver::TickFlush)) return false;
        if (!InitializeFunction("World::WelcomePlayer", Patterns::World_WelcomePlayer, World::WelcomePlayer)) return false;
        if (!InitializeFunction("World::SpawnPlayActor", Patterns::World_SpawnPlayActor, World::SpawnPlayActor)) return false;
        if (!InitializeFunction("World::NotifyControlMessage", Patterns::World_NotifyControlMessage, World::NotifyControlMessage)) return false;
        
        // Initialize beacon functions
        if (!InitializeFunction("OnlineBeaconHost::InitHost", Patterns::OnlineBeaconHost_InitHost, OnlineBeaconHost::InitHost)) return false;
        if (!InitializeFunction("OnlineBeaconHost::NotifyControlMessage", Patterns::OnlineBeaconHost_NotifyControlMessage, OnlineBeaconHost::NotifyControlMessage)) return false;
        if (!InitializeFunction("OnlineBeacon::PauseBeaconRequests", Patterns::OnlineBeacon_PauseBeaconRequests, OnlineBeacon::PauseBeaconRequests)) return false;
        
        // Initialize NetConnection functions
        if (!InitializeFunction("NetConnection::ReceiveFString", Patterns::NetConnection_ReceiveFString, NetConnection::ReceiveFString)) return false;
        if (!InitializeFunction("NetConnection::ReceiveUniqueIdRepl", Patterns::NetConnection_ReceiveUniqueIdRepl, NetConnection::ReceiveUniqueIdRepl)) return false;
        
        // Initialize PlayerController functions
        if (!InitializeFunction("PlayerController::GetPlayerViewPoint", Patterns::PlayerController_GetPlayerViewPoint, PlayerController::GetPlayerViewPoint)) return false;
        if (!InitializeFunction("LocalPlayer::SpawnPlayActor", Patterns::LocalPlayer_SpawnPlayActor, LocalPlayer::SpawnPlayActor)) return false;
        
        // Initialize other functions
        if (!InitializeFunction("GameSession::KickPlayer", Patterns::GameSession_KickPlayer, OnlineSession::KickPlayer)) return false;
        if (!InitializeFunction("Actor::GetNetMode", Patterns::Actor_GetNetMode, Actor::GetNetMode)) return false;
        if (!InitializeFunction("GC::CollectGarbage", Patterns::GC_CollectGarbage, GC::CollectGarbage, true, 1)) return false;
        if (!InitializeFunction("GameViewportClient::PostRender", Patterns::GameViewportClient_PostRender, GameViewportClient::PostRender)) return false;
        
        // Initialize ability system functions
        InitializeFunction("AbilitySystemComponent::GiveAbility", Patterns::AbilitySystemComponent_GiveAbility, AbilitySystemComponent::GiveAbility);
        InitializeFunction("AbilitySystemComponent::InternalTryActivateAbility", Patterns::AbilitySystemComponent_InternalTryActivateAbility, AbilitySystemComponent::InternalTryActivateAbility);
        InitializeFunction("AbilitySystemComponent::MarkAbilitySpecDirty", Patterns::AbilitySystemComponent_MarkAbilitySpecDirty, AbilitySystemComponent::MarkAbilitySpecDirty);
        
        // Initialize replication functions
        InitializeFunction("ReplicationDriver::ServerReplicateActors", Patterns::ReplicationDriver_ServerReplicateActors, ReplicationDriver::ServerReplicateActors);
        
        LOG_INFO("Native functions initialized successfully");
        return true;
    }
}
