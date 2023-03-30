#pragma once
#ifndef CATA_SRC_ACHIEVEMENT_STEAM_H
#define CATA_SRC_ACHIEVEMENT_STEAM_H

#define APP_ID 2330750

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wsuggest-override"
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include <steam/steam_api.h>
#pragma GCC diagnostic pop

#include "debug.h"
#include "event.h"
#include "event_subscriber.h"

enum EAchievements {
    a_kill_1_monster,
    a_kill_10_monsters,
    a_kill_100_monsters,
    a_kill_1000_monsters,
    a_kill_10000_monsters,
    a_kill_100000_monsters,
    a_kill_fungus,
    a_kill_in_first_minute,
    a_kill_insect,
    a_kill_robot,
    a_kill_zombie,
    a_marathon,
    a_survive_1_day,
    a_survive_7_days,
    a_survive_28_days,
    a_survive_91_days,
    a_survive_365_days,
    a_walk_1000_miles,
    a_crosses_mutation_threshold,
    a_cut_1_tree,
    a_cut_100_trees,
    a_install_1_cbm,
    a_install_10_cbms,
    a_reach_max_z_level,
    a_traverse_sharp_terrain,
    a_die_1_time,
    a_die_9_times,
    a_die_99_times,
    a_kill_nether,
    a_kill_cyborg,
    a_swim_merit_badge,
    a_reach_min_z_level,
    a_wield_crowbar
};

#define _ACH_ID( id, name ) { id, #id, name, "", 0, 0 }
struct Achievement_t {
    EAchievements m_eAchievementID;
    const char *m_pchAchievementID;
    char m_rgchName[128];
    char m_rgchDescription[256];
    bool m_bAchieved;
    int m_iIconImage;
};

class CSteamAchievements : public event_subscriber
{
    private:
        int64 m_iAppID = APP_ID; // Our current AppID
        Achievement_t *m_pAchievements; // Achievements data
        int m_iNumAchievements; // The number of Achievements
        bool m_bInitialized; // Have we called Request stats and received the callback?

        // Steam User interface
        ISteamUser *m_pSteamUser;

        // Steam UserStats interface
        ISteamUserStats *m_pSteamUserStats;

        // Did we get the stats from Steam?
        bool m_bRequestedStats;
        bool m_bStatsValid;
    public:
        CSteamAchievements();
        CSteamAchievements( Achievement_t *Achievements, int NumAchievements );
        ~CSteamAchievements();

        bool RequestStats();
        bool SetAchievement( const char *ID );

        STEAM_CALLBACK( CSteamAchievements, OnUserStatsReceived, UserStatsReceived_t,
                        m_CallbackUserStatsReceived );
        STEAM_CALLBACK( CSteamAchievements, OnUserStatsStored, UserStatsStored_t,
                        m_CallbackUserStatsStored );
        STEAM_CALLBACK( CSteamAchievements, OnAchievementStored,
                        UserAchievementStored_t, m_CallbackAchievementStored );

        // acumulator using event bus
        void notify( const cata::event &e ) override;
    private:
        void EvaluateAchievement( Achievement_t &achieve );
        void UnlockAchievement( Achievement_t &achieve );
        void UnlockAchievement( const achievement_id &achieve );

        int deaths = 0;
};

CSteamAchievements &get_steam_achievements();

#endif
