#pragma once

#include "Definitions.h"

// Global object array for UE4-style object management
struct FUObjectItem {
    UObject* Object;
    int32_t Flags;
    int32_t ClusterRootIndex;
    int32_t SerialNumber;
};

struct FUObjectArray {
    FUObjectItem** Objects;
    FUObjectItem* PreAllocatedObjects;
    int32_t MaxElements;
    int32_t NumElements;
    int32_t MaxChunks;
    int32_t NumChunks;
    
    UObject* GetByIndex(int32_t Index) const;
    int32_t Num() const { return NumElements; }
};

extern FUObjectArray* GObjects;

class UObject {
public:
    void** VTable;
    EObjectFlags ObjectFlags;
    int32_t InternalIndex;
    UClass* Class;
    FName Name;
    UObject* Outer;
    
    // Core UObject functions
    template<typename T = UObject>
    static T* FindObject(const std::wstring& Name);
    
    template<typename T = UObject>
    static T* FindObject(const std::string& Name);
    
    template<typename T = UObject>
    static T* LoadObject(const std::wstring& Name);
    
    static std::vector<UObject*> FindAllObjectsOfClass(UClass* Class);
    
    // Property access
    template<typename T>
    T* GetProperty(const std::string& PropertyName);
    
    template<typename T>
    T GetPropertyValue(const std::string& PropertyName);
    
    template<typename T>
    void SetPropertyValue(const std::string& PropertyName, const T& Value);
    
    // Function execution
    void ProcessEvent(UFunction* Function, void* Params = nullptr);
    
    // Object information
    std::string GetName() const;
    std::string GetFullName() const;
    std::string GetPathName() const;
    UClass* GetClass() const { return Class; }
    
    // Type checking
    bool IsA(UClass* SomeBaseClass) const;
    
    template<typename T>
    bool IsA() const {
        return IsA(T::StaticClass());
    }
    
    // Casting
    template<typename T>
    T* Cast() {
        return IsA<T>() ? static_cast<T*>(this) : nullptr;
    }
    
    template<typename T>
    const T* Cast() const {
        return IsA<T>() ? static_cast<const T*>(this) : nullptr;
    }
    
    // Offset helpers
    ptrdiff_t GetOffset(const std::string& PropertyName) const;
    
    template<typename T>
    T* GetPtr(ptrdiff_t Offset) {
        return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(this) + Offset);
    }
    
    template<typename T>
    T& Get(ptrdiff_t Offset) {
        return *GetPtr<T>(Offset);
    }
    
    // Utility functions
    bool IsValidLowLevel() const;
    bool IsDefaultSubobject() const;
    void MarkPendingKill();
    
protected:
    UObject() = default;
    virtual ~UObject() = default;
};

class UField : public UObject {
public:
    UField* Next;
    
    static UClass* StaticClass();
};

class UStruct : public UField {
public:
    UStruct* SuperStruct;
    UField* Children;
    int32_t PropertiesSize;
    int32_t MinAlignment;
    TArray<uint8_t> Script;
    UProperty* PropertyLink;
    UProperty* RefLink;
    UProperty* DestructorLink;
    UProperty* PostConstructLink;
    
    int32_t GetPropertiesSize() const { return PropertiesSize; }
    
    static UClass* StaticClass();
};

class UFunction : public UStruct {
public:
    uint32_t FunctionFlags;
    uint16_t RepOffset;
    uint8_t NumParms;
    uint16_t ParmsSize;
    uint16_t ReturnValueOffset;
    uint16_t RPCId;
    uint16_t RPCResponseId;
    UProperty* FirstPropertyToInit;
    UFunction* EventGraphFunction;
    int32_t EventGraphCallOffset;
    void* Func;
    
    static UClass* StaticClass();
};

class UProperty : public UField {
public:
    int32_t ArrayDim;
    int32_t ElementSize;
    uint64_t PropertyFlags;
    uint16_t RepIndex;
    TArray<int32_t> BlueprintReplicationCondition;
    int32_t Offset_Internal;
    FName RepNotifyFunc;
    UProperty* PropertyLinkNext;
    UProperty* NextRef;
    UProperty* DestructorLinkNext;
    UProperty* PostConstructLinkNext;
    
    int32_t GetOffset() const { return Offset_Internal; }
    
    static UClass* StaticClass();
};

class UClass : public UStruct {
public:
    // Class implementation details
    uint32_t ClassFlags;
    UClass* ClassWithin;
    UObject* ClassGeneratedBy;
    FName ClassConfigName;
    TArray<void*> ComponentTypes;
    TArray<void*> Interfaces;
    UObject* ClassDefaultObject;
    void* SparseClassData;
    TArray<void*> FunctionMap;
    TArray<void*> SuperFunctionMap;
    
    // Create a new instance of this class
    UObject* CreateDefaultObject();
    
    static UClass* StaticClass();
};

// Template implementations
template<typename T>
T* UObject::FindObject(const std::wstring& Name) {
    if (!GObjects) {
        return nullptr;
    }
    
    std::string SearchName(Name.begin(), Name.end());
    
    for (int32_t i = 0; i < GObjects->Num(); ++i) {
        UObject* Object = GObjects->GetByIndex(i);
        if (!Object || !Object->IsValidLowLevel()) continue;
        
        std::string ObjectName = Object->GetName();
        std::string FullName = Object->GetFullName();
        
        // Check both object name and full name
        if (ObjectName.find(SearchName) != std::string::npos ||
            FullName.find(SearchName) != std::string::npos) {
            
            // Try to cast to the requested type
            T* CastedObject = Object->Cast<T>();
            if (CastedObject) {
                return CastedObject;
            }
        }
    }
    return nullptr;
}

template<typename T>
T* UObject::FindObject(const std::string& Name) {
    if (!GObjects) {
        return nullptr;
    }
    
    for (int32_t i = 0; i < GObjects->Num(); ++i) {
        UObject* Object = GObjects->GetByIndex(i);
        if (!Object || !Object->IsValidLowLevel()) continue;
        
        std::string ObjectName = Object->GetName();
        std::string FullName = Object->GetFullName();
        
        // Check both object name and full name  
        if (ObjectName.find(Name) != std::string::npos ||
            FullName.find(Name) != std::string::npos) {
            
            // Try to cast to the requested type
            T* CastedObject = Object->Cast<T>();
            if (CastedObject) {
                return CastedObject;
            }
        }
    }
    return nullptr;
}

template<typename T>
T* UObject::LoadObject(const std::wstring& Name) {
    // For now, LoadObject is the same as FindObject
    // In a full implementation, this would load from disk if not found
    return FindObject<T>(Name);
}

// Property access template implementations
template<typename T>
T* UObject::GetProperty(const std::string& PropertyName) {
    ptrdiff_t Offset = GetOffset(PropertyName);
    if (Offset == -1) {
        return nullptr;
    }
    return GetPtr<T>(Offset);
}

template<typename T>
T UObject::GetPropertyValue(const std::string& PropertyName) {
    T* PropertyPtr = GetProperty<T>(PropertyName);
    if (PropertyPtr) {
        return *PropertyPtr;
    }
    return T{}; // Return default value if property not found
}

template<typename T>
void UObject::SetPropertyValue(const std::string& PropertyName, const T& Value) {
    T* PropertyPtr = GetProperty<T>(PropertyName);
    if (PropertyPtr) {
        *PropertyPtr = Value;
    }
}
