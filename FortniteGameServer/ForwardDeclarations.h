#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

// Forward declarations for all major classes to avoid circular dependencies
// This should be included first in headers that need these types

// Core UE4 types
class UObject;
class UClass;
class UFunction;
class UProperty;
class UStruct;
class UField;

// Engine types  
class AActor;
class APawn;
class APlayerController;
class AGameModeBase;
class AGameStateBase;
class APlayerState;
class UWorld;
class UEngine;

// Fortnite specific types
class AFortPlayerControllerAthena;
class AFortPlayerPawnAthena;
class AFortPlayerStateAthena;
class AFortGameModeAthena;
class UFortItemDefinition;
class UFortWeaponItemDefinition;

// Manager classes
class PlayerManager;
class InventoryManager;
class BuildingManager;
class SafezoneManager;
class MatchManager;
class NetworkManager;
class EventManager;
class ObjectManager;
class TypeRegistry;

// Structure forward declarations
struct FVector;
struct FRotator;
struct FTransform;
struct FName;
struct FString;
struct FQuat;

// Game system structures
struct FPlayerStats;
struct FEliminationInfo;
struct FSpawnInfo;
struct FBuildingPiece;
struct FInventoryItem;
struct FMatchSettings;
struct FNetworkMessage;
struct FGameEvent;

// Function pointer declarations
extern void StartGameServer();
extern void StopGameServer(); 
extern void SetupSystemCallbacks();