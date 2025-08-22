#include <Windows.h>
#include <iostream>

#include "Logger.h"
#include "Native.h"
#include "Hooks.h"
#include "Game.h"
#include "UObject.h"

using namespace FortniteGS::Utils;

DWORD WINAPI Main(LPVOID lpParam) {
    // Allocate console for debugging
    AllocConsole();
    FILE* pFile;
    freopen_s(&pFile, "CONOUT$", "w", stdout);
    freopen_s(&pFile, "CONOUT$", "w", stderr);
    freopen_s(&pFile, "CONIN$", "r", stdin);
    
    // Initialize logger
    Logger::Initialize();
    
    LOG_INFO("==========================================");
    LOG_INFO("     Fortnite Game Server Starting       ");
    LOG_INFO("==========================================");
    LOG_INFO("Based on Project Reboot 3.0 architecture");
    LOG_INFO("Built with love for the Fortnite community");
    LOG_INFO("==========================================");
    
    // Get module base address
    Imagebase = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    LOG_INFO("Base Address: 0x{:X}", Imagebase);
    
    try {
        // Initialize native function pointers
        LOG_INFO("Initializing native functions...");
        if (!Native::InitializeAll()) {
            LOG_CRITICAL("Failed to initialize native functions!");
            return 1;
        }
        LOG_INFO("Native functions initialized successfully");
        
        // Initialize basic hooks that don't require level switching
        LOG_INFO("Initializing basic hooks...");
        if (!Hooks::InitializeBasicHooks()) {
            LOG_CRITICAL("Failed to initialize basic hooks!");
            return 1;
        }
        LOG_INFO("Basic hooks initialized successfully");
        
        // Initialize game configuration
        Game::Config::Initialize();
        
        LOG_INFO("==========================================");
        LOG_INFO("    Server initialization complete!      ");
        LOG_INFO("    Waiting for Fortnite to start...     ");
        LOG_INFO("==========================================");
        
        // The rest of the initialization happens in the ProcessEvent hook
        // when the play button is pressed
        
    } catch (const std::exception& e) {
        LOG_CRITICAL("Exception during initialization: {}", e.what());
        return 1;
    } catch (...) {
        LOG_CRITICAL("Unknown exception during initialization!");
        return 1;
    }
    
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        // Disable thread notifications for performance
        DisableThreadLibraryCalls(hModule);
        
        // Create initialization thread
        CreateThread(nullptr, 0, Main, hModule, 0, nullptr);
        break;
        
    case DLL_PROCESS_DETACH:
        // Cleanup
        Game::Shutdown();
        Logger::Shutdown();
        
        // Free console
        FreeConsole();
        break;
    }
    
    return TRUE;
}
