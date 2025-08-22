#pragma once

#include "../FortniteClasses.h"
#include "../Native.h"

// Player loadout structure
struct PlayerLoadout {
    UFortWeaponItemDefinition* Pickaxe;
    UFortItemDefinition* Slot1; // Usually shotgun
    UFortItemDefinition* Slot2; // Usually assault rifle
    UFortItemDefinition* Slot3; // Usually SMG or sniper
    UFortItemDefinition* Slot4; // Usually explosives
    UFortItemDefinition* Slot5; // Usually consumables
    
    PlayerLoadout() : Pickaxe(nullptr), Slot1(nullptr), Slot2(nullptr), 
                     Slot3(nullptr), Slot4(nullptr), Slot5(nullptr) {}
    
    PlayerLoadout(UFortWeaponItemDefinition* pickaxe, UFortItemDefinition* slot1, 
                 UFortItemDefinition* slot2, UFortItemDefinition* slot3, 
                 UFortItemDefinition* slot4, UFortItemDefinition* slot5)
        : Pickaxe(pickaxe), Slot1(slot1), Slot2(slot2), Slot3(slot3), Slot4(slot4), Slot5(slot5) {}
};

// Inventory entry structure
struct FInventoryEntry {
    FString ItemGuid;
    UFortItemDefinition* ItemDefinition;
    int32_t Count;
    int32_t LoadedAmmo;
    
    FInventoryEntry() : ItemDefinition(nullptr), Count(1), LoadedAmmo(0) {}
};

namespace Inventory {
    
    /**
     * Initialize player inventory system
     */
    void Init(AFortPlayerControllerAthena* Controller);
    
    /**
     * Equip a loadout to the player
     */
    void EquipLoadout(AFortPlayerControllerAthena* Controller, PlayerLoadout& Loadout);
    
    /**
     * Add item to player inventory
     */
    bool AddItem(AFortPlayerControllerAthena* Controller, UFortItemDefinition* ItemDef, 
                int32_t Count = 1, int32_t LoadedAmmo = 0);
    
    /**
     * Remove item from player inventory
     */
    bool RemoveItem(AFortPlayerControllerAthena* Controller, const FString& ItemGuid, int32_t Count = 1);
    
    /**
     * Find item in inventory by type
     */
    template<typename T = UFortItemDefinition>
    FInventoryEntry FindItemInInventory(AFortPlayerControllerAthena* Controller, bool& bFound) {
        bFound = false;
        FInventoryEntry Entry;
        
        if (!Controller || !Controller->Pawn) {
            return Entry;
        }
        
        // This would normally access the player's inventory component
        // For now, we'll return a placeholder
        // In a real implementation, this would search through the player's inventory
        // for items of type T and return the first match
        
        return Entry;
    }
    
    /**
     * Find specific item in inventory by definition
     */
    FInventoryEntry FindItemInInventory(AFortPlayerControllerAthena* Controller, UFortItemDefinition* ItemDef, bool& bFound);
    
    /**
     * Equip item by GUID
     */
    void EquipInventoryItem(AFortPlayerControllerAthena* Controller, const FString& ItemGuid);
    
    /**
     * Drop item from inventory
     */
    void DropInventoryItem(AFortPlayerControllerAthena* Controller, const FString& ItemGuid, int32_t Count = 1);
    
    /**
     * Use consumable item
     */
    bool UseConsumableItem(AFortPlayerControllerAthena* Controller, const FString& ItemGuid);
    
    /**
     * Get inventory item count
     */
    int32_t GetItemCount(AFortPlayerControllerAthena* Controller, UFortItemDefinition* ItemDef);
    
    /**
     * Check if inventory has space
     */
    bool HasInventorySpace(AFortPlayerControllerAthena* Controller, int32_t SlotsNeeded = 1);
    
    /**
     * Get inventory size
     */
    int32_t GetInventorySize(AFortPlayerControllerAthena* Controller);
    
    /**
     * Clear inventory
     */
    void ClearInventory(AFortPlayerControllerAthena* Controller);
    
    /**
     * Give materials (wood, stone, metal)
     */
    void GiveMaterials(AFortPlayerControllerAthena* Controller, int32_t Wood = 0, int32_t Stone = 0, int32_t Metal = 0);
    
    /**
     * Give ammo
     */
    void GiveAmmo(AFortPlayerControllerAthena* Controller, UFortItemDefinition* AmmoType, int32_t Amount);
    
    /**
     * Set up default inventory for new players
     */
    void SetupDefaultInventory(AFortPlayerControllerAthena* Controller);
    
    // Weapon definitions cache
    namespace WeaponDefs {
        extern UFortWeaponItemDefinition* Pickaxe_Default;
        extern UFortItemDefinition* AR_Common;
        extern UFortItemDefinition* AR_Uncommon;
        extern UFortItemDefinition* AR_Rare;
        extern UFortItemDefinition* Shotgun_Common;
        extern UFortItemDefinition* Shotgun_Uncommon;
        extern UFortItemDefinition* Shotgun_Rare;
        extern UFortItemDefinition* SMG_Common;
        extern UFortItemDefinition* SMG_Uncommon;
        extern UFortItemDefinition* Sniper_Common;
        extern UFortItemDefinition* Sniper_Rare;
        extern UFortItemDefinition* Shields_Small;
        extern UFortItemDefinition* Shields_Big;
        extern UFortItemDefinition* Medkit;
        extern UFortItemDefinition* Bandages;
        
        // Initialize weapon definition cache
        void Initialize();
    }
}
