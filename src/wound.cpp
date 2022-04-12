#include "wound.h"

#include "generic_factory.h"

namespace
{
    generic_factory<wound_type> wound_factory( "wound" );
} // namespace

template<>
const wound_type &string_id<wound_type>::obj() const
{
    return wound_factory.obj( *this );
}

template<>
bool string_id<wound_type>::is_valid() const
{
    return wound_factory.is_valid( *this );
}

void wound_type::load_wound( const JsonObject &jo, const std::string &src )
{
    wound_factory.load( jo, src );
}

void wound_type::load( const JsonObject &jo, const std::string & )
{
    optional( jo, was_loaded, "name", name );
    optional( jo, was_loaded, "description", description );
    optional( jo, was_loaded, "pain", pain );
    optional( jo, was_loaded, "min_damage", min_damage );
    optional( jo, was_loaded, "max_damage", max_damage );
    mandatory( jo, was_loaded, "damage_type", dmg_type );
    mandatory( jo, was_loaded, "limbs", limbs );
    optional( jo, was_loaded, "bleed", bleed );
    optional( jo, was_loaded, "heal_time", heal_time );
    optional( jo, was_loaded, "heals_into", heals_into );
    optional( jo, was_loaded, "infects_into", infects_into );
}

wound::wound( const damage_unit &damage )
{
    
}

bool wound::process( const time_duration &t, double healing_factor )
{
    age += t * healing_factor / infection;
    // stand-in sentinel value
    infection += 0.01 * contamination;

    return id->heal_time && age >= *id->heal_time || is_infected();
}

std::optional<wound_id> wound::heals_into() const
{
    return id->heals_into;
}

std::optional<wound_id> wound::infects_into() const
{
    return id->infects_into;
}

double wound::infection_progression() const
{
    return infection;
}

bool wound::is_infected() const
{
    return infection_progression() >= 100.0;
}

void limb_wounds::add_wound( const wound_id &id )
{
    wounds.push_back( wound( id ) );
}

// for this function, the bulk of it is actually the loop with the iterators.
// the actual processing is wound::process and any special rules should be put into there.
void limb_wounds::process( const time_duration &t, double healing_factor )
{
    // start at the ending because we'll be adding more to the real end as we go along
    for( auto wound_iter = wounds.rbegin(); wound_iter != wounds.rend(); ) {
        if( wound_iter->process( t, healing_factor ) ) {
            // the wound's age renews when it transforms
            if( wound_iter->is_infected() ) {
                // "infects" the wound
                add_wound( *wound_iter->infects_into() );
            } else if( wound_iter->heals_into() ) {
                // "heals" the wound
                add_wound( *wound_iter->heals_into() );
            }
            wound_iter = decltype( wound_iter )( wounds.erase( std::next( wound_iter ).base() ) );
        } else {
            ++wound_iter;
        }
    }
}
