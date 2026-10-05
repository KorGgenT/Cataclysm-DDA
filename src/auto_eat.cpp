#include "auto_eat.h"
#include "avatar.h"
#include "character.h"
#include "morale.h"
#include "item.h"
#include "itype.h"

static const trait_id trait_SAPROPHAGE( "SAPROPHAGE" );
static const trait_id trait_SAPROVORE( "SAPROVORE" );

int auto_eat_settings::joy_score( const Character &guy, const item &food ) const
{
    const int enjoyability = guy.fun_for( food ).first;
    const int morale = guy.morale->get_level();
    return enjoyability * 10;
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

int auto_eat_settings::spoil_score( const Character &guy, const item_location &food ) const
{
    time_duration time_left = get_comestible_time_left( food );
    if( food.has_parent() && food.parent_pocket()->spoil_multiplier() == 0.0f ) {
        time_left = calendar::INDEFINITELY_LONG_DURATION;
    }
    // if we are here we should technically not have to check the traits
    if( food->rotten() ) {
        if( guy.has_trait( trait_SAPROPHAGE ) || guy.has_trait( trait_SAPROVORE ) ) {
            return 50;
        } else {
            // not int mind because there needs to be some sorting
            // but a reasonably low number
            return -15000;
        }
    }

    if( time_left == calendar::INDEFINITELY_LONG_DURATION ) {
        return -5;
    } else if( time_left > 4_weeks ) {
        return 0;
    } else if( time_left > 1_weeks ) {
        return 5;
    } else if( time_left > 1_days ) {
        return 15;
    }
    // this food is going to rot in the process of eating it.
    else if( time_left < 5_minutes ) {
        return INT_MIN;
    }
    return 50;
}

int auto_eat_settings::calorie_score( const Character &guy, const item &food ) const
{
    return guy.compute_effective_nutrients( food ).kcal() / 10;
}

int auto_eat_settings::quench_score( const Character &guy, const item &food ) const
{
    if( !food.is_comestible() ) {
        return 0;
    }
    const int quench = food.get_comestible()->quench;
    const int thirst = guy.get_instant_thirst();
    float thirst_multiplier = 1.0f;
    // not thirsty. deprioritize quench.
    if( thirst < -40 ) {
        thirst_multiplier = -5.0f;
    } else if( thirst < 0 ) {
        thirst_multiplier = -1.0f;
    } else if( thirst < 20 ) {
        thirst_multiplier = 1.0f;
    } else if( thirst < 40 ) {
        thirst_multiplier = 2.0f;
    } else if( thirst < 80 ) {
        thirst_multiplier = 5.0f;
    } else {
        thirst_multiplier = 15.0f;
    }
    return std::round( quench * thirst_multiplier );
}

int auto_eat_settings::vitamin_score( const Character &guy, const item &food ) const
{
    return 0;
}

int auto_eat_settings::score_food( const Character &guy, const item_location &food ) const
{
    return spoil_score( guy, food ) + vitamin_score( guy, *food ) +
           calorie_score( guy, *food ) + quench_score( guy, *food ) +
           joy_score( guy, *food );
}

int auto_eat_settings::score_food( const Character &guy, const item_craft_pair &food ) const
{
    int recipe_score;
    if( food.second ) {
        recipe_score = -30;
    }
    return recipe_score + score_food( guy, food.first );
}

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

bool auto_eat_settings::comestible_sort_compare( Character &you, const item_craft_pair &lhs,
        const item_craft_pair &rhs ) const
{
    return score_food( you, lhs ) < score_food( you, rhs );
}
