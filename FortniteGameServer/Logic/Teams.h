#pragma once

#include "../FortniteClasses.h"
#include <vector>
#include <unordered_map>

class PlayerTeams {
public:
    PlayerTeams(int32_t MaxTeamSize = 1);
    ~PlayerTeams() = default;
    
    // Team management
    void AddPlayerToRandomTeam(AFortPlayerControllerAthena* Controller);
    void AddPlayerToTeam(AFortPlayerControllerAthena* Controller, uint8_t TeamId);
    void RemovePlayerFromTeam(AFortPlayerControllerAthena* Controller);
    
    // Team queries
    uint8_t GetPlayerTeam(AFortPlayerControllerAthena* Controller) const;
    std::vector<AFortPlayerControllerAthena*> GetTeamMembers(uint8_t TeamId) const;
    std::vector<AFortPlayerControllerAthena*> GetAllPlayers() const;
    
    // Team statistics
    int32_t GetTeamCount() const;
    int32_t GetPlayerCount() const;
    int32_t GetTeamPlayerCount(uint8_t TeamId) const;
    bool IsTeamFull(uint8_t TeamId) const;
    bool ArePlayersOnSameTeam(AFortPlayerControllerAthena* Player1, AFortPlayerControllerAthena* Player2) const;
    
    // Team utilities
    uint8_t FindSmallestTeam() const;
    void BalanceTeams();
    void ClearAllTeams();
    
    // Configuration
    int32_t GetMaxTeamSize() const { return m_MaxTeamSize; }
    void SetMaxTeamSize(int32_t NewMaxSize) { m_MaxTeamSize = NewMaxSize; }
    
private:
    struct Team {
        uint8_t TeamId;
        std::vector<AFortPlayerControllerAthena*> Members;
        
        Team(uint8_t Id) : TeamId(Id) {}
    };
    
    std::unordered_map<uint8_t, std::unique_ptr<Team>> m_Teams;
    std::unordered_map<AFortPlayerControllerAthena*, uint8_t> m_PlayerToTeam;
    int32_t m_MaxTeamSize;
    uint8_t m_NextTeamId = 0;
    
    // Helper functions
    uint8_t CreateNewTeam();
    Team* GetTeam(uint8_t TeamId);
    const Team* GetTeam(uint8_t TeamId) const;
    void SetPlayerTeamId(AFortPlayerControllerAthena* Controller, uint8_t TeamId);
};
