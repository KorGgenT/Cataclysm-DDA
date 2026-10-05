#pragma once
#ifndef CATA_SRC_AUTO_EAT_H
#define CATA_SRC_AUTO_EAT_H

class Character;
class item;
class item_location;

class auto_eat_settings
{
    private:
        // the minimum kcal value to consider an item food
        int min_kcal_for_food = 50;
        // the minimum quench value for a drink
        int min_quench = 15;
        // minimum fun value for consumption
        int min_fun = -5;
        // will drink past necessary quench values to eat calories
        bool over_quench = true;
        // the amount of kcal a "meal" consists of when eating multiple items
        int meal_size = 1000;

        int joy_score() const;
        int spoil_score() const;
        int calorie_score() const;
        int quench_score() const;
        int vitamin_score() const;
    public:
        int get_meal_size() const {
            return meal_size;
        }
        // compare food against settings which are mutable for player
        // does not consider anything out of Character::will_eat
        bool will_eat( const Character &guy, const item &food ) const;
        // which item is better? considers settings.
        bool comestible_sort_compare( Character &you, const item_location &lhs,
                                      const item_location &rhs ) const;
};

#endif // CATA_SRC_AUTO_EAT_H
