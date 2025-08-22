#pragma once

#include "UObject.h"

// Forward declarations
class UActorComponent;
class USceneComponent;
class UNetConnection;
class UNetDriver;
class AGameModeBase;
class AGameStateBase;
class APlayerController;
class APawn;

// Actor base class - everything in the world inherits from this
class AActor : public UObject {
public:
    // Core Actor properties
    USceneComponent* RootComponent;
    TArray<UActorComponent*> OwnedComponents;
    TArray<AActor*> Children;
    AActor* Owner;
    TArray<FName> Tags;
    
    // Networking
    float NetCullDistanceSquared;
    int32_t NetTag;
    float NetUpdateFrequency;
    float MinNetUpdateFrequency;
    ENetRole Role;
    ENetRole RemoteRole;
    AActor* Instigator;
    
    // Replication
    uint8_t bReplicates : 1;
    uint8_t bReplicateMovement : 1;
    uint8_t bNetLoadOnClient : 1;
    uint8_t bNetUseOwnerRelevancy : 1;
    uint8_t bBlockInput : 1;
    
    // Transform
    virtual FVector GetActorLocation() const;\n    virtual FRotator GetActorRotation() const;\n    virtual FVector GetActorScale3D() const;\n    virtual FTransform GetActorTransform() const;\n    \n    virtual bool SetActorLocation(const FVector& NewLocation, bool bSweep = false, bool bTeleport = false);\n    virtual bool SetActorRotation(const FRotator& NewRotation, bool bTeleport = false);\n    virtual bool SetActorScale3D(const FVector& NewScale3D);\n    virtual bool SetActorTransform(const FTransform& NewTransform, bool bSweep = false, bool bTeleport = false);\n    \n    // K2 Blueprint callable versions\n    FVector K2_GetActorLocation() const { return GetActorLocation(); }\n    FRotator K2_GetActorRotation() const { return GetActorRotation(); }\n    bool K2_SetActorLocation(const FVector& NewLocation, bool bSweep, bool bTeleport) {\n        return SetActorLocation(NewLocation, bSweep, bTeleport);\n    }\n    bool K2_TeleportTo(const FVector& DestLocation, const FRotator& DestRotation) {\n        return SetActorLocation(DestLocation, false, true) && SetActorRotation(DestRotation, true);\n    }\n    \n    // Destruction\n    virtual void Destroy();\n    void K2_DestroyActor() { Destroy(); }\n    \n    // Networking\n    virtual void GetLifetimeReplicatedProps(TArray<void*>& OutLifetimeProps) const;\n    virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const;\n    \n    // Components\n    template<typename T>\n    T* GetComponentByClass() const;\n    \n    UActorComponent* GetComponentByClass(UClass* ComponentClass) const;\n    \n    // World access\n    UWorld* GetWorld() const;\n    \n    // Utility\n    void FlushNetDormancy();\n    void ForceNetUpdate();\n    \n    // Events (can be overridden)\n    virtual void BeginPlay() {}\n    virtual void EndPlay() {}\n    virtual void Tick(float DeltaTime) {}\n    \n    static UClass* StaticClass();\n};\n\n// Pawn - can be possessed by a controller\nclass APawn : public AActor {\npublic:\n    APlayerController* Controller;\n    \n    // Health system\n    float Health;\n    float MaxHealth;\n    uint8_t bCanBeDamaged : 1;\n    \n    // Possession\n    virtual void PossessedBy(APlayerController* NewController);\n    virtual void UnPossessed();\n    \n    // Health\n    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, \n                           APlayerController* EventInstigator, AActor* DamageCauser);\n    \n    virtual void SetMaxHealth(float NewMaxHealth) { MaxHealth = NewMaxHealth; }\n    virtual void SetMaxShield(float NewMaxShield) {} // Override in derived classes\n    \n    static UClass* StaticClass();\n};\n\n// Player Controller - handles input and player logic\nclass APlayerController : public AActor {\npublic:\n    APawn* Pawn;\n    APawn* AcknowledgedPawn;\n    class APlayerState* PlayerState;\n    class AHUD* MyHUD;\n    class UPlayer* Player;\n    class UCameraComponent* PlayerCameraManager;\n    \n    // Input\n    uint8_t bShowMouseCursor : 1;\n    uint8_t bEnableClickEvents : 1;\n    uint8_t bInputEnabled : 1;\n    \n    // Connection\n    uint8_t bIsDisconnecting : 1;\n    uint8_t bHasClientFinishedLoading : 1;\n    uint8_t bHasServerFinishedLoading : 1;\n    uint8_t bHasInitiallySpawned : 1;\n    \n    // Possession\n    virtual void Possess(APawn* InPawn);\n    virtual void UnPossess();\n    \n    // Replication callbacks\n    void OnRep_Pawn();\n    void OnRep_bHasServerFinishedLoading();\n    \n    // Networking\n    virtual void SwitchLevel(const FString& URL);\n    void SwitchLevel(const wchar_t* URL) {\n        FString URLString(URL);\n        SwitchLevel(URLString);\n    }\n    \n    // View target\n    virtual AActor* GetViewTarget() const;\n    virtual void SetViewTarget(AActor* NewViewTarget);\n    virtual void GetPlayerViewPoint(FVector& Location, FRotator& Rotation) const;\n    \n    // Respawn\n    virtual void RespawnPlayerAfterDeath();\n    \n    static UClass* StaticClass();\n};\n\n// Game Mode Base - manages game rules\nclass AGameModeBase : public AActor {\npublic:\n    TArray<APlayerController*> PlayerControllers;\n    AGameStateBase* GameState;\n    \n    // Match state\n    FName MatchState;\n    float MinRespawnDelay;\n    uint8_t bDisableGCOnServerDuringMatch : 1;\n    uint8_t bAllowSpectateAfterDeath : 1;\n    uint8_t bEnableReplicationGraph : 1;\n    \n    // Game flow\n    virtual void StartPlay();\n    virtual void StartMatch();\n    virtual void EndMatch();\n    virtual void ResetLevel();\n    \n    // Player management\n    virtual void PostLogin(APlayerController* NewPlayer);\n    virtual void Logout(APlayerController* Exiting);\n    \n    // Match state\n    virtual void K2_OnSetMatchState(const FName& NewMatchState);\n    \n    static UClass* StaticClass();\n};\n\n// Game State - replicates game state to clients\nclass AGameStateBase : public AActor {\npublic:\n    AGameModeBase* AuthorityGameMode;\n    TArray<class APlayerState*> PlayerArray;\n    \n    // Match state\n    FName MatchState;\n    int32_t ElapsedTime;\n    uint8_t bReplicatedHasBegunPlay : 1;\n    \n    // Match queries\n    virtual bool HasMatchStarted() const;\n    virtual bool IsMatchInProgress() const;\n    \n    // Replication callbacks\n    void OnRep_MatchState();\n    void OnRep_ReplicatedHasBegunPlay();\n    \n    static UClass* StaticClass();\n};\n\n// World - contains all actors and manages the game world\nclass UWorld : public UObject {\npublic:\n    TArray<class ULevel*> Levels;\n    class ULevel* PersistentLevel;\n    class UNetDriver* NetDriver;\n    class UGameInstance* OwningGameInstance;\n    TArray<APlayerController*> PlayerControllerList;\n    \n    AGameModeBase* AuthorityGameMode;\n    AGameStateBase* GameState;\n    \n    // World state\n    uint8_t bBegunPlay : 1;\n    uint8_t bPlayersOnly : 1;\n    \n    // Actor management\n    virtual bool SpawnActor(UClass* Class, const FVector& Location, const FRotator& Rotation, AActor*& OutActor);\n    virtual bool DestroyActor(AActor* ThisActor, bool bNetForce = false, bool bShouldModifyLevel = true);\n    \n    // Networking\n    virtual void WelcomePlayer(UNetConnection* Connection);\n    virtual void NotifyControlMessage(UNetConnection* Connection, uint8_t MessageType, void* Bunch);\n    virtual APlayerController* SpawnPlayActor(class UPlayer* NewPlayer, ENetRole RemoteRole, const class FURL& InURL, \n                                            const class FUniqueNetIdRepl& UniqueId, FString& Error, uint8_t NetPlayerIndex = 0);\n    \n    // Getters\n    AGameModeBase* GetAuthGameMode() const { return AuthorityGameMode; }\n    AGameStateBase* GetGameState() const { return GameState; }\n    UNetDriver* GetNetDriver() const { return NetDriver; }\n    \n    static UClass* StaticClass();\n};\n\n// Engine - top level game management\nclass UEngine : public UObject {\npublic:\n    class UGameInstance* GameInstance;\n    class UGameViewportClient* GameViewport;\n    \n    static UClass* StaticClass();\n};\n\n// Global accessors (these will be implemented to find the current instances)\nextern UWorld* GetWorld();\nextern UEngine* GetEngine();\nextern APlayerController* GetLocalPlayerController();\nextern class UGameInstance* GetGameInstance();
