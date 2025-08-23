#include "BuildingManager.h"
#include "InventoryManager.h"
#include "UObject.h"
#include "Definitions.h"
#include <algorithm>

BuildingManager& BuildingManager::Get() {
    static BuildingManager Instance;
    return Instance;
}

void BuildingManager::Update(float DeltaTime) {
    std::lock_guard<std::mutex> Lock(BuildingMutex);
    
    // Update building piece health regeneration
    for (auto& piece : BuildingPieces) {
        if (piece.Health < piece.MaxHealth && piece.bCanRegenerate) {
            piece.Health += Settings.HealthRegenerationRate * DeltaTime;
            piece.Health = std::min(piece.Health, piece.MaxHealth);
        }
    }
    
    // Process pending destructions
    auto now = std::chrono::steady_clock::now();
    PendingDestructions.erase(
        std::remove_if(PendingDestructions.begin(), PendingDestructions.end(),
            [this, now](const FPendingDestruction& destruction) {
                if (now >= destruction.DestructionTime) {
                    ActuallyDestroyBuilding(destruction.PieceId);
                    return true;
                }
                return false;
            }),
        PendingDestructions.end());
}

bool BuildingManager::CanBuild(AFortPlayerControllerAthena* Player, EBuildingType Type, const FVector& Location, const FRotator& Rotation) {
    if (!Player || !Settings.bBuildingEnabled) return false;
    
    std::lock_guard<std::mutex> Lock(BuildingMutex);
    
    // Check if location is valid
    if (!IsValidBuildLocation(Location)) {
        return false;
    }
    
    // Check material requirements
    auto& inventoryMgr = InventoryManager::Get();
    EMaterialType requiredMaterial = GetMaterialTypeForBuilding(Type);
    int32_t requiredAmount = GetMaterialCostForBuilding(Type);
    
    if (inventoryMgr.GetMaterialCount(Player, requiredMaterial) < requiredAmount) {
        return false;
    }
    
    // Check for overlapping structures
    if (IsLocationOccupied(Location, Type)) {
        return false;
    }
    
    return true;
}

FBuildingPiece* BuildingManager::PlaceBuilding(AFortPlayerControllerAthena* Player, EBuildingType Type, 
                                              const FVector& Location, const FRotator& Rotation, EMaterialType Material) {
    if (!CanBuild(Player, Type, Location, Rotation)) {
        return nullptr;
    }
    
    std::lock_guard<std::mutex> Lock(BuildingMutex);
    
    // Consume materials
    auto& inventoryMgr = InventoryManager::Get();
    int32_t materialCost = GetMaterialCostForBuilding(Type);
    if (!inventoryMgr.ConsumeMaterial(Player, Material, materialCost)) {
        return nullptr;
    }
    
    // Create building piece
    FBuildingPiece piece;
    piece.Id = NextBuildingId++;
    piece.Owner = Player;
    piece.Type = Type;
    piece.Location = Location;
    piece.Rotation = Rotation;
    piece.Material = Material;
    piece.Health = GetMaxHealthForBuilding(Type, Material);
    piece.MaxHealth = piece.Health;
    piece.PlacementTime = std::chrono::steady_clock::now();
    piece.bCanRegenerate = true;
    
    BuildingPieces.push_back(piece);
    
    // Update grid
    FGridCoordinate gridCoord = WorldToGrid(Location);
    BuildingGrid[gridCoord] = &BuildingPieces.back();
    
    // Fire callbacks
    FireBuildingPlacedCallbacks(Player, BuildingPieces.back());
    
    LOG_INFO("Player " + Player->GetName() + " placed " + GetBuildingTypeName(Type) + " at (" + 
             std::to_string(Location.X) + ", " + std::to_string(Location.Y) + ", " + std::to_string(Location.Z) + ")");
    
    return &BuildingPieces.back();
}

bool BuildingManager::DestroyBuilding(uint32_t BuildingId, AFortPlayerControllerAthena* Destroyer, bool bImmediate) {
    std::lock_guard<std::mutex> Lock(BuildingMutex);
    
    auto it = std::find_if(BuildingPieces.begin(), BuildingPieces.end(),
        [BuildingId](const FBuildingPiece& piece) {
            return piece.Id == BuildingId;
        });
    
    if (it == BuildingPieces.end()) {
        return false;
    }
    
    if (bImmediate) {
        ActuallyDestroyBuilding(BuildingId);
    } else {
        // Schedule destruction
        FPendingDestruction destruction;
        destruction.PieceId = BuildingId;
        destruction.DestructionTime = std::chrono::steady_clock::now() + 
                                    std::chrono::milliseconds(static_cast<int>(Settings.DestructionDelay * 1000));
        PendingDestructions.push_back(destruction);
    }
    
    FireBuildingDestroyedCallbacks(Destroyer, *it);
    
    LOG_INFO("Building " + std::to_string(BuildingId) + " marked for destruction");
    return true;
}

bool BuildingManager::DamageBuilding(uint32_t BuildingId, float Damage, AFortPlayerControllerAthena* Attacker) {
    std::lock_guard<std::mutex> Lock(BuildingMutex);
    
    auto it = std::find_if(BuildingPieces.begin(), BuildingPieces.end(),
        [BuildingId](FBuildingPiece& piece) {
            return piece.Id == BuildingId;
        });
    
    if (it == BuildingPieces.end()) {
        return false;
    }
    
    it->Health -= Damage;
    
    FireBuildingDamagedCallbacks(Attacker, *it, Damage);
    
    if (it->Health <= 0) {
        DestroyBuilding(BuildingId, Attacker, true);
        return true;
    }
    
    LOG_INFO("Building " + std::to_string(BuildingId) + " damaged for " + std::to_string(Damage) + 
             " (Health: " + std::to_string(it->Health) + "/" + std::to_string(it->MaxHealth) + ")");
    
    return false; // Not destroyed
}

FBuildingPiece* BuildingManager::GetBuildingById(uint32_t BuildingId) {
    std::lock_guard<std::mutex> Lock(BuildingMutex);
    
    auto it = std::find_if(BuildingPieces.begin(), BuildingPieces.end(),
        [BuildingId](const FBuildingPiece& piece) {
            return piece.Id == BuildingId;
        });
    
    return it != BuildingPieces.end() ? &(*it) : nullptr;
}

std::vector<FBuildingPiece*> BuildingManager::GetBuildingsInRadius(const FVector& Center, float Radius) {
    std::lock_guard<std::mutex> Lock(BuildingMutex);
    
    std::vector<FBuildingPiece*> result;
    float radiusSquared = Radius * Radius;
    
    for (auto& piece : BuildingPieces) {
        FVector diff = piece.Location - Center;
        if (diff.X * diff.X + diff.Y * diff.Y + diff.Z * diff.Z <= radiusSquared) {
            result.push_back(&piece);
        }
    }
    
    return result;
}

std::vector<FBuildingPiece*> BuildingManager::GetPlayerBuildings(AFortPlayerControllerAthena* Player) {
    if (!Player) return {};
    
    std::lock_guard<std::mutex> Lock(BuildingMutex);
    
    std::vector<FBuildingPiece*> result;
    for (auto& piece : BuildingPieces) {
        if (piece.Owner == Player) {
            result.push_back(&piece);
        }
    }
    
    return result;
}

bool BuildingManager::IsValidBuildLocation(const FVector& Location) {
    // Basic validation - check if location is not too low or too high
    if (Location.Z < Settings.MinBuildHeight || Location.Z > Settings.MaxBuildHeight) {
        return false;
    }
    
    // Check if location is within map bounds (simplified)
    float maxDistance = 50000.0f; // 500m from center
    if (Location.X * Location.X + Location.Y * Location.Y > maxDistance * maxDistance) {
        return false;
    }
    
    return true;
}

bool BuildingManager::IsLocationOccupied(const FVector& Location, EBuildingType Type) {
    FGridCoordinate gridCoord = WorldToGrid(Location);
    
    auto it = BuildingGrid.find(gridCoord);
    if (it != BuildingGrid.end() && it->second != nullptr) {
        // Location is occupied
        return true;
    }
    
    return false;
}

FGridCoordinate BuildingManager::WorldToGrid(const FVector& WorldLocation) {
    FGridCoordinate coord;
    coord.X = static_cast<int32_t>(WorldLocation.X / Settings.GridSize);
    coord.Y = static_cast<int32_t>(WorldLocation.Y / Settings.GridSize);
    coord.Z = static_cast<int32_t>(WorldLocation.Z / Settings.GridSize);
    return coord;
}

FVector BuildingManager::GridToWorld(const FGridCoordinate& GridCoord) {
    FVector world;
    world.X = GridCoord.X * Settings.GridSize;
    world.Y = GridCoord.Y * Settings.GridSize;
    world.Z = GridCoord.Z * Settings.GridSize;
    return world;
}

EMaterialType BuildingManager::GetMaterialTypeForBuilding(EBuildingType Type) {
    // Default material type mapping
    return EMaterialType::Wood; // Simplified - in real game this would be more complex
}

int32_t BuildingManager::GetMaterialCostForBuilding(EBuildingType Type) {
    switch (Type) {
        case EBuildingType::Wall:
        case EBuildingType::Floor:
        case EBuildingType::Roof:
        case EBuildingType::Stairs:
            return 10;
        default:
            return 10;
    }
}

float BuildingManager::GetMaxHealthForBuilding(EBuildingType Type, EMaterialType Material) {
    float baseHealth = 100.0f;
    
    // Material multiplier
    float materialMultiplier = 1.0f;
    switch (Material) {
        case EMaterialType::Wood: materialMultiplier = 1.0f; break;
        case EMaterialType::Stone: materialMultiplier = 1.5f; break;
        case EMaterialType::Metal: materialMultiplier = 2.0f; break;
    }
    
    // Type multiplier
    float typeMultiplier = 1.0f;
    switch (Type) {
        case EBuildingType::Wall: typeMultiplier = 1.0f; break;
        case EBuildingType::Floor: typeMultiplier = 1.2f; break;
        case EBuildingType::Stairs: typeMultiplier = 0.8f; break;
        case EBuildingType::Roof: typeMultiplier = 1.1f; break;
        default: typeMultiplier = 1.0f; break;
    }
    
    return baseHealth * materialMultiplier * typeMultiplier;
}

std::string BuildingManager::GetBuildingTypeName(EBuildingType Type) {
    switch (Type) {
        case EBuildingType::Wall: return "Wall";
        case EBuildingType::Floor: return "Floor";
        case EBuildingType::Stairs: return "Stairs";
        case EBuildingType::Roof: return "Roof";
        case EBuildingType::Trap: return "Trap";
        default: return "Unknown";
    }
}

void BuildingManager::ActuallyDestroyBuilding(uint32_t BuildingId) {
    auto it = std::find_if(BuildingPieces.begin(), BuildingPieces.end(),
        [BuildingId](const FBuildingPiece& piece) {
            return piece.Id == BuildingId;
        });
    
    if (it != BuildingPieces.end()) {
        // Remove from grid
        FGridCoordinate gridCoord = WorldToGrid(it->Location);
        BuildingGrid.erase(gridCoord);
        
        // Remove from list
        BuildingPieces.erase(it);
        
        LOG_INFO("Building " + std::to_string(BuildingId) + " destroyed");
    }
}

// Callback management
void BuildingManager::RegisterBuildingPlacedCallback(const std::string& Name, BuildingPlacedCallback Callback) {
    std::lock_guard<std::mutex> Lock(BuildingMutex);
    BuildingPlacedCallbacks[Name] = Callback;
}

void BuildingManager::RegisterBuildingDestroyedCallback(const std::string& Name, BuildingDestroyedCallback Callback) {
    std::lock_guard<std::mutex> Lock(BuildingMutex);
    BuildingDestroyedCallbacks[Name] = Callback;
}

void BuildingManager::RegisterBuildingDamagedCallback(const std::string& Name, BuildingDamagedCallback Callback) {
    std::lock_guard<std::mutex> Lock(BuildingMutex);
    BuildingDamagedCallbacks[Name] = Callback;
}

void BuildingManager::UnregisterCallback(const std::string& Name) {
    std::lock_guard<std::mutex> Lock(BuildingMutex);
    BuildingPlacedCallbacks.erase(Name);
    BuildingDestroyedCallbacks.erase(Name);
    BuildingDamagedCallbacks.erase(Name);
}

void BuildingManager::FireBuildingPlacedCallbacks(AFortPlayerControllerAthena* Player, const FBuildingPiece& Building) {
    for (const auto& pair : BuildingPlacedCallbacks) {
        try {
            pair.second(Player, Building);
        } catch (...) {
            LOG_ERROR("Exception in BuildingPlaced callback: " + pair.first);
        }
    }
}

void BuildingManager::FireBuildingDestroyedCallbacks(AFortPlayerControllerAthena* Player, const FBuildingPiece& Building) {
    for (const auto& pair : BuildingDestroyedCallbacks) {
        try {
            pair.second(Player, Building);
        } catch (...) {
            LOG_ERROR("Exception in BuildingDestroyed callback: " + pair.first);
        }
    }
}

void BuildingManager::FireBuildingDamagedCallbacks(AFortPlayerControllerAthena* Player, const FBuildingPiece& Building, float Damage) {
    for (const auto& pair : BuildingDamagedCallbacks) {
        try {
            pair.second(Player, Building, Damage);
        } catch (...) {
            LOG_ERROR("Exception in BuildingDamaged callback: " + pair.first);
        }
    }
}