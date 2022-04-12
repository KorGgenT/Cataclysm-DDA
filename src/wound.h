#pragma once
#ifndef CATA_SRC_WOUND_H
#define CATA_SRC_WOUND_H

#include <optional>
#include <vector>

#include "body_part_set.h"
#include "calendar.h"
#include "string_id.h"
#include "translation.h"

class damage_instance;
class JsonObject;
class wound_type;

enum class damage_type;

using wound_id = string_id<wound_type>;

// this is the json object, so it needs to all be public
class wound_type
{
    public:
        wound_type() = default;

        void load( const JsonObject &jo, const std::string & );
        void load_wound( const JsonObject &jo, const std::string &src );

        wound_id id;
        bool was_loaded;

        translation name;
        translation description;

        int pain;
        int min_damage = 1;
        int max_damage = INT_MAX;
        damage_type dmg_type;

        body_part_set limbs;
        // the amount of bleeding per second
        units::volume bleed;
        // how long it takes for this wound to heal.
        std::optional<time_duration> heal_time;
        // transforms into this wound once age has reached heal_time
        std::optional<wound_id> heals_into;
        // transforms into this wound once infection reaches 1.0 (100%)
        std::optional<wound_id> infects_into;
};

// this is the mutable object in code that points to the wound_type
class wound
{
    private:
        // wound_id, intensity multiplier (which multiplies various wound effects)
        std::vector<std::pair<wound_id, double>> wound_group;
        // how old the wound is. if older than heal_time, turns this wound into the next one.
        time_duration age;
        // the percentage to full infection this wound is at. ticks up based on how dirty it is and perhaps other factors.
        double infection;
        // the multiplier for normal infection rate
        double contamination = 1.0;
    public:
        wound() = default;
        wound( const damage_unit &damage );
        // if true, this wound is ready to be removed.
        bool process( const time_duration &t, double healing_factor );
        // value that represents the wound progression to healed status. the wound is healed at 1.0
        double wound_progression() const;
        // value that represents infection progression. the wound is infected at 1.0
        double infection_progression() const;

        bool is_infected() const;

        std::optional<wound_id> heals_into() const;
        std::optional<wound_id> infects_into() const;
};

// this is all of the wounds that are attached to a limb.
// essentially a wrapper object for a list of wounds
class limb_wounds
{
    private:
        std::vector<wound> wounds;
    public:
        limb_wounds() = default;
        // constructs a new wound and adds it to the list
        void add_wound( const wound_id &id );
        // healing factor is a multiple to duration for age so the wound heals faster.
        void process( const time_duration &t, double healing_factor );
};

#endif // CATA_SRC_WOUND_H
