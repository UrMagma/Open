#include "UObject.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

// Global object array
FUObjectArray* GObjects = nullptr;

// UObject implementations
std::string UObject::GetName() const {
    return FNameToString_Safe(Name);
}

std::string UObject::GetFullName() const {
    std::string ClassName = Class ? Class->GetName() : "Unknown";
    std::string ObjectName = GetName();
    std::string OuterName = Outer ? Outer->GetName() : "None";
    
    return ClassName + " " + OuterName + "." + ObjectName;
}

std::string UObject::GetPathName() const {
    if (!Outer) {
        return GetName();
    }
    return Outer->GetPathName() + "." + GetName();
}

ptrdiff_t UObject::GetOffset(const std::string& PropertyName) const {
    // This would normally use UE4's reflection system to find property offsets
    LOG_WARN("GetOffset not fully implemented for property: " + PropertyName);
    return -1;
}

bool UObject::IsValidLowLevel() const {
    return this != nullptr && 
           Class != nullptr && 
           reinterpret_cast<uintptr_t>(this) > 0x10000; // Basic pointer validation
}

bool UObject::IsDefaultSubobject() const {
    return (static_cast<uint32_t>(ObjectFlags) & static_cast<uint32_t>(EObjectFlags::RF_DefaultSubObject)) != 0;
}

void UObject::MarkPendingKill() {
    ObjectFlags = static_cast<EObjectFlags>(
        static_cast<uint32_t>(ObjectFlags) | 
        static_cast<uint32_t>(EObjectFlags::RF_BeginDestroyed)
    );
}

// Static helper functions
std::vector<UObject*> UObject::FindAllObjectsOfClass(UClass* TargetClass) {
    std::vector<UObject*> FoundObjects;
    
    if (!GObjects || !TargetClass) {
        return FoundObjects;
    }
    
    for (int32_t i = 0; i < GObjects->Num(); ++i) {
        UObject* Object = GObjects->GetByIndex(i);
        if (!Object || !Object->IsValidLowLevel()) continue;
        
        if (Object->IsA(TargetClass)) {
            FoundObjects.push_back(Object);
        }
    }
    
    LOG_INFO("Found " + std::to_string(FoundObjects.size()) + " objects of class " + TargetClass->GetName());
    return FoundObjects;
}

bool UObject::IsA(UClass* SomeBaseClass) const {
    if (!Class || !SomeBaseClass) return false;
    
    UClass* CurrentClass = Class;
    while (CurrentClass) {
        if (CurrentClass == SomeBaseClass) {
            return true;
        }
        // Cast SuperStruct to UClass since all classes inherit from UClass
        CurrentClass = static_cast<UClass*>(CurrentClass->SuperStruct);
    }
    
    return false;
}

void UObject::ProcessEvent(UFunction* Function, void* Params) {
    if (!Function) {
        LOG_ERROR("ProcessEvent called with null function on object: " + GetName());
        return;
    }
    
    if (!::ProcessEvent) {
        LOG_ERROR("Global ProcessEvent function pointer is null");
        return;
    }
    
    try {
        ::ProcessEvent(this, Function, Params);
    } catch (...) {
        LOG_ERROR("Exception occurred in ProcessEvent for function: " + Function->GetName() + " on object: " + GetName());
    }
}

// UObject array implementation
UObject* FUObjectArray::GetByIndex(int32_t Index) const {
    if (!Objects) {
        LOG_ERROR("Objects array is null");
        return nullptr;
    }
    
    if (Index < 0 || Index >= NumElements) {
        LOG_WARN("Index out of bounds: " + std::to_string(Index) + " (max: " + std::to_string(NumElements) + ")");
        return nullptr;
    }
    
    // UE4 uses a chunked array system
    int32_t ChunkIndex = Index / 65536;
    int32_t WithinChunkIndex = Index % 65536;
    
    if (ChunkIndex >= MaxChunks || !Objects[ChunkIndex]) {
        LOG_WARN("Invalid chunk at index " + std::to_string(ChunkIndex));
        return nullptr;
    }
    
    FUObjectItem& Item = Objects[ChunkIndex][WithinChunkIndex];
    if (!Item.Object) {
        return nullptr; // This is normal for freed slots
    }
    
    return Item.Object;
}

// Helper function to find objects in the global array
template<typename T>
static T* FindObjectInternal(const std::string& Name) {
    if (!GObjects) {
        LOG_WARN("GObjects is null, cannot find object: " + Name);
        return nullptr;
    }
    
    for (int32_t i = 0; i < GObjects->Num(); ++i) {
        UObject* Object = GObjects->GetByIndex(i);
        if (!Object || !Object->IsValidLowLevel()) continue;
        
        std::string ObjectName = Object->GetName();
        if (ObjectName.find(Name) != std::string::npos) {
            T* CastedObject = Object->Cast<T>();
            if (CastedObject) {
                return CastedObject;
            }
        }
    }
    
    LOG_WARN("Object not found: " + Name);
    return nullptr;
}

// Static class implementations
UClass* UField::StaticClass() {
    static UClass* Class = nullptr;
    if (!Class) {
        Class = FindObjectInternal<UClass>("Field");
    }
    return Class;
}

UClass* UStruct::StaticClass() {
    static UClass* Class = nullptr;
    if (!Class) {
        Class = FindObjectInternal<UClass>("Struct");
    }
    return Class;
}

UClass* UFunction::StaticClass() {
    static UClass* Class = nullptr;
    if (!Class) {
        Class = FindObjectInternal<UClass>("Function");
    }
    return Class;
}

UClass* UProperty::StaticClass() {
    static UClass* Class = nullptr;
    if (!Class) {
        Class = FindObjectInternal<UClass>("Property");
    }
    return Class;
}

UClass* UClass::StaticClass() {
    static UClass* Class = nullptr;
    if (!Class) {
        Class = FindObjectInternal<UClass>("Class");
    }
    return Class;
}

UObject* UClass::CreateDefaultObject() {
    // Check if we already have a default object
    if (ClassDefaultObject) {
        return ClassDefaultObject;
    }
    
    LOG_WARN("CreateDefaultObject not fully implemented for class: " + GetName());
    
    // This would normally allocate memory for a new instance of this class
    // and call the appropriate constructors. For now, we return nullptr
    // as this requires deep engine integration.
    return nullptr;
}

// FVector method implementations
float FVector::Size() const {
    return sqrtf(X * X + Y * Y + Z * Z);
}

FVector FVector::GetSafeNormal(float Tolerance) const {
    const float SquareSum = X * X + Y * Y + Z * Z;
    
    if (SquareSum > Tolerance) {
        const float Scale = 1.0f / sqrtf(SquareSum);
        return FVector(X * Scale, Y * Scale, Z * Scale);
    }
    
    return FVector(0, 0, 0);
}

// FRotator method implementations
FQuat FRotator::Quaternion() const {
    const float DEG_TO_RAD = 3.14159265359f / 180.0f;
    const float DIVIDE_BY_2 = DEG_TO_RAD / 2.0f;
    
    float SP, SY, SR;
    float CP, CY, CR;
    
    SP = sinf(Pitch * DIVIDE_BY_2);
    CP = cosf(Pitch * DIVIDE_BY_2);
    SY = sinf(Yaw * DIVIDE_BY_2);
    CY = cosf(Yaw * DIVIDE_BY_2);
    SR = sinf(Roll * DIVIDE_BY_2);
    CR = cosf(Roll * DIVIDE_BY_2);
    
    FQuat RotationQuat;
    RotationQuat.X = CR * SP * SY - SR * CP * CY;
    RotationQuat.Y = -CR * SP * CY - SR * CP * SY;
    RotationQuat.Z = CR * CP * SY - SR * SP * CY;
    RotationQuat.W = CR * CP * CY + SR * SP * SY;
    
    return RotationQuat;
}

// FQuat method implementations
FRotator FQuat::Rotator() const {
    const float RAD_TO_DEG = 180.0f / 3.14159265359f;
    const float SingularityTest = Z * X - W * Y;
    const float YawY = 2.0f * (W * Z + X * Y);
    const float YawX = (1.0f - 2.0f * (Y * Y + Z * Z));
    
    const float SINGULARITY_THRESHOLD = 0.4999995f;
    FRotator RotatorFromQuat;
    
    if (SingularityTest < -SINGULARITY_THRESHOLD) {
        RotatorFromQuat.Pitch = -90.0f;
        RotatorFromQuat.Yaw = atan2f(YawY, YawX) * RAD_TO_DEG;
        RotatorFromQuat.Roll = -RotatorFromQuat.Yaw - (2.0f * atan2f(X, W) * RAD_TO_DEG);
    } else if (SingularityTest > SINGULARITY_THRESHOLD) {
        RotatorFromQuat.Pitch = 90.0f;
        RotatorFromQuat.Yaw = atan2f(YawY, YawX) * RAD_TO_DEG;
        RotatorFromQuat.Roll = RotatorFromQuat.Yaw - (2.0f * atan2f(X, W) * RAD_TO_DEG);
    } else {
        RotatorFromQuat.Pitch = asinf(2.0f * SingularityTest) * RAD_TO_DEG;
        RotatorFromQuat.Yaw = atan2f(YawY, YawX) * RAD_TO_DEG;
        RotatorFromQuat.Roll = atan2f(2.0f * (W * X + Y * Z), (1.0f - 2.0f * (X * X + Y * Y))) * RAD_TO_DEG;
    }
    
    return RotatorFromQuat;
}
