-- Tuning values for the prototype. Distances use the simulation's world units.
-- Level 1 is on the sphere-of-influence edge; higher levels are progressively
-- closer to the planet. Level 4 reaches the current parking-orbit radius.
local rules = {
    -- The starter scenario and its public IDs live here so projects can replace
    -- the SPASIM roster without changing the simulation code.
    scenario = {
        ids = {first_entity = 1, first_player = 0, default_weapon = 0},
        -- Runtime and entity templates are copied into the world at startup.
        world = {
            random_seed = 0x53504153494D,
            fixed_step_seconds = 0.1,
            session_duration_seconds = 900,
            friendly_fire = false,
            paused = false,
        },
        player_defaults = {goal_orbit_level = 1, phasor_power = 1, phasor_diameter = 1},
        player_ship_defaults = {
            class = "small", turn_rate = 30, throttle_level = 3, collision_radius = 20,
            velocity = {0, 0, 0}, yaw = 0, pitch = 0, desired_yaw = 0, desired_pitch = 0,
            speed = 0, warp = 0, acceleration = "cruise", energy = 10000, material = 0,
            hull = 100, hull_damage = 0, shields = 100, skill = 1, destroyed = false,
        },
        practice_ship_defaults = {
            class = "small", turn_rate = 0, throttle_level = 3, collision_radius = 20,
            velocity = {0, 0, 0}, yaw = 0, pitch = 0, desired_yaw = 0, desired_pitch = 0,
            speed = 0, warp = 0, acceleration = "cruise", energy = 10000, material = 0,
            hull = 100, hull_damage = 0, shields = 100, skill = 0, destroyed = false,
        },
        base_defaults = {
            collision_radius = 50, energy = 10000, material = 100,
            hull = 100, hull_damage = 0, shields = 100, skill = 1, destroyed = false,
        },
        torpedo_defaults = {launch_speed = 100, life_seconds = 60, collision_radius = 2},
        -- Random spawns use a sphere around the home planet; fixed spawns use spawn_offset.
        -- Orbit level 0 starts in free flight. Positive levels start in circular orbit.
        teams = {
            {id = 0, name = "Aggstroms", home_planet_id = 0, ship_class = "small",
             starting_orbit_level = 0, random_spawn = true, spawn_radius = 70,
             spawn_offset = {0, 0, 0}, base_offset = {0, 0, 0},
             spawn_vertical_minimum = -1, spawn_vertical_maximum = 1, allied = {0}},
            {id = 1, name = "Fouriers", home_planet_id = 1, ship_class = "small",
             starting_orbit_level = 0, random_spawn = true, spawn_radius = 70,
             spawn_offset = {0, 0, 0}, base_offset = {0, 0, 0},
             spawn_vertical_minimum = -1, spawn_vertical_maximum = 1, allied = {1}},
            {id = 2, name = "Diffractions", home_planet_id = 2, ship_class = "small",
             starting_orbit_level = 0, random_spawn = true, spawn_radius = 70,
             spawn_offset = {0, 0, 0}, base_offset = {0, 0, 0},
             spawn_vertical_minimum = -1, spawn_vertical_maximum = 1, allied = {2}},
            {id = 3, name = "Lasers", home_planet_id = 3, ship_class = "small",
             starting_orbit_level = 0, random_spawn = true, spawn_radius = 70,
             spawn_offset = {0, 0, 0}, base_offset = {0, 0, 0},
             spawn_vertical_minimum = -1, spawn_vertical_maximum = 1, allied = {3}},
        },
        planets = {
            {id = 0, name = "Planet 1", position = {-1000, 0, 0}, radius = 30,
             sphere_of_influence_radius = 500,
             gravity = 2000, parking_altitude = 90, population = 100, energy = 100,
             material = 100, standard_of_living = 0.3, skill = 0.3,
             controlling_team_id = -1, revolt_at_seconds = 0},
            {id = 1, name = "Planet 2", position = {1000, 0, 0}, radius = 30,
             sphere_of_influence_radius = 500,
             gravity = 2000, parking_altitude = 90, population = 100, energy = 100,
             material = 100, standard_of_living = 0.3, skill = 0.3,
             controlling_team_id = -1, revolt_at_seconds = 0},
            {id = 2, name = "Planet 3", position = {0, 1000, 0}, radius = 30,
             sphere_of_influence_radius = 500,
             gravity = 2000, parking_altitude = 90, population = 100, energy = 100,
             material = 100, standard_of_living = 0.3, skill = 0.3,
             controlling_team_id = -1, revolt_at_seconds = 0},
            {id = 3, name = "Planet 4", position = {0, 0, 1000}, radius = 30,
             sphere_of_influence_radius = 500,
             gravity = 2000, parking_altitude = 90, population = 100, energy = 100,
             material = 100, standard_of_living = 0.3, skill = 0.3,
             controlling_team_id = -1, revolt_at_seconds = 0},
        },
        weapons = {
            {id = 0, name = "Aggstroms", energy_cost = 10, damage = 12, range = 2000,
             projectile_speed = 0, explosive = false, splash_radius = 0},
            {id = 1, name = "Fouriers", energy_cost = 20, damage = 20, range = 2500,
             projectile_speed = 0, explosive = false, splash_radius = 0},
            {id = 2, name = "Diffractions", energy_cost = 30, damage = 28, range = 3000,
             projectile_speed = 0, explosive = false, splash_radius = 0},
            {id = 3, name = "Lasers", energy_cost = 15, damage = 16, range = 3500,
             projectile_speed = 0, explosive = false, splash_radius = 0},
        },
        practice_ships = {
            {team_id = 0, name = "Practice target Aggstroms", position = {100, 0, 0}},
            {team_id = 1, name = "Practice target Fouriers", position = {0, 100, 0}},
            {team_id = 2, name = "Practice target Diffractions", position = {-100, 0, 0}},
            {team_id = 3, name = "Practice target Lasers", position = {0, -100, 0}},
        },
    },

    orbit = {
        sphere_of_influence_radius = 500.0,
        level_count = 4,
        radius_fractions = {1.0, 2.0 / 3.0, 1.0 / 3.0, 0.0},

        -- Maximum radial speed and acceleration of the scripted spiral policy.
        transfer_max_radial_speed = 25.0,
        transfer_radial_acceleration = 12.0,
    },

    ship_classes = {
        small = {max_orbit_level = 4},
        medium = {max_orbit_level = 2},
        large = {max_orbit_level = 1},
    },

    -- Speed caps are multiples of the L1 circular speed at the reference
    -- gravity and influence radius. With current planet defaults, this is
    -- sqrt(1000 / 500) ~= 1.41 world-units/second.
    flight = {
        reference_gravitational_parameter = 1000.0,
        thrust_acceleration = 1.0,
        braking_acceleration = 1.0,
        maximum_aim_lead_seconds = 60.0,
        throttle_speed_multipliers = {0.5, 1.0, 1.5, 2.5},
    },
}

-- Return the radial distance to travel during this fixed physics step. The
-- square-root speed profile accelerates on long transfers and brakes smoothly
-- near the target radius, so the policy cannot oscillate across the orbit.
rules.orbit_transfer_step = function(current_radius, target_radius, delta_seconds)
    local difference = target_radius - current_radius
    local remaining = math.abs(difference)
    if remaining <= 0.01 then
        return difference
    end

    local acceleration = rules.orbit.transfer_radial_acceleration
    local maximum_speed = rules.orbit.transfer_max_radial_speed
    local speed = math.min(maximum_speed, math.sqrt(2.0 * acceleration * remaining))
    return (difference > 0.0 and 1.0 or -1.0) * math.min(remaining, speed * delta_seconds)
end

-- Follow a stopping-distance speed profile on the way to the influence sphere.
-- Return -1 to brake, 0 to coast, or 1 to accelerate.
rules.approach_throttle = function(distance_to_capture, current_speed, closing_speed, maximum_speed,
                                   thrust_acceleration, braking_acceleration, delta_seconds)
    local remaining = math.max(0.0, distance_to_capture - current_speed * delta_seconds)
    local safe_speed = math.sqrt(2.0 * braking_acceleration * remaining)
    local target_speed = math.min(maximum_speed, safe_speed)
    local tolerance = math.max(0.05, math.max(thrust_acceleration, braking_acceleration) * delta_seconds)

    if current_speed > target_speed + tolerance then
        return -1
    end
    if closing_speed < target_speed - tolerance then
        return 1
    end
    return 0
end

-- Aim directly for instant weapons, or lead a moving target for finite-speed
-- projectiles. Inputs are relative position and velocity in world units.
rules.weapon_aim_direction = function(rx, ry, rz, vx, vy, vz, projectile_speed, maximum_lead)
    local distance_squared = rx * rx + ry * ry + rz * rz
    if distance_squared <= 1.0e-12 or projectile_speed <= 1.0e-9 then
        return rx, ry, rz
    end

    local velocity_squared = vx * vx + vy * vy + vz * vz
    local a = velocity_squared - projectile_speed * projectile_speed
    local b = 2.0 * (rx * vx + ry * vy + rz * vz)
    local time = 0.0
    if math.abs(a) <= 1.0e-9 then
        if b < -1.0e-9 then
            time = -distance_squared / b
        end
    else
        local discriminant = b * b - 4.0 * a * distance_squared
        if discriminant >= 0.0 then
            local root = math.sqrt(discriminant)
            local t1 = (-b - root) / (2.0 * a)
            local t2 = (-b + root) / (2.0 * a)
            if t1 > 0.0 and t2 > 0.0 then
                time = math.min(t1, t2)
            else
                time = math.max(t1, t2, 0.0)
            end
        end
    end

    time = math.min(math.max(time, 0.0), maximum_lead)
    return rx + vx * time, ry + vy * time, rz + vz * time
end

return rules
