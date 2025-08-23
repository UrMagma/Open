#pragma once

#include "ForwardDeclarations.h"
#include "Definitions.h"
#include <unordered_set>
#include <vector>
#include <mutex>

// Forward declarations
class AFortPlayerControllerAthena;

/**
 * BuildingManager - Advanced building system inspired by Project Reboot
 * 
 * Features:
 * - Structure placement (walls, floors, stairs, roofs)
 * - Building editing and destruction
 * - Material consumption tracking
 * - Structure health and damage
 * - Building permissions and validation
 * - Grid-based placement system
 */

enum class EBuildingType : uint8_t {
    None = 0,
    Wall = 1,
    Floor = 2,
    Stairs = 3,
    Roof = 4,
    Trap = 5
};

enum class EBuildingMaterial : uint8_t {
    Wood = 0,
    Stone = 1,
    Metal = 2
};

enum class EStructureState : uint8_t {
    Blueprint = 0,   // Being placed, transparent
    Building = 1,    // Under construction
    Complete = 2,    // Fully built
    Damaged = 3,     // Taking damage
    Destroyed = 4    // Destroyed
};

struct FBuildingPiece {
    uint64_t Id = 0;
    EBuildingType Type = EBuildingType::None;
    EBuildingMaterial Material = EBuildingMaterial::Wood;
    EStructureState State = EStructureState::Blueprint;
    
    FVector Location = FVector();
    FRotator Rotation = FRotator();
    FVector GridPosition = FVector(); // Grid coordinates for snapping
    
    AFortPlayerControllerAthena* Builder = nullptr;
    int32_t TeamId = -1;
    
    float MaxHealth = 100.0f;
    float CurrentHealth = 100.0f;
    float BuildTime = 0.0f;
    float MaxBuildTime = 3.0f;
    
    std::chrono::steady_clock::time_point PlacementTime;
    std::chrono::steady_clock::time_point LastDamageTime;
    
    // Connected structures for stability
    std::unordered_set<uint64_t> ConnectedPieces;
    bool bIsFoundation = false;
    
    FBuildingPiece() {
        PlacementTime = std::chrono::steady_clock::now();
    }
    
    bool IsDestroyed() const {
        return State == EStructureState::Destroyed || CurrentHealth <= 0.0f;
    }
    
    bool IsComplete() const {
        return State == EStructureState::Complete;
    }
    
    float GetHealthPercentage() const {
        return MaxHealth > 0 ? (CurrentHealth / MaxHealth) : 0.0f;
    }
    
    int32_t GetMaterialCost() const {
        switch (Type) {
            case EBuildingType::Wall:
            case EBuildingType::Stairs:
            case EBuildingType::Roof:
                return 10;
            case EBuildingType::Floor:
                return 10;
            default:
                return 0;
        }
    }
};

struct FBuildingGrid {
    static constexpr float GRID_SIZE = 512.0f; // UE4 units
    static constexpr float GRID_HEIGHT = 384.0f;
    
    static FVector WorldToGrid(const FVector& WorldLocation) {
        return FVector(
            std::round(WorldLocation.X / GRID_SIZE),
            std::round(WorldLocation.Y / GRID_SIZE),
            std::round(WorldLocation.Z / GRID_HEIGHT)
        );
    }
    
    static FVector GridToWorld(const FVector& GridLocation) {
        return FVector(
            GridLocation.X * GRID_SIZE,
            GridLocation.Y * GRID_SIZE,
            GridLocation.Z * GRID_HEIGHT
        );
    }
    
    static uint64_t GridToKey(const FVector& GridLocation) {
        // Pack grid coordinates into a single key
        uint64_t x = static_cast<uint64_t>(GridLocation.X + 10000) & 0xFFFFF;
        uint64_t y = static_cast<uint64_t>(GridLocation.Y + 10000) & 0xFFFFF;
        uint64_t z = static_cast<uint64_t>(GridLocation.Z + 1000) & 0xFFF;
        return (x << 32) | (y << 12) | z;
    }
};

struct FBuildingPreview {
    EBuildingType Type = EBuildingType::None;
    EBuildingMaterial Material = EBuildingMaterial::Wood;
    FVector GridPosition = FVector();
    FRotator Rotation = FRotator();
    bool bCanPlace = false;
    FString ErrorReason = FString();
};

class BuildingManager {
public:
    static BuildingManager& Get();
    
    // Building placement
    bool CanPlaceStructure(AFortPlayerControllerAthena* Player, EBuildingType Type, 
                          const FVector& Location, const FRotator& Rotation = FRotator());
    uint64_t PlaceStructure(AFortPlayerControllerAthena* Player, EBuildingType Type, 
                           const FVector& Location, EBuildingMaterial Material = EBuildingMaterial::Wood,
                           const FRotator& Rotation = FRotator());
    bool RemoveStructure(uint64_t StructureId, AFortPlayerControllerAthena* Player = nullptr);
    bool DamageStructure(uint64_t StructureId, float Damage, AFortPlayerControllerAthena* Attacker = nullptr);
    
    // Structure queries
    FBuildingPiece* GetStructure(uint64_t StructureId);
    const FBuildingPiece* GetStructure(uint64_t StructureId) const;
    FBuildingPiece* GetStructureAtLocation(const FVector& Location, EBuildingType Type = EBuildingType::None);
    std::vector<FBuildingPiece*> GetStructuresInRadius(const FVector& Center, float Radius);
    std::vector<FBuildingPiece*> GetPlayerStructures(AFortPlayerControllerAthena* Player);
    std::vector<FBuildingPiece*> GetTeamStructures(int32_t TeamId);
    
    // Building validation
    bool IsValidPlacement(EBuildingType Type, const FVector& GridPosition, 
                         AFortPlayerControllerAthena* Player, FString* OutErrorReason = nullptr);
    bool HasSupport(const FVector& GridPosition, EBuildingType Type);
    bool IsLocationBlocked(const FVector& GridPosition, EBuildingType Type);
    bool HasMaterials(AFortPlayerControllerAthena* Player, EBuildingMaterial Material, int32_t Amount);
    
    // Preview system
    FBuildingPreview GetBuildingPreview(AFortPlayerControllerAthena* Player, EBuildingType Type,
                                       const FVector& Location, const FRotator& Rotation = FRotator());
    void UpdateBuildingPreview(AFortPlayerControllerAthena* Player, const FVector& Location);
    void ClearBuildingPreview(AFortPlayerControllerAthena* Player);
    
    // Structure editing
    bool StartEditing(AFortPlayerControllerAthena* Player, uint64_t StructureId);
    bool StopEditing(AFortPlayerControllerAthena* Player);
    bool IsEditing(AFortPlayerControllerAthena* Player, uint64_t StructureId = 0);
    bool CanEdit(AFortPlayerControllerAthena* Player, uint64_t StructureId);
    
    // Material management
    bool ConsumeMaterials(AFortPlayerControllerAthena* Player, EBuildingMaterial Material, int32_t Amount);
    void RefundMaterials(AFortPlayerControllerAthena* Player, EBuildingMaterial Material, int32_t Amount);
    int32_t GetMaterialCost(EBuildingType Type, EBuildingMaterial Material);
    
    // Structure properties
    float GetStructureMaxHealth(EBuildingType Type, EBuildingMaterial Material);
    float GetBuildTime(EBuildingType Type, EBuildingMaterial Material);
    void UpgradeStructure(uint64_t StructureId, EBuildingMaterial NewMaterial);
    bool CanUpgrade(uint64_t StructureId, EBuildingMaterial NewMaterial);
    
    // Area operations
    void DestroyStructuresInRadius(const FVector& Center, float Radius, 
                                  AFortPlayerControllerAthena* Causer = nullptr);
    void DamageStructuresInRadius(const FVector& Center, float Radius, float Damage,
                                 AFortPlayerControllerAthena* Causer = nullptr);
    uint32_t GetStructureCountInRadius(const FVector& Center, float Radius);
    
    // Building permissions
    void SetBuildingPermissions(AFortPlayerControllerAthena* Player, bool bCanBuild);
    bool CanPlayerBuild(AFortPlayerControllerAthena* Player);
    void SetTeamBuildingArea(int32_t TeamId, const FVector& Center, float Radius);
    bool IsInBuildingArea(AFortPlayerControllerAthena* Player, const FVector& Location);
    
    // Events and callbacks
    using StructurePlacedCallback = std::function<void(AFortPlayerControllerAthena*, const FBuildingPiece&)>;
    using StructureDestroyedCallback = std::function<void(const FBuildingPiece&, AFortPlayerControllerAthena*)>;
    using StructureDamagedCallback = std::function<void(FBuildingPiece&, float, AFortPlayerControllerAthena*)>;
    
    void RegisterStructurePlacedCallback(const std::string& Name, StructurePlacedCallback Callback);
    void RegisterStructureDestroyedCallback(const std::string& Name, StructureDestroyedCallback Callback);
    void RegisterStructureDamagedCallback(const std::string& Name, StructureDamagedCallback Callback);
    void UnregisterCallback(const std::string& Name);
    
    // Configuration
    struct Config {
        float MaxBuildDistance = 1000.0f;
        float MaxBuildHeight = 10000.0f;
        uint32_t MaxStructuresPerPlayer = 3000;
        uint32_t MaxStructuresPerTeam = 10000;
        bool bRequireSupport = true;
        bool bAllowTeamEditing = true;
        float StructureDecayTime = 600.0f; // 10 minutes
        bool bAutoDecay = false;
    } Settings;
    
    // System management
    void Update(float DeltaTime);
    void ClearAllStructures();
    void ClearPlayerStructures(AFortPlayerControllerAthena* Player);
    void ClearTeamStructures(int32_t TeamId);
    
    // Statistics
    uint32_t GetTotalStructureCount() const;
    uint32_t GetPlayerStructureCount(AFortPlayerControllerAthena* Player) const;
    uint32_t GetTeamStructureCount(int32_t TeamId) const;
    std::unordered_map<EBuildingType, uint32_t> GetStructureCountsByType() const;
    
    // Debugging and utilities
    void DumpStructureInfo() const;
    void GenerateBuildingReport(const std::string& FilePath = "") const;
    bool ValidateStructureIntegrity();
    
private:
    BuildingManager() = default;
    ~BuildingManager() = default;
    
    BuildingManager(const BuildingManager&) = delete;
    BuildingManager& operator=(const BuildingManager&) = delete;
    
    // Data structures
    std::unordered_map<uint64_t, FBuildingPiece> Structures;
    std::unordered_map<uint64_t, std::unordered_set<uint64_t>> GridOccupancy; // Grid key -> Structure IDs
    std::unordered_map<AFortPlayerControllerAthena*, uint64_t> EditingSessions;
    std::unordered_map<AFortPlayerControllerAthena*, FBuildingPreview> BuildingPreviews;
    std::unordered_map<AFortPlayerControllerAthena*, bool> BuildingPermissions;
    
    // Callbacks
    std::unordered_map<std::string, StructurePlacedCallback> PlacedCallbacks;
    std::unordered_map<std::string, StructureDestroyedCallback> DestroyedCallbacks;
    std::unordered_map<std::string, StructureDamagedCallback> DamagedCallbacks;
    
    // ID generation
    uint64_t NextStructureId = 1;
    
    // Thread safety
    mutable std::mutex BuildingMutex;
    
    // Internal helpers
    uint64_t GenerateStructureId();
    void AddToGrid(uint64_t StructureId, const FVector& GridPosition);
    void RemoveFromGrid(uint64_t StructureId, const FVector& GridPosition);
    void UpdateStructureConnections(uint64_t StructureId);
    void CheckStructuralIntegrity(uint64_t RemovedStructureId);
    bool IsStructureSupported(const FBuildingPiece& Structure);
    
    void FireStructurePlacedCallbacks(AFortPlayerControllerAthena* Player, const FBuildingPiece& Structure);
    void FireStructureDestroyedCallbacks(const FBuildingPiece& Structure, AFortPlayerControllerAthena* Destroyer);
    void FireStructureDamagedCallbacks(FBuildingPiece& Structure, float Damage, AFortPlayerControllerAthena* Attacker);
    
    // Building rules
    bool CanPlaceAtGrid(const FVector& GridPosition, EBuildingType Type, AFortPlayerControllerAthena* Player);
    std::vector<FVector> GetRequiredSupportPositions(const FVector& GridPosition, EBuildingType Type);
    std::vector<FVector> GetAdjacentPositions(const FVector& GridPosition);
};