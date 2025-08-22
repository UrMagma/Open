#pragma once

#include "SDK.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <thread>
#include <mutex>

enum class PacketType : uint8_t
{
    PlayerConnect,
    PlayerDisconnect,
    PlayerMove,
    PlayerAction,
    PlayerChat,
    GameStateUpdate,
    WorldUpdate,
    BuildingPlace,
    BuildingDestroy,
    WeaponFire,
    ItemPickup,
    ItemDrop,
    PlayerElimination
};

struct NetworkPacket
{
    PacketType type;
    std::string playerId;
    std::vector<uint8_t> data;
    float timestamp;
};

struct ClientConnection
{
    std::string playerId;
    std::string playerName;
    std::string ipAddress;
    uint16_t port;
    bool isConnected;
    float lastPingTime;
    float connectionTime;
    int packetsReceived;
    int packetsSent;
};

class NetworkManager
{
public:
    NetworkManager();
    ~NetworkManager();

    // Network lifecycle
    bool Initialize(uint16_t port = 7777);
    void Start();
    void Stop();
    void Shutdown();

    // Client management
    bool AcceptNewConnection(const std::string& playerId, const std::string& playerName, 
                           const std::string& ipAddress, uint16_t port);
    void DisconnectClient(const std::string& playerId, const std::string& reason = "");
    bool IsClientConnected(const std::string& playerId) const;
    ClientConnection* GetClientConnection(const std::string& playerId);

    // Packet handling
    void SendPacket(const std::string& playerId, const NetworkPacket& packet);
    void BroadcastPacket(const NetworkPacket& packet, const std::string& excludePlayerId = "");
    void BroadcastToTeam(int teamId, const NetworkPacket& packet, const std::string& excludePlayerId = "");
    
    // Message handling
    void SendMessage(const std::string& playerId, const std::string& message);
    void BroadcastMessage(const std::string& message, const std::string& excludePlayerId = "");
    void BroadcastSystemMessage(const std::string& message);

    // Game state synchronization
    void SendGameStateUpdate(const std::string& playerId = "");
    void SendPlayerUpdate(const std::string& playerId, const std::string& targetPlayerId = "");
    void SendWorldUpdate(const std::string& playerId = "");

    // Specific game events
    void NotifyPlayerJoined(const std::string& playerId, const std::string& playerName);
    void NotifyPlayerLeft(const std::string& playerId, const std::string& reason = "");
    void NotifyPlayerElimination(const std::string& victimId, const std::string& killerId = "");
    void NotifyBuildingPlaced(const std::string& playerId, SDK::ABuildingSMActor* building);
    void NotifyBuildingDestroyed(SDK::ABuildingSMActor* building, const std::string& destroyerId = "");
    void NotifyWeaponFired(const std::string& playerId, const SDK::FVector& location, const SDK::FVector& direction);
    void NotifyItemPickup(const std::string& playerId, SDK::UFortItemDefinition* item, int quantity);

    // Connection monitoring
    void UpdateConnectionStatus();
    void SendKeepAlive();
    void ProcessTimeouts();
    std::vector<ClientConnection> GetAllConnections() const;
    int GetConnectionCount() const { return static_cast<int>(m_connections.size()); }

    // Statistics
    struct NetworkStats
    {
        int totalConnections = 0;
        int activeConnections = 0;
        int packetsReceived = 0;
        int packetsSent = 0;
        float averagePing = 0.0f;
        int bytesReceived = 0;
        int bytesSent = 0;
    };
    
    NetworkStats GetNetworkStats() const { return m_stats; }
    void ResetStats();

    // Packet callbacks
    using PacketCallback = std::function<void(const std::string&, const NetworkPacket&)>;
    void SetPacketCallback(PacketType type, PacketCallback callback);
    void RemovePacketCallback(PacketType type);

private:
    // Core networking
    uint16_t m_serverPort;
    bool m_isRunning;
    bool m_isInitialized;
    
    // Connection management
    std::unordered_map<std::string, ClientConnection> m_connections;
    std::mutex m_connectionsMutex;
    
    // Packet processing
    std::vector<NetworkPacket> m_incomingPackets;
    std::vector<NetworkPacket> m_outgoingPackets;
    std::mutex m_packetMutex;
    
    // Callbacks
    std::unordered_map<PacketType, PacketCallback> m_packetCallbacks;
    
    // Threading
    std::thread m_networkThread;
    std::thread m_processingThread;
    
    // Statistics
    NetworkStats m_stats;
    
    // Internal methods
    void NetworkLoop();
    void ProcessIncomingPackets();
    void ProcessOutgoingPackets();
    void HandleIncomingData();
    void SendOutgoingData();
    
    // Packet serialization/deserialization
    std::vector<uint8_t> SerializePacket(const NetworkPacket& packet);
    NetworkPacket DeserializePacket(const std::vector<uint8_t>& data);
    
    // Connection helpers
    void UpdateClientStats(const std::string& playerId);
    void CleanupConnection(const std::string& playerId);
    bool ValidateConnection(const std::string& playerId);
    
    // Default packet handlers
    void HandlePlayerConnect(const std::string& playerId, const NetworkPacket& packet);
    void HandlePlayerDisconnect(const std::string& playerId, const NetworkPacket& packet);
    void HandlePlayerMove(const std::string& playerId, const NetworkPacket& packet);
    void HandlePlayerAction(const std::string& playerId, const NetworkPacket& packet);
    void HandlePlayerChat(const std::string& playerId, const NetworkPacket& packet);
    
    // Logging
    void LogNetworkEvent(const std::string& event, const std::string& details = "");
};
