#include "steam_workshop.h"

#include <optional>
#ifdef __linux__
#include <limits.h>
#define MAX_PATH PATH_MAX
#endif

#include "achievement_steam.h"
#include "debug.h"
#include "mod_manager.h"
#include "popup.h"
#include "uilist.h"

using mod_id = string_id<MOD_INFORMATION>;

#ifndef APP_ID
#define APP_ID 2330750
#endif // !APP_ID

const AppId_t dda_app_id( APP_ID );

static std::string get_mod_path( const mod_id mod )
{
    char buffer[MAX_PATH];
    //GetModuleFileName( NULL, buffer, MAX_PATH );
    const std::string f( buffer );
    const std::string p( f.substr( 0, f.find_last_of( "\\/" ) ) );

    const std::string path( mod->path.get_unrelative_path().string() );
    return string_format( "%s%c%s", p, std::filesystem::path::preferred_separator, path );
}

void load_workshop_legal_agreement()
{
    SteamFriends()->ActivateGameOverlayToWebPage( legal_agreement_link.c_str() );
}

static std::optional<mod_id> pick_mod()
{
    mod_manager mman;
    mman.refresh_mod_list();
    uilist modlist;
    std::vector<mod_id> moduilist;
    for( const mod_id &mod : mman.all_mods() ) {
        if( !( mod->steam_id && *mod->steam_id == 0 ) ) {
            modlist.addentry_desc( mod->name(), mod->description.translated() );
            moduilist.push_back( mod );
        }
    }
    modlist.desc_enabled = true;
    modlist.title = "Choose a mod to upload to Steam Workshop.";
    modlist.query();
    if( modlist.ret < 0 ) {
        return std::nullopt;
    }

    auto mod_iter = moduilist.begin() + modlist.ret;

    return *mod_iter;
}

static void steam_workshop_update( const mod_id &mod, const std::string path )
{
    UGCUpdateHandle_t handle = SteamUGC()->StartItemUpdate( APP_ID, *mod->steam_id );
    SteamUGC()->SetItemContent( handle, path.c_str() );
    SteamUGC()->SetItemTitle( handle, mod->name().c_str() );
    SteamUGC()->SetItemDescription( handle, mod->description.translated().c_str() );
    SteamUGC()->SubmitItemUpdate( handle, "Update" );
    query_popup pop;
    pop.message( "%s", "Update Complete." );
    pop.allow_anykey( true );
    pop.query_once();
}

void steam_workshop_upload()
{
    std::optional<mod_id> mod_opt = pick_mod();
    if( !mod_opt ) {
        return;
    }
    const mod_id mod = *mod_opt;
    const std::string path = get_mod_path( mod );

    if( mod->steam_id ) {
        steam_workshop_update( mod, path );
        return;
    }

    SteamUser();
    dda_call_result_listener upload_me;
    upload_me.create_item();
    SteamAPI_RunCallbacks();
    if( !upload_me.accepted_agreement() && upload_me.get_id() == 1 ) {
        load_workshop_legal_agreement();
        return;
    }
    if( upload_me.get_result() != 1 ) {
        debugmsg( "Error in uploading item. Please try again. Error code %i", upload_me.get_result() );
        return;
    }
    UGCUpdateHandle_t handle = SteamUGC()->StartItemUpdate( APP_ID, upload_me.get_id() );
    SteamUGC()->SetItemContent( handle, path.c_str() );
    SteamUGC()->SetItemTitle( handle, mod->name().c_str() );
    SteamUGC()->SetItemDescription( handle, mod->description.translated().c_str() );
    SteamUGC()->SubmitItemUpdate( handle, "New upload" );
    query_popup pop;
    pop.message( "Upload Complete. Please add the workshop ID (%u) to the json file and re-upload it.",
                 upload_me.get_id() );
    pop.allow_anykey( true );
    pop.query_once();
}

PublishedFileId_t dda_call_result_listener::get_id()
{
    SteamAPI_RunCallbacks();
    if( !workshop_item ) {
        return 0;
    }
    return workshop_item->m_nPublishedFileId;
}

EResult dda_call_result_listener::get_result()
{
    SteamAPI_RunCallbacks();
    if( !workshop_item ) {
        return EResult::k_EResultNone;
    }
    return workshop_item->m_eResult;
}

bool dda_call_result_listener::accepted_agreement()
{
    SteamAPI_RunCallbacks();
    if( !workshop_item ) {
        return false;
    }
    return !workshop_item->m_bUserNeedsToAcceptWorkshopLegalAgreement;
}

void dda_call_result_listener::create_item()
{
    SteamAPICall_t api_call = SteamUGC()->CreateItem( dda_app_id,
                              EWorkshopFileType::k_EWorkshopFileTypeCommunity );
    SteamAPI_RunCallbacks();
    workshop_item_result.Set( api_call, this, &dda_call_result_listener::on_create_item );
    while( !workshop_item ) {
        SteamAPI_RunCallbacks();
    }
}

void dda_call_result_listener::on_create_item( CreateItemResult_t *pCallback, bool bIOFailure )
{
    if( bIOFailure ) {
        debugmsg( "callback failed" );
        return;
    }
    workshop_item = pCallback;
}
