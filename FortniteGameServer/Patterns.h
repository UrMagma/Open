#pragma once

#include <string>

namespace Patterns {
    
    // Core Engine Patterns
    inline const std::string GObjects = "48 8B 0D ? ? ? ? 48 98 4C 8B 04 D1 48 8D 0C 40 49 8D 04 C8";
    inline const std::string ProcessEvent = "40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8D 6C 24 ? 48 89 9D ? ? ? ?";
    inline const std::string FNameToString = "48 89 5C 24 08 57 48 83 EC ? 83 79 04 ? 48 8B DA";
    
    // Memory Management
    inline const std::string FMemory_Malloc = "48 89 5C 24 08 57 48 83 EC ? 48 8B F9 8B DA 48 8B 0D ? ? ? ? 48 85 C9";
    inline const std::string FMemory_Free = "48 85 C9 74 ? 53 48 83 EC ? 48 8B D9 48 8B 0D";
    inline const std::string FMemory_Realloc = "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC ? 48 8B F1 41 8B D8 48 8B 0D";
    
    // Networking Patterns
    inline const std::string NetDriver_TickFlush = "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 41 56 48 83 EC ? 0F 29 70 E8";
    inline const std::string NetDriver_InitListen = "48 89 5C 24 10 48 89 6C 24 18 48 89 74 24 20 57 48 83 EC ? 49 8B F0 4C 8B DA";
    inline const std::string NetDriver_IsLevelInitializedForActor = "40 53 48 83 EC ? 48 8B DA 48 85 D2 75 ? 32 C0";
    
    // World Management
    inline const std::string World_WelcomePlayer = "48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 4C 89 60 20 55 41 56 41 57 48 8D A8 ? ? ? ?";
    inline const std::string World_SpawnPlayActor = "48 8B C4 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ?";
    inline const std::string World_NotifyControlMessage = "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 54 41 55 41 56 41 57 48 83 EC ?";
    inline const std::string World_NotifyAcceptingConnection = "40 53 48 83 EC ? 48 8B D9 E8 ? ? ? ? 84 C0 74 ? 48 8B CB";
    
    // Beacon Patterns
    inline const std::string OnlineBeacon_PauseBeaconRequests = "40 53 48 83 EC ? 0F B6 DA 48 8B 91 ? ? ? ? 48 85 D2 74 ?";
    inline const std::string OnlineBeaconHost_InitHost = "40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ?";
    inline const std::string OnlineBeaconHost_NotifyControlMessage = "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 54 41 55 41 56 41 57 48 83 EC ?";
    
    // NetConnection Patterns
    inline const std::string NetConnection_ReceiveFString = "48 89 5C 24 08 57 48 83 EC ? 33 FF 48 8B DA 8B CF";
    inline const std::string NetConnection_ReceiveUniqueIdRepl = "48 89 5C 24 08 57 48 83 EC ? 48 8B FA 48 8B D9 E8 ? ? ? ?";
    inline const std::string NetConnection_LowLevelGetRemoteAddress = "48 89 5C 24 08 57 48 83 EC ? 48 8B F9 33 DB 84 D2 74 ?";
    
    // PlayerController Patterns
    inline const std::string PlayerController_GetPlayerViewPoint = "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC ? 48 8B F2 48 8B D9 48 8B 89 ? ? ? ?";
    inline const std::string LocalPlayer_SpawnPlayActor = "40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ?";
    
    // GameSession Patterns
    inline const std::string GameSession_KickPlayer = "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC ? 49 8B F8 4C 8B E2";
    
    // Actor Patterns
    inline const std::string Actor_GetNetMode = "48 85 C9 74 ? 48 8B 81 ? ? ? ? 48 85 C0 74 ? 8B 80 ? ? ? ?";
    
    // Ability System Patterns
    inline const std::string AbilitySystemComponent_GiveAbility = "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 41 56 48 83 EC ?";
    inline const std::string AbilitySystemComponent_InternalTryActivateAbility = "48 8B C4 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 ? ? ? ?";
    inline const std::string AbilitySystemComponent_MarkAbilitySpecDirty = "48 89 5C 24 08 57 48 83 EC ? 48 8B FA 48 8B D9 48 85 D2 0F 84 ? ? ? ?";
    
    // GameViewport Patterns
    inline const std::string GameViewportClient_PostRender = "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 56 41 57 48 83 EC ?";
    
    // Garbage Collection
    inline const std::string GC_CollectGarbage = "48 89 5C 24 10 48 89 74 24 18 55 57 41 56 48 8D AC 24 ? ? ? ?";
    
    // Engine Patterns
    inline const std::string Engine_SeamlessTravelHandlerForWorld = "48 89 5C 24 08 57 48 83 EC ? 48 8B FA 48 8B D9 48 85 D2 75 ?";
    
    // Debug Patterns
    inline const std::string NetDebug = "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC ? 33 ED";
    
    // Replication Patterns
    inline const std::string ReplicationDriver_ServerReplicateActors = "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 41 56 48 83 EC ?";
    
    // BuildingActor Patterns
    inline const std::string BuildingActor_OnDamageServer = "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 4C 89 60 20 55 41 56 41 57";
    
    // Season-specific patterns (these may need adjustment based on target season)
    namespace Season35 {
        // Patterns specific to Fortnite Season 3.5
        inline const std::string FortGameModeAthena_ReadyToStartMatch = "48 89 5C 24 08 57 48 83 EC ? 48 8B F9 E8 ? ? ? ? 48 8B 8F ? ? ? ?";
        inline const std::string FortPlayerControllerAthena_ServerReadyToStartMatch = "40 53 48 83 EC ? 48 8B D9 E8 ? ? ? ? 48 8B CB E8 ? ? ? ?";
        inline const std::string AthenaGameState_OnRep_GamePhase = "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC ? 48 8B F9 0F B6 F2";
    }
    
    // Version-agnostic patterns that should work across multiple seasons
    namespace Universal {
        inline const std::string SwitchLevel = "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC ? 33 ED";
        inline const std::string LoadStreamLevel = "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 41 56 48 83 EC ?";
        inline const std::string UnloadStreamLevel = "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 41 56 48 83 EC ?";
    }
}
