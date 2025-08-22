#pragma once

#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <format>

// Forward declarations
class UObject;
class UClass;
class UFunction;
class UProperty;
class UStruct;
class UWorld;
class UEngine;
class APlayerController;
class APawn;
class AGameModeBase;
class AGameStateBase;

// Basic types
using int8 = int8_t;
using uint8 = uint8_t;
using int16 = int16_t;
using uint16 = uint16_t;
using int32 = int32_t;
using uint32 = uint32_t;
using int64 = int64_t;
using uint64 = uint64_t;

// UE4 Basic Types
struct FName {
    uint32_t ComparisonIndex;
    uint32_t Number;
    
    FName() : ComparisonIndex(0), Number(0) {}
    FName(uint32_t Index) : ComparisonIndex(Index), Number(0) {}
    
    std::string ToString() const;
};

struct FString {
    wchar_t* Data;
    int32_t ArrayNum;
    int32_t ArrayMax;
    
    FString() : Data(nullptr), ArrayNum(0), ArrayMax(0) {}
    FString(const wchar_t* Str);
    ~FString();
    
    std::string ToString() const;
    std::wstring ToWString() const;
};

struct FVector {
    float X, Y, Z;
    
    FVector() : X(0), Y(0), Z(0) {}
    FVector(float x, float y, float z) : X(x), Y(y), Z(z) {}
    
    FVector operator+(const FVector& V) const { return FVector(X + V.X, Y + V.Y, Z + V.Z); }
    FVector operator-(const FVector& V) const { return FVector(X - V.X, Y - V.Y, Z - V.Z); }
    FVector operator*(float Scale) const { return FVector(X * Scale, Y * Scale, Z * Scale); }
    
    float Size() const;
    FVector GetSafeNormal(float Tolerance = 1.e-8f) const;
};

struct FRotator {
    float Pitch, Yaw, Roll;
    
    FRotator() : Pitch(0), Yaw(0), Roll(0) {}
    FRotator(float pitch, float yaw, float roll) : Pitch(pitch), Yaw(yaw), Roll(roll) {}
    
    class FQuat Quaternion() const;
};

struct FQuat {
    float X, Y, Z, W;
    
    FQuat() : X(0), Y(0), Z(0), W(1) {}
    FQuat(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w) {}
    
    FRotator Rotator() const;
};

struct FTransform {
    FQuat Rotation;
    FVector Translation;
    FVector Scale3D;
    
    FTransform() : Rotation(), Translation(), Scale3D(1, 1, 1) {}
    FTransform(const FQuat& InRotation, const FVector& InTranslation, const FVector& InScale3D = FVector(1, 1, 1))
        : Rotation(InRotation), Translation(InTranslation), Scale3D(InScale3D) {}
};

template<typename T>
struct TArray {
    T* Data;
    int32_t ArrayNum;
    int32_t ArrayMax;
    
    TArray() : Data(nullptr), ArrayNum(0), ArrayMax(0) {}
    
    T& operator[](int32_t Index) { return Data[Index]; }
    const T& operator[](int32_t Index) const { return Data[Index]; }
    
    T& At(int32_t Index) { return Data[Index]; }
    const T& At(int32_t Index) const { return Data[Index]; }
    
    int32_t Num() const { return ArrayNum; }
    int32_t Max() const { return ArrayMax; }
    
    bool IsValidIndex(int32_t Index) const { return Index >= 0 && Index < ArrayNum; }
    
    void Add(const T& Item);
    void Remove(int32_t Index);
    void Empty();
};

// Function pointer types
extern void* (*ProcessEvent)(UObject* Object, UFunction* Function, void* Params);
extern void* (*FMemory_Malloc)(size_t Size, uint32_t Alignment = 0);
extern void* (*FMemory_Realloc)(void* Ptr, size_t NewSize, uint32_t Alignment = 0);
extern void (*FMemory_Free)(void* Ptr);
extern void (*FNameToString)(FName* Name, FString* OutString);

// Global engine pointers
extern uintptr_t Imagebase;

// Enums
enum class ENetRole : uint8_t {
    ROLE_None = 0,
    ROLE_SimulatedProxy = 1,
    ROLE_AutonomousProxy = 2,
    ROLE_Authority = 3,
    ROLE_MAX = 4
};

enum class ENetMode : uint8_t {
    NM_Standalone = 0,
    NM_DedicatedServer = 1,
    NM_ListenServer = 2,
    NM_Client = 3,
    NM_MAX = 4
};

enum class EObjectFlags : uint32_t {
    RF_NoFlags = 0x00000000,
    RF_Public = 0x00000001,
    RF_Standalone = 0x00000002,
    RF_MarkAsNative = 0x00000004,
    RF_Transactional = 0x00000008,
    RF_ClassDefaultObject = 0x00000010,
    RF_ArchetypeObject = 0x00000020,
    RF_Transient = 0x00000040,
    RF_MarkAsRootSet = 0x00000080,
    RF_TagGarbageTemp = 0x00000100,
    RF_NeedInitialization = 0x00000200,
    RF_NeedLoad = 0x00000400,
    RF_KeepForCooker = 0x00000800,
    RF_NeedPostLoad = 0x00001000,
    RF_NeedPostLoadSubobjects = 0x00002000,
    RF_NewerVersionExists = 0x00004000,
    RF_BeginDestroyed = 0x00008000,
    RF_FinishDestroyed = 0x00010000,
    RF_BeingRegenerated = 0x00020000,
    RF_DefaultSubObject = 0x00040000,
    RF_WasLoaded = 0x00080000,
    RF_TextExportTransient = 0x00100000,
    RF_LoadCompleted = 0x00200000,
    RF_InheritableComponentTemplate = 0x00400000,
    RF_DuplicateTransient = 0x00800000,
    RF_StrongRefOnFrame = 0x01000000,
    RF_NonPIEDuplicateTransient = 0x02000000,
    RF_Dynamic = 0x04000000,
    RF_WillBeLoaded = 0x08000000
};

// Macros
#define LOG_INFO(msg) std::wcout << L"[INFO] " << msg << std::endl
#define LOG_WARN(msg) std::wcout << L"[WARN] " << msg << std::endl
#define LOG_ERROR(msg) std::wcout << L"[ERROR] " << msg << std::endl

#define DECLARE_FUNCTION(func) \
    extern decltype(func) func##Original; \
    decltype(func) func##Hook

// Memory helpers
template<typename T>
inline T Read(void* Address) {
    return *reinterpret_cast<T*>(Address);
}

template<typename T>
inline void Write(void* Address, T Value) {
    *reinterpret_cast<T*>(Address) = Value;
}

inline void* OffsetPointer(void* Base, ptrdiff_t Offset) {
    return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(Base) + Offset);
}
