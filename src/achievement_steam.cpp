#include "achievement_steam.h"

#include "achievement.h"
#include "character.h"
#include "event.h"
#include "game.h"
#include "kill_tracker.h"

// Stubs for non-Windows builds
#if !defined(_WIN32)
#define ARRAYSIZE(a) \
    ((sizeof(a) / sizeof(*(a))) / \
     static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))

static void OutputDebugString( const std::string &msg )
{
    DebugLog( D_INFO, D_GAME ) << msg;
}
#endif

Achievement_t g_rgAchievements[] = {
    _ACH_ID( a_kill_10_monsters, "Decamate" ),
    _ACH_ID( a_kill_100_monsters, "Centinel" ),
    _ACH_ID( a_kill_1000_monsters, "Killo" ),
    _ACH_ID( a_kill_10000_monsters, "Murdyriad" ),
    _ACH_ID( a_kill_100000_monsters, "Hitsy" ),
    _ACH_ID( a_kill_zombie, "One down, billions to go..." ),
    _ACH_ID( a_crosses_mutation_threshold, "Homo Evolutis" ),
    _ACH_ID( a_cut_1_tree, "Timber" ),
    _ACH_ID( a_cut_100_trees, "Lumberjack" ),
    _ACH_ID( a_install_1_cbm, "Cyberpunk" ),
    _ACH_ID( a_install_10_cbms, "Clockwork Man" ),
    _ACH_ID( a_kill_fungus, "Fungicide" ),
    _ACH_ID( a_kill_in_first_minute, "Rude Awakening" ),
    _ACH_ID( a_marathon, "Pheidippides was a hack" ),
    _ACH_ID( a_kill_insect, "DDT" ),
    _ACH_ID( a_kill_robot, "Disassemble Number Five!" ),
    _ACH_ID( a_reach_max_z_level, "Ain't no mountain high enough" ),
    _ACH_ID( a_survive_1_day, "The first day of the rest of their unlives" ),
    _ACH_ID( a_survive_7_days, "Thank God it's Friday" ),
    _ACH_ID( a_survive_28_days, "28 days later" ),
    _ACH_ID( a_survive_91_days, "A time to every purpose under heaven" ),
    _ACH_ID( a_survive_365_days, "Brighter days ahead?" ),
    _ACH_ID( a_traverse_sharp_terrain, "Every rose has its thorn" ),
    _ACH_ID( a_walk_1000_miles, "Please don't fall down at my door" ),
    _ACH_ID( a_die_1_time, "Die" ),
    _ACH_ID( a_die_9_times, "Curiosity" ),
    _ACH_ID( a_die_99_times, "Ouroboros" ),
    _ACH_ID( a_kill_nether, "Silence your nightmares" ),
    _ACH_ID( a_kill_cyborg, "Resistance is not futile" ),
    _ACH_ID( a_swim_merit_badge, "Swimming merit badge" ),
    _ACH_ID( a_reach_min_z_level, "Ain't no valley low enough" ),
    _ACH_ID( a_wield_crowbar, "Freeman's favorite" )
};

// leave out achievements that use stats!
std::map<std::string, EAchievements> achievement_map = {
    { "achievement_kill_zombie", EAchievements::a_kill_zombie },
    { "achievement_crosses_mutation_threshold", EAchievements::a_crosses_mutation_threshold },
    { "achievement_cut_1_tree", EAchievements::a_cut_1_tree },
    { "achievement_cut_100_trees", EAchievements::a_cut_100_trees },
    { "achievement_install_1_cbm", EAchievements::a_install_1_cbm },
    { "achievement_install_10_cbms", EAchievements::a_install_10_cbms },
    { "achievement_kill_fungus", EAchievements::a_kill_fungus },
    { "achievement_kill_in_first_minute", EAchievements::a_kill_in_first_minute },
    { "achievement_marathon", EAchievements::a_marathon },
    { "achievement_kill_insect", EAchievements::a_kill_insect },
    { "achievement_kill_robot", EAchievements::a_kill_robot },
    { "achievement_reach_max_z_level", EAchievements::a_reach_max_z_level },
    { "achievement_survive_one_day", EAchievements::a_survive_1_day },
    { "achievement_survive_7_days", EAchievements::a_survive_7_days },
    { "achievement_survive_28_days", EAchievements::a_survive_28_days },
    { "achievement_survive_91_days", EAchievements::a_survive_91_days },
    { "achievement_survive_365_days", EAchievements::a_survive_365_days },
    { "achievement_traverse_sharp_terrain", EAchievements::a_traverse_sharp_terrain },
    { "achievement_walk_1000_miles", EAchievements::a_walk_1000_miles },
    { "achievement_kill_nether", EAchievements::a_kill_nether },
    { "achievement_kill_cyborg", EAchievements::a_kill_cyborg },
    { "achievement_swim_merit_badge", EAchievements::a_swim_merit_badge },
    { "achievement_reach_min_z_level", EAchievements::a_reach_min_z_level },
    { "achievement_wield_crowbar", EAchievements::a_wield_crowbar }
};

CSteamAchievements::CSteamAchievements() :
    m_CallbackUserStatsReceived( this, &CSteamAchievements::OnUserStatsReceived ),
    m_CallbackUserStatsStored( this, &CSteamAchievements::OnUserStatsStored ),
    m_CallbackAchievementStored( this, &CSteamAchievements::OnAchievementStored )
{
    m_pSteamUser = SteamUser();
    m_pSteamUserStats = SteamUserStats();
}

CSteamAchievements::~CSteamAchievements()
{

}

CSteamAchievements::CSteamAchievements( Achievement_t *Achievements, int NumAchievements )
    : m_pAchievements( Achievements ), m_iNumAchievements( NumAchievements ),
      m_CallbackUserStatsReceived( this, &CSteamAchievements::OnUserStatsReceived ),
      m_CallbackUserStatsStored( this, &CSteamAchievements::OnUserStatsStored ),
      m_CallbackAchievementStored( this, &CSteamAchievements::OnAchievementStored )
{
    m_pSteamUser = SteamUser();
    m_pSteamUserStats = SteamUserStats();
    if( m_pSteamUserStats ) {
        m_pSteamUserStats->GetStat( "deaths", &deaths );
    }
}

//-----------------------------------------------------------------------------
// Purpose: We have stats data from Steam. It is authoritative, so update
//          our data with those results now.
//-----------------------------------------------------------------------------
void CSteamAchievements::OnUserStatsReceived( UserStatsReceived_t *pCallback )
{
    if( !m_pSteamUserStats ) {
        return;
    }

    // we may get callbacks for other games' stats arriving, ignore them
    if( static_cast<uint64>( APP_ID ) == pCallback->m_nGameID ) {
        if( k_EResultOK == pCallback->m_eResult ) {
            OutputDebugString( "Received stats and achievements from Steam\n" );

            m_bStatsValid = true;

            // load achievements
            for( size_t iAch = 0; iAch < ARRAYSIZE( g_rgAchievements ); ++iAch ) {
                Achievement_t &ach = g_rgAchievements[iAch];
                m_pSteamUserStats->GetAchievement( ach.m_pchAchievementID, &ach.m_bAchieved );
                //sprintf_safe( ach.m_rgchName, "%s",
                //    m_pSteamUserStats->GetAchievementDisplayAttribute( ach.m_pchAchievementID, "name" ) );
                //sprintf_safe( ach.m_rgchDescription, "%s",
                //    m_pSteamUserStats->GetAchievementDisplayAttribute( ach.m_pchAchievementID, "desc" ) );
            }

            // load stats
        } else {
            char buffer[128];
            //sprintf_safe( buffer, "RequestStats - failed, %d\n", pCallback->m_eResult );
            buffer[sizeof( buffer ) - 1] = 0;
            OutputDebugString( buffer );
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: An achievement was stored
//-----------------------------------------------------------------------------
void CSteamAchievements::OnAchievementStored( UserAchievementStored_t *pCallback )
{
    // we may get callbacks for other games' stats arriving, ignore them
    if( static_cast<uint64>( APP_ID ) == pCallback->m_nGameID ) {
        if( 0 == pCallback->m_nMaxProgress ) {
            char buffer[128];
            //sprintf_safe( buffer, "Achievement '%s' unlocked!", pCallback->m_rgchAchievementName );
            buffer[sizeof( buffer ) - 1] = 0;
            OutputDebugString( buffer );
        } else {
            char buffer[128];
            //sprintf_safe( buffer, "Achievement '%s' progress callback, (%d,%d)\n",
            //    pCallback->m_rgchAchievementName, pCallback->m_nCurProgress, pCallback->m_nMaxProgress );
            buffer[sizeof( buffer ) - 1] = 0;
            OutputDebugString( buffer );
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: Our stats data was stored!
//-----------------------------------------------------------------------------
void CSteamAchievements::OnUserStatsStored( UserStatsStored_t *pCallback )
{
    // we may get callbacks for other games' stats arriving, ignore them
    if( static_cast<uint64>( APP_ID ) == pCallback->m_nGameID ) {
        if( k_EResultOK == pCallback->m_eResult ) {
            OutputDebugString( "StoreStats - success\n" );
        } else if( k_EResultInvalidParam == pCallback->m_eResult ) {
            // One or more stats we set broke a constraint. They've been reverted,
            // and we should re-iterate the values now to keep in sync.
            OutputDebugString( "StoreStats - some failed to validate\n" );
            // Fake up a callback here so that we re-load the values.
            UserStatsReceived_t callback;
            callback.m_eResult = k_EResultOK;
            callback.m_nGameID = static_cast<uint64>( APP_ID );
            OnUserStatsReceived( &callback );
        } else {
            char buffer[128];
            //sprintf_safe( buffer, "StoreStats - failed, %d\n", pCallback->m_eResult );
            buffer[sizeof( buffer ) - 1] = 0;
            OutputDebugString( buffer );
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: see if we should unlock this achievement
//-----------------------------------------------------------------------------
void CSteamAchievements::EvaluateAchievement( Achievement_t &achieve )
{
    // Already have it?
    if( achieve.m_bAchieved ) {
        return;
    }

    const int mon_kills = g->get_kill_tracker().monster_kill_count();
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
#endif
    switch( achieve.m_eAchievementID ) {
        case a_kill_10_monsters: {
            if( mon_kills >= 10 ) {
                UnlockAchievement( achieve );
            }
            break;
        }
        case a_kill_100_monsters: {
            if( mon_kills >= 100 ) {
                UnlockAchievement( achieve );
            }
            break;
        }
        case a_kill_1000_monsters: {
            if( mon_kills >= 1000 ) {
                UnlockAchievement( achieve );
            }
            break;
        }
        case a_kill_10000_monsters: {
            if( mon_kills >= 10000 ) {
                UnlockAchievement( achieve );
            }
            break;
        }
        case a_kill_100000_monsters: {
            if( mon_kills > 100000 ) {
                UnlockAchievement( achieve );
            }
            break;
        }
        case a_die_1_time: {
            if( deaths > 0 ) {
                UnlockAchievement( achieve );
            }
        }
        case a_die_9_times: {
            if( deaths > 8 ) {
                UnlockAchievement( achieve );
            }
        }
        case a_die_99_times: {
            if( deaths > 98 ) {
                UnlockAchievement( achieve );
            }
        }
    }
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Unlock this achievement
//-----------------------------------------------------------------------------
void CSteamAchievements::UnlockAchievement( Achievement_t &achieve )
{
    achieve.m_bAchieved = true;

    // the icon may change once it's unlocked
    achieve.m_iIconImage = 0;

    // mark it down
    m_pSteamUserStats->SetAchievement( achieve.m_pchAchievementID );
}

void CSteamAchievements::UnlockAchievement( const achievement_id &achieve )
{
    auto found = achievement_map.find( achieve.str() );
    if( found != achievement_map.end() ) {
        for( Achievement_t &ach : g_rgAchievements ) {
            if( ach.m_eAchievementID == found->second ) {
                UnlockAchievement( ach );
            }
        }
    }
}

void CSteamAchievements::notify( const cata::event &e )
{
    if( !m_pSteamUserStats || !m_pSteamUser ) {
        // no steam achievements.
        return;
    }
    switch( e.type() ) {
        case event_type::character_kills_monster: {
            m_pSteamUserStats->SetStat( "mon_kills", g->get_kill_tracker().monster_kill_count() );
            break;
        }
        case event_type::avatar_dies: {
            m_pSteamUserStats->SetStat( "deaths", ++deaths );
            break;
        }
        case event_type::player_gets_achievement: {
            UnlockAchievement( achievement_id( e.data().find( "achievement" )->second.as_pair().second ) );
        }
        default:
            break;
    }
    m_pSteamUserStats->StoreStats();
    SteamInput()->RunFrame();
}
