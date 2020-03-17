#pragma once
#ifndef ITEM_POCKET_H
#define ITEM_POCKET_H

#include <list>

#include "enums.h"
#include "enum_traits.h"
#include "optional.h"
#include "type_id.h"
#include "ret_val.h"
#include "translations.h"
#include "units.h"
#include "visitable.h"

class Character;
class item;
class item_location;
class player;
class pocket_data;

struct iteminfo;
struct itype;
struct tripoint;

using itype_id = std::string;

class item_pocket
{
    public:
        enum pocket_type {
            CONTAINER,
            MAGAZINE,
            MOD, // the gunmods or toolmods
            CORPSE, // the "corpse" pocket - bionics embedded in a corpse
            SOFTWARE, // software put into usb or some such
            LAST
        };
        enum class contain_code {
            SUCCESS,
            // only mods can go into the pocket for mods
            ERR_MOD,
            // trying to put a liquid into a non-watertight container
            ERR_LIQUID,
            // trying to put a gas in a non-gastight container
            ERR_GAS,
            // trying to put an item that wouldn't fit if the container were empty
            ERR_TOO_BIG,
            // trying to put an item that wouldn't fit if the container were empty
            ERR_TOO_HEAVY,
            // trying to put an item that wouldn't fit if the container were empty
            ERR_TOO_SMALL,
            // pocket doesn't have sufficient space left
            ERR_NO_SPACE,
            // pocket doesn't have sufficient weight left
            ERR_CANNOT_SUPPORT,
            // requires a flag
            ERR_FLAG,
            // requires item be a specific ammotype
            ERR_AMMO
        };

        item_pocket() = default;
        item_pocket( const pocket_data *data ) : data( data ) {}

        bool stacks_with( const item_pocket &rhs ) const;
        bool is_funnel_container( units::volume &bigger_than ) const;
        bool has_any_with( const std::function<bool( const item & )> &filter ) const;

        bool is_valid() const;
        bool is_type( pocket_type ptype ) const;
        bool empty() const;
        bool full( bool allow_bucket ) const;

        bool rigid() const;
        bool watertight() const;

        std::list<item *> all_items_top();
        std::list<const item *> all_items_top() const;
        std::list<item *> all_items_ptr( pocket_type pk_type );
        std::list<const item *> all_items_ptr( pocket_type pk_type ) const;

        item &back();
        const item &back() const;
        item &front();
        const item &front() const;
        size_t size() const;
        void pop_back();

        ret_val<contain_code> can_contain( const item &it ) const;
        bool can_contain_liquid( bool held_or_ground ) const;

        // combined volume of contained items
        units::volume contains_volume() const;
        units::volume remaining_volume() const;
        units::volume volume_capacity() const;
        // combined weight of contained items
        units::mass contains_weight() const;
        units::mass remaining_weight() const;

        units::volume item_size_modifier() const;
        units::mass item_weight_modifier() const;

        int moves() const;

        int best_quality( const quality_id &id ) const;

        // heats up contents
        void heat_up();
        // returns a list of pointers of all gunmods in the pocket
        std::vector<item *> gunmods();
        // returns a list of pointers of all gunmods in the pocket
        std::vector<const item *> gunmods() const;
        item *magazine_current();
        int ammo_consume( int qty );
        void casings_handle( const std::function<bool( item & )> &func );
        bool use_amount( const itype_id &it, int &quantity, std::list<item> &used );
        bool will_explode_in_a_fire() const;
        bool item_has_uses_recursive() const;
        // will the items inside this pocket fall out of this pocket if it is placed into another item?
        bool will_spill() const;
        bool detonate( const tripoint &p, std::vector<item> &drops );
        bool process( const itype &type, player *carrier, const tripoint &pos, bool activate,
                      float insulation, temperature_flag flag );
        void remove_all_ammo( Character &guy );
        void remove_all_mods( Character &guy );

        // removes and returns the item from the pocket.
        cata::optional<item> remove_item( const item &it );
        cata::optional<item> remove_item( const item_location &it );
        // spills any contents that can't fit into the pocket, largest items first
        void overflow( const tripoint &pos );
        bool spill_contents( const tripoint &pos );
        void on_pickup( Character &guy );
        void on_contents_changed();
        void handle_liquid_or_spill( Character &guy );
        void clear_items();
        bool has_item( const item &it ) const;
        item *get_item_with( const std::function<bool( const item & )> &filter );
        void remove_items_if( const std::function<bool( item & )> &filter );
        void has_rotten_away( const tripoint &pnt );
        void remove_rotten( const tripoint &pnt );
        /**
         * Is part of the recursive call of item::process. see that function for additional comments
         * NOTE: this destroys the items that get processed
         */
        void process( player *carrier, const tripoint &pos, bool activate, float insulation = 1,
                      temperature_flag flag = temperature_flag::TEMP_NORMAL, float spoil_multiplier = 1.0f );
        pocket_type saved_type() const {
            return _saved_type;
        }

        // tries to put an item in the pocket. returns false if failure
        ret_val<contain_code> insert_item( const item &it );
        /**
          * adds an item to the pocket with no checks
          * may create a new pocket
          */
        void add( const item &it );
        /** fills the pocket to the brim with the item */
        void fill_with( item contained );
        bool can_unload_liquid() const;

        // only available to help with migration from previous usage of std::list<item>
        std::list<item> &edit_contents();

        void migrate_item( item &obj, const std::set<itype_id> &migrations );

        // cost of getting an item from this pocket
        // @TODO: make move cost vary based on other contained items
        int obtain_cost( const item &it ) const;

        // this is used for the visitable interface. returns true if no further visiting is required
        bool remove_internal( const std::function<bool( item & )> &filter,
                              int &count, std::list<item> &res );
        // @relates visitable
        VisitResponse visit_contents( const std::function<VisitResponse( item *, item * )> &func,
                                      item *parent = nullptr );

        void general_info( std::vector<iteminfo> &info, int pocket_number, bool disp_pocket_number ) const;
        void contents_info( std::vector<iteminfo> &info, int pocket_number, bool disp_pocket_number ) const;

        void serialize( JsonOut &json ) const;
        void deserialize( JsonIn &jsin );

        bool same_contents( const item_pocket &rhs ) const;
        /** stacks like items inside the pocket */
        void restack();
        bool has_item_stacks_with( const item &it ) const;

        bool better_pocket( const item_pocket &rhs, const item &it ) const;

        bool operator==( const item_pocket &rhs ) const;
    private:
        // the type of pocket, saved to json
        pocket_type _saved_type = pocket_type::LAST;
        const pocket_data *data = nullptr;
        // the items inside the pocket
        std::list<item> contents;
        bool _sealed = true;
};

// an object that has data on how many items can exist inside an item
struct item_number_overrides {
    bool was_loaded;
    // if false any of the other data is useless.
    // loading any data changes this to true
    bool has_override = false;

    int num_items = 0;
    // the number applies to how many stacks of items
    // if false, it takes the absolute total (with charges)
    bool item_stacks = true;

    void load( const JsonObject &jo );
    void deserialize( JsonIn &jsin );
};

class pocket_data
{
    public:
        bool was_loaded;

        item_pocket::pocket_type type = item_pocket::pocket_type::CONTAINER;
        // max volume of stuff the pocket can hold
        units::volume max_contains_volume = 0_ml;
        // min volume of item that can be contained, otherwise it spills
        units::volume min_item_volume = 0_ml;
        // max weight of stuff the pocket can hold
        units::mass max_contains_weight = 0_gram;
        // an override to force the container to only have a specific number of items
        item_number_overrides _item_number_overrides;
        // multiplier for spoilage rate of contained items
        float spoil_multiplier = 1.0f;
        // items' weight in this pocket are modified by this number
        float weight_multiplier = 1.0f;
        // the size that gets subtracted from the contents before it starts enlarging the item
        units::volume magazine_well = 0_ml;
        // base time it takes to pull an item out of the pocket
        int moves = 100;
        // protects contents from exploding in a fire
        bool fire_protection = false;
        // can hold liquids
        bool watertight = false;
        // can hold gas
        bool gastight = false;
        // the pocket will spill its contents if placed in another container
        bool open_container = false;
        // the pocket is not resealable.
        bool resealable = true;
        // allows only items with at least one of the following flags to be stored inside
        // empty means no restriction
        std::vector<std::string> flag_restriction;
        // items stored are restricted to this ammotype
        std::set<ammotype> ammo_restriction;
        // container's size and encumbrance does not change based on contents.
        bool rigid = false;

        bool operator==( const pocket_data &rhs ) const;

        void load( const JsonObject &jo );
        void deserialize( JsonIn &jsin );
};

template<>
struct enum_traits<item_pocket::pocket_type> {
    static constexpr auto last = item_pocket::pocket_type::LAST;
};

template<>
struct ret_val<item_pocket::contain_code>::default_success
    : public std::integral_constant<item_pocket::contain_code,
      item_pocket::contain_code::SUCCESS> {};

#endif
