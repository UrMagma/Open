#include "FortniteServer.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>

FortniteServer::FortniteServer()
    : m_isRunning(false)
    , m_isInitialized(false)
    , m_serverVersion("1.8.0")
{
    LogInfo("Fortnite Server v" + m_serverVersion + " created");
}

FortniteServer::~FortniteServer()
{
    if (m_isRunning)
    {
        Stop();
    }
    
    if (m_isInitialized)
    {
        Shutdown();
    }
    
    LogInfo("Fortnite Server destroyed");
}

bool FortniteServer::Initialize()
{
    if (m_isInitialized)
    {
        LogWarning("Server is already initialized");
        return true;
    }
    
    LogInfo("Initializing Fortnite Server...");
    
    try
    {
        // Load configuration
        LoadConfig();
        
        if (!ValidateConfig())
        {
            LogError("Configuration validation failed");
            return false;
        }
        
        // Initialize console manager first for logging
        m_consoleManager = std::make_unique<ConsoleManager>();
        if (!m_consoleManager->Initialize())
        {
            LogError("Failed to initialize console manager");
            return false;
        }
        
        // Initialize network manager
        m_networkManager = std::make_unique<NetworkManager>();
        if (!m_networkManager->Initialize())
        {
            LogError("Failed to initialize network manager");
            return false;
        }
        
        // Initialize player manager
        m_playerManager = std::make_unique<PlayerManager>();
        if (!m_playerManager)
        {
            LogError("Failed to create player manager");
            return false;
        }
        
        // Initialize game mode
        m_gameMode = std::make_unique<GameMode>();
        GameSettings defaultSettings; // Use default settings for now
        if (!m_gameMode->Initialize(defaultSettings))
        {
            LogError("Failed to initialize game mode");
            return false;
        }
        
        LogInfo("All components initialized successfully");
        m_isInitialized = true;
        return true;
    }
    catch (const std::exception& e)
    {
        LogError("Exception during initialization: " + std::string(e.what()));
        return false;
    }
}

void FortniteServer::Start()
{
    if (!m_isInitialized)
    {
        LogError("Server must be initialized before starting");
        return;
    }
    
    if (m_isRunning)
    {
        LogWarning("Server is already running");
        return;
    }
    
    LogInfo("Starting Fortnite Server...");
    
    // Start console manager
    m_consoleManager->Start();
    
    // Start network manager
    m_networkManager->Start();
    
    // Start console input thread
    m_consoleThread = std::thread(&FortniteServer::HandleConsoleInput, this);
    
    m_isRunning = true;
    LogInfo("Server started successfully!");
}

void FortniteServer::Stop()
{
    if (!m_isRunning)
    {
        LogWarning("Server is not running");
        return;
    }
    
    LogInfo("Stopping Fortnite Server...");
    
    m_isRunning = false;
    
    // Stop game mode first
    if (m_gameMode)
    {
        m_gameMode->EndMatch();
    }
    
    // Stop network manager
    if (m_networkManager)
    {
        m_networkManager->Stop();
    }
    
    // Stop console manager
    if (m_consoleManager)
    {
        m_consoleManager->Stop();
    }
    
    // Wait for threads to finish
    if (m_serverThread.joinable())
    {
        m_serverThread.join();
    }
    
    if (m_consoleThread.joinable())
    {
        m_consoleThread.join();
    }
    
    LogInfo("Server stopped");
}

void FortniteServer::Shutdown()
{
    if (!m_isInitialized)
    {
        return;
    }
    
    LogInfo("Shutting down Fortnite Server...");
    
    // Shutdown all components
    if (m_gameMode)
    {
        m_gameMode.reset();
    }
    
    if (m_playerManager)
    {
        m_playerManager->ClearAllPlayers();
        m_playerManager.reset();
    }
    
    if (m_networkManager)
    {
        m_networkManager->Shutdown();
        m_networkManager.reset();
    }
    
    if (m_consoleManager)
    {
        m_consoleManager->Shutdown();
        m_consoleManager.reset();
    }
    
    m_isInitialized = false;
    LogInfo("Server shutdown complete");
}

void FortniteServer::Run()
{
    if (!m_isRunning)
    {
        LogError("Server must be started before running main loop");
        return;
    }
    
    LogInfo("Entering main server loop...");
    
    // Start the server loop in a separate thread
    m_serverThread = std::thread(&FortniteServer::ServerLoop, this);
    
    // Wait for the server thread to finish (when server stops)
    if (m_serverThread.joinable())
    {
        m_serverThread.join();
    }
    
    LogInfo("Main server loop exited");
}

int FortniteServer::GetPlayerCount() const
{
    if (m_playerManager)
    {
        return m_playerManager->GetPlayerCount();
    }
    return 0;
}

void FortniteServer::LoadConfig()
{
    LogInfo("Loading server configuration...");
    
    // For now, we'll use default configuration
    // In a full implementation, this would load from JSON file
    LogInfo("Using default configuration (JSON loading not implemented yet)");
}

bool FortniteServer::ValidateConfig()
{
    // Basic validation
    LogInfo("Validating server configuration...");
    
    // Check if we have all required components
    if (m_serverVersion.empty())
    {
        LogError("Server version is not set");
        return false;
    }
    
    LogInfo("Configuration validation passed");
    return true;
}

void FortniteServer::ServerLoop()
{
    LogInfo("Server loop started");
    
    const float targetFPS = 60.0f;
    const auto targetFrameTime = std::chrono::microseconds(static_cast<int64_t>(1000000.0f / targetFPS));
    
    auto lastTime = std::chrono::high_resolution_clock::now();
    
    while (m_isRunning)
    {
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastTime);
        float deltaSeconds = deltaTime.count() / 1000000.0f;
        
        // Process server tick
        ProcessTick();
        
        // Update game mode
        if (m_gameMode)
        {
            m_gameMode->Tick(deltaSeconds);
        }
        
        // Update network manager
        if (m_networkManager)
        {
            m_networkManager->UpdateConnectionStatus();
        }
        
        // Update player manager
        if (m_playerManager)
        {
            m_playerManager->UpdatePlayerStats();
        }
        
        lastTime = currentTime;
        
        // Sleep to maintain target FPS
        auto frameEndTime = std::chrono::high_resolution_clock::now();
        auto frameTime = std::chrono::duration_cast<std::chrono::microseconds>(frameEndTime - currentTime);
        
        if (frameTime < targetFrameTime)
        {
            std::this_thread::sleep_for(targetFrameTime - frameTime);
        }
    }
    
    LogInfo("Server loop ended");
}

void FortniteServer::ProcessTick()
{
    // This is where we'd process game logic, network packets, etc.
    // For now, it's mostly empty as we're just setting up the framework
}

void FortniteServer::HandleConsoleInput()
{
    LogInfo("Console input handler started");
    
    while (m_isRunning)
    {
        if (m_consoleManager)
        {
            m_consoleManager->ProcessInput();
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    LogInfo("Console input handler ended");
}

void FortniteServer::LogInfo(const std::string& message)
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    char timestamp[64];
    std::strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S]", &tm);
    
    std::cout << timestamp << " [INFO] " << message << std::endl;
}

void FortniteServer::LogWarning(const std::string& message)
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    char timestamp[64];
    std::strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S]", &tm);
    
    std::cout << timestamp << " [WARNING] " << message << std::endl;
}

void FortniteServer::LogError(const std::string& message)
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    char timestamp[64];
    std::strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S]", &tm);
    
    std::cerr << timestamp << " [ERROR] " << message << std::endl;
}
