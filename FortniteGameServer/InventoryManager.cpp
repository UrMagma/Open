#include "InventoryManager.h"
#include "Logger.h"
#include <algorithm>
#include <chrono>

// InventoryManager Implementation
InventoryManager& InventoryManager::Get() {
    static InventoryManager Instance;
    return Instance;
}

bool InventoryManager::GiveItem(AFortPlayerControllerAthena* Player, const FString& ItemName, int32_t Quantity, bool bForceToInventory) {
    if (!Player || Quantity <= 0) {
        LOG_WARN("Invalid player or quantity in GiveItem");
        return false;
    }
    
    FItemDefinition* itemDef = GetItemDefinition(ItemName);
    if (!itemDef) {
        LOG_WARN("Item definition not found: " + ItemName.ToString());
        return false;
    }
    
    std::lock_guard<std::mutex> Lock(InventoryMutex);
    auto& inventory = GetOrCreatePlayerInventory(Player);
    
    // Try to stack with existing items first
    int32_t remainingQuantity = Quantity;
    if (itemDef->bCanStack) {
        for (auto& existingItem : inventory) {
            if (existingItem.CanStackWith(FInventoryItem(ItemName, *itemDef, 1))) {
                int32_t spaceInStack = existingItem.GetMaxStack() - existingItem.Quantity;
                int32_t toAdd = std::min(spaceInStack, remainingQuantity);
                
                existingItem.Quantity += toAdd;
                remainingQuantity -= toAdd;
                
                if (remainingQuantity <= 0) {
                    FireItemGivenCallbacks(Player, existingItem);
                    return true;
                }
            }
        }
    }
    
    // Create new item for remaining quantity
    if (remainingQuantity > 0) {
        FInventoryItem newItem(ItemName, *itemDef, remainingQuantity);
        newItem.Durability = 100; // Full durability for new items
        
        inventory.push_back(newItem);
        FireItemGivenCallbacks(Player, newItem);
    }
    
    LOG_INFO("Gave " + std::to_string(Quantity) + "x " + ItemName.ToString() + " to player");
    return true;
}

bool InventoryManager::RemoveItem(AFortPlayerControllerAthena* Player, const FString& ItemName, int32_t Quantity) {
    if (!Player || Quantity <= 0) return false;
    
    std::lock_guard<std::mutex> Lock(InventoryMutex);
    auto& inventory = GetOrCreatePlayerInventory(Player);
    
    int32_t remainingToRemove = Quantity;
    
    for (auto it = inventory.begin(); it != inventory.end(); ) {
        if (it->ItemId.ToString() == ItemName.ToString()) {
            int32_t toRemove = std::min(it->Quantity, remainingToRemove);
            it->Quantity -= toRemove;
            remainingToRemove -= toRemove;
            
            if (it->Quantity <= 0) {
                FireItemRemovedCallbacks(Player, *it);
                it = inventory.erase(it);
            } else {
                ++it;
            }
            
            if (remainingToRemove <= 0) {
                return true;
            }
        } else {
            ++it;
        }
    }
    
    return remainingToRemove < Quantity; // Return true if we removed at least some
}

bool InventoryManager::EquipWeapon(AFortPlayerControllerAthena* Player, int32_t SlotIndex) {
    if (!Player || SlotIndex < 0 || SlotIndex >= Settings.QuickbarSlots) {
        LOG_WARN("Invalid player or slot index in EquipWeapon");
        return false;
    }
    
    std::lock_guard<std::mutex> Lock(InventoryMutex);
    auto& quickbar = GetOrCreatePlayerQuickbar(Player);
    
    FQuickbarSlot* slot = &quickbar[SlotIndex];
    if (!slot || slot->bIsEmpty || slot->Item.Definition.Type != EItemType::Weapon) {
        LOG_WARN("No weapon in slot " + std::to_string(SlotIndex));
        return false;
    }
    
    // Unequip current weapon
    for (auto& qbSlot : quickbar) {
        qbSlot.bIsActive = false;
    }
    
    // Equip new weapon
    slot->bIsActive = true;
    ActiveSlots[Player] = SlotIndex;
    
    FireWeaponEquippedCallbacks(Player, slot->Item);
    LOG_INFO("Equipped weapon: " + slot->Item.ItemId.ToString() + " in slot " + std::to_string(SlotIndex));
    return true;
}

void InventoryManager::RegisterItemDefinition(const FItemDefinition& Definition) {
    std::lock_guard<std::mutex> Lock(InventoryMutex);
    ItemDefinitions[Definition.ItemName] = Definition;
    LOG_INFO("Registered item definition: " + Definition.ItemName.ToString());
}

FItemDefinition* InventoryManager::GetItemDefinition(const FString& ItemName) {
    std::lock_guard<std::mutex> Lock(InventoryMutex);
    auto it = ItemDefinitions.find(ItemName);
    return it != ItemDefinitions.end() ? &it->second : nullptr;
}

bool InventoryManager::DoesItemExist(const FString& ItemName) const {
    std::lock_guard<std::mutex> Lock(InventoryMutex);
    return ItemDefinitions.find(ItemName) != ItemDefinitions.end();
}

void InventoryManager::InitializeDefaultItems() {
    LOG_INFO("Initializing default item definitions");
    
    // Example weapons
    FItemDefinition ar_common;
    ar_common.ItemName = "AR_Common";
    ar_common.Type = EItemType::Weapon;
    ar_common.Rarity = EItemRarity::Common;
    RegisterItemDefinition(ar_common);
    
    FItemDefinition shotgun_rare;
    shotgun_rare.ItemName = "Shotgun_Rare";
    shotgun_rare.Type = EItemType::Weapon;
    shotgun_rare.Rarity = EItemRarity::Rare;
    RegisterItemDefinition(shotgun_rare);
    
    FItemDefinition smg_epic;
    smg_epic.ItemName = "SMG_Epic";
    smg_epic.Type = EItemType::Weapon;
    smg_epic.Rarity = EItemRarity::Epic;
    RegisterItemDefinition(smg_epic);
    
    // Materials
    FItemDefinition wood;
    wood.ItemName = "Wood";
    wood.Type = EItemType::Material;
    wood.Rarity = EItemRarity::Common;
    RegisterItemDefinition(wood);
    
    FItemDefinition stone;
    stone.ItemName = "Stone";
    stone.Type = EItemType::Material;
    stone.Rarity = EItemRarity::Common;
    RegisterItemDefinition(stone);
    
    FItemDefinition metal;
    metal.ItemName = "Metal";
    metal.Type = EItemType::Material;
    metal.Rarity = EItemRarity::Common;
    RegisterItemDefinition(metal);
    
    // Consumables
    FItemDefinition shield_small;
    shield_small.ItemName = "Shield_Small";
    shield_small.Type = EItemType::Consumable;
    shield_small.Rarity = EItemRarity::Common;
    RegisterItemDefinition(shield_small);
    
    FItemDefinition shield_big;
    shield_big.ItemName = "Shield_Big";
    shield_big.Type = EItemType::Consumable;
    shield_big.Rarity = EItemRarity::Rare;
    RegisterItemDefinition(shield_big);
    
    FItemDefinition medkit;
    medkit.ItemName = "Medkit";
    medkit.Type = EItemType::Consumable;
    medkit.Rarity = EItemRarity::Uncommon;
    RegisterItemDefinition(medkit);
    
    LOG_INFO("Default item definitions initialized");
}

void InventoryManager::GiveStartingItems(AFortPlayerControllerAthena* Player) {
    if (!Player) return;
    
    LOG_INFO("Giving starting items to player");
    
    // Give basic starting items
    GiveItem(Player, "AR_Common", 1);
    GiveItem(Player, "Wood", 100);
    GiveItem(Player, "Stone", 100);
    GiveItem(Player, "Metal", 100);
}

std::vector<FInventoryItem>& InventoryManager::GetOrCreatePlayerInventory(AFortPlayerControllerAthena* Player) {
    auto it = PlayerInventories.find(Player);
    if (it == PlayerInventories.end()) {
        PlayerInventories[Player] = std::vector<FInventoryItem>();
        PlayerInventories[Player].reserve(Settings.DefaultInventorySlots);
    }
    return PlayerInventories[Player];
}

std::array<FQuickbarSlot, 10>& InventoryManager::GetOrCreatePlayerQuickbar(AFortPlayerControllerAthena* Player) {
    auto it = PlayerQuickbars.find(Player);
    if (it == PlayerQuickbars.end()) {
        PlayerQuickbars[Player] = std::array<FQuickbarSlot, 10>();
    }
    return PlayerQuickbars[Player];
}

void InventoryManager::FireItemGivenCallbacks(AFortPlayerControllerAthena* Player, const FInventoryItem& Item) {
    for (const auto& callback : ItemGivenCallbacks) {
        try {
            callback.second(Player, Item);
        } catch (...) {
            LOG_ERROR("Exception in ItemGiven callback: " + callback.first);
        }
    }
}

void InventoryManager::FireItemRemovedCallbacks(AFortPlayerControllerAthena* Player, const FInventoryItem& Item) {
    for (const auto& callback : ItemRemovedCallbacks) {
        try {
            callback.second(Player, Item);
        } catch (...) {
            LOG_ERROR("Exception in ItemRemoved callback: " + callback.first);
        }
    }
}

void InventoryManager::FireWeaponEquippedCallbacks(AFortPlayerControllerAthena* Player, const FInventoryItem& Item) {
    for (const auto& callback : WeaponEquippedCallbacks) {
        try {
            callback.second(Player, Item);
        } catch (...) {
            LOG_ERROR("Exception in WeaponEquipped callback: " + callback.first);
        }
    }
}

void InventoryManager::FireItemUsedCallbacks(AFortPlayerControllerAthena* Player, const FInventoryItem& Item) {
    for (const auto& callback : ItemUsedCallbacks) {
        try {
            callback.second(Player, Item);
        } catch (...) {
            LOG_ERROR("Exception in ItemUsed callback: " + callback.first);
        }
    }
}

void InventoryManager::RegisterItemGivenCallback(const std::string& Name, ItemGivenCallback Callback) {
    std::lock_guard<std::mutex> Lock(InventoryMutex);
    ItemGivenCallbacks[Name] = Callback;
    LOG_INFO("Registered ItemGiven callback: " + Name);
}

void InventoryManager::RegisterItemRemovedCallback(const std::string& Name, ItemRemovedCallback Callback) {
    std::lock_guard<std::mutex> Lock(InventoryMutex);
    ItemRemovedCallbacks[Name] = Callback;
    LOG_INFO("Registered ItemRemoved callback: " + Name);
}

void InventoryManager::RegisterWeaponEquippedCallback(const std::string& Name, WeaponEquippedCallback Callback) {
    std::lock_guard<std::mutex> Lock(InventoryMutex);
    WeaponEquippedCallbacks[Name] = Callback;
    LOG_INFO("Registered WeaponEquipped callback: " + Name);
}

void InventoryManager::RegisterItemUsedCallback(const std::string& Name, ItemUsedCallback Callback) {
    std::lock_guard<std::mutex> Lock(InventoryMutex);
    ItemUsedCallbacks[Name] = Callback;
    LOG_INFO("Registered ItemUsed callback: " + Name);
}

void InventoryManager::UnregisterCallback(const std::string& Name) {
    std::lock_guard<std::mutex> Lock(InventoryMutex);
    ItemGivenCallbacks.erase(Name);
    ItemRemovedCallbacks.erase(Name);
    WeaponEquippedCallbacks.erase(Name);
    ItemUsedCallbacks.erase(Name);
    LOG_INFO("Unregistered callbacks for: " + Name);
}