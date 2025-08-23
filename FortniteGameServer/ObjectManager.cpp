#include "ObjectManager.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>

// ObjectManager Implementation
ObjectManager& ObjectManager::Get() {
    static ObjectManager Instance;
    return Instance;
}

void ObjectManager::InvalidateCache() {
    std::lock_guard<std::mutex> Lock(CacheMutex);
    ObjectCache.clear();
    CacheTimestamps.clear();
    LOG_INFO("Object cache invalidated");
}

void ObjectManager::InvalidateCacheForType(const std::string& TypeName) {
    std::lock_guard<std::mutex> Lock(CacheMutex);
    
    auto it = ObjectCache.find(TypeName);
    if (it != ObjectCache.end()) {
        ObjectCache.erase(it);
        LOG_INFO("Cache invalidated for type: " + TypeName);
    }
}

size_t ObjectManager::GetCacheSize() const {
    std::lock_guard<std::mutex> Lock(CacheMutex);
    size_t totalSize = 0;
    for (const auto& pair : ObjectCache) {
        totalSize += pair.second.size();
    }
    return totalSize;
}

void ObjectManager::RegisterObjectCreatedCallback(const std::string& Name, ObjectCreatedCallback Callback) {
    std::lock_guard<std::mutex> Lock(CallbackMutex);
    CreatedCallbacks[Name] = Callback;
    LOG_INFO("Registered object created callback: " + Name);
}

void ObjectManager::RegisterObjectDestroyedCallback(const std::string& Name, ObjectDestroyedCallback Callback) {
    std::lock_guard<std::mutex> Lock(CallbackMutex);
    DestroyedCallbacks[Name] = Callback;
    LOG_INFO("Registered object destroyed callback: " + Name);
}

void ObjectManager::UnregisterCallback(const std::string& Name) {
    std::lock_guard<std::mutex> Lock(CallbackMutex);
    CreatedCallbacks.erase(Name);
    DestroyedCallbacks.erase(Name);
    LOG_INFO("Unregistered callbacks for: " + Name);
}

void ObjectManager::ResetStats() {
    std::lock_guard<std::mutex> Lock(StatsMutex);
    Stats = PerformanceStats{};
    LOG_INFO("Performance statistics reset");
}

bool ObjectManager::IsValidObject(UObject* Object) const {
    if (!Object) return false;
    
    try {
        return Object->IsValidLowLevel() && 
               Object->GetClass() != nullptr &&
               reinterpret_cast<uintptr_t>(Object) > 0x10000;
    } catch (...) {
        return false;
    }
}

size_t ObjectManager::ValidateAllObjects() {
    size_t validCount = 0;
    
    if (!GObjects) {
        LOG_WARN("GObjects is null during validation");
        return 0;
    }
    
    for (int32_t i = 0; i < GObjects->Num(); ++i) {
        UObject* Object = GObjects->GetByIndex(i);
        if (IsValidObject(Object)) {
            validCount++;
        }
    }
    
    LOG_INFO("Validated " + std::to_string(validCount) + " objects out of " + std::to_string(GObjects->Num()));
    return validCount;
}

size_t ObjectManager::GetTotalObjectCount() const {
    return GObjects ? GObjects->Num() : 0;
}

size_t ObjectManager::GetObjectCountByType(const std::string& TypeName) const {
    if (!GObjects) return 0;
    
    UClass* TargetClass = TypeRegistry::Get().GetClass(TypeName);
    if (!TargetClass) return 0;
    
    size_t count = 0;
    for (int32_t i = 0; i < GObjects->Num(); ++i) {
        UObject* Object = GObjects->GetByIndex(i);
        if (Object && Object->IsValidLowLevel() && Object->IsA(TargetClass)) {
            count++;
        }
    }
    
    return count;
}

std::vector<std::pair<std::string, size_t>> ObjectManager::GetObjectCountsByType() const {
    std::unordered_map<std::string, size_t> typeCounts;
    
    if (!GObjects) {
        return {};
    }
    
    for (int32_t i = 0; i < GObjects->Num(); ++i) {
        UObject* Object = GObjects->GetByIndex(i);
        if (Object && Object->IsValidLowLevel() && Object->GetClass()) {
            std::string typeName = Object->GetClass()->GetName();
            typeCounts[typeName]++;
        }
    }
    
    std::vector<std::pair<std::string, size_t>> result;
    for (const auto& pair : typeCounts) {
        result.emplace_back(pair.first, pair.second);
    }
    
    // Sort by count (descending)
    std::sort(result.begin(), result.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    return result;
}

void ObjectManager::DumpObjectInfo(const std::string& OutputFile) const {
    std::ostringstream oss;
    
    oss << "=== Object Manager Report ===\n\n";
    
    // Basic statistics
    oss << "Total Objects: " << GetTotalObjectCount() << "\n";
    oss << "Valid Objects: " << const_cast<ObjectManager*>(this)->ValidateAllObjects() << "\n";
    oss << "Cache Size: " << GetCacheSize() << "\n\n";
    
    // Performance statistics
    auto stats = GetStats();
    oss << "Performance Statistics:\n";
    oss << "  Total Searches: " << stats.TotalSearches << "\n";
    oss << "  Cache Hits: " << stats.CacheHits << "\n";
    oss << "  Cache Misses: " << stats.CacheMisses << "\n";
    oss << "  Hit Ratio: " << std::fixed << std::setprecision(2) << stats.GetHitRatio() * 100 << "%\n";
    oss << "  Average Search Time: " << std::fixed << std::setprecision(2) << stats.GetAverageSearchTime() << "ms\n\n";
    
    // Object counts by type
    oss << "Object Counts by Type:\n";
    auto typeCounts = GetObjectCountsByType();
    for (const auto& pair : typeCounts) {
        oss << "  " << pair.first << ": " << pair.second << "\n";
    }
    
    std::string output = oss.str();
    
    if (OutputFile.empty()) {
        LOG_INFO("Object Manager Report:\n" + output);
    } else {
        std::ofstream file(OutputFile);
        if (file) {
            file << output;
            LOG_INFO("Object report written to: " + OutputFile);
        } else {
            LOG_ERROR("Failed to write report to: " + OutputFile);
        }
    }
}

void ObjectManager::DumpCacheInfo() const {
    std::lock_guard<std::mutex> Lock(CacheMutex);
    
    std::ostringstream oss;
    oss << "=== Cache Information ===\n";
    oss << "Total Cache Entries: " << ObjectCache.size() << "\n\n";
    
    for (const auto& pair : ObjectCache) {
        oss << "Cache Key: " << pair.first << " (Entries: " << pair.second.size() << ")\n";
        for (size_t i = 0; i < pair.second.size(); ++i) {
            const auto& entry = pair.second[i];
            auto obj = entry.Object.lock();
            oss << "  [" << i << "] ";
            if (obj) {
                oss << "Valid - Access Count: " << entry.AccessCount;
            } else {
                oss << "Expired";
            }
            oss << "\n";
        }
        oss << "\n";
    }
    
    LOG_INFO(oss.str());
}

std::string ObjectManager::GetObjectHierarchy(UObject* Object) const {
    if (!Object) return "null";
    
    std::vector<std::string> hierarchy;
    UObject* current = Object;
    
    while (current) {
        std::string name = current->GetName();
        std::string className = current->GetClass() ? current->GetClass()->GetName() : "Unknown";
        hierarchy.push_back(className + " '" + name + "'");
        current = current->Outer;
    }
    
    std::ostringstream oss;
    for (int i = static_cast<int>(hierarchy.size()) - 1; i >= 0; --i) {
        if (i < static_cast<int>(hierarchy.size()) - 1) {
            oss << ".";
        }
        oss << hierarchy[i];
    }
    
    return oss.str();
}

void ObjectManager::UpdateCacheEntry(const std::string& Key, UObject* Object) {
    std::lock_guard<std::mutex> Lock(CacheMutex);
    
    if (ObjectCache.size() >= MAX_CACHE_SIZE) {
        CleanupExpiredEntries();
        
        // If still at max size, remove oldest entry
        if (ObjectCache.size() >= MAX_CACHE_SIZE) {
            ObjectCache.erase(ObjectCache.begin());
        }
    }
    
    CacheEntry entry;
    entry.Object = std::shared_ptr<UObject>(Object, [](UObject*) {}); // Non-owning shared_ptr
    entry.LastAccess = std::chrono::steady_clock::now();
    entry.AccessCount = 1;
    
    ObjectCache[Key].push_back(entry);
}

void ObjectManager::CleanupExpiredEntries() {
    // This method assumes CacheMutex is already locked
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = ObjectCache.begin(); it != ObjectCache.end();) {
        auto& entries = it->second;
        
        // Remove expired entries
        entries.erase(
            std::remove_if(entries.begin(), entries.end(),
                [](const CacheEntry& entry) {
                    return entry.Object.expired();
                }),
            entries.end()
        );
        
        // Remove empty cache keys
        if (entries.empty()) {
            it = ObjectCache.erase(it);
        } else {
            ++it;
        }
    }
}

// TypeRegistry Implementation
TypeRegistry& TypeRegistry::Get() {
    static TypeRegistry Instance;
    return Instance;
}

void TypeRegistry::RegisterType(const std::string& TypeName, UClass* Class) {
    std::lock_guard<std::mutex> Lock(RegistryMutex);
    
    TypeMap[TypeName] = Class;
    ReverseTypeMap[Class] = TypeName;
    
    TypeInfo& Info = TypeInfoMap[TypeName];
    Info.Name = TypeName;
    Info.Class = Class;
    
    LOG_INFO("Registered type: " + TypeName);
}

void TypeRegistry::RegisterTypeAlias(const std::string& Alias, const std::string& ActualTypeName) {
    std::lock_guard<std::mutex> Lock(RegistryMutex);
    
    TypeAliases[Alias] = ActualTypeName;
    LOG_INFO("Registered type alias: " + Alias + " -> " + ActualTypeName);
}

UClass* TypeRegistry::GetClass(const std::string& TypeName) const {
    std::lock_guard<std::mutex> Lock(RegistryMutex);
    
    // Check aliases first
    auto aliasIt = TypeAliases.find(TypeName);
    std::string actualTypeName = aliasIt != TypeAliases.end() ? aliasIt->second : TypeName;
    
    auto it = TypeMap.find(actualTypeName);
    return it != TypeMap.end() ? it->second : nullptr;
}

std::string TypeRegistry::GetTypeName(UClass* Class) const {
    std::lock_guard<std::mutex> Lock(RegistryMutex);
    
    auto it = ReverseTypeMap.find(Class);
    return it != ReverseTypeMap.end() ? it->second : "Unknown";
}

bool TypeRegistry::IsTypeRegistered(const std::string& TypeName) const {
    return GetClass(TypeName) != nullptr;
}

bool TypeRegistry::IsSubclassOf(const std::string& ChildType, const std::string& ParentType) const {
    UClass* ChildClass = GetClass(ChildType);
    UClass* ParentClass = GetClass(ParentType);
    
    return IsSubclassOf(ChildClass, ParentClass);
}

bool TypeRegistry::IsSubclassOf(UClass* ChildClass, UClass* ParentClass) const {
    if (!ChildClass || !ParentClass) return false;
    
    UClass* CurrentClass = ChildClass;
    while (CurrentClass) {
        if (CurrentClass == ParentClass) {
            return true;
        }
        CurrentClass = static_cast<UClass*>(CurrentClass->SuperStruct);
    }
    
    return false;
}

std::vector<std::string> TypeRegistry::GetSubclasses(const std::string& ParentType) const {
    std::vector<std::string> subclasses;
    UClass* ParentClass = GetClass(ParentType);
    
    if (!ParentClass) return subclasses;
    
    std::lock_guard<std::mutex> Lock(RegistryMutex);
    for (const auto& pair : TypeMap) {
        if (IsSubclassOf(pair.second, ParentClass) && pair.second != ParentClass) {
            subclasses.push_back(pair.first);
        }
    }
    
    return subclasses;
}

std::vector<std::string> TypeRegistry::GetAllTypes() const {
    std::lock_guard<std::mutex> Lock(RegistryMutex);
    
    std::vector<std::string> types;
    for (const auto& pair : TypeMap) {
        types.push_back(pair.first);
    }
    
    std::sort(types.begin(), types.end());
    return types;
}

TypeRegistry::TypeInfo TypeRegistry::GetTypeInfo(const std::string& TypeName) const {
    std::lock_guard<std::mutex> Lock(RegistryMutex);
    
    auto it = TypeInfoMap.find(TypeName);
    if (it != TypeInfoMap.end()) {
        return it->second;
    }
    
    return TypeInfo{};
}

void TypeRegistry::UpdateInstanceCounts() {
    if (!GObjects) return;
    
    std::lock_guard<std::mutex> Lock(RegistryMutex);
    
    // Reset all counts
    for (auto& pair : TypeInfoMap) {
        pair.second.InstanceCount = 0;
    }
    
    // Count instances
    for (int32_t i = 0; i < GObjects->Num(); ++i) {
        UObject* Object = GObjects->GetByIndex(i);
        if (Object && Object->IsValidLowLevel() && Object->GetClass()) {
            std::string typeName = GetTypeName(Object->GetClass());
            if (!typeName.empty() && typeName != "Unknown") {
                TypeInfoMap[typeName].InstanceCount++;
            }
        }
    }
}

void TypeRegistry::DumpTypeHierarchy(const std::string& OutputFile) const {
    std::ostringstream oss;
    
    oss << "=== Type Registry Report ===\n\n";
    
    auto types = GetAllTypes();
    oss << "Total Registered Types: " << types.size() << "\n\n";
    
    oss << "Type Hierarchy:\n";
    for (const auto& typeName : types) {
        TypeInfo info = GetTypeInfo(typeName);
        oss << typeName;
        if (info.bIsNative) {
            oss << " (Native)";
        }
        oss << " - Instances: " << info.InstanceCount << "\n";
        
        auto subclasses = GetSubclasses(typeName);
        if (!subclasses.empty()) {
            oss << "  Subclasses: ";
            for (size_t i = 0; i < subclasses.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << subclasses[i];
            }
            oss << "\n";
        }
        oss << "\n";
    }
    
    std::string output = oss.str();
    
    if (OutputFile.empty()) {
        LOG_INFO("Type Registry Report:\n" + output);
    } else {
        std::ofstream file(OutputFile);
        if (file) {
            file << output;
            LOG_INFO("Type registry report written to: " + OutputFile);
        } else {
            LOG_ERROR("Failed to write type registry report to: " + OutputFile);
        }
    }
}