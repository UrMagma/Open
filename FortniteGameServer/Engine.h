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
    virtual FVector GetActorLocation() const;
    virtual FRotator GetActorRotation() const;
    virtual FVector GetActorScale3D() const;
    virtual FTransform GetActorTransform() const;
    
    virtual bool SetActorLocation(const FVector& NewLocation, bool bSweep = false, bool bTeleport = false);
    virtual bool SetActorRotation(const FRotator& NewRotation, bool bTeleport = false);
    virtual bool SetActorScale3D(const FVector& NewScale3D);
    virtual bool SetActorTransform(const FTransform& NewTransform, bool bSweep = false, bool bTeleport = false);
