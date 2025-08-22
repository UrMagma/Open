#pragma once

#include "GameModeBase.h"

class GameModeSolos : public AbstractGameModeBase {
public:
    GameModeSolos() : GameModeSolos("FortPlaylistAthena Playlist_DefaultSolo.Playlist_DefaultSolo") {}
    
    GameModeSolos(const std::string& SoloPlaylistName) 
        : AbstractGameModeBase(SoloPlaylistName, false, 1) {
        LOG_INFO("Initializing GameMode Solo!");
    }
    
    void OnPlayerJoined(AFortPlayerControllerAthena* Controller) override {
        // In solos, each player gets their own team
        if (m_Teams) {
            m_Teams->AddPlayerToRandomTeam(Controller);
        }
    }
    
    void InitializeGameplay() {
        // Solo-specific initialization would go here
    }
};
