#pragma once

#include "SDK.hpp"
#include "GameMode.h"
#include "PlayerManager.h"
#include "NetworkManager.h"
#include "ConsoleManager.h"
#include <memory>
#include <string>
#include <thread>

class FortniteServer
{
public:
    FortniteServer();
    ~FortniteServer();

    // Server lifecycle
    bool Initialize();
    void Start();
    void Stop();
    void Shutdown();
    
    // Main server loop
    void Run();

    // Getters
    GameMode* GetGameMode() const { return m_gameMode.get(); }
    PlayerManager* GetPlayerManager() const { return m_playerManager.get(); }
    NetworkManager* GetNetworkManager() const { return m_networkManager.get(); }
    ConsoleManager* GetConsoleManager() const { return m_consoleManager.get(); }

    // Server information
    bool IsRunning() const { return m_isRunning; }
    int GetPlayerCount() const;
    const std::string& GetServerVersion() const { return m_serverVersion; }

private:
    // Core components
    std::unique_ptr<GameMode> m_gameMode;
    std::unique_ptr<PlayerManager> m_playerManager;
    std::unique_ptr<NetworkManager> m_networkManager;
    std::unique_ptr<ConsoleManager> m_consoleManager;

    // Server state
    bool m_isRunning;
    bool m_isInitialized;
    std::string m_serverVersion;
    
    // Threading
    std::thread m_serverThread;
    std::thread m_consoleThread;
    
    // Configuration
    void LoadConfig();
    bool ValidateConfig();
    
    // Internal methods
    void ServerLoop();
    void ProcessTick();
    void HandleConsoleInput();
    
    // Logging
    void LogInfo(const std::string& message);
    void LogWarning(const std::string& message);
    void LogError(const std::string& message);
};
