#pragma once
#ifndef CATA_SRC_WORN_DATA_H
#define CATA_SRC_WORN_DATA_H

#include "bodypart.h"
#include "item.h"
#include "optional.h"
#include "units.h"

class Character;
class holster_actor;
class inventory;
class player_morale;

struct item_tweaks;

using item_filter = const std::function<bool( const item & )>;

struct dispose_option {
    std::string prompt;
    bool enabled;
    char invlet;
    int moves;
    std::function<bool()> action;
};
/*
 * This is an individual unit, which is all of the data attached to a single item.
 * It keeps track of which item is attached to what bodypart, and has all the requisite functions.
 */
class worn_data
{
    private:
        item worn_clothing;
        body_part_set covering;

        // does a naive calculation of bodyparts that should wear the item.
        body_part_set calculate_initial_limb_coverage( const std::map<body_part_type::type, int> &covers,
                const Character &guy ) const;
    public:
        worn_data( const item &clothing, const Character &guy );
        bool is_worn( const item &thing ) const;
        void on_wear( Character &p, inventory &inv );
        bool covers( const bodypart_str_id &bp ) const;
        bool covers( const body_part_type::type type ) const;
        bool has_flag( const flag_id &flag ) const;
        const body_part_set &get_covered_body_parts() const {
            return covering;
        };
        int get_coverage( const bodypart_str_id &bp ) const;
        const item &get_item() const {
            return worn_clothing;
        }
        // not ideal to use. here for legacy purposes.
        item *inv_dump() {
            return &worn_clothing;
        }
        // checks the item's encumbrance flag cache and resets it to false
        bool check_item_encumbrance_flag();
        // contains all of the flags that allow you to do natural attacks properly
        bool natural_attack_restricted() const;
        bool is_helmet() const;
        // all the things that are NOT a shoe
        bool exempt_shoe() const;
        // does a check if all items can contain what they have in them. if they can't, drops it to the ground.
        void overflow( const tripoint &location );
        bool use_amount( const itype_id &it, int quantity, std::list <item> &used, item_filter &filter );
        bool store_in_holster( Character &guy, const holster_actor *ptr, item_location obj );
        bool burn( fire_data &frd );
        bool mod_damage( int qty, damage_type dt );
        bool is_active_power_armor() const;
        std::pair<item_location, item_pocket *> best_pocket( Character &parent, const item &it );
        std::vector<item_location> all_items_loc( Character &parent );
        int encumb( const bodypart_str_id &bp, const Character &guy ) const;
};

/*
 * This is the class that contains all of the information about a character's worn items.
 * As the name suggest, it's a container for the smaller units which are where the individual data are.
 */
class worn_data_container
{
    private:
        std::vector<worn_data> data;
        bool recalculate_encumbrance = false;

        ret_val<bool> power_armor_conflicts( const worn_data &clothing ) const;
        ret_val<bool> helmet_conflicts( const worn_data &clothing, const Character &guy ) const;
        ret_val<bool> head_cloth_conflicts( const worn_data &clothing, const Character &guy ) const;
    public:
        std::vector<const item &> find_items_with( item_filter &filter ) const;
        std::vector<const item &> find_items_with( const std::function<bool( const worn_data & )> &filter )
        const;
        void remove_items_with( item_filter &filter, Character &guy );
        // gets the integer index of the item. used for legacy code related things
        cata::optional<int> get_item_position( const item &clothing ) const;
        bool is_worn( const item &thing ) const;
        // checks for equipment conflicts
        ret_val<bool> can_wear( const item &clothing, const Character &guy ) const;
        ret_val<bool> exclusivity_conflicts( const worn_data &clothing ) const;
        // this is not allowed to fail! anything that would preclude wearing should come before in can_wear()
        const worn_data &wear_item( Character &guy, const item &clothing, inventory &inv );
        std::vector<worn_data>::iterator position_to_wear_new_item( const item &clothing );
        // removes the worn_data associated with the item ref
        void remove( const item &clothing );
        bool wearing_something_on( const bodypart_id &bp ) const;
        /** Returns the ratio of feet wearing a shoe (between 0 and 1) */
        double footwear_factor( std::map<bodypart_str_id, bool> feet ) const;
        // wears a copy of @clothing on @body. returns false if it fails
        bool put_on( item clothing, const body_part_set &body );
        // the movecost penalty on swim speed based on the volume of clothing worn
        int swim_drag_movecost_modifier( int swim_skill ) const;
        // returns true if any items need to be checked for encumbrance
        // is not const since this changes the cached variable in the item
        bool check_item_encumbrance_flag();
        // is a natural attack restricted at all by an item worn on this bodypart?
        bool natural_attack_restricted_on( const bodypart_id &bp ) const;
        // the maximum length of item that can be contained in any worn container
        units::length max_containable_length() const;
        // the maximum volume of item that can be contained in any worn container
        units::volume max_containable_volume() const;
        // checks clothing coverages and adjusts @dam
        bool immune_to( const bodypart_id &bp, damage_unit &dam ) const;
        /** are there items that must be taken off before taking off this item */
        bool has_dependent_worn_items( const item &it ) const;
        units::mass weight_carried_with_tweaks( const std::map<const item *, int> &without ) const;
        float get_weight_capacity_modifier() const;
        units::mass get_weight_capacity_bonus() const;
        units::mass weight() const;
        units::volume volume_carried_with_tweaks( const std::map<const item *, int> &without ) const;
        units::volume volume_capacity_with_tweaks( const std::map<const item *, int> &without ) const;
        units::volume get_total_capacity() const;
        units::volume free_space() const;
        ret_val<bool> can_contain( const item &it ) const;
        void overflow( const tripoint &location );
        // the total coverage related to carrying a light
        int lumen_coverage( const bodypart_str_id &bp ) const;
        // total coverage that is filthy clothing
        int filthy_coverage( const bodypart_str_id &bp ) const;
        void use_amount( Character &guy, const itype_id &it, int &quantity, std::list<item> &used,
                         item_filter &filter );
        void covered_body_parts( std::map<bodypart_id, std::vector<const item *>> &clothing_map );
        float damage_resist( bodypart_id bp, damage_type dt ) const;
        float env_resist( bodypart_id bp ) const;
        void check_dispose_option( Character &guy, item_location obj, std::vector<dispose_option> &opts );
        // mutates worn_data (damages clothing) and mutates @dam
        // returns true if any armor is destroyed
        bool absorb_hit( Character &guy, const bodypart_id &bp, damage_unit &dam,
                         std::list<item> &worn_remains );
        std::list<item> get_visible_worn_items() const;
        bool is_worn_item_visible( std::vector<worn_data>::const_iterator worn_item ) const;
        void on_item_wear( player_morale &morale ) const;
        bool covered_with_flag( const flag_id &f, body_part_set parts ) const;
        body_part_set exclusive_flag_coverage( const flag_id &flag, body_part_set parts ) const;
        // not ideal to use. here for legacy purposes.
        std::vector<item *> inv_dump();
        std::vector<item_location> all_items_loc( Character &parent );
        std::vector<item_location> top_items_loc( Character &parent );
        bool has_climate_control() const;
        bool is_wearing_power_armor() const;
        bool is_wearing_power_armor_helmet() const;
        bool is_wearing_active_power_armor() const;
        bool is_wearing_active_optcloak() const;
        // TODO: worry about correct order of clothing overlays
        void get_overlay_ids( std::vector<std::pair<std::string, std::string>> &rval ) const;
        std::pair<item_location, item_pocket *> best_pocket( Character &parent, const item &it,
                const item *avoid );
        void bodypart_exposure( std::map<bodypart_id, float> &bp_exposure ) const;
        VisitResponse visit_items( const std::function<VisitResponse( item *, item * )> &func ) const;
};

#endif
