#include "auto_eat.h"
#include "character.h"
#include "item.h"

bool auto_eat_settings::will_eat( const Character &guy, const item &it ) const
{

    if( guy.fun_for( it ).first < min_fun ) {
        // not good eatings.
        return false;
    }

    const bool is_food = it.get_comestible()->comesttype == "FOOD";

    if( is_food && guy.compute_effective_nutrients( it ).kcal() < min_kcal_for_food ) {
        // not filling enough
        return false;
    }
    if( !is_food && it.get_comestible()->quench < min_quench ) {
        // not quenching enough
        return false;
    }
    return true;
}

static time_duration get_comestible_time_left( const item_location &loc )
{
    time_duration time_left = 0_turns;
    const time_duration shelf_life = loc->is_comestible() ? loc->get_comestible()->spoils :
                                     calendar::INDEFINITELY_LONG_DURATION;
    if( shelf_life > 0_turns ) {
        const item &it = *loc;
        const double relative_rot = it.get_relative_rot();
        time_left = shelf_life - shelf_life * relative_rot;

        // Correct for an estimate that exceeds shelf life -- this happens especially with
        // fresh items.
        if( time_left > shelf_life ) {
            time_left = shelf_life;
        }
    }

    return time_left;
}

static bool comestible_sort_compare( Character &you, const item_location &lhs,
                                     const item_location &rhs )
{
    auto_eat_settings rating;
    if( you.is_avatar() ) {
        rating = you.as_avatar()->auto_eat_handler;
    }
    return rating.comestible_sort_compare( you, lhs, rhs );
}

bool auto_eat_settings::comestible_sort_compare( Character &you, const item_location &lhs,
        const item_location &rhs ) const
{
    time_duration time_a = get_comestible_time_left( lhs );
    time_duration time_b = get_comestible_time_left( rhs );
    int order_a = get_comestible_order( you, lhs, time_a );
    int order_b = get_comestible_order( you, rhs, time_b );

    return order_a < order_b
           || ( order_a == order_b && time_a < time_b )
           || ( order_a == order_b && time_a == time_b );
}
