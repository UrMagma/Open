#include "FortniteServer.h"
#include <iostream>
#include <string>
#include <signal.h>
#include <memory>

// Global server instance for signal handling
std::unique_ptr<FortniteServer> g_server = nullptr;

// Signal handler for graceful shutdown
void SignalHandler(int signal)
{
    std::cout << "\nReceived signal " << signal << ", shutting down server gracefully..." << std::endl;
    
    if (g_server && g_server->IsRunning())
    {
        g_server->Stop();
        g_server->Shutdown();
    }
    
    exit(0);
}

void PrintBanner()
{
    std::cout << "========================================================" << std::endl;
    std::cout << "             Fortnite Private Server v1.8.0            " << std::endl;
    std::cout << "                   Built with SDK                      " << std::endl;
    std::cout << "========================================================" << std::endl;
    std::cout << std::endl;
}

void PrintUsage()
{
    std::cout << "Usage: FortniteServer [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help          Show this help message" << std::endl;
    std::cout << "  -v, --version       Show version information" << std::endl;
    std::cout << "  -c, --config FILE   Specify configuration file" << std::endl;
    std::cout << "  -p, --port PORT     Specify server port (default: 7777)" << std::endl;
    std::cout << "  -m, --max-players N Maximum number of players (default: 100)" << std::endl;
    std::cout << "  -d, --debug         Enable debug mode" << std::endl;
    std::cout << "  --no-console        Disable console input" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char* argv[])
{
    PrintBanner();
    
    // Parse command line arguments
    std::string configFile = "server_config.json";
    int serverPort = 7777;
    int maxPlayers = 100;
    bool debugMode = false;
    bool consoleEnabled = true;
    
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help")
        {
            PrintUsage();
            return 0;
        }
        else if (arg == "-v" || arg == "--version")
        {
            std::cout << "Fortnite Private Server v1.8.0" << std::endl;
            std::cout << "Built with Fortnite SDK 1.8.0" << std::endl;
            return 0;
        }
        else if (arg == "-c" || arg == "--config")
        {
            if (i + 1 < argc)
            {
                configFile = argv[++i];
            }
            else
            {
                std::cerr << "Error: --config requires a filename argument" << std::endl;
                return 1;
            }
        }
        else if (arg == "-p" || arg == "--port")
        {
            if (i + 1 < argc)
            {
                serverPort = std::stoi(argv[++i]);
            }
            else
            {
                std::cerr << "Error: --port requires a port number argument" << std::endl;
                return 1;
            }
        }
        else if (arg == "-m" || arg == "--max-players")
        {
            if (i + 1 < argc)
            {
                maxPlayers = std::stoi(argv[++i]);
            }
            else
            {
                std::cerr << "Error: --max-players requires a number argument" << std::endl;
                return 1;
            }
        }
        else if (arg == "-d" || arg == "--debug")
        {
            debugMode = true;
        }
        else if (arg == "--no-console")
        {
            consoleEnabled = false;
        }
        else
        {
            std::cerr << "Error: Unknown argument: " << arg << std::endl;
            PrintUsage();
            return 1;
        }
    }
    
    // Set up signal handlers for graceful shutdown
    signal(SIGINT, SignalHandler);   // Ctrl+C
    signal(SIGTERM, SignalHandler);  // Termination signal
    
#ifdef _WIN32
    signal(SIGBREAK, SignalHandler); // Ctrl+Break on Windows
#endif
    
    try
    {
        // Create and initialize the server
        std::cout << "Initializing Fortnite Server..." << std::endl;
        std::cout << "Config file: " << configFile << std::endl;
        std::cout << "Port: " << serverPort << std::endl;
        std::cout << "Max players: " << maxPlayers << std::endl;
        std::cout << "Debug mode: " << (debugMode ? "enabled" : "disabled") << std::endl;
        std::cout << "Console: " << (consoleEnabled ? "enabled" : "disabled") << std::endl;
        std::cout << std::endl;
        
        g_server = std::make_unique<FortniteServer>();
        
        if (!g_server->Initialize())
        {
            std::cerr << "Failed to initialize server!" << std::endl;
            return 1;
        }
        
        std::cout << "Server initialized successfully!" << std::endl;
        std::cout << "Starting server..." << std::endl;
        
        // Start the server
        g_server->Start();
        
        std::cout << "Server started! Listening on port " << serverPort << std::endl;
        std::cout << "Press Ctrl+C to stop the server." << std::endl;
        std::cout << std::endl;
        
        // Run the server main loop
        g_server->Run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        if (g_server)
        {
            g_server->Stop();
            g_server->Shutdown();
        }
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unknown fatal error occurred!" << std::endl;
        if (g_server)
        {
            g_server->Stop();
            g_server->Shutdown();
        }
        return 1;
    }
    
    std::cout << "Server shut down successfully." << std::endl;
    return 0;
}
