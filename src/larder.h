#pragma once
#ifndef CATA_SRC_LARDER_H
#define CATA_SRC_LARDER_H

#include <optional>
#include <set>
#include <unordered_map>

#include "calendar.h"
#include "coordinates.h"
#include "type_id.h"
#include "value_ptr.h"

enum class pocket_type;

class item;
class item_location;
class map;

/**
 * larders are stashes of items created for and used by faction camps at overmap scale.
 * a larder spans an entire OMT and once a player has put a larder into the reality bubble
 * it will unload as items. consider some ways to reduce the number of times loaded/unloaded,
 * by keeping as many items still in the larder as is appropriate for as long as possible
 */
class larder
{
    public:
        /**
         * larder::stored_item is a lightweight wrapper as a way to drop item metadata
         * and only keep the metadata that's important
         */
        class stored_item
        {
            private:
                itype_id id;
                time_point birthday;
                int damage;
                cata::heap<std::set<fault_id>> faults;
                // for count_by_charges - ignored if type doesn't count by charges
                int charges = 0;
                std::optional<std::pair<itype_id, pocket_type>> contained_by;
                time_point expiration_date;
            public:
                stored_item( const item_location &it );

                bool operator<( const stored_item &rhs ) const;

                item_category_id get_category() const;
                int get_charges() const {
                    return charges;
                }

                std::optional<std::pair<itype_id, pocket_type>> get_contained_by() const {
                    return contained_by;
                }
                item retrieve() const;
        };

        // this is a rating for how good of a storage this tripoint is.
        enum class tripoint_rating : int8_t {
            never,
            // under a bush outside
            bad,
            // on the street
            poor,
            // in the middle of the floor of a house, or a shelf outside
            neutral,
            // on a shelf inside
            good
        };

        larder();

        bool is_empty() const;
        void clear_unused();

        // stores an item and then destroys it
        void store_item( item_location loc );
        // finds the first appropriate item in the list and removes it from the larder.
        // make sure to do something with the item afterward!
        std::optional<item> retrieve_item( const itype_id &stored );
        // for count_by_charges items: will not attempt to find more than one stack.
        // grab items until charges is empty.
        std::optional<item> retrieve_item( int &charges, const itype_id &stored );

        int num_items( const itype_id &type ) const;

        faction_id get_owner() const;
        bool is_owned_by( const Character &c, bool available_to_take ) const;

        tripoint_rating rate_local_point( const map &here, const tripoint_bub_ms &local_point ) const;
        // tries to empty one category of larder onto a single point.
        // returns false if larder still has items left
        bool dump_point( map &here, const tripoint_bub_ms &local_point );
    private:
        tripoint_rating rate_outside_point( const map &here, const tripoint_bub_ms &local_point ) const;
        tripoint_rating rate_inside_point( const map &here, const tripoint_bub_ms &local_point ) const;
        // may emplace new empty list!
        std::vector<stored_item> &get_hoard_stack( const item_category_id &cat, const itype_id &type );
        std::vector<stored_item> get_hoard_stack( const item_category_id &cat, const itype_id &type ) const;

        std::unordered_map<item_category_id, std::unordered_map<itype_id, std::vector<stored_item>>> hoard;
        faction_id owner;
};

#endif // CATA_SRC_LARDER_H
