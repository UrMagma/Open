#include "Hooks.h"
#include "FortniteClasses.h"

// Hook function declarations
namespace Hooks {
    
    // Original function pointers
    decltype(Native::LocalPlayer::SpawnPlayActor) Native::LocalPlayer::SpawnPlayActorOriginal = nullptr;
    decltype(Native::NetDriver::TickFlush) Native::NetDriver::TickFlushOriginal = nullptr;
    decltype(Native::PlayerController::GetPlayerViewPoint) Native::PlayerController::GetPlayerViewPointOriginal = nullptr;
    decltype(Native::World::WelcomePlayer) Native::World::WelcomePlayerOriginal = nullptr;
    decltype(Native::World::NotifyControlMessage) Native::World::NotifyControlMessageOriginal = nullptr;
    decltype(Native::World::SpawnPlayActor) Native::World::SpawnPlayActorOriginal = nullptr;
    decltype(Native::OnlineBeaconHost::NotifyControlMessage) Native::OnlineBeaconHost::NotifyControlMessageOriginal = nullptr;
    decltype(Native::OnlineSession::KickPlayer) Native::OnlineSession::KickPlayerOriginal = nullptr;
    decltype(Native::GameViewportClient::PostRender) Native::GameViewportClient::PostRenderOriginal = nullptr;
    decltype(Native::GC::CollectGarbage) Native::GC::CollectGarbageOriginal = nullptr;
    decltype(Native::Actor::GetNetMode) Native::Actor::GetNetModeOriginal = nullptr;
    void* (*ProcessEventOriginal)(UObject* Object, UFunction* Function, void* Params) = nullptr;
    void* (*NetDebugOriginal)(UObject* Object) = nullptr;
    
    // Hook implementations
    bool LocalPlayerSpawnPlayActorHook(class ULocalPlayer* Player, const FString& URL, FString& OutError, UWorld* World) {
        if (!bTraveled) {
            return Native::LocalPlayer::SpawnPlayActorOriginal(Player, URL, OutError, World);
        }
        return true;
    }
    
    void TickFlushHook(class UNetDriver* NetDriver, float DeltaSeconds) {
        if (!NetDriver) {
            return;
        }
        
        // Check if this is the IpNetDriver with clients connected
        if (NetDriver->IsA(FindObject<UClass>("IpNetDriver")) && 
            NetDriver->GetProperty<TArray<class UNetConnection*>>("ClientConnections")->Num() > 0) {
            
            auto& ClientConnections = *NetDriver->GetProperty<TArray<class UNetConnection*>>("ClientConnections");
            if (ClientConnections.Num() > 0 && !ClientConnections[0]->Get<bool>("InternalAck")) {
                // Handle replication
                auto ReplicationDriver = NetDriver->GetProperty<class UReplicationDriver*>("ReplicationDriver");
                if (ReplicationDriver && Native::ReplicationDriver::ServerReplicateActors) {
                    Native::ReplicationDriver::ServerReplicateActors(*ReplicationDriver);
                }
            }
        }
        
        Native::NetDriver::TickFlushOriginal(NetDriver, DeltaSeconds);
    }
    
    void GetPlayerViewPointHook(APlayerController* PC, FVector* OutLocation, FRotator* OutRotation) {
        // Custom view point logic for server
        if (PC && OutLocation && OutRotation) {
            // Get the view target
            auto ViewTarget = PC->GetViewTarget();
            if (ViewTarget) {
                *OutLocation = ViewTarget->K2_GetActorLocation();
                *OutRotation = ViewTarget->K2_GetActorRotation();
                return;
            }
        }
        
        Native::PlayerController::GetPlayerViewPointOriginal(PC, OutLocation, OutRotation);
    }
    
    void WelcomePlayerHook(UWorld* World, class UNetConnection* Connection) {
        Native::World::WelcomePlayerOriginal(GetWorld(), Connection);
    }
    
    void World_NotifyControlMessageHook(UWorld* World, class UNetConnection* Connection, uint8_t MessageType, void* Bunch) {
        Native::World::NotifyControlMessageOriginal(GetWorld(), Connection, MessageType, Bunch);
    }
    
    APlayerController* SpawnPlayActorHook(UWorld* World, class UPlayer* NewPlayer, ENetRole RemoteRole, 
                                         void* URL, void* UniqueId, FString& Error, uint8_t NetPlayerIndex) {
        
        auto PlayerController = static_cast<AFortPlayerControllerAthena*>(
            Native::World::SpawnPlayActorOriginal(GetWorld(), NewPlayer, RemoteRole, URL, UniqueId, Error, NetPlayerIndex));
        
        if (PlayerController) {
            NewPlayer->PlayerController = PlayerController;
            
            // Initialize the player through the game mode
            if (Game::Mode) {
                Game::Mode->LoadJoiningPlayer(PlayerController);
            }
            
            // Set up player properties
            PlayerController->OverriddenBackpackSize = 100;
            
            LOG_INFO("Player spawned: {}", PlayerController->PlayerState ? 
                    PlayerController->PlayerState->GetPlayerName().ToString() : "Unknown");
        }
        
        return PlayerController;
    }
    
    void Beacon_NotifyControlMessageHook(class AOnlineBeaconHost* Beacon, class UNetConnection* Connection, 
                                        uint8_t MessageType, int64_t* Bunch) {
        switch (MessageType) {
        case 4: // NMT_Netspeed
            Connection->CurrentNetSpeed = 30000;
            return;
            
        case 5: // NMT_Login
        {
            if (GetWorld()->GameState->HasMatchStarted()) {
                return;
            }
            
            // Increase bunch size for login
            Bunch[7] += (16 * 1024 * 1024);
            
            FString OnlinePlatformName;
            
            // Read login data
            Native::NetConnection::ReceiveFString(Bunch, Connection->ClientResponse);
            Native::NetConnection::ReceiveFString(Bunch, Connection->RequestURL);
            Native::NetConnection::ReceiveUniqueIdRepl(Bunch, &Connection->PlayerID);
            Native::NetConnection::ReceiveFString(Bunch, OnlinePlatformName);
            
            // Restore bunch size
            Bunch[7] -= (16 * 1024 * 1024);
            
            // Welcome the player
            Native::World::WelcomePlayer(GetWorld(), Connection);
            
            return;
        }
        case 15: // NMT_PCSwap
            break;
        }
        
        Native::World::NotifyControlMessage(GetWorld(), Connection, MessageType, Bunch);
    }
    
    uint8_t KickPlayerHook(class AGameSession* GameSession, APlayerController* PC, FString Reason) {
        // Don't actually kick players in our server
        return 0;
    }
    
    void PostRenderHook(class UGameViewportClient* GameViewport, class UCanvas* Canvas) {
        // GUI rendering would go here
        return Native::GameViewportClient::PostRenderOriginal(GameViewport, Canvas);
    }
    
    int64_t CollectGarbageHook(int64_t Flags) {
        // Disable garbage collection during matches
        return 0;
    }
    
    int64_t GetNetModeHook(int64_t* Actor) {
        // Always return ListenServer mode
        return static_cast<int64_t>(ENetMode::NM_ListenServer);
    }
    
    // Special hooks
    void* ProcessEventHook(UObject* Object, UFunction* Function, void* Params) {
        if (!bPlayButton) {
            // Look for play button function
            static auto PlayButtonFn = UObject::FindObject<UFunction>("BndEvt__BP_PlayButton_K2Node_ComponentBoundEvent_1_CommonButtonClicked__DelegateSignature");
            if (Function == PlayButtonFn) {
                bPlayButton = true;
                LOG_INFO("Play button pressed! Initializing game server...");
                
                // Start the game
                Game::Start();
                
                // Initialize network hooks
                LOG_INFO("Initializing network hooks...");
                InitializeNetworkHooks();
            }
        }
        
        if (bTraveled) {
            // Log RPCs if needed
            #ifdef LOGGING_RPC
            auto FunctionName = Function->GetName();
            if (Function->FunctionFlags & 0x00200000 || 
                (Function->FunctionFlags & 0x01000000 && 
                 FunctionName.find("Ack") == std::string::npos && 
                 FunctionName.find("AdjustPos") == std::string::npos)) {
                
                if (FunctionName.find("ServerUpdateCamera") == std::string::npos && 
                    FunctionName.find("ServerMove") == std::string::npos) {
                    LOG_DEBUG("RPC Called: {}", FunctionName);
                }
            }
            #endif
        }
        
        return ProcessEventOriginal(Object, Function, Params);
    }
    
    void* NetDebugHook(UObject* Object) {
        // Disable net debug
        return nullptr;
    }
    
    // Utility hooks
    uint8_t Beacon_NotifyAcceptingConnectionHook(class AOnlineBeacon* Beacon) {
        return Native::World::NotifyAcceptingConnection(GetWorld());
    }
    
    void* SeamlessTravelHandlerForWorldHook(class UEngine* Engine, UWorld* World) {
        return Native::Engine::SeamlessTravelHandlerForWorld(Engine, GetWorld());
    }
    
    uint8_t World_NotifyAcceptingConnectionHook(UWorld* World) {
        return Native::World::NotifyAcceptingConnection(GetWorld());
    }
    
    // Hook initialization functions
    bool InitializeBasicHooks() {
        LOG_INFO("Initializing basic hooks...");
        
        DETOUR_START
        
        // Core hooks that are needed immediately
        DetourAttachE(Native::LocalPlayer::SpawnPlayActor, LocalPlayerSpawnPlayActorHook);
        DetourAttachE(Native::NetDriver::TickFlush, TickFlushHook);
        DetourAttachE(ProcessEvent, ProcessEventHook);
        DetourAttachE(Native::PlayerController::GetPlayerViewPoint, GetPlayerViewPointHook);
        
        // Find and hook NetDebug
        auto NetDebugAddress = Utils::FindPattern(Patterns::NetDebug);
        if (NetDebugAddress) {
            DetourAttachE(reinterpret_cast<void*>(NetDebugAddress), NetDebugHook);
        }
        
        DETOUR_END
        
        LOG_INFO("Basic hooks initialized successfully");
        return true;
    }
    
    bool InitializeNetworkHooks() {
        LOG_INFO("Initializing network hooks...");
        
        DETOUR_START
        
        // Network hooks for player management
        DetourAttachE(Native::World::WelcomePlayer, WelcomePlayerHook);
        DetourAttachE(Native::Actor::GetNetMode, GetNetModeHook);
        DetourAttachE(Native::World::NotifyControlMessage, World_NotifyControlMessageHook);
        DetourAttachE(Native::World::SpawnPlayActor, SpawnPlayActorHook);
        DetourAttachE(Native::OnlineBeaconHost::NotifyControlMessage, Beacon_NotifyControlMessageHook);
        DetourAttachE(Native::OnlineSession::KickPlayer, KickPlayerHook);
        DetourAttachE(Native::GameViewportClient::PostRender, PostRenderHook);
        DetourAttachE(Native::GC::CollectGarbage, CollectGarbageHook);
        
        DETOUR_END
        
        LOG_INFO("Network hooks initialized successfully");
        return true;
    }
    
    bool InitializeAll() {
        return InitializeBasicHooks();
    }
}
