#pragma once
#ifndef CATA_SRC_AUTO_EAT_H
#define CATA_SRC_AUTO_EAT_H

class Character;
class item;
class item_location;
class recipe;

using item_craft_pair = std::pair<item_location, const recipe *>;

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

        bool avoid_crafting = false;

        int joy_score( const Character &guy, const item &food ) const;
        // spoilage absolutely needs item_location because of sealed containers
        int spoil_score( const Character &guy, const item_location &food ) const;
        int calorie_score( const Character &guy, const item &food ) const;
        int quench_score( const Character &guy, const item &food ) const;
        int vitamin_score( const Character &guy, const item &food ) const;

        int score_food( const Character &guy, const item_location &food ) const;
    public:
        int score_food( const Character &guy, const item_craft_pair &food ) const;
        int get_meal_size() const {
            return meal_size;
        }
        // compare food against settings which are mutable for player
        // does not consider anything out of Character::will_eat
        bool will_eat( const Character &guy, const item &food ) const;
        // which item is better? considers settings.
        bool comestible_sort_compare( Character &you, const item_craft_pair &lhs,
                                      const item_craft_pair &rhs ) const;
};

#endif // CATA_SRC_AUTO_EAT_H
