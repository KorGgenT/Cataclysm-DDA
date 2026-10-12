#include "calendar.h"
#include "character.h"
#include "debug.h"
#include "faction.h"
#include "larder.h"
#include "map.h"
#include "item.h"
#include "item_category.h"
#include "item_location.h"
#include "itype.h"

using hoard_stack = std::vector<larder::stored_item>;

static item_category_id cat_from_itype( const itype_id &id )
{
    const item_category_id cat = id->category_force;
    return cat.is_valid() ? cat : item_category_id::NULL_ID();
}

larder::larder()
{
    owner = faction_id::NULL_ID();
}

void larder::store_item( item_location loc )
{
    const stored_item stored( loc );
    hoard_stack &vec = hoard[loc->get_category_shallow().get_id()][loc->typeId()];
    auto pos = lower_bound( vec.begin(), vec.end(), stored );
    vec.insert( pos, stored );
    loc.remove_item();
}

std::optional<item>larder::retrieve_item( const itype_id &stored )
{
    int charges = stored->count_by_charges() ? 1 : 0;
    return retrieve_item( charges, stored );
}

std::optional<item> larder::retrieve_item( int &charges, const itype_id &stored )
{
    if( stored->count_by_charges() && charges == 0 ) {
        debugmsg( "did not assign charges to item that counts by charges" );
    } else if( !stored->count_by_charges() && charges != 0 ) {
        debugmsg( "tried to retrieve item with charges that is not count_by_charges" );
    }
    hoard_stack stack = get_hoard_stack( cat_from_itype( stored ), stored );
    if( stack.empty() ) {
        return std::nullopt;
    }
    const stored_item retrieved = stack.back();
    const item retrieved_item = retrieved.retrieve();
    if( charges > 0 ) {
        charges -= retrieved.get_charges();
    }
    stack.pop_back();
    std::optional<std::pair<itype_id, pocket_type>> contained_by = retrieved.get_contained_by();
    if( contained_by ) {
        // containers can't have charges...
        std::optional<item> container = retrieve_item( contained_by->first );
        if( container ) {
            bool success = container->put_in( retrieved_item,
                                              contained_by->second, false, nullptr, true ).success();
            if( success ) {
                return container;
            }
            // fallthrough
        }
        // fallthrough; just fail silently if the container is gone.
    }
    return retrieved_item;
}

bool larder::dump_point( map &here, const tripoint_bub_ms &local_point )
{
    if( is_empty() ) {
        return true;
    }

    units::volume vol_left = here.free_volume( local_point );
    // first we unload drinks, because they require containers
    // and whatever containers are in the larder might get stolen elsewhere.
    if( hoard.find( item_category_id( "DRINK" ) ) != hoard.end() ) {
        for( const std::pair<const itype_id, hoard_stack> &pair : hoard[item_category_id( "DRINK" )] ) {

        }
    }
}

bool larder::is_empty() const
{
    if( hoard.empty() ) {
        return true;
    }
    for( const auto &itype_map : hoard ) {
        if( !itype_map.second.empty() ) {
            for( const auto &stack : itype_map.second ) {
                if( !stack.second.empty() ) {
                    return false;
                }
            }
        }
    }
    return true;
}

hoard_stack larder::get_hoard_stack( const item_category_id &cat, const itype_id &type ) const
{
    auto found_cat = hoard.find( cat );
    if( found_cat != hoard.end() ) {
        auto found_type = found_cat->second.find( type );
        if( found_type != found_cat->second.end() ) {
            return found_type->second;
        }
    }
    return hoard_stack();
}

hoard_stack &larder::get_hoard_stack( const item_category_id &cat, const itype_id &type )
{
    return hoard[cat][type];
}

int larder::num_items( const itype_id &type ) const
{
    int count = 0;
    if( type->count_by_charges() ) {
        for( const stored_item &it : get_hoard_stack( cat_from_itype( type ), type ) ) {
            count += it.get_charges();
        }
    } else {
        count = get_hoard_stack( cat_from_itype( type ), type ).size();
    }
    return count;
}

faction_id larder::get_owner() const
{
    return owner;
}

bool larder::is_owned_by( const Character &c, bool available_to_take ) const
{
    // owner.is_null() implies faction_id( "no_faction" ) which shouldn't happen, or no owner at all.
    // either way, certain situations this means the thing is available to take.
    // in other scenarios we actually really want to check for id == id, even for no_faction
    if( get_owner().is_null() ) {
        return available_to_take;
    }
    if( !c.get_faction() ) {
        debugmsg( "Character %s has no faction", c.disp_name() );
        return false;
    }
    return c.get_faction()->id == get_owner();
}

larder::stored_item::stored_item( const item_location &it ) :
    id( it->typeId() ), birthday( it->birthday() ), damage( it->damage() ), faults( it->faults )
{
    if( it.has_parent() ) {
        contained_by = std::make_pair( it.parent_item()->typeId(), it.parent_pocket()->saved_type() );
    } else {
        contained_by = std::nullopt;
    }

    if( !it->is_comestible() ) {
        expiration_date = calendar::turn_max;
    } else {
        const int spoil_time_i = it->spoilage_sort_order();
        // sort order gives some extra data but we don't need it here
        if( spoil_time_i >= INT_MAX - 3 ) {
            expiration_date = calendar::turn_max;
        } else {
            expiration_date = birthday + time_duration::from_turns( spoil_time_i );
        }
    }

    if( it->count_by_charges() ) {
        charges = it->charges;
    }
}

bool larder::stored_item::operator<( const stored_item &rhs ) const
{
    if( id->comestible ) {
        return expiration_date < rhs.expiration_date;
    } else {
        return damage < rhs.damage;
    }
}

item_category_id larder::stored_item::get_category() const
{
    return cat_from_itype( id );
}

item larder::stored_item::retrieve() const
{
    item stored( id, birthday );
    if( id->count_by_charges() ) {
        stored.charges = charges;
    }
    stored.set_damage( damage );
    stored.faults = faults;
    return stored;
}
