#pragma once

#include <Windows.h>
#include <cstdint>
#include <string>
#include <vector>

namespace Utils {
    
    /**
     * Find a pattern in memory using IDA-style signatures
     * @param Pattern - IDA-style pattern (e.g., "48 89 5C 24 ? 48 89 6C 24")
     * @param bRelative - If true, treat result as RIP-relative address
     * @param Offset - Additional offset to add to result
     * @param Module - Module to search in (nullptr for main module)
     * @return Address of found pattern or 0 if not found
     */
    uintptr_t FindPattern(const std::string& Pattern, bool bRelative = false, int32_t Offset = 0, HMODULE Module = nullptr);
    
    /**
     * Find pattern in specific memory range
     */
    uintptr_t FindPatternInRange(const std::string& Pattern, uintptr_t StartAddress, size_t Size);
    
    /**
     * Convert IDA-style pattern to bytes and mask
     * @param Pattern - IDA-style pattern string
     * @param Bytes - Output byte array
     * @param Mask - Output mask string ('x' = exact match, '?' = wildcard)
     * @return true if pattern was valid
     */
    bool PatternToBytes(const std::string& Pattern, std::vector<uint8_t>& Bytes, std::string& Mask);
    
    /**
     * Search for bytes with mask in memory
     */
    uintptr_t FindBytesWithMask(const uint8_t* StartAddress, size_t SearchSize, 
                               const std::vector<uint8_t>& Bytes, const std::string& Mask);
    
    /**
     * Get module information
     */
    struct ModuleInfo {
        uintptr_t BaseAddress;
        size_t Size;
        std::string Name;
    };
    
    ModuleInfo GetModuleInfo(const std::string& ModuleName = "");
    ModuleInfo GetModuleInfo(HMODULE Module);
    
    /**
     * Resolve RIP-relative address
     * @param Address - Address containing RIP-relative offset
     * @param InstructionSize - Size of the instruction (usually 7 for call/jmp with 32-bit offset)
     * @return Resolved address
     */
    uintptr_t ResolveRIPRelative(uintptr_t Address, int32_t InstructionSize = 7);
    
    /**
     * Check if address is valid readable memory
     */
    bool IsValidReadPtr(void* Ptr);
    
    /**
     * Get function address from pattern with proper error checking
     */
    template<typename T>
    bool GetFunctionFromPattern(const std::string& Pattern, T*& OutFunction, 
                               bool bRelative = false, int32_t Offset = 0) {
        uintptr_t Address = FindPattern(Pattern, bRelative, Offset);
        if (!Address) {
            return false;
        }
        
        OutFunction = reinterpret_cast<T*>(Address);
        return true;
    }
}

// Macros for easier pattern resolution
#define FIND_PATTERN(name, pattern) \
    uintptr_t name##_Address = Utils::FindPattern(pattern); \
    if (!name##_Address) { \
        LOG_ERROR("Failed to find pattern for " #name); \
        return false; \
    }

#define FIND_PATTERN_RELATIVE(name, pattern, offset) \
    uintptr_t name##_Address = Utils::FindPattern(pattern, true, offset); \
    if (!name##_Address) { \
        LOG_ERROR("Failed to find pattern for " #name); \
        return false; \
    }

#define ADDRESS_TO_FUNCTION(address, func) \
    func = reinterpret_cast<decltype(func)>(address)

#define CHECK_NULL_FATAL(ptr, msg) \
    if (!ptr) { \
        LOG_ERROR(msg); \
        return false; \
    }
