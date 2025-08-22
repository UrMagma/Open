#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

enum class LogLevel : uint8_t
{
    Info,
    Warning,
    Error,
    Debug,
    Network,
    Game
};

struct LogEntry
{
    LogLevel level;
    std::string message;
    std::string timestamp;
    std::string category;
};

struct ConsoleCommand
{
    std::string name;
    std::string description;
    std::function<void(const std::vector<std::string>&)> callback;
    int minArgs;
    int maxArgs;
    bool requiresServer;
};

class ConsoleManager
{
public:
    ConsoleManager();
    ~ConsoleManager();

    // Console lifecycle
    bool Initialize();
    void Start();
    void Stop();
    void Shutdown();

    // Logging
    void Log(LogLevel level, const std::string& message, const std::string& category = "General");
    void LogInfo(const std::string& message, const std::string& category = "General");
    void LogWarning(const std::string& message, const std::string& category = "General");
    void LogError(const std::string& message, const std::string& category = "General");
    void LogDebug(const std::string& message, const std::string& category = "General");
    void LogNetwork(const std::string& message);
    void LogGame(const std::string& message);

    // Console commands
    void RegisterCommand(const std::string& name, const std::string& description,
                        std::function<void(const std::vector<std::string>&)> callback,
                        int minArgs = 0, int maxArgs = -1, bool requiresServer = false);
    void UnregisterCommand(const std::string& name);
    bool ExecuteCommand(const std::string& commandLine);
    std::vector<std::string> GetCommandNames() const;
    std::string GetCommandHelp(const std::string& command) const;

    // Input processing
    void ProcessInput();
    std::string ReadInput();
    void HandleInput(const std::string& input);

    // Console output
    void Print(const std::string& message);
    void PrintLine(const std::string& message = "");
    void PrintColored(const std::string& message, int color);
    void ClearScreen();
    void ShowServerStatus();
    void ShowPlayerList();
    void ShowNetworkStats();
    void ShowGameStats();

    // Configuration
    void SetLogLevel(LogLevel minLevel) { m_minLogLevel = minLevel; }
    void SetLogToFile(bool enable, const std::string& filename = "server.log");
    void SetMaxLogEntries(int maxEntries) { m_maxLogEntries = maxEntries; }

    // History and filtering
    std::vector<LogEntry> GetLogHistory(LogLevel minLevel = LogLevel::Info) const;
    std::vector<LogEntry> GetLogsByCategory(const std::string& category) const;
    void ClearLogs();

private:
    // State
    bool m_isInitialized;
    bool m_isRunning;
    LogLevel m_minLogLevel;
    int m_maxLogEntries;
    
    // Logging
    std::vector<LogEntry> m_logHistory;
    bool m_logToFile;
    std::string m_logFilename;
    
    // Commands
    std::unordered_map<std::string, ConsoleCommand> m_commands;
    
    // Input/Output
    bool m_inputReady;
    std::string m_currentInput;
    
    // Internal methods
    void InitializeDefaultCommands();
    void WriteLogToFile(const LogEntry& entry);
    std::string GetTimestamp() const;
    std::string LogLevelToString(LogLevel level) const;
    int LogLevelToColor(LogLevel level) const;
    std::vector<std::string> ParseCommandLine(const std::string& commandLine) const;
    void TrimLogHistory();
    
    // Default commands
    void CmdHelp(const std::vector<std::string>& args);
    void CmdQuit(const std::vector<std::string>& args);
    void CmdStatus(const std::vector<std::string>& args);
    void CmdPlayers(const std::vector<std::string>& args);
    void CmdKick(const std::vector<std::string>& args);
    void CmdBan(const std::vector<std::string>& args);
    void CmdSay(const std::vector<std::string>& args);
    void CmdRestart(const std::vector<std::string>& args);
    void CmdStats(const std::vector<std::string>& args);
    void CmdClear(const std::vector<std::string>& args);
    void CmdSave(const std::vector<std::string>& args);
    void CmdLoad(const std::vector<std::string>& args);
    void CmdConfig(const std::vector<std::string>& args);
    void CmdDebug(const std::vector<std::string>& args);
    void CmdNetwork(const std::vector<std::string>& args);
    void CmdGameMode(const std::vector<std::string>& args);
    void CmdSpawn(const std::vector<std::string>& args);
    void CmdTeleport(const std::vector<std::string>& args);
    void CmdGive(const std::vector<std::string>& args);
    void CmdWeather(const std::vector<std::string>& args);
    void CmdTime(const std::vector<std::string>& args);
};
