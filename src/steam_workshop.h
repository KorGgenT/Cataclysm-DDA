#pragma once
#ifndef CATA_SRC_ACHIEVEMENT_STEAM_H
#define CATA_SRC_ACHIEVEMENT_STEAM_H

#include <string>

#include "path_info.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wsuggest-override"
#include "steam/steam_api.h"
#include "steam/isteamfriends.h"
#include "steam/isteamugc.h"
#pragma GCC diagnostic pop

#define MAX_WORKSHOP_ITEMS 16

const std::string legal_agreement_link =
    "http://steamcommunity.com/sharedfiles/workshoplegalagreement";

class dda_call_result_listener
{
    public:
        void create_item();
        bool accepted_agreement();
        PublishedFileId_t get_id();
        EResult get_result();
    private:
        CreateItemResult_t *workshop_item = nullptr;
        void on_create_item( CreateItemResult_t *pCallback, bool bIOFailure );
        CCallResult<dda_call_result_listener, CreateItemResult_t> workshop_item_result;
};

void load_workshop_legal_agreement();
void steam_workshop_upload();

#endif
