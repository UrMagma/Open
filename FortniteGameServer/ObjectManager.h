#pragma once

#include "UObject.h"
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>

/**
 * ObjectManager - Utility class to simplify object management and feature addition
 * 
 * This class provides:
 * - Easy object finding and caching
 * - Type registration and management
 * - Event system for object lifecycle
 * - Thread-safe operations
 * - Performance monitoring
 * - Batch operations
 */
class ObjectManager {
public:
    // Singleton access
    static ObjectManager& Get();
    
    // Object finding with caching
    template<typename T = UObject>
    T* FindObject(const std::string& Name, bool bUseCache = true);
    
    template<typename T = UObject>
    std::vector<T*> FindAllObjects(bool bUseCache = true);
    
    template<typename T = UObject>
    T* FindObjectByClass(UClass* Class);
    
    // Advanced searching
    template<typename T = UObject>
    std::vector<T*> FindObjectsMatching(std::function<bool(T*)> Predicate);
    
    template<typename T = UObject>
    T* FindFirstObjectMatching(std::function<bool(T*)> Predicate);
    
    // Cache management
    void InvalidateCache();
    void InvalidateCacheForType(const std::string& TypeName);
    size_t GetCacheSize() const;
    
    // Object lifecycle events
    using ObjectCreatedCallback = std::function<void(UObject*)>;
    using ObjectDestroyedCallback = std::function<void(UObject*)>;
    
    void RegisterObjectCreatedCallback(const std::string& Name, ObjectCreatedCallback Callback);
    void RegisterObjectDestroyedCallback(const std::string& Name, ObjectDestroyedCallback Callback);
    void UnregisterCallback(const std::string& Name);
    
    // Performance monitoring
    struct PerformanceStats {
        size_t TotalSearches = 0;
        size_t CacheHits = 0;
        size_t CacheMisses = 0;
        std::chrono::milliseconds TotalSearchTime{0};
        
        double GetHitRatio() const {
            return TotalSearches > 0 ? static_cast<double>(CacheHits) / TotalSearches : 0.0;
        }
        
        double GetAverageSearchTime() const {
            return TotalSearches > 0 ? TotalSearchTime.count() / static_cast<double>(TotalSearches) : 0.0;
        }
    };
    
    PerformanceStats GetStats() const { return Stats; }
    void ResetStats();
    
    // Batch operations
    template<typename T>
    void ProcessObjectsBatch(std::function<void(T*)> Processor, size_t BatchSize = 100);
    
    // Object validation
    bool IsValidObject(UObject* Object) const;
    size_t ValidateAllObjects();
    
    // Memory management helpers
    size_t GetTotalObjectCount() const;
    size_t GetObjectCountByType(const std::string& TypeName) const;
    std::vector<std::pair<std::string, size_t>> GetObjectCountsByType() const;
    
    // Debugging utilities
    void DumpObjectInfo(const std::string& OutputFile = "") const;
    void DumpCacheInfo() const;
    std::string GetObjectHierarchy(UObject* Object) const;
    
private:
    ObjectManager() = default;
    ~ObjectManager() = default;
    
    // Non-copyable
    ObjectManager(const ObjectManager&) = delete;
    ObjectManager& operator=(const ObjectManager&) = delete;
    
    // Internal cache management
    struct CacheEntry {
        std::weak_ptr<UObject> Object;
        std::chrono::steady_clock::time_point LastAccess;
        size_t AccessCount = 0;
    };
    
    mutable std::mutex CacheMutex;
    std::unordered_map<std::string, std::vector<CacheEntry>> ObjectCache;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> CacheTimestamps;
    
    // Callbacks
    mutable std::mutex CallbackMutex;
    std::unordered_map<std::string, ObjectCreatedCallback> CreatedCallbacks;
    std::unordered_map<std::string, ObjectDestroyedCallback> DestroyedCallbacks;
    
    // Performance tracking
    mutable std::mutex StatsMutex;
    mutable PerformanceStats Stats;
    
    // Internal helpers
    template<typename T>
    std::vector<T*> SearchObjectsInternal(std::function<bool(T*)> Filter = nullptr);
    
    void UpdateCacheEntry(const std::string& Key, UObject* Object);
    void CleanupExpiredEntries();
    
    static constexpr size_t MAX_CACHE_SIZE = 10000;
    static constexpr std::chrono::minutes CACHE_CLEANUP_INTERVAL{5};
};

/**
 * TypeRegistry - Utility class for managing UE4 types and their relationships
 */
class TypeRegistry {
public:
    static TypeRegistry& Get();
    
    // Type registration
    void RegisterType(const std::string& TypeName, UClass* Class);
    void RegisterTypeAlias(const std::string& Alias, const std::string& ActualTypeName);
    
    // Type queries
    UClass* GetClass(const std::string& TypeName) const;
    std::string GetTypeName(UClass* Class) const;
    bool IsTypeRegistered(const std::string& TypeName) const;
    
    // Type relationships
    bool IsSubclassOf(const std::string& ChildType, const std::string& ParentType) const;
    bool IsSubclassOf(UClass* ChildClass, UClass* ParentClass) const;
    std::vector<std::string> GetSubclasses(const std::string& ParentType) const;
    std::vector<std::string> GetAllTypes() const;
    
    // Type information
    struct TypeInfo {
        std::string Name;
        UClass* Class = nullptr;
        std::string ParentType;
        std::vector<std::string> Children;
        size_t InstanceCount = 0;
        bool bIsNative = false;
        std::unordered_map<std::string, std::string> Properties;
    };
    
    TypeInfo GetTypeInfo(const std::string& TypeName) const;
    void UpdateInstanceCounts();
    
    // Reflection helpers
    std::vector<std::string> GetTypeProperties(const std::string& TypeName) const;
    std::string GetPropertyType(const std::string& TypeName, const std::string& PropertyName) const;
    
    // Type creation helpers
    template<typename T>
    void RegisterNativeType(const std::string& TypeName);
    
    void DumpTypeHierarchy(const std::string& OutputFile = "") const;
    
private:
    TypeRegistry() = default;
    ~TypeRegistry() = default;
    
    TypeRegistry(const TypeRegistry&) = delete;
    TypeRegistry& operator=(const TypeRegistry&) = delete;
    
    mutable std::mutex RegistryMutex;
    std::unordered_map<std::string, UClass*> TypeMap;
    std::unordered_map<std::string, std::string> TypeAliases;
    std::unordered_map<UClass*, std::string> ReverseTypeMap;
    std::unordered_map<std::string, TypeInfo> TypeInfoMap;
};

// Template implementations for ObjectManager
template<typename T>
T* ObjectManager::FindObject(const std::string& Name, bool bUseCache) {
    auto start = std::chrono::steady_clock::now();
    
    // Check cache first if enabled
    if (bUseCache) {
        std::lock_guard<std::mutex> Lock(CacheMutex);
        auto it = ObjectCache.find(Name);
        if (it != ObjectCache.end() && !it->second.empty()) {
            for (auto& entry : it->second) {
                if (auto obj = entry.Object.lock()) {
                    T* castedObj = static_cast<T*>(obj.get());
                    if (castedObj && castedObj->IsValidLowLevel()) {
                        entry.LastAccess = std::chrono::steady_clock::now();
                        entry.AccessCount++;
                        
                        std::lock_guard<std::mutex> StatsLock(StatsMutex);
                        Stats.CacheHits++;
                        Stats.TotalSearches++;
                        return castedObj;
                    }
                }
            }
        }
    }
    
    // Search in global object array
    T* result = UObject::FindObject<T>(Name);
    
    // Update cache if found
    if (result && bUseCache) {
        UpdateCacheEntry(Name, result);
    }
    
    // Update stats
    {
        std::lock_guard<std::mutex> StatsLock(StatsMutex);
        Stats.TotalSearches++;
        if (!result) {
            Stats.CacheMisses++;
        }
        Stats.TotalSearchTime += std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);
    }
    
    return result;
}

template<typename T>
std::vector<T*> ObjectManager::FindAllObjects(bool bUseCache) {
    return SearchObjectsInternal<T>();
}

template<typename T>
std::vector<T*> ObjectManager::SearchObjectsInternal(std::function<bool(T*)> Filter) {
    std::vector<T*> Results;
    
    if (!GObjects) {
        return Results;
    }
    
    for (int32_t i = 0; i < GObjects->Num(); ++i) {
        UObject* Object = GObjects->GetByIndex(i);
        if (!Object || !Object->IsValidLowLevel()) continue;
        
        T* CastedObject = Object->Cast<T>();
        if (CastedObject && (!Filter || Filter(CastedObject))) {
            Results.push_back(CastedObject);
        }
    }
    
    return Results;
}

template<typename T>
std::vector<T*> ObjectManager::FindObjectsMatching(std::function<bool(T*)> Predicate) {
    return SearchObjectsInternal<T>(Predicate);
}

template<typename T>
T* ObjectManager::FindFirstObjectMatching(std::function<bool(T*)> Predicate) {
    auto results = FindObjectsMatching<T>(Predicate);
    return results.empty() ? nullptr : results[0];
}

template<typename T>
void ObjectManager::ProcessObjectsBatch(std::function<void(T*)> Processor, size_t BatchSize) {
    if (!Processor || BatchSize == 0) return;
    
    std::vector<T*> objects = FindAllObjects<T>();
    
    for (size_t i = 0; i < objects.size(); i += BatchSize) {
        size_t end = std::min(i + BatchSize, objects.size());
        
        for (size_t j = i; j < end; ++j) {
            if (objects[j] && objects[j]->IsValidLowLevel()) {
                try {
                    Processor(objects[j]);
                } catch (...) {
                    LOG_ERROR("Exception in batch processor for object: " + objects[j]->GetName());
                }
            }
        }
        
        // Optional: yield between batches to prevent blocking
        std::this_thread::yield();
    }
}

template<typename T>
void TypeRegistry::RegisterNativeType(const std::string& TypeName) {
    std::lock_guard<std::mutex> Lock(RegistryMutex);
    
    UClass* Class = T::StaticClass();
    if (Class) {
        TypeMap[TypeName] = Class;
        ReverseTypeMap[Class] = TypeName;
        
        TypeInfo& Info = TypeInfoMap[TypeName];
        Info.Name = TypeName;
        Info.Class = Class;
        Info.bIsNative = true;
        
        LOG_INFO("Registered native type: " + TypeName);
    }
}