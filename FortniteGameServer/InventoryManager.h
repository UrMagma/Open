#pragma once

#include "ForwardDeclarations.h"
#include "Definitions.h"
#include <array>
#include <unordered_map>
#include <functional>
#include <mutex>

// Forward declarations
class AFortPlayerControllerAthena;

/**
 * InventoryManager - Advanced inventory and item system inspired by Project Reboot
 * 
 * Features:
 * - Hotbar/Quickbar management
 * - Weapon switching and handling
 * - Item stacking and combining
 * - Backpack inventory management
 * - Resource management (materials)
 * - Ammo tracking
 * - Item rarity and stats
 */

enum class EItemType : uint8_t {
    None = 0,
    Weapon = 1,
    Consumable = 2,
    Material = 3,
    Ammo = 4,
    Trap = 5,
    Gadget = 6,
    Resource = 7
};

enum class EWeaponType : uint8_t {
    None = 0,
    AssaultRifle = 1,
    Shotgun = 2,
    SMG = 3,
    Sniper = 4,
    Pistol = 5,
    Explosives = 6,
    Melee = 7,
    Bow = 8
};

enum class EItemRarity : uint8_t {
    Common = 0,     // Gray
    Uncommon = 1,   // Green  
    Rare = 2,       // Blue
    Epic = 3,       // Purple
    Legendary = 4,  // Gold
    Mythic = 5      // Orange
};

enum class EMaterialType : uint8_t {
    Wood = 0,
    Stone = 1,
    Metal = 2
};

struct FItemDefinition {
    FString ItemName;
    FString DisplayName;
    EItemType Type = EItemType::None;
    EWeaponType WeaponType = EWeaponType::None;
    EItemRarity Rarity = EItemRarity::Common;
    
    int32_t MaxStackSize = 1;
    bool bCanStack = false;
    bool bAutoPickup = false;
    
    // Weapon stats
    float Damage = 0.0f;
    float HeadshotMultiplier = 1.5f;
    float FireRate = 1.0f;
    float Range = 1000.0f;
    float Accuracy = 1.0f;
    int32_t MagazineSize = 30;
    float ReloadTime = 2.0f;
    
    // Item value
    int32_t SellValue = 0;
    
    FItemDefinition() = default;
    FItemDefinition(const FString& name, EItemType type, EItemRarity rarity = EItemRarity::Common)
        : ItemName(name), DisplayName(name), Type(type), Rarity(rarity) {}
};

struct FInventoryItem {
    FString ItemId;
    FItemDefinition Definition;
    int32_t Quantity = 1;
    int32_t Durability = 100;
    std::unordered_map<FString, FString> Attributes;
    
    FInventoryItem() = default;
    FInventoryItem(const FString& id, const FItemDefinition& def, int32_t qty = 1) 
        : ItemId(id), Definition(def), Quantity(qty) {}
    
    bool CanStackWith(const FInventoryItem& Other) const {
        return Definition.bCanStack && 
               ItemId == Other.ItemId && 
               Definition.ItemName == Other.Definition.ItemName;
    }
    
    int32_t GetMaxStack() const {
        return Definition.MaxStackSize;
    }
    
    bool IsFull() const {
        return Quantity >= GetMaxStack();
    }
};

struct FQuickbarSlot {
    FInventoryItem Item;
    bool bIsActive = false;
    bool bIsEmpty = true;
    
    void SetItem(const FInventoryItem& NewItem) {
        Item = NewItem;
        bIsEmpty = false;
    }
    
    void Clear() {
        Item = FInventoryItem();
        bIsActive = false;
        bIsEmpty = true;
    }
};

class InventoryManager {
public:
    static InventoryManager& Get();
    
    // Item management
    void RegisterItemDefinition(const FItemDefinition& Definition);
    FItemDefinition* GetItemDefinition(const FString& ItemName);
    bool DoesItemExist(const FString& ItemName) const;
    
    // Player inventory management
    bool GiveItem(AFortPlayerControllerAthena* Player, const FString& ItemName, int32_t Quantity = 1, bool bForceToInventory = false);
    bool GiveItem(AFortPlayerControllerAthena* Player, const FInventoryItem& Item, bool bForceToInventory = false);
    bool RemoveItem(AFortPlayerControllerAthena* Player, const FString& ItemName, int32_t Quantity = 1);
    bool RemoveItemFromSlot(AFortPlayerControllerAthena* Player, int32_t SlotIndex, int32_t Quantity = 1);
    bool HasItem(AFortPlayerControllerAthena* Player, const FString& ItemName, int32_t MinQuantity = 1) const;
    int32_t GetItemCount(AFortPlayerControllerAthena* Player, const FString& ItemName) const;
    
    // Quickbar management
    bool SetQuickbarSlot(AFortPlayerControllerAthena* Player, int32_t SlotIndex, const FInventoryItem& Item);
    bool SwapQuickbarSlots(AFortPlayerControllerAthena* Player, int32_t FromSlot, int32_t ToSlot);
    void ClearQuickbarSlot(AFortPlayerControllerAthena* Player, int32_t SlotIndex);
    FQuickbarSlot* GetQuickbarSlot(AFortPlayerControllerAthena* Player, int32_t SlotIndex);
    int32_t GetActiveSlot(AFortPlayerControllerAthena* Player) const;
    void SetActiveSlot(AFortPlayerControllerAthena* Player, int32_t SlotIndex);
    
    // Weapon management
    bool EquipWeapon(AFortPlayerControllerAthena* Player, int32_t SlotIndex);
    bool UnequipWeapon(AFortPlayerControllerAthena* Player);
    FInventoryItem* GetEquippedWeapon(AFortPlayerControllerAthena* Player);
    bool CanEquipWeapon(AFortPlayerControllerAthena* Player, const FString& WeaponName) const;
    void ReloadWeapon(AFortPlayerControllerAthena* Player);
    bool UseAmmo(AFortPlayerControllerAthena* Player, const FString& AmmoType, int32_t Amount);
    
    // Resource management
    void SetPlayerMaterials(AFortPlayerControllerAthena* Player, EMaterialType Type, int32_t Amount);
    void AddPlayerMaterials(AFortPlayerControllerAthena* Player, EMaterialType Type, int32_t Amount);
    bool RemovePlayerMaterials(AFortPlayerControllerAthena* Player, EMaterialType Type, int32_t Amount);
    int32_t GetPlayerMaterials(AFortPlayerControllerAthena* Player, EMaterialType Type) const;
    bool HasEnoughMaterials(AFortPlayerControllerAthena* Player, EMaterialType Type, int32_t RequiredAmount) const;
    
    // Inventory queries
    std::vector<FInventoryItem> GetPlayerInventory(AFortPlayerControllerAthena* Player) const;
    std::array<FQuickbarSlot, 10> GetPlayerQuickbar(AFortPlayerControllerAthena* Player) const;
    int32_t GetInventorySlotCount(AFortPlayerControllerAthena* Player) const;
    int32_t GetUsedInventorySlots(AFortPlayerControllerAthena* Player) const;
    int32_t GetFreeInventorySlots(AFortPlayerControllerAthena* Player) const;
    bool IsInventoryFull(AFortPlayerControllerAthena* Player) const;
    
    // Item operations
    bool DropItem(AFortPlayerControllerAthena* Player, int32_t SlotIndex, int32_t Quantity = -1, const FVector& DropLocation = FVector());
    bool PickupItem(AFortPlayerControllerAthena* Player, const FInventoryItem& Item);
    bool MoveItem(AFortPlayerControllerAthena* Player, int32_t FromSlot, int32_t ToSlot, int32_t Quantity = -1);
    bool SplitItem(AFortPlayerControllerAthena* Player, int32_t SlotIndex, int32_t SplitQuantity);
    bool CombineItems(AFortPlayerControllerAthena* Player, int32_t Slot1, int32_t Slot2);
    
    // Consumables
    bool UseConsumable(AFortPlayerControllerAthena* Player, int32_t SlotIndex);
    bool CanUseItem(AFortPlayerControllerAthena* Player, const FString& ItemName) const;
    
    // Events and callbacks
    using ItemGivenCallback = std::function<void(AFortPlayerControllerAthena*, const FInventoryItem&)>;
    using ItemRemovedCallback = std::function<void(AFortPlayerControllerAthena*, const FInventoryItem&)>;
    using WeaponEquippedCallback = std::function<void(AFortPlayerControllerAthena*, const FInventoryItem&)>;
    using ItemUsedCallback = std::function<void(AFortPlayerControllerAthena*, const FInventoryItem&)>;
    
    void RegisterItemGivenCallback(const std::string& Name, ItemGivenCallback Callback);
    void RegisterItemRemovedCallback(const std::string& Name, ItemRemovedCallback Callback);
    void RegisterWeaponEquippedCallback(const std::string& Name, WeaponEquippedCallback Callback);
    void RegisterItemUsedCallback(const std::string& Name, ItemUsedCallback Callback);
    void UnregisterCallback(const std::string& Name);
    
    // Configuration
    struct Config {
        int32_t DefaultInventorySlots = 16;
        int32_t QuickbarSlots = 6;
        int32_t MaxMaterials = 999;
        bool bAutoPickupAmmo = true;
        bool bAutoPickupMaterials = true;
        bool bAutoStackItems = true;
        float DropItemDistance = 200.0f;
    } Settings;
    
    // Utilities
    void ClearPlayerInventory(AFortPlayerControllerAthena* Player);
    void ResetPlayerInventory(AFortPlayerControllerAthena* Player);
    void SavePlayerInventory(AFortPlayerControllerAthena* Player, const std::string& SaveName);
    bool LoadPlayerInventory(AFortPlayerControllerAthena* Player, const std::string& SaveName);
    
    // Debugging
    void DumpPlayerInventory(AFortPlayerControllerAthena* Player) const;
    void DumpAllInventories() const;
    
    // Default items
    void InitializeDefaultItems();
    void GiveStartingItems(AFortPlayerControllerAthena* Player);
    
private:
    InventoryManager() = default;
    ~InventoryManager() = default;
    
    InventoryManager(const InventoryManager&) = delete;
    InventoryManager& operator=(const InventoryManager&) = delete;
    
    // Data structures
    std::unordered_map<FString, FItemDefinition> ItemDefinitions;
    std::unordered_map<AFortPlayerControllerAthena*, std::vector<FInventoryItem>> PlayerInventories;
    std::unordered_map<AFortPlayerControllerAthena*, std::array<FQuickbarSlot, 10>> PlayerQuickbars;
    std::unordered_map<AFortPlayerControllerAthena*, int32_t> ActiveSlots;
    std::unordered_map<AFortPlayerControllerAthena*, std::array<int32_t, 3>> PlayerMaterials; // Wood, Stone, Metal
    
    // Callbacks  
    std::unordered_map<std::string, ItemGivenCallback> ItemGivenCallbacks;
    std::unordered_map<std::string, ItemRemovedCallback> ItemRemovedCallbacks;
    std::unordered_map<std::string, WeaponEquippedCallback> WeaponEquippedCallbacks;
    std::unordered_map<std::string, ItemUsedCallback> ItemUsedCallbacks;
    
    // Thread safety
    mutable std::mutex InventoryMutex;
    
    // Internal helpers
    std::vector<FInventoryItem>& GetOrCreatePlayerInventory(AFortPlayerControllerAthena* Player);
    std::array<FQuickbarSlot, 10>& GetOrCreatePlayerQuickbar(AFortPlayerControllerAthena* Player);
    int32_t FindInventorySlot(AFortPlayerControllerAthena* Player, const FString& ItemName) const;
    int32_t FindEmptyInventorySlot(AFortPlayerControllerAthena* Player) const;
    bool AddToExistingStack(AFortPlayerControllerAthena* Player, const FInventoryItem& Item, int32_t& RemainingQuantity);
    
    void FireItemGivenCallbacks(AFortPlayerControllerAthena* Player, const FInventoryItem& Item);
    void FireItemRemovedCallbacks(AFortPlayerControllerAthena* Player, const FInventoryItem& Item);
    void FireWeaponEquippedCallbacks(AFortPlayerControllerAthena* Player, const FInventoryItem& Item);
    void FireItemUsedCallbacks(AFortPlayerControllerAthena* Player, const FInventoryItem& Item);
};