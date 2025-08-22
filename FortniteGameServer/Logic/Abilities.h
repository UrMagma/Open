#pragma once

#include "../FortniteClasses.h"
#include "../Native.h"

namespace Abilities {
    
    /**
     * Apply default abilities to a player pawn
     */
    void ApplyAbilities(AFortPlayerPawnAthena* Pawn);
    
    /**
     * Give specific ability to player
     */
    bool GiveAbility(AFortPlayerPawnAthena* Pawn, UClass* AbilityClass);
    
    /**
     * Try to activate ability by class
     */
    bool TryActivateAbility(AFortPlayerPawnAthena* Pawn, UClass* AbilityClass);
    
    /**
     * Remove ability from player
     */
    bool RemoveAbility(AFortPlayerPawnAthena* Pawn, UClass* AbilityClass);
    
    /**
     * Get player's ability system component
     */
    UAbilitySystemComponent* GetAbilitySystemComponent(AFortPlayerPawnAthena* Pawn);
    
    /**
     * Initialize ability system for player
     */
    void InitializeAbilitySystem(AFortPlayerPawnAthena* Pawn);
    
    /**
     * Clear all abilities from player
     */
    void ClearAllAbilities(AFortPlayerPawnAthena* Pawn);
    
    // Ability sets
    namespace AbilitySets {
        extern UFortAbilitySet* DefaultPlayerAbilitySet;
        extern UFortAbilitySet* AthenaPlayerAbilitySet;
        
        // Initialize ability set cache
        void Initialize();
    }
    
    // Common ability classes
    namespace AbilityClasses {
        extern UClass* Jump;
        extern UClass* Sprint;
        extern UClass* Crouch;
        extern UClass* Build;
        extern UClass* Edit;
        extern UClass* Harvest;
        extern UClass* WeaponFire;
        extern UClass* WeaponReload;
        extern UClass* WeaponAim;
        extern UClass* Emote;
        extern UClass* Interact;
        
        // Initialize ability class cache
        void Initialize();
    }
}
