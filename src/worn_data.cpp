#include "worn_data.h"

#include "itype.h"
#include "string_id.h"

struct itype;

using itype_id = string_id<itype>;

worn_data::worn_data( const item &clothing, const Character &guy )
{
    worn_clothing = clothing;
    covering = calculate_initial_limb_coverage( clothing.coverage_data(), guy );
}

body_part_set worn_data::calculate_initial_limb_coverage(
    const std::map<body_part_type::type, int> &covers, const Character &guy ) const
{

}

bool worn_data_container::put_on( item clothing, const body_part_set &body )
{
    const islot_armor *armor = clothing.find_armor_data();
    if( armor == nullptr ) {
        return false;
    }
    std::map<body_part_type::type, int> total_limbs_covered;
    for( const armor_portion_data &portion : armor->data ) {
        for( const std::pair<body_part_type::type, int> &pair : portion.covers ) {
            total_limbs_covered[pair.first] += pair.second;
        }
    }
    // TODO: intelligently pick limbs
    body_part_set covering;
    for( const bodypart_str_id &part_id : body ) {
        if( total_limbs_covered[part_id->limb_type] <= 0 ) {
            return false;
        } else {
            covering.set( part_id );
            --total_limbs_covered[part_id->limb_type];
        }
    }
    data.emplace_back( clothing, covering );
    return true;
}

bool worn_data::is_worn( const item &thing ) const
{
    return &thing == &worn_clothing;
}

bool worn_data_container::is_worn( const item &thing ) const
{
    for( const worn_data &worn : data ) {
        if( worn.is_worn( thing ) ) {
            return true;
        }
    }
    return false;
}

void worn_data_container::remove( const item &clothing )
{
    for( auto worn_iter = data.begin(); worn_iter != data.end(); ) {
        if( worn_iter->is_worn( clothing ) ) {
            worn_iter = data.erase( worn_iter );
        } else {
            ++worn_iter;
        }
    }
}

int worn_data_container::swim_drag_movecost_modifier( const int swim_skill ) const
{
    int ret = 0;
    if( swim_skill < 10 ) {
        for( const worn_data &worn : data ) {
            ret += worn.get_item().volume() / 125_ml * ( 10 - swim_skill );
        }
    }
    return ret;
}

bool worn_data_container::check_item_encumbrance_flag()
{
    bool update = false;
    for( worn_data &worn : data ) {
        update = worn.check_item_encumbrance_flag() || update;
    }
    return update;
}

bool worn_data::check_item_encumbrance_flag()
{
    if( worn_clothing.encumbrance_update_ ) {
        worn_clothing.encumbrance_update_ = false;
        return true;
    }
    return false;
}

bool worn_data::covers( const bodypart_str_id &bp ) const
{
    return covering.test( bp );
}

bool worn_data::covers( const body_part_type::type type ) const
{
    for( const bodypart_str_id &bp : covering ) {
        if( bp->limb_type == type ) {
            return true;
        }
    }
    return false;
}

int worn_data::get_coverage( const bodypart_str_id &bp ) const
{
    return worn_clothing.get_coverage( bp );
}

bool worn_data_container::natural_attack_restricted_on( const bodypart_id &bp ) const
{
    for( const worn_data &worn : data ) {
        if( worn.covers( bp ) && worn.natural_attack_restricted() ) {
            return true;
        }
    }
    return false;
}

bool worn_data::natural_attack_restricted() const
{
    return !worn_clothing.has_flag( flag_ALLOWS_NATURAL_ATTACKS ) &&
           !worn_clothing.has_flag( flag_SEMITANGIBLE ) &&
           !worn_clothing.has_flag( flag_PERSONAL ) && !worn_clothing.has_flag( flag_AURA );
}

units::length worn_data_container::max_containable_length() const
{
    units::length ret = 0_mm;
    for( const worn_data &worn : data ) {
        units::length candidate = worn.get_item().max_containable_length();
        if( candidate > ret ) {
            ret = candidate;
        }
    }
    return ret;
}

units::volume worn_data_container::max_containable_volume() const
{
    units::volume ret = 0_ml;
    for( const worn_data &worn : data ) {
        units::volume candidate = worn.get_item().max_containable_volume();
        if( candidate > ret ) {
            ret = candidate;
        }
    }
    return ret;
}

void worn_data_container::remove_items_with( item_filter &filter, Character &guy )
{
    for( auto iter = data.begin(); iter != data.end(); ) {
        if( filter( iter->get_item() ) ) {
            iter->get_item().on_takeoff( guy );
            iter = data.erase( iter );
            recalculate_encumbrance = true;
        } else {
            ++iter;
        }
    }
}

std::vector<const item &> worn_data_container::find_items_with( item_filter &filter ) const
{
    std::vector<const item &> ret;
    for( const worn_data &worn : data ) {
        if( filter( worn.get_item() ) ) {
            ret.push_back( worn.get_item() );
        }
    }
    return ret;
}

std::vector<const item &> worn_data_container::find_items_with( const
        std::function<bool( const worn_data & )> &filter )
const
{
    std::vector<const item &> ret;
    for( const worn_data &worn : data ) {
        if( filter( worn ) ) {
            ret.push_back( worn.get_item() );
        }
    }
    return ret;
}

bool worn_data_container::wearing_something_on( const bodypart_id &bp ) const
{
    for( const worn_data &worn : data ) {
        if( worn.covers( bp ) ) {
            return true;
        }
    }
    return false;
}

double worn_data_container::footwear_factor( std::map<bodypart_str_id, bool> feet ) const
{
    for( const worn_data &worn : data ) {
        for( std::pair<const bodypart_str_id, bool> &foot : feet ) {
            if( worn.covers( foot.first ) ) {
                if( !worn.exempt_shoe() ) {
                    foot.second = true;
                }
            }
        }
    }
    int num_shoes;
    for( const std::pair<const bodypart_id, bool> &foot : feet ) {
        if( foot.second ) {
            num_shoes++;
        }
    }
    return static_cast<double>( num_shoes ) / static_cast<double>( feet.size() );
}

bool worn_data::is_helmet() const
{
    return !worn_clothing.has_flag( flag_HELMET_COMPAT ) &&
           !worn_clothing.has_flag( flag_SKINTIGHT ) &&
           !worn_clothing.has_flag( flag_PERSONAL ) &&
           !worn_clothing.has_flag( flag_AURA ) &&
           !worn_clothing.has_flag( flag_SEMITANGIBLE ) &&
           !worn_clothing.has_flag( flag_OVERSIZE );
}

bool worn_data::exempt_shoe() const
{
    return worn_clothing.has_flag( flag_BELTED ) ||
           worn_clothing.has_flag( flag_PERSONAL ) ||
           worn_clothing.has_flag( flag_AURA ) ||
           worn_clothing.has_flag( flag_SEMITANGIBLE ) ||
           worn_clothing.has_flag( flag_SKINTIGHT );
}

bool worn_data_container::immune_to( const bodypart_id &bp, damage_unit &dam ) const
{
    for( const worn_data &worn : data ) {
        if( worn.covers( bp.id() ) && worn.get_coverage( bp.id() ) == 100 ) {
            worn.get_item().mitigate_damage( dam );
        }
    }
    return dam.amount <= 0;
}

bool worn_data_container::has_dependent_worn_items( const item &it ) const
{
    std::vector<const worn_data &> dependent;
    // Adds dependent worn items recursively
    const std::function<void( const item &it )> add_dependent = [&]( const item & it ) {
        for( const worn_data &worn : data ) {
            const item &wit = worn.get_item();
            if( &wit == &it || !wit.is_worn_only_with( it ) ) {
                continue;
            }
            const auto iter = std::find_if( dependent.begin(), dependent.end(),
            [&wit]( const item * dit ) {
                return &wit == dit;
            } );
            if( iter == dependent.end() ) { // Not in the list yet
                add_dependent( wit );
                dependent.push_back( &worn );
            }
        }
    };

    if( is_worn( it ) ) {
        add_dependent( it );
    }
    return !dependent.empty();
}

units::mass worn_data_container::weight_carried_with_tweaks( const std::map<const item *, int>
        &without ) const
{
    units::mass ret = 0_gram;
    for( const worn_data &worn : data ) {
        if( !without.count( &worn.get_item() ) ) {
            for( const item *it : worn.get_item().all_items_ptr( item_pocket::pocket_type::CONTAINER ) ) {
                if( it->count_by_charges() ) {
                    ret -= get_selected_stack_weight( it, without );
                } else if( without.count( it ) ) {
                    ret -= it->weight();
                }
            }
            ret += worn.get_item().weight();
        }
    }
    return ret;
}

units::volume worn_data_container::volume_carried_with_tweaks( const std::map<const item *, int>
        &without ) const
{
    units::volume ret = 0_ml;
    for( const worn_data &worn : data ) {
        if( !without.count( &worn.get_item() ) ) {
            ret += worn.get_item().get_contents_volume_with_tweaks( without );
        }
    }
    return ret;
}

units::volume worn_data_container::volume_capacity_with_tweaks( const std::map<const item *, int>
        &without ) const
{
    units::volume volume_capacity = 0_ml;
    for( const worn_data &worn : data ) {
        if( !without.count( &worn.get_item() ) ) {
            volume_capacity += worn.get_item().get_total_capacity();
        }
    }
    return volume_capacity;
}

units::mass worn_data_container::get_weight_capacity_bonus() const
{
    units::mass ret = 0_gram;
    for( const worn_data &worn : data ) {
        ret += worn.get_item().get_weight_capacity_bonus();
    }
    return ret;
}

units::mass worn_data_container::weight() const
{
    units::mass ret = 0_gram;
    for( const worn_data &worn : data ) {
        ret += worn.get_item().weight();
    }
    return ret;
}

float worn_data_container::get_weight_capacity_modifier() const
{
    float ret = 0.0f;
    for( const worn_data &worn : data ) {
        ret *= worn.get_item().get_weight_capacity_modifier();
    }
    return ret;
}

ret_val<bool> worn_data_container::can_contain( const item &it ) const
{
    ret_val<bool> ret = ret_val<bool>::make_failure();
    for( const worn_data &worn : data ) {
        ret = worn.get_item().can_contain( it );
        if( ret.success() ) {
            return ret;
        }
    }
    // it only returns the most recently checked fail state.
    return ret;
}

void worn_data_container::overflow( const tripoint &location )
{
    for( worn_data &worn : data ) {
        worn.overflow( location );
    }
}

void worn_data::overflow( const tripoint &location )
{
    worn_clothing.overflow( location );
}

int worn_data_container::lumen_coverage( const bodypart_str_id &bp ) const
{
    int coverage = 0;
    for( const worn_data &worn : data ) {
        if( worn.covers( bp ) && natural_attack_restricted_on() ) {
            coverage += worn.get_coverage( bp );
        }
    }
}

int worn_data_container::filthy_coverage( const bodypart_str_id &bp ) const
{
    int sum_coverage = 0;
    for( const worn_data &worn : data ) {
        if( worn.covers( bp ) && worn.get_item().is_filthy() ) {
            sum_coverage += worn.get_coverage( bp );
        }
    }
    return sum_coverage;
}

void worn_data_container::use_amount( Character &guy, const itype_id &it, int &quantity,
                                      std::list<item> &used, item_filter &filter )
{
    for( auto iter = data.begin(); iter != data.end(); ) {
        if( quantity <= 0 ) {
            return;
        }
        if( iter->use_amount( it, quantity, used, filter ) ) {
            iter->get_item().on_takeoff( guy );
            iter = data.erase();
        } else {
            ++iter;
        }
    }
}

bool worn_data::use_amount( const itype_id &it, int quantity, std::list <item> &used,
                            item_filter &filter )
{
    return worn_clothing.use_amount( it, quantity, used, filter );
}

void worn_data_container::covered_body_parts( std::map<bodypart_id, std::vector<const item *>>
        &clothing_map )
{
    for( const worn_data &worn : data ) {
        for( const bodypart_str_id &covered : worn.get_covered_body_parts() ) {
            clothing_map[covered.id()].emplace_back( &worn.get_item() );
        }
    }
}

float worn_data_container::damage_resist( bodypart_id bp, damage_type dt ) const
{
    float ret = 0.0f;
    for( const worn_data &worn : data ) {
        if( worn.covers( bp.id() ) ) {
            ret += worn.get_item().damage_resist( dt );
        }
    }
    return ret;
}

float worn_data_container::env_resist( bodypart_id bp ) const
{
    float ret = 0.0f;
    for( const worn_data &worn : data ) {
        if( worn.covers( bp.id() ) ) {
            ret += worn.get_item().get_env_resist();
        }
    }
    return ret;
}

bool worn_data::burn( fire_data &frd )
{
    worn_clothing.burn( frd );
}

bool worn_data::mod_damage( int qty, damage_type dt = damage_type::NONE )
{
    worn_clothing.mod_damage( qty, dt );
}

std::list<item> worn_data_container::get_visible_worn_items() const
{
    std::list<item> result;
    for( auto i = data.cbegin(), end = data.cend(); i != end; ++i ) {
        if( is_worn_item_visible( i ) ) {
            result.push_back( i->get_item() );
        }
    }
    return result;
}

bool worn_data_container::is_worn_item_visible( std::vector<worn_data>::const_iterator worn_item )
const
{
    const body_part_set worn_item_body_parts = worn_item->get_covered_body_parts();
    return std::any_of( worn_item_body_parts.begin(), worn_item_body_parts.end(),
    [this, &worn_item]( const bodypart_str_id & bp ) {
        // no need to check items that are worn under worn_item in the armor sort order
        for( auto i = std::next( worn_item ), end = data.end(); i != end; ++i ) {
            if( i->covers( bp ) && i->get_item().get_layer() != layer_level::BELTED &&
                i->get_item().get_layer() != layer_level::WAIST &&
                i->get_coverage( bp ) >= worn_item->get_coverage( bp ) ) {
                return false;
            }
        }
        return true;
    }
                      );
}

units::volume worn_data_container::get_total_capacity() const
{
    units::volume vol = 0_ml;
    for( const worn_data &worn : data ) {
        vol += worn.get_item().get_total_capacity();
    }
    return vol;
}

bool worn_data_container::covered_with_flag( const flag_id &f, body_part_set parts ) const
{
    if( parts.none() ) {
        return true;
    }
    for( const worn_data &worn : data ) {
        if( !worn.has_flag( f ) ) {
            continue;
        }
        parts.substract_set( worn.get_covered_body_parts() );

        if( parts.none() ) {
            return true;
        }
    }

    return parts.none();
}

body_part_set worn_data_container::exclusive_flag_coverage( const flag_id &flag,
        body_part_set parts ) const
{
    for( const worn_data &worn : data ) {
        if( !worn.has_flag( flag ) ) {
            parts.substract_set( worn.get_covered_body_parts() );
        }
    }
    return parts;
}

std::vector<item *> worn_data_container::inv_dump()
{
    std::vector<item *> dump;
    for( worn_data &worn : data ) {
        dump.push_back( worn.inv_dump() );
    }
    return dump;
}

units::volume worn_data_container::free_space() const
{
    units::volume volume_capacity = 0_ml;
    for( const worn_data &worn : data ) {
        volume_capacity += worn.get_item().get_total_capacity();
        for( const item_pocket *pocket : worn.get_item().get_all_contained_pockets().value() ) {
            if( pocket->contains_phase( phase_id::SOLID ) ) {
                for( const item *it : pocket->all_items_top() ) {
                    volume_capacity -= it->volume();
                }
            } else if( !pocket->empty() ) {
                volume_capacity -= pocket->volume_capacity();
            }
        }
        volume_capacity += worn.get_item().check_for_free_space();
    }
    return volume_capacity;
}

bool worn_data::is_active_power_armor() const
{
    return worn_clothing.active && worn_clothing.is_power_armor();
}

bool worn_data_container::has_climate_control() const
{
    for( const worn_data &worn : data ) {
        if( worn.is_active_power_armor() ) {
            return true;
        }
        if( worn.has_flag( flag_CLIMATE_CONTROL ) ) {
            return true;
        }
    }
}

bool worn_data_container::is_wearing_active_power_armor() const
{
    for( const worn_data &worn : data ) {
        if( worn.is_active_power_armor() ) {
            return true;
        }
    }
    return false;
}

bool worn_data_container::is_wearing_power_armor() const
{
    for( const worn_data &worn : data ) {
        if( worn.get_item().is_power_armor() ) {
            return true;
        }
    }
    return false;
}

bool worn_data_container::is_wearing_power_armor_helmet() const
{
    for( const worn_data &worn : data ) {
        if( worn.is_helmet() && worn.get_item().is_power_armor() ) {
            return true;
        }
    }
    return false;
}

bool worn_data_container::is_wearing_active_optcloak() const
{
    for( const worn_data &worn : data ) {
        if( worn.get_item().active && worn.has_flag( flag_ACTIVE_CLOAKING ) ) {
            return true;
        }
    }
    return false;
}

void worn_data_container::get_overlay_ids( std::vector<std::pair<std::string, std::string>> &rval )
const
{
    for( const worn_data &worn : data ) {
        const item &worn_item = worn.get_item();
        const std::string variant = worn_item.has_gun_variant() ? worn_item.gun_variant().id : "";
        rval.emplace_back( "worn_" + worn_item.typeId().str(), variant );
    }
}

std::vector<worn_data>::iterator worn_data_container::position_to_wear_new_item(
    const item &clothing )
{
    // By default we put this item on after the last item on the same or any
    // lower layer.
    return std::find_if(
               data.rbegin(), data.rend(),
    [&clothing]( const worn_data & worn ) {
        return worn.get_item().get_layer() <= clothing.get_layer();
    }
           ).base();
}

std::vector<item_location> worn_data_container::top_items_loc( Character &parent )
{
    std::vector<item_location> ret;
    for( worn_data &worn : data ) {
        item_location worn_loc( parent, worn.inv_dump() );
        ret.push_back( worn_loc );
    }
    return ret;
}

ret_val<bool> worn_data_container::can_wear( const item &clothing, const Character &guy ) const
{
    ret_val<bool> ret = power_armor_conflicts( clothing, parts );
    if( !ret.success() ) {
        return ret;
    }
    ret = exclusivity_conflicts( clothing, parts );
    if( !ret.success() ) {
        return ret;
    }
    ret = helmet_conflicts( clothing );
    if( !ret.success() ) {
        return ret;
    }
    return head_cloth_conflicts( clothing, guy );

}

ret_val<bool> worn_data_container::exclusivity_conflicts( const worn_data &clothing ) const
{
    const bool this_restricts_only_one = clothing.has_flag( flag_id( "ONE_PER_LAYER" ) );
    std::map<side, bool> sidedness;
    sidedness[side::BOTH] = false;
    sidedness[side::LEFT] = false;
    sidedness[side::RIGHT] = false;
    const auto sidedness_conflicts = [&sidedness]( side s ) -> bool {
        const bool ret = sidedness[s];
        sidedness[s] = true;
        if( sidedness[side::LEFT] && sidedness[side::RIGHT] )
        {
            sidedness[side::BOTH] = true;
            return true;
        }
        return ret;
    };
    for( const worn_data &worn : data ) {
        const item &i = worn.get_item();
        if( i.has_flag( flag_ONLY_ONE ) && i.typeId() == clothing.get_item().typeId() ) {
            return ret_val<bool>::make_failure( _( "Can't wear more than one %s!" ),
                                                clothing.get_item().tname() );
        }

        if( this_restricts_only_one || i.has_flag( flag_id( "ONE_PER_LAYER" ) ) ) {
            cata::optional<side> overlaps = clothing.get_item().covers_overlaps( i );
            if( overlaps && sidedness_conflicts( *overlaps ) ) {
                return ret_val<bool>::make_failure( _( "%1$s conflicts with %2$s!" ), clothing.get_item().tname(),
                                                    i.tname() );
            }
        }
    }
}

ret_val<bool> worn_data_container::power_armor_conflicts( const worn_data &clothing ) const
{
    const bool power_armor = clothing.get_item().is_power_armor();
    if( power_armor && clothing.has_flag( flag_POWERARMOR_COMPONENT ) ) {
        if( is_wearing_power_armor() ) {
            return ret_val<bool>::make_success();
        } else {
            return ret_val<bool>::make_failure(
                       _( "You can only wear power armor components with power armor!" ) );
        }
    }
    for( const worn_data &worn : data ) {
        if( power_armor ) {
            if( worn.get_covered_body_parts().make_intersection( clothing.get_covered_body_parts() ).any() &&
                !worn.has_flag( flag_POWERARMOR_COMPATIBLE ) ) {
                return ret_val<bool>::make_failure( _( "Can't wear power armor over other gear!" ) );
            }
        } else if( worn.get_item().is_power_armor() &&
                   !worn.has_flag( flag_POWERARMOR_COMPONENT ) ) {
            if( worn.get_covered_body_parts().make_intersection( clothing.get_covered_body_parts() ).any() &&
                !clothing.has_flag( flag_POWERARMOR_COMPATIBLE ) ) {
                return ret_val<bool>::make_failure( _( "Can't wear %s with power armor!" ),
                                                    clothing.get_item().tname() );
            }
        }
    }

    return ret_val<bool>::make_success();
}

int worn_data::encumb( const bodypart_str_id &bp, const Character &guy ) const
{
    return worn_clothing.get_encumber( guy, bp );
}

void worn_data_container::bodypart_exposure( std::map<bodypart_id, float> &bp_exposure ) const
{
    // For every item worn, for every body part, adjust coverage
    for( const worn_data &worn : data ) {
        // What body parts does this item cover?
        for( const bodypart_str_id &bp : worn.get_covered_body_parts() ) {
            // How much exposure does this item leave on this part? (1.0 == naked)
            const float part_exposure = ( 100 - worn.get_coverage( bp ) ) / 100.0f;
            // Coverage multiplies, so two layers with 50% coverage will together give 75%
            bp_exposure[bp] *= part_exposure;
        }
    }
}

bool worn_data::has_flag( const flag_id &flag ) const
{
    return worn_clothing.has_flag( flag );
}
