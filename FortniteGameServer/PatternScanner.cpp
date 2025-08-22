#include "PatternScanner.h"
#include "Logger.h"
#include <Psapi.h>

namespace Utils {
    
    uintptr_t FindPattern(const std::string& Pattern, bool bRelative, int32_t Offset, HMODULE Module) {
        if (Module == nullptr) {
            Module = GetModuleHandleA(nullptr);
        }
        
        ModuleInfo moduleInfo = GetModuleInfo(Module);
        if (moduleInfo.BaseAddress == 0) {
            LOG_ERROR("Failed to get module info for pattern: {}", Pattern);
            return 0;
        }
        
        uintptr_t result = FindPatternInRange(Pattern, moduleInfo.BaseAddress, moduleInfo.Size);
        
        if (result == 0) {
            LOG_WARN("Pattern not found: {}", Pattern);
            return 0;
        }
        
        if (bRelative) {
            result = ResolveRIPRelative(result);
        }
        
        if (Offset != 0) {
            result += Offset;
        }
        
        LOG_DEBUG("Found pattern: {} at 0x{:X}", Pattern, result);
        return result;
    }
    
    uintptr_t FindPatternInRange(const std::string& Pattern, uintptr_t StartAddress, size_t Size) {
        std::vector<uint8_t> bytes;
        std::string mask;
        
        if (!PatternToBytes(Pattern, bytes, mask)) {
            return 0;
        }
        
        return FindBytesWithMask(reinterpret_cast<const uint8_t*>(StartAddress), Size, bytes, mask);
    }
    
    bool PatternToBytes(const std::string& Pattern, std::vector<uint8_t>& Bytes, std::string& Mask) {
        Bytes.clear();
        Mask.clear();
        
        std::string patternCopy = Pattern;
        
        // Remove spaces and convert to uppercase
        patternCopy.erase(std::remove(patternCopy.begin(), patternCopy.end(), ' '), patternCopy.end());
        
        for (size_t i = 0; i < patternCopy.length(); i += 2) {
            if (i + 1 >= patternCopy.length()) break;
            
            std::string byteStr = patternCopy.substr(i, 2);
            
            if (byteStr == "??") {
                Bytes.push_back(0);
                Mask.push_back('?');
            } else {
                try {
                    uint8_t byte = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
                    Bytes.push_back(byte);
                    Mask.push_back('x');
                } catch (...) {
                    LOG_ERROR("Invalid byte pattern: {}", byteStr);
                    return false;
                }
            }
        }
        
        return !Bytes.empty();
    }
    
    uintptr_t FindBytesWithMask(const uint8_t* StartAddress, size_t SearchSize, 
                               const std::vector<uint8_t>& Bytes, const std::string& Mask) {
        if (Bytes.empty() || Mask.empty() || Bytes.size() != Mask.size()) {
            return 0;
        }
        
        for (size_t i = 0; i <= SearchSize - Bytes.size(); ++i) {
            bool found = true;
            
            for (size_t j = 0; j < Bytes.size(); ++j) {
                if (Mask[j] == 'x' && StartAddress[i + j] != Bytes[j]) {
                    found = false;
                    break;
                }
            }
            
            if (found) {
                return reinterpret_cast<uintptr_t>(StartAddress + i);
            }
        }
        
        return 0;
    }
    
    ModuleInfo GetModuleInfo(const std::string& ModuleName) {
        HMODULE module = ModuleName.empty() ? GetModuleHandleA(nullptr) : GetModuleHandleA(ModuleName.c_str());
        return GetModuleInfo(module);
    }
    
    ModuleInfo GetModuleInfo(HMODULE Module) {
        ModuleInfo info = {};
        
        if (!Module) {
            return info;
        }
        
        MODULEINFO moduleInfo;
        if (GetModuleInformation(GetCurrentProcess(), Module, &moduleInfo, sizeof(moduleInfo))) {
            info.BaseAddress = reinterpret_cast<uintptr_t>(moduleInfo.lpBaseOfDll);
            info.Size = moduleInfo.SizeOfImage;
            
            char moduleName[MAX_PATH];
            if (GetModuleBaseNameA(GetCurrentProcess(), Module, moduleName, sizeof(moduleName))) {
                info.Name = moduleName;
            }
        }
        
        return info;
    }
    
    uintptr_t ResolveRIPRelative(uintptr_t Address, int32_t InstructionSize) {
        if (!IsValidReadPtr(reinterpret_cast<void*>(Address))) {
            return 0;
        }
        
        int32_t offset = *reinterpret_cast<int32_t*>(Address + InstructionSize - 4);
        return Address + InstructionSize + offset;
    }
    
    bool IsValidReadPtr(void* Ptr) {
        if (!Ptr) return false;
        
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(Ptr, &mbi, sizeof(mbi)) == 0) {
            return false;
        }
        
        return (mbi.State == MEM_COMMIT) && 
               (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE));
    }
}
