#include "NetworkManager.h"
#include "PlayerManager.h"
#include "UObject.h"
#include "Definitions.h"
#include <algorithm>

NetworkManager& NetworkManager::Get() {
    static NetworkManager Instance;
    return Instance;
}

bool NetworkManager::Initialize(bool bIsServer) {
    std::lock_guard<std::mutex> Lock(NetworkMutex);
    
    bServerMode = bIsServer;
    bInitialized = true;
    
    if (bIsServer) {
        bListening = true;
        ListenPort = Settings.ServerPort;
        
        LOG_INFO("Network Manager initialized in server mode on port " + std::to_string(ListenPort));
    } else {
        LOG_INFO("Network Manager initialized in client mode");
    }
    
    return true;
}

void NetworkManager::Update(float DeltaTime) {
    if (!bInitialized) return;
    
    std::lock_guard<std::mutex> Lock(NetworkMutex);
    
    // Process incoming messages
    ProcessIncomingMessages();
    
    // Update connection states
    UpdateConnections(DeltaTime);
    
    // Send pending messages
    FlushOutgoingMessages();
    
    // Update statistics
    UpdateNetworkStats(DeltaTime);
}

void NetworkManager::Shutdown() {
    std::lock_guard<std::mutex> Lock(NetworkMutex);
    
    if (!bInitialized) return;
    
    // Disconnect all players
    DisconnectAllPlayers("Server shutting down");
    
    bListening = false;
    bInitialized = false;
    
    LOG_INFO("Network Manager shut down");
}

bool NetworkManager::SendMessageToPlayer(AFortPlayerControllerAthena* Player, const FNetworkMessage& Message) {
    if (!Player || !bInitialized) return false;
    
    std::lock_guard<std::mutex> Lock(NetworkMutex);
    
    // Add to outgoing queue
    FPendingMessage pending;
    pending.Recipient = Player;
    pending.Message = Message;
    pending.Timestamp = std::chrono::steady_clock::now();
    
    OutgoingMessages.push(pending);
    
    Stats.MessagesSent++;
    return true;
}

bool NetworkManager::BroadcastMessage(const FNetworkMessage& Message, AFortPlayerControllerAthena* ExcludePlayer) {
    if (!bInitialized) return false;
    
    std::lock_guard<std::mutex> Lock(NetworkMutex);
    
    auto& playerMgr = PlayerManager::Get();
    auto allPlayers = playerMgr.GetAllPlayers();
    
    for (auto* player : allPlayers) {
        if (player != ExcludePlayer) {
            SendMessageToPlayer(player, Message);
        }
    }
    
    return true;
}

bool NetworkManager::SendRPC(AFortPlayerControllerAthena* Player, const FString& FunctionName, const std::vector<FString>& Parameters) {
    if (!Player) return false;
    
    FNetworkMessage message;
    message.Type = ENetworkMessageType::RPC;
    message.Data = FunctionName.ToString();
    
    // Serialize parameters (simplified)
    for (const auto& param : Parameters) {
        message.Data += "|" + param.ToString();
    }
    
    return SendMessageToPlayer(Player, message);
}

void NetworkManager::OnPlayerConnected(AFortPlayerControllerAthena* Player) {
    if (!Player) return;
    
    std::lock_guard<std::mutex> Lock(NetworkMutex);
    
    ConnectedPlayers.insert(Player);
    
    // Initialize connection info
    FConnectionInfo& connInfo = ConnectionInfos[Player];
    connInfo.Player = Player;
    connInfo.ConnectTime = std::chrono::steady_clock::now();
    connInfo.LastPingTime = connInfo.ConnectTime;
    connInfo.bIsConnected = true;
    
    Stats.TotalConnections++;
    
    LOG_INFO("Player connected: " + Player->GetName() + " (" + std::to_string(ConnectedPlayers.size()) + " total)");
    
    FirePlayerConnectedCallbacks(Player);
}

void NetworkManager::OnPlayerDisconnected(AFortPlayerControllerAthena* Player, const FString& Reason) {
    if (!Player) return;
    
    std::lock_guard<std::mutex> Lock(NetworkMutex);
    
    ConnectedPlayers.erase(Player);
    
    auto it = ConnectionInfos.find(Player);
    if (it != ConnectionInfos.end()) {
        it->second.bIsConnected = false;
        it->second.DisconnectReason = Reason;
        it->second.DisconnectTime = std::chrono::steady_clock::now();
    }
    
    Stats.TotalDisconnections++;
    
    LOG_INFO("Player disconnected: " + Player->GetName() + " - " + Reason.ToString() + 
             " (" + std::to_string(ConnectedPlayers.size()) + " remaining)");
    
    FirePlayerDisconnectedCallbacks(Player, Reason);
}

bool NetworkManager::IsPlayerConnected(AFortPlayerControllerAthena* Player) const {
    if (!Player) return false;
    
    std::lock_guard<std::mutex> Lock(NetworkMutex);
    return ConnectedPlayers.find(Player) != ConnectedPlayers.end();
}

FConnectionInfo NetworkManager::GetConnectionInfo(AFortPlayerControllerAthena* Player) const {
    std::lock_guard<std::mutex> Lock(NetworkMutex);
    
    auto it = ConnectionInfos.find(Player);
    if (it != ConnectionInfos.end()) {
        return it->second;
    }
    
    return FConnectionInfo();
}

uint32_t NetworkManager::GetConnectedPlayerCount() const {
    std::lock_guard<std::mutex> Lock(NetworkMutex);
    return static_cast<uint32_t>(ConnectedPlayers.size());
}

std::vector<AFortPlayerControllerAthena*> NetworkManager::GetConnectedPlayers() const {
    std::lock_guard<std::mutex> Lock(NetworkMutex);
    
    std::vector<AFortPlayerControllerAthena*> players;
    for (auto* player : ConnectedPlayers) {
        players.push_back(player);
    }
    
    return players;
}

void NetworkManager::DisconnectPlayer(AFortPlayerControllerAthena* Player, const FString& Reason) {
    if (!Player) return;
    
    // Send disconnect message
    FNetworkMessage message;
    message.Type = ENetworkMessageType::Disconnect;
    message.Data = Reason.ToString();
    
    SendMessageToPlayer(Player, message);
    
    // Process disconnection
    OnPlayerDisconnected(Player, Reason);
}

void NetworkManager::DisconnectAllPlayers(const FString& Reason) {
    auto players = GetConnectedPlayers();
    for (auto* player : players) {
        DisconnectPlayer(player, Reason);
    }
}

void NetworkManager::SetPlayerPing(AFortPlayerControllerAthena* Player, float PingMs) {
    if (!Player) return;
    
    std::lock_guard<std::mutex> Lock(NetworkMutex);
    
    auto it = ConnectionInfos.find(Player);
    if (it != ConnectionInfos.end()) {
        it->second.PingMs = PingMs;
        it->second.LastPingTime = std::chrono::steady_clock::now();
    }
}

float NetworkManager::GetPlayerPing(AFortPlayerControllerAthena* Player) const {
    if (!Player) return 999.0f;
    
    std::lock_guard<std::mutex> Lock(NetworkMutex);
    
    auto it = ConnectionInfos.find(Player);
    if (it != ConnectionInfos.end()) {
        return it->second.PingMs;
    }
    
    return 999.0f;
}

// Internal methods
void NetworkManager::ProcessIncomingMessages() {
    // In a real implementation, this would process actual network packets
    // For now, this is a stub
    
    Stats.MessagesReceived++; // Placeholder
}

void NetworkManager::UpdateConnections(float DeltaTime) {
    auto now = std::chrono::steady_clock::now();
    
    for (auto& pair : ConnectionInfos) {
        auto& connInfo =pair.second;
        
        if (!connInfo.bIsConnected) continue;
        
        // Update connection duration
        connInfo.ConnectionDuration = std::chrono::duration<float>(now - connInfo.ConnectTime).count();
        
        // Check for timeout
        auto timeSinceLastPing = std::chrono::duration<float>(now - connInfo.LastPingTime).count();
        if (timeSinceLastPing > Settings.ConnectionTimeout) {
            DisconnectPlayer(connInfo.Player, FString("Connection timeout"));
        }
    }
}

void NetworkManager::FlushOutgoingMessages() {
    // Process outgoing message queue
    while (!OutgoingMessages.empty()) {
        const auto& pending = OutgoingMessages.front();
        
        // In real implementation, send the actual network packet
        LOG_DEBUG("Sending message to " + pending.Recipient->GetName() + ": " + pending.Message.Data);
        
        OutgoingMessages.pop();
    }
}

void NetworkManager::UpdateNetworkStats(float DeltaTime) {
    auto now = std::chrono::steady_clock::now();
    
    // Update bandwidth stats (simplified)
    Stats.CurrentBandwidthOut = static_cast<float>(Stats.MessagesSent) * 100.0f; // Rough estimate
    Stats.CurrentBandwidthIn = static_cast<float>(Stats.MessagesReceived) * 100.0f;
    
    // Reset counters periodically
    static float resetTimer = 0.0f;
    resetTimer += DeltaTime;
    
    if (resetTimer >= 1.0f) {
        Stats.MessagesSent = 0;
        Stats.MessagesReceived = 0;
        resetTimer = 0.0f;
    }
}

// Callback management
void NetworkManager::RegisterPlayerConnectedCallback(const std::string& Name, PlayerConnectedCallback Callback) {
    std::lock_guard<std::mutex> Lock(NetworkMutex);
    PlayerConnectedCallbacks[Name] = Callback;
}

void NetworkManager::RegisterPlayerDisconnectedCallback(const std::string& Name, PlayerDisconnectedCallback Callback) {
    std::lock_guard<std::mutex> Lock(NetworkMutex);
    PlayerDisconnectedCallbacks[Name] = Callback;
}

void NetworkManager::RegisterMessageReceivedCallback(const std::string& Name, MessageReceivedCallback Callback) {
    std::lock_guard<std::mutex> Lock(NetworkMutex);
    MessageReceivedCallbacks[Name] = Callback;
}

void NetworkManager::UnregisterCallback(const std::string& Name) {
    std::lock_guard<std::mutex> Lock(NetworkMutex);
    PlayerConnectedCallbacks.erase(Name);
    PlayerDisconnectedCallbacks.erase(Name);
    MessageReceivedCallbacks.erase(Name);
}

void NetworkManager::FirePlayerConnectedCallbacks(AFortPlayerControllerAthena* Player) {
    for (const auto& pair : PlayerConnectedCallbacks) {
        try {
            pair.second(Player);
        } catch (...) {
            LOG_ERROR("Exception in PlayerConnected callback: " + pair.first);
        }
    }
}

void NetworkManager::FirePlayerDisconnectedCallbacks(AFortPlayerControllerAthena* Player, const FString& Reason) {
    for (const auto& pair : PlayerDisconnectedCallbacks) {
        try {
            pair.second(Player, Reason);
        } catch (...) {
            LOG_ERROR("Exception in PlayerDisconnected callback: " + pair.first);
        }
    }
}

void NetworkManager::FireMessageReceivedCallbacks(AFortPlayerControllerAthena* Player, const FNetworkMessage& Message) {
    for (const auto& pair : MessageReceivedCallbacks) {
        try {
            pair.second(Player, Message);
        } catch (...) {
            LOG_ERROR("Exception in MessageReceived callback: " + pair.first);
        }
    }
}