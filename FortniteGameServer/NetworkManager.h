#pragma once

#include "ForwardDeclarations.h"
#include "Definitions.h"
#include <unordered_map>
#include <queue>
#include <functional>
#include <thread>
#include <mutex>

// Forward declarations
class AFortPlayerControllerAthena;

/**
 * NetworkManager - Advanced networking and replication system
 * 
 * Features:
 * - Client-server communication
 * - Object replication
 * - RPC (Remote Procedure Call) system
 * - Network optimization
 * - Anti-cheat integration
 * - Bandwidth management
 */

enum class ENetworkRole : uint8_t {
    None = 0,
    SimulatedProxy = 1,    // Simulated on client
    AutonomousProxy = 2,   // Player-controlled
    Authority = 3          // Server authority
};

enum class EReplicationMode : uint8_t {
    None = 0,
    ToAll = 1,            // Replicate to all clients
    ToOwner = 2,          // Only to owning client
    ToTeam = 3,           // Only to team members
    ToRelevant = 4,       // Only to relevant clients
    Unreliable = 5        // Unreliable replication
};

struct FNetworkMessage {
    uint32_t MessageId = 0;
    AFortPlayerControllerAthena* Sender = nullptr;
    std::vector<uint8_t> Data;
    EReplicationMode Mode = EReplicationMode::ToAll;
    bool bReliable = true;
    std::chrono::steady_clock::time_point Timestamp;
    
    FNetworkMessage() {
        Timestamp = std::chrono::steady_clock::now();
    }
};

struct FReplicationData {
    UObject* Object = nullptr;
    std::string PropertyName;
    std::vector<uint8_t> Data;
    EReplicationMode Mode = EReplicationMode::ToAll;
    bool bInitialOnly = false;
    uint32_t LastUpdateFrame = 0;
    
    FReplicationData() = default;
    FReplicationData(UObject* obj, const std::string& prop, const std::vector<uint8_t>& data)
        : Object(obj), PropertyName(prop), Data(data) {}
};

struct FRPCCall {
    UObject* Target = nullptr;
    std::string FunctionName;
    std::vector<uint8_t> Parameters;
    AFortPlayerControllerAthena* Caller = nullptr;
    EReplicationMode Mode = EReplicationMode::ToAll;
    bool bReliable = true;
    
    FRPCCall() = default;
    FRPCCall(UObject* target, const std::string& func, AFortPlayerControllerAthena* caller = nullptr)
        : Target(target), FunctionName(func), Caller(caller) {}
};

struct FNetworkStats {
    uint64_t BytesSent = 0;
    uint64_t BytesReceived = 0;
    uint32_t MessagesSent = 0;
    uint32_t MessagesReceived = 0;
    uint32_t DroppedMessages = 0;
    uint32_t ReplicatedObjects = 0;
    uint32_t RPCsSent = 0;
    float AveragePing = 0.0f;
    float PacketLoss = 0.0f;
    
    void Reset() {
        *this = FNetworkStats{};
    }
};

class NetworkManager {
public:
    static NetworkManager& Get();
    
    // Core networking
    void Initialize(bool bIsServer = true);
    void Shutdown();
    void Update(float DeltaTime);
    bool IsServer() const { return bIsServer; }
    bool IsClient() const { return !bIsServer; }
    
    // Message system
    void SendMessage(const FNetworkMessage& Message);
    void SendMessageToPlayer(AFortPlayerControllerAthena* Player, const FNetworkMessage& Message);
    void SendMessageToTeam(int32_t TeamId, const FNetworkMessage& Message);
    void BroadcastMessage(const FNetworkMessage& Message);
    
    // RPC system
    void CallRPC(const FRPCCall& RPC);
    void CallClientRPC(AFortPlayerControllerAthena* Client, const FRPCCall& RPC);
    void CallServerRPC(const FRPCCall& RPC);
    void RegisterRPCHandler(const std::string& FunctionName, std::function<void(const FRPCCall&)> Handler);
    
    // Object replication
    void ReplicateObject(UObject* Object, EReplicationMode Mode = EReplicationMode::ToAll);
    void ReplicateProperty(UObject* Object, const std::string& PropertyName, 
                          const std::vector<uint8_t>& Data, EReplicationMode Mode = EReplicationMode::ToAll);
    void StopReplicating(UObject* Object);
    void SetReplicationMode(UObject* Object, EReplicationMode Mode);
    
    // Player management
    void RegisterPlayer(AFortPlayerControllerAthena* Player);
    void UnregisterPlayer(AFortPlayerControllerAthena* Player);
    bool IsPlayerConnected(AFortPlayerControllerAthena* Player) const;
    std::vector<AFortPlayerControllerAthena*> GetConnectedPlayers() const;
    
    // Network optimization
    void SetUpdateRate(float Rate) { UpdateRate = Rate; }
    void SetMaxBandwidth(uint32_t MaxBytesPerSecond) { MaxBandwidth = MaxBytesPerSecond; }
    void EnableCompression(bool bEnabled = true) { bCompressionEnabled = bEnabled; }
    void SetRelevanceDistance(float Distance) { RelevanceDistance = Distance; }
    
    // Anti-cheat
    bool ValidatePlayerAction(AFortPlayerControllerAthena* Player, const std::string& Action, 
                             const std::vector<uint8_t>& Data);
    void FlagSuspiciousActivity(AFortPlayerControllerAthena* Player, const std::string& Reason);
    void KickPlayer(AFortPlayerControllerAthena* Player, const std::string& Reason);
    
    // Statistics
    const FNetworkStats& GetNetworkStats() const { return NetworkStats; }
    FNetworkStats GetPlayerNetworkStats(AFortPlayerControllerAthena* Player) const;
    void ResetNetworkStats();
    
    // Configuration
    struct Config {
        float TickRate = 60.0f;             // Server tick rate
        float ClientUpdateRate = 20.0f;     // Client update rate
        uint32_t MaxPlayers = 100;
        uint32_t MaxBandwidthPerPlayer = 50000; // Bytes per second
        float TimeoutDuration = 30.0f;      // Connection timeout
        bool bAntiCheatEnabled = true;
        float MaxMovementSpeed = 1000.0f;
        float MaxInteractionDistance = 500.0f;
        bool bLogNetworkActivity = false;
    } Settings;
    
    // Events and callbacks
    using PlayerConnectedCallback = std::function<void(AFortPlayerControllerAthena*)>;
    using PlayerDisconnectedCallback = std::function<void(AFortPlayerControllerAthena*)>;
    using MessageReceivedCallback = std::function<void(const FNetworkMessage&)>;
    using AntiCheatCallback = std::function<void(AFortPlayerControllerAthena*, const std::string&)>;
    
    void RegisterPlayerConnectedCallback(const std::string& Name, PlayerConnectedCallback Callback);
    void RegisterPlayerDisconnectedCallback(const std::string& Name, PlayerDisconnectedCallback Callback);
    void RegisterMessageReceivedCallback(const std::string& Name, MessageReceivedCallback Callback);
    void RegisterAntiCheatCallback(const std::string& Name, AntiCheatCallback Callback);
    void UnregisterCallback(const std::string& Name);
    
    // Utilities
    void DumpNetworkInfo() const;
    void GenerateNetworkReport(const std::string& FilePath = "") const;
    bool IsRelevant(UObject* Object, AFortPlayerControllerAthena* Player) const;
    
private:
    NetworkManager() = default;
    ~NetworkManager() = default;
    
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;
    
    // Core state
    bool bIsServer = true;
    bool bInitialized = false;
    
    // Network configuration
    float UpdateRate = 20.0f;
    uint32_t MaxBandwidth = 1000000; // 1MB/s total
    bool bCompressionEnabled = true;
    float RelevanceDistance = 10000.0f;
    
    // Connected players
    std::unordered_set<AFortPlayerControllerAthena*> ConnectedPlayers;
    std::unordered_map<AFortPlayerControllerAthena*, std::chrono::steady_clock::time_point> LastPlayerUpdate;
    
    // Replication system
    std::unordered_map<UObject*, EReplicationMode> ReplicatedObjects;
    std::unordered_map<UObject*, std::vector<FReplicationData>> PendingReplication;
    
    // RPC system
    std::unordered_map<std::string, std::function<void(const FRPCCall&)>> RPCHandlers;
    std::queue<FRPCCall> PendingRPCs;
    
    // Message system
    std::queue<FNetworkMessage> IncomingMessages;
    std::queue<FNetworkMessage> OutgoingMessages;
    
    // Statistics
    FNetworkStats NetworkStats;
    std::unordered_map<AFortPlayerControllerAthena*, FNetworkStats> PlayerStats;
    
    // Anti-cheat
    std::unordered_map<AFortPlayerControllerAthena*, std::vector<std::string>> PlayerFlags;
    std::unordered_map<AFortPlayerControllerAthena*, std::chrono::steady_clock::time_point> LastValidation;
    
    // Callbacks
    std::unordered_map<std::string, PlayerConnectedCallback> ConnectedCallbacks;
    std::unordered_map<std::string, PlayerDisconnectedCallback> DisconnectedCallbacks;
    std::unordered_map<std::string, MessageReceivedCallback> MessageCallbacks;
    std::unordered_map<std::string, AntiCheatCallback> AntiCheatCallbacks;
    
    // Threading
    std::thread NetworkThread;
    std::mutex NetworkMutex;
    bool bShutdownRequested = false;
    
    // Timing
    std::chrono::steady_clock::time_point LastNetworkUpdate;
    uint32_t CurrentFrame = 0;
    
    // Internal methods
    void NetworkThreadFunction();
    void ProcessIncomingMessages();
    void ProcessOutgoingMessages();
    void ProcessReplication();
    void ProcessRPCs();
    void ValidateConnections();
    
    // Message handling
    void HandlePlayerMessage(AFortPlayerControllerAthena* Player, const FNetworkMessage& Message);
    void RouteMessage(const FNetworkMessage& Message);
    
    // Replication helpers
    void ReplicateToPlayer(AFortPlayerControllerAthena* Player, const FReplicationData& Data);
    bool ShouldReplicate(UObject* Object, AFortPlayerControllerAthena* Player) const;
    std::vector<uint8_t> SerializeObject(UObject* Object, const std::string& PropertyName);
    void DeserializeObject(UObject* Object, const std::string& PropertyName, const std::vector<uint8_t>& Data);
    
    // Anti-cheat helpers
    bool ValidateMovement(AFortPlayerControllerAthena* Player, const FVector& OldPos, const FVector& NewPos, float DeltaTime);
    bool ValidateAction(AFortPlayerControllerAthena* Player, const std::string& Action, const FVector& Location);
    void LogSuspiciousActivity(AFortPlayerControllerAthena* Player, const std::string& Activity);
    
    // Callback firing
    void FirePlayerConnectedCallbacks(AFortPlayerControllerAthena* Player);
    void FirePlayerDisconnectedCallbacks(AFortPlayerControllerAthena* Player);
    void FireMessageReceivedCallbacks(const FNetworkMessage& Message);
    void FireAntiCheatCallbacks(AFortPlayerControllerAthena* Player, const std::string& Reason);
};