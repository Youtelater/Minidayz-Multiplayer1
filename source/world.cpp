#include "minidayz/world.hpp"
#include "minidayz/source_mapgen_geometry.hpp"
#include "minidayz/generated/original_initial_building_loot.hpp"
#include "minidayz/stage1_data_access.hpp"
#include "minidayz/generated/world_source_data.hpp"
#include "minidayz/location_event_runtime.hpp"
#include "minidayz/generated/source_tree_impact_points.hpp"
#include "minidayz/generated/source_door_points.hpp"
#include "minidayz/generated/source_house_companions.hpp"
#include "minidayz/generated/source_building_frame_geometry.hpp"
#include "minidayz/source_furniture_interaction.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <random>

namespace minidayz {
namespace {

bool equals(const char* value, const char* expected) noexcept {
    return value && std::strcmp(value, expected) == 0;
}

bool contains(const char* value, const char* fragment) noexcept {
    if (!value || !fragment) return false;
    const std::string_view text(value);
    return text.find(fragment) != std::string_view::npos;
}

bool classify_source_layer(const char* name, WorldFeatureKind& kind) noexcept {
    if (equals(name, "asphalt")) {
        kind = WorldFeatureKind::Road;
        return true;
    }
    if (equals(name, "Buildings") || equals(name, "Buildings_bg") ||
        equals(name, "Buildings_base") || equals(name, "Buildings_base_top") ||
        equals(name, "Buildings_shadows")) {
        kind = WorldFeatureKind::Building;
        return true;
    }
    if (equals(name, "zed_resp") || equals(name, "Zed_resp") ||
        equals(name, "LOOT_respawn") || equals(name, "Bot_respawn") ||
        equals(name, "Tourist_points")) {
        kind = WorldFeatureKind::SpawnPoint;
        return true;
    }
    if (contains(name, "Bunker")) {
        kind = WorldFeatureKind::Bunker;
        return true;
    }
    if (contains(name, "Bridge")) {
        kind = WorldFeatureKind::Bridge;
        return true;
    }
    if (equals(name, "Water")) {
        kind = WorldFeatureKind::Water;
        return true;
    }
    if (equals(name, "Obstacle_enviroment") || equals(name, "Forests") ||
        equals(name, "Trees")) {
        kind = WorldFeatureKind::Obstacle;
        return true;
    }
    if (equals(name, "bg") || equals(name, "snow") || equals(name, "sand") ||
        equals(name, "swamp") || equals(name, "ground_enviroment")) {
        kind = WorldFeatureKind::Terrain;
        return true;
    }
    return false;
}


bool compare_source_value(std::uint8_t op, double actual,
                          std::uint8_t expected) noexcept {
    switch (op) {
    case 0: return actual == expected;
    case 1: return actual != expected;
    case 2: return actual < expected;
    case 3: return actual <= expected;
    case 4: return actual > expected;
    case 5: return actual >= expected;
    default: return true; // 255 is the source-table wildcard.
    }
}

// Source close event 243854400993751 creates object 348 from Door's
// frame-0 Imagepoint 1, assigns animation 0/frame 15 and Door.Angle+6.
// Keep the exact original companion transform when the door opens/closes.
// Root c2runtime.js Sprite frame-change handler r.Zi:
// width *= new.width/old.width; height *= new.height/old.height;
// update hotspot and collision data. Preserve authored SetSize scaling
// instead of stretching the opened door into its closed frame dimensions.
void apply_source_building_frame_geometry(WorldPlacement& p,
    std::uint16_t animation,std::uint32_t frame) noexcept {
    if(p.source_animation==animation && p.source_frame==frame)return;
    const auto* old_frame=stage1::source_building_frame_geometry(
        p.object,p.source_animation,p.source_frame);
    const auto* new_frame=stage1::source_building_frame_geometry(
        p.object,animation,frame);
    if(!old_frame || !new_frame || old_frame->width<=0 || old_frame->height<=0)return;
    if(p.width>0.0f)p.width*=new_frame->width/old_frame->width;
    if(p.height>0.0f)p.height*=new_frame->height/old_frame->height;
    p.origin_x=new_frame->origin_x;
    p.origin_y=new_frame->origin_y;
}

bool sync_source_door_solid_placement(
    const WorldPlacement& door,WorldPlacement& solid) noexcept {
    if(door.object!=349 ||
       solid.object!=world_source::door_solid348::kObject)return false;
    const auto* point=world_source::source_door_point(
        door.source_animation,0);
    if(!point || !(door.width>0.0f) || !(door.height>0.0f))
        return false;
    constexpr float radians=0.01745329251994329577f;
    const float angle=door.angle*radians;
    // Initial/reloaded doors may be open. The parked companion still
    // belongs to the CLOSED frame's size, not the taller open artwork.
    const auto* active=world_source::source_door_point(
        door.source_animation,door.source_frame);
    const float closed_width=active&&active->width>0.0f?
        door.width*point->width/active->width:door.width;
    const float closed_height=active&&active->height>0.0f?
        door.height*point->height/active->height:door.height;
    const float dx=closed_width*
        (point->solid_anchor_x-point->origin_x)*
        (door.scale_x<0.0f?-1.0f:1.0f);
    const float dy=closed_height*
        (point->solid_anchor_y-point->origin_y);
    const float x=door.x+dx*std::cos(angle)-dy*std::sin(angle);
    const float y=door.y+dx*std::sin(angle)+dy*std::cos(angle);
    if(!std::isfinite(x)||!std::isfinite(y))return false;
    solid.x=x;
    solid.y=y;
    solid.width=world_source::door_solid348::kWidth;
    solid.height=world_source::door_solid348::kHeight;
    solid.origin_x=world_source::door_solid348::kOriginX;
    solid.origin_y=world_source::door_solid348::kOriginY;
    solid.angle=door.angle+
        world_source::door_solid348::kDoorAngleOffsetDegrees;
    solid.source_animation=0;
    solid.source_frame=world_source::door_solid348::kClosedFrame;
    solid.opacity=door.source_frame==0?1.0f:0.0f;
    return true;
}

std::uint16_t find_layer_index_by_name(std::uint16_t layout,
                                       const char* name) noexcept {
    if (!name) return 0xffff;
    for (const auto& layer : stage1::kLayerTable) {
        if (layer.layout == layout && std::strcmp(layer.name, name) == 0)
            return layer.index;
    }
    return 0xffff;
}

bool is_road_anchor(std::uint16_t code) noexcept {
    return code == 9 || code == 6 || code == 103 || code == 11 || code == 15;
}

void generate_source_roads(WorldGrid& grid, std::uint8_t level, std::uint8_t active_y_count) {
    // Root function generate_array_roads_1 (UID 921061167763666)
    // repeats Map_size_X (=16) rows, even on the 16x33 split-map worlds.
    // Only generate_array_roads_2 makes an additional pass at Y+17.
    // The previous grid.height() loop invented second-half horizontal roads.
    for (int row = 0; row < grid.width() && row < grid.height(); ++row) {
        int first = -1;
        int second = -1;
        for (int x = 0; x < grid.width(); ++x) {
            if (!is_road_anchor(grid.at(x, row))) continue;
            if (first == -1) first = x;
            else if (second == -1) second = x;
        }
        if (first >= 0 && second > first) {
            for (int offset = 0; offset < second - first; ++offset) {
                const int x = first + offset;
                if (grid.at(x, row) == 0) grid.set(x, row, 13);
            }
        }
    }

    const auto add_vertical_roads = [&grid, active_y_count](int y_offset) {
        // generate_array_roads_2 loops Map_size_Y on both axes. Array reads
        // outside the declared dimensions return zero and writes are ignored.
        for (int column = 0; column < active_y_count; ++column) {
            int first = -1;
            int second = -1;
            for (int offset = 0; offset < active_y_count; ++offset) {
                const int y = offset + y_offset;
                if (!is_road_anchor(grid.at(column, y))) continue;
                if (first == -1) first = y;
                else if (second == -1) second = y;
            }
            if (first < 0 || second <= first) continue;
            for (int offset = 0; offset < second - first; ++offset) {
                const int y = first + offset;
                const std::uint16_t cell = grid.at(column, y);
                if (cell == 13) grid.set(column, y, 14);
                else if (cell == 0) grid.set(column, y, 12);
            }
        }
    };

    add_vertical_roads(0);
    if (level >= 4) add_vertical_roads(17);
}

// std::uniform_int_distribution is not required to use the same mapping on
// every standard-library implementation. Multiplayer clients must turn the
// same seed and MT stream into the same world, so use an explicit unbiased
// bounded draw instead of a library-defined distribution.
std::uint32_t source_random_bounded(std::mt19937& rng, std::uint32_t bound) {
    if (bound <= 1) return 0;
    const std::uint32_t threshold = static_cast<std::uint32_t>(-bound) % bound;
    for (;;) {
        const std::uint32_t value = static_cast<std::uint32_t>(rng());
        if (value >= threshold) return value % bound;
    }
}

template <std::size_t N>
std::uint16_t choose_marker(const std::uint16_t (&values)[N], std::mt19937& rng) {
    return values[source_random_bounded(rng, static_cast<std::uint32_t>(N))];
}

std::uint16_t source_marker_type(std::uint16_t code, std::uint8_t level,
                                 std::mt19937& rng) {
    static constexpr std::uint16_t kDefaultMarkers[] =
        {3,1,2,8,10,1,2,8,10,52,52};
    static constexpr std::uint16_t kLevelFourMarkers[] =
        {1,2,8,10,1,2,8,10,52,52};
    static constexpr std::uint16_t kVillageMarkers[] = {9,4,16,17};
    static constexpr std::uint16_t kCityMarkers[] = {6,21};
    static constexpr std::uint16_t kCodeTwoMarkers[] =
        {23,24,25,26,27,28,29,40,42,43,44,45,46,47,48,49,60,61,62,63,64};

    switch (code) {
    case 0:
        return level >= 4 ? choose_marker(kLevelFourMarkers, rng)
                          : choose_marker(kDefaultMarkers, rng);
    case 9: return choose_marker(kVillageMarkers, rng);
    case 6: return choose_marker(kCityMarkers, rng);
    case 5: return 22;
    case 14: case 15: case 13: case 12: case 11: case 50: case 51: case 52:
    case 53: case 54: case 55: case 56: case 57: case 99: case 100: case 101:
    case 102: case 103: case 104: case 105: return code;
    case 17: return 51;
    case 1: return 20;
    case 2: return choose_marker(kCodeTwoMarkers, rng);
    default: return code;
    }
}

bool place_poi_count(WorldGrid& grid, std::uint8_t count, std::uint16_t code,
                     std::mt19937& rng, int min_y, int max_y) {
    if (count == 0) return true;
    if (grid.width() < 3 || min_y < 0 || max_y >= grid.height() || min_y > max_y) return false;
    // Root generate_array_locations event 214943915460313 (and
    // corresponding level branches) sets check_X=floor(random(1,Map_size_X))
    // and check_Y=floor(random(1,Map_size_Y)), then writes Array at
    // check_X-1/check_Y-1. Construct random() has an EXCLUSIVE upper
    // limit: for Map_size_X=16, possible centers are columns 0..14,
    // NOT 0..15. The previous inclusive range shifted POIs toward the
    // outermost column/row, breaking original road and location placement.
    const std::uint32_t x_bound = static_cast<std::uint32_t>(grid.width() - 1u);
    const std::uint32_t y_bound = static_cast<std::uint32_t>(max_y - min_y + 1);
    const int max_attempts = static_cast<int>(grid.width()) *
                             static_cast<int>(grid.height()) * 64;
    for (std::uint8_t placed = 0; placed < count; ++placed) {
        bool found = false;
        for (int attempt = 0; attempt < max_attempts; ++attempt) {
            // The source stores at check_X - 1 and check_Y - 1.
            const int x = static_cast<int>(source_random_bounded(rng, x_bound));
            const int y = min_y + static_cast<int>(source_random_bounded(rng, y_bound)) - 1;
            if (!grid.has_empty_3x3(x, y)) continue;
            if (!grid.set(x, y, code)) return false;
            found = true;
            break;
        }
        if (!found) return false;
    }
    return true;
}
 
bool place_gas_station_count(WorldGrid& grid, std::uint8_t count,
                             std::mt19937& rng, int min_y, int max_y) {
    // Exact five-cell footprint written by the original Map generation
    // event: center 99, north/south 100, west/east 101. The event first
    // verifies that the surrounding 3x3 Array cells are all zero.
    if (count == 0) return true;
    if (grid.width() < 3 || min_y < 1 || max_y >= grid.height()) return false;
    const std::uint32_t x_bound = static_cast<std::uint32_t>(grid.width() - 1u);
    const std::uint32_t y_bound = static_cast<std::uint32_t>(max_y - min_y + 1);
    const int max_attempts = static_cast<int>(grid.width()) *
                             static_cast<int>(grid.height()) * 64;
    for (std::uint8_t placed = 0; placed < count; ++placed) {
        bool found = false;
        for (int attempt = 0; attempt < max_attempts; ++attempt) {
            const int x = static_cast<int>(source_random_bounded(rng, x_bound));
            const int y = min_y + static_cast<int>(source_random_bounded(rng, y_bound)) - 1;
            if (!grid.has_empty_3x3(x, y)) continue;
            // Validate the full footprint before writing any part of it.
            // Construct Array writes outside its dimensions are ignored;
            // edge locations remain valid when the 3x3 emptiness test passes.
            (void)grid.set(x, y, 99);
            (void)grid.set(x, y - 1, 100);
            (void)grid.set(x, y + 1, 100);
            (void)grid.set(x - 1, y, 101);
            (void)grid.set(x + 1, y, 101);
            found = true;
            break;
        }
        if (!found) return false;
    }
    return true;
}

} // namespace

bool WorldBounds::contains(WorldPoint p) const noexcept {
    return std::isfinite(p.x) && std::isfinite(p.y) &&
           p.x >= 0.0f && p.y >= 0.0f && p.x <= width && p.y <= height;
}

WorldPoint WorldBounds::clamp(WorldPoint p) const noexcept {
    if (!std::isfinite(p.x)) p.x = 0.0f;
    if (!std::isfinite(p.y)) p.y = 0.0f;
    return {
        std::clamp(p.x, 0.0f, std::max(0.0f, width)),
        std::clamp(p.y, 0.0f, std::max(0.0f, height))
    };
}

bool WorldGrid::resize(std::uint8_t width, std::uint8_t height) {
    if (width == 0 || height == 0) return false;
    width_ = width;
    height_ = height;
    cells_.assign(static_cast<std::size_t>(width) * height, 0);
    return true;
}

void WorldGrid::clear() noexcept {
    width_ = 0;
    height_ = 0;
    cells_.clear();
}

std::uint16_t WorldGrid::at(int x, int y) const noexcept {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return 0;
    return cells_[static_cast<std::size_t>(y) * width_ + static_cast<std::size_t>(x)];
}

bool WorldGrid::set(int x, int y, std::uint16_t value) noexcept {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) return false;
    cells_[static_cast<std::size_t>(y) * width_ + static_cast<std::size_t>(x)] = value;
    return true;
}

bool WorldGrid::has_empty_3x3(int x, int y) const noexcept {
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (at(x + dx, y + dy) != 0) return false;
        }
    }
    return true;
}

bool source_world_profile(std::uint8_t level, WorldGenerationProfile& p) noexcept {
    p = {};
    p.map_size_x = 16;
    p.map_size_y = 16;
    p.grid_height = 16;

    switch (level) {
    case 0:
        p.villages=20; p.cities=3; p.hospitals=1; p.firestations=1;
        p.gas_stations=1; p.roadside_cafes=1; return true;
    case 1:
        p.villages=15; p.militaries=2; p.cities=5; p.hospitals=2;
        p.firestations=2; p.chaoshi=1; p.gas_stations=2;
        p.lite_shop=1; p.roadside_cafes=2; return true;
    case 2:
        p.villages=6; p.militaries=4; p.cities=10; p.hospitals=3;
        p.firestations=3; p.chaoshi=1; p.gas_stations=3;
        p.roadside_cafes=3; return true;
    case 3:
        p.villages=3; p.militaries=5; p.cities=11; p.hospitals=4;
        p.firestations=2; p.chaoshi=1; p.gas_stations=4;
        p.roadside_cafes=4; return true;
    case 4:
        p.grid_height=33; p.second_half_start_y=18;
        p.villages=4; p.militaries=8; p.cities=6; p.hospitals=4;
        p.firestations=4; p.chaoshi=1;
        p.second_half_villages=4; p.second_half_militaries=6;
        p.second_half_cities=3; p.second_half_hospitals=2;
        p.second_half_firestations=2; p.second_half_chaoshi=2;
        p.gas_stations=8; p.roadside_cafes=8; return true;
    default:
        if (level != 5) return false;
        // The source's CurrentLevel > 4 and >= 4 branches combine a first
        // and second map half with these independent POI quotas.
        p.grid_height=33; p.second_half_start_y=18;
        p.villages=3; p.militaries=6; p.cities=3; p.hospitals=2;
        p.firestations=2; p.chaoshi=2;
        p.second_half_villages=4; p.second_half_militaries=6;
        p.second_half_cities=3; p.second_half_hospitals=2;
        p.second_half_firestations=2; p.second_half_chaoshi=2;
        p.gas_stations=8; p.roadside_cafes=8; return true;
    }
}

bool WorldSystem::initialize(std::uint32_t seed, WorldBounds bounds) noexcept {
    if (!std::isfinite(bounds.width) || !std::isfinite(bounds.height) ||
        bounds.width <= 0.0f || bounds.height <= 0.0f) return false;
    bounds_ = bounds;
    seed_ = seed;
    grid_.clear();
    map_markers_.clear();
    features_.clear();
    placements_.clear();
    original_sprite_states_.clear();
    authored_placement_count_=0;
    source_location_spawn_count_ = 0;
    generation_phase_ = 0;
    generation_level_ = 0;
    generation_is_map_ = false;
    initialized_ = true;
    return true;
}

bool WorldSystem::begin_source_generation(std::uint32_t seed,
                                          std::string_view name,
                                          std::uint8_t level) {
    if (!initialize(seed) || !load_source_layout(name)) {
        reset();
        return false;
    }
    generation_level_ = level;
    generation_is_map_ = name == "Map";
    generation_phase_ = 1;
    return true;
}

bool WorldSystem::advance_source_generation() {
    if (!initialized_ || generation_phase_ == 0) return false;
    if (generation_phase_ == 6) return true;
    bool ok = false;
    switch (generation_phase_) {
    case 1: ok = generate_procedural_grid(generation_level_, seed_); break;
    case 2: ok = build_map_markers(generation_level_, seed_); break;
    case 3: ok = !generation_is_map_ || generate_source_locations(generation_level_); break;
    case 4: ok = !generation_is_map_ || load_source_bunker_spawns(false); break;
    case 5: ok = !generation_is_map_ || load_source_bridge_elements(); break;
    default: return false;
    }
    if (!ok) {
        reset();
        return false;
    }
    ++generation_phase_;
    if (generation_phase_ == 6) {
        // The original Doors_auto close event creates a separate Solid
        // Sprite 348 linked to each visual door 349. Do this AFTER the
        // original terrain/locations, before assigning snapshot IDs.
        if (!create_source_door_companions()) {
            reset();
            return false;
        }
        // Save the generated baseline for change-only snapshots. This runs
        // after ALL source location/bunker/bridge creation has completed.
        original_sprite_states_.clear();
        authored_placement_count_=placements_.size();
        original_sprite_states_.reserve(placements_.size());
        for (const auto& p : placements_)
            original_sprite_states_.push_back({
                p.runtime_uid,p.object,p.source_animation,
                p.source_frame,p.opacity,p.layer,p.source_variable_0});
    }
    return true;
}

bool WorldSystem::generate_from_source(std::uint32_t seed, std::string_view name,
                                       std::uint8_t level) {
    if (!begin_source_generation(seed, name, level)) return false;
    while (!generation_complete()) {
        if (!advance_source_generation()) return false;
    }
    return true;
}

bool WorldSystem::generate_procedural_grid(std::uint8_t level, std::uint32_t seed) {
    if (!initialized_) return false;
    map_markers_.clear();
    WorldGenerationProfile profile{};
    if (!source_world_profile(level, profile) ||
        !grid_.resize(profile.map_size_x, profile.grid_height)) return false;

    seed_ = seed;
    rng_.seed(seed);
    const int first_min_y = 1;
    // Floor(random(1,Map_size_Y)) has an exclusive end; for 16 rows it
    // yields check_Y 1..15 and stores at Y-1 -> rows 0..14.
    // On split maps rows 15/16 remain the original separator.
    const int first_max_y = static_cast<int>(profile.map_size_y) - 1;
    const std::uint8_t first_gas_stations = profile.second_half_start_y
        ? static_cast<std::uint8_t>(profile.gas_stations / 2)
        : profile.gas_stations;
    const std::uint8_t second_gas_stations = static_cast<std::uint8_t>(
        profile.gas_stations - first_gas_stations);
    const std::uint8_t first_roadside_cafes = profile.second_half_start_y
        ? static_cast<std::uint8_t>(profile.roadside_cafes / 2)
        : profile.roadside_cafes;
    const std::uint8_t second_roadside_cafes = static_cast<std::uint8_t>(
        profile.roadside_cafes - first_roadside_cafes);
    const std::uint8_t first_lite_shops = profile.second_half_start_y
        ? static_cast<std::uint8_t>(profile.lite_shop / 2)
        : profile.lite_shop;
    const std::uint8_t second_lite_shops = static_cast<std::uint8_t>(
        profile.lite_shop - first_lite_shops);
    bool placed =
        place_poi_count(grid_, profile.villages, 9, rng_, first_min_y, first_max_y) &&
        place_poi_count(grid_, profile.militaries, 5, rng_, first_min_y, first_max_y) &&
        place_poi_count(grid_, profile.cities, 6, rng_, first_min_y, first_max_y) &&
        place_poi_count(grid_, profile.hospitals, 11, rng_, first_min_y, first_max_y) &&
        place_poi_count(grid_, profile.firestations, 15, rng_, first_min_y, first_max_y) &&
        place_poi_count(grid_, profile.chaoshi, 103, rng_, first_min_y, first_max_y) &&
        // Keep the gas-station footprint (center plus one row north/south)
        // inside the first map half, away from the split-map separator.
        place_gas_station_count(grid_, first_gas_stations, rng_,
                                first_min_y, first_max_y - (profile.second_half_start_y ? 1 : 0)) &&
        place_poi_count(grid_, first_lite_shops, 51, rng_, first_min_y, first_max_y) &&
        place_poi_count(grid_, first_roadside_cafes, 50, rng_, first_min_y, first_max_y);
    if (!placed) return false;

    if (profile.second_half_start_y != 0) {
        const int second_min_y = profile.second_half_start_y;
        // Original second-half random(18,Map_size_Y+17) yields 18..32,
        // and Array writes at Y-1 -> rows 17..31; upper bound 33 excluded.
        const int second_max_y = static_cast<int>(profile.grid_height) - 1;
        placed =
            place_poi_count(grid_, profile.second_half_villages, 9, rng_, second_min_y, second_max_y) &&
            place_poi_count(grid_, profile.second_half_militaries, 5, rng_, second_min_y, second_max_y) &&
            place_poi_count(grid_, profile.second_half_cities, 6, rng_, second_min_y, second_max_y) &&
            place_poi_count(grid_, profile.second_half_hospitals, 11, rng_, second_min_y, second_max_y) &&
            place_poi_count(grid_, profile.second_half_firestations, 15, rng_, second_min_y, second_max_y) &&
            place_poi_count(grid_, profile.second_half_chaoshi, 103, rng_, second_min_y, second_max_y) &&
            // The five-cell gas-station footprint extends one row above its
            // center. Keep that footprint out of the two-row split-map gap.
            place_gas_station_count(grid_, second_gas_stations, rng_,
                                    second_min_y + 1, second_max_y) &&
            place_poi_count(grid_, second_lite_shops, 51, rng_, second_min_y, second_max_y) &&
            place_poi_count(grid_, second_roadside_cafes, 50, rng_, second_min_y, second_max_y);
        if (!placed) return false;
    }

    generate_source_roads(grid_, level, profile.map_size_y);
    return true;
}

bool WorldSystem::build_map_markers(std::uint8_t level, std::uint32_t seed) {
    if (!initialized_ || grid_.width() == 0 || grid_.height() == 0) return false;
    map_markers_.clear();
    map_markers_.reserve(static_cast<std::size_t>(grid_.width()) * grid_.height());
    (void)seed; // The source uses one global random stream across generation stages.
    for (std::uint8_t y = 0; y < grid_.height(); ++y) {
        for (std::uint8_t x = 0; x < grid_.width(); ++x) {
            const std::uint16_t source_code = grid_.at(x, y);
            map_markers_.push_back({
                x, y, source_code, source_marker_type(source_code, level, rng_)
            });
        }
    }
    return true;
}

bool WorldSystem::load_source_layout(std::string_view name) {
    const auto* layout = stage1::find_layout(name);
    if (!layout) return false;
    const auto index = static_cast<std::uint16_t>(layout - stage1::kLayoutTable);
    const auto range = stage1::instances_for_layout(index);
    bounds_ = {layout->width, layout->height};
    grid_.clear();
    map_markers_.clear();
    features_.clear();
    placements_.clear();
    original_sprite_states_.clear();
    placements_.reserve(range.size);
    features_.reserve(range.size + stage1::kBunkerSpawnSourceCount +
                      stage1::kBridgeElementSourceCount);

    for (std::size_t i = 0; i < range.size; ++i) {
        const auto& src = range.data[i];
        placements_.push_back({
            src.layer, src.object, src.uid, src.x, src.y, src.z, src.w, src.h,
            src.angle, src.opacity, src.scale_x, src.ox, src.oy
        });
        // Keep Construct's original UID as evidence, but never assume it
        // uniquely identifies a runtime object. The source export repeats
        // some authored UIDs. Give duplicates an index-namespaced ID just
        // as we do for procedural creations, without rewriting source_uid.
        std::uint64_t runtime_id = src.uid;
        const auto already_assigned = std::find_if(
            placements_.begin(), placements_.end()-1,
            [runtime_id](const WorldPlacement& p) {
                return p.runtime_uid == runtime_id;
            });
        if (runtime_id == 0 || already_assigned != placements_.end()-1)
            runtime_id = (std::uint64_t{1} << 63) |
                static_cast<std::uint64_t>(placements_.size());
        placements_.back().runtime_uid = runtime_id;
        placements_.back().instance_id = static_cast<std::uint64_t>(placements_.size());
        const auto* layer = stage1::find_layer(index, src.layer);
        WorldFeatureKind kind{};
        if (!layer || !classify_source_layer(layer->name, kind)) continue;
        features_.push_back({kind,{src.x,src.y},src.uid,src.object,src.layer,
                             src.object,src.w,src.h,src.angle,0});
    }
    initialized_ = true;
    return true;
}


bool WorldSystem::generate_source_locations(std::uint8_t level) {
    if (!initialized_ || grid_.width() == 0 || grid_.height() == 0) return false;
    const auto* layout = stage1::find_layout("Map");
    if (!layout) return false;
    const auto layout_index = static_cast<std::uint16_t>(layout - stage1::kLayoutTable);

    // Feed the generated grid into the original Construct map Array. The
    // location event interpreter then runs the actual per-tile event branch;
    // no pre-expanded System Create offset table is used here.
    construct::Array map_array;
    if (!map_array.set_size(grid_.width(), grid_.height(), 1)) return false;
    for (int y = 0; y < grid_.height(); ++y) {
        for (int x = 0; x < grid_.width(); ++x) {
            if (!map_array.set_at_xy(x, y,
                    construct::Value::from_number(grid_.at(x, y)))) return false;
        }
    }

    construct::SourceLocationEventRuntime runtime;
    if (!runtime.initialize(level, map_array, rng_)) return false;

    const double cell_width = bounds_.width / static_cast<double>(grid_.width());
    const double cell_height = bounds_.height / static_cast<double>(grid_.height());
    // Construct generates a terrain/location marker for EVERY source Array
    // cell, including cells whose raw POI/road code is zero. Those empty
    // cells are assigned one of the original wilderness marker types by
    // build_map_markers(). Routing only raw nonzero codes loses virtually
    // all grassland, forest, and other procedurally placed world objects.
    if (map_markers_.size() !=
        static_cast<std::size_t>(grid_.width()) * grid_.height()) return false;
    std::size_t added = 0;
    for (int y = 0; y < grid_.height(); ++y) {
        for (int x = 0; x < grid_.width(); ++x) {
            const auto& marker = map_markers_[
                static_cast<std::size_t>(y) * grid_.width() + x];
            const std::uint16_t code = marker.marker_type;
            // Original Mapgen514 physical position is transposed and
            // uses 1020px pitch, not normalized world-layout cell centres.
            const auto source_hotspot=world_source::mapgen_hotspot(x,y);
            const WorldPoint hotspot{source_hotspot.x,source_hotspot.y};
            if(!bounds_.contains(hotspot))return false;

            // The root game's create_map_location reads Array 525 after
            // each generate_location() call. Rows 0, 1, 2, 3, 5 and 6
            // supply create_map_element with var1, var2, x, y, var3 and var6.
            // Without translating these location proposals, only invisible
            // zombie respawn markers are created and the map looks empty.
            if (!runtime.reset_location_array()) return false;
            for (const auto& source : stage1::kLocationTemplateSource) {
                bool tile_matches = false;
                for (std::uint8_t i = 0; i < source.tile_code_count; ++i) {
                    if (source.tile_codes[i] == code) {
                        tile_matches = true;
                        break;
                    }
                }
                if (!tile_matches) continue;

                runtime.set_mapgen(code, static_cast<std::uint8_t>(x),
                                   static_cast<std::uint8_t>(y),
                                   hotspot.x, hotspot.y, cell_width, cell_height);
                bool matched = false;
                if (!runtime.execute_branch(source.event_uid, matched)) return false;
                if (!matched) continue;
            }
            const auto& locations=runtime.location_array();
            // Construct's original location templates legitimately resize
            // Array 525 to 5, 6, 7 or 8 rows. The element's X/Y are in
            // rows 2/3; optional row 6 is an out-of-range zero on short
            // arrays, exactly like Construct Array.at(x,6).
            if (locations.height()<4 || locations.width()>256)
                return false;
            for (std::size_t entry=0;entry<locations.width();++entry) {
                const double raw_type=locations.at(static_cast<int>(entry),0).as_number();
                const double raw_variant=locations.at(static_cast<int>(entry),1).as_number();
                const double raw_x=locations.at(static_cast<int>(entry),2).as_number();
                const double raw_y=locations.at(static_cast<int>(entry),3).as_number();
                const double element_state=locations.at(static_cast<int>(entry),5).as_number();
                const double raw_extra=locations.at(static_cast<int>(entry),6).as_number();
                if (!std::isfinite(raw_type)||!std::isfinite(raw_variant)||
                    !std::isfinite(raw_x)||!std::isfinite(raw_y)||
                    !std::isfinite(element_state)||!std::isfinite(raw_extra)) return false;
                // The source Array includes unused rows. Only populated
                // entries select one of the original element categories.
                if (raw_type<1.0 || raw_variant<1.0) continue;
                if (raw_type>255.0 || raw_variant>255.0 ||
                    raw_extra<0.0 || raw_extra>255.0) continue;
                (void)expand_map_element(
                    static_cast<std::uint8_t>(raw_type),
                    static_cast<std::uint8_t>(raw_variant),
                    static_cast<std::uint8_t>(raw_extra),
                    WorldPoint{static_cast<float>(raw_x),
                               static_cast<float>(raw_y)}, element_state);
            }
        }
    }

    for (const auto& spawn : runtime.spawns()) {
        const std::uint16_t layer =
            find_layer_index_by_name(layout_index, spawn.layer.c_str());
        if (layer == 0xffff) return false;
        const WorldPoint position{static_cast<float>(spawn.x),
                                  static_cast<float>(spawn.y)};
        if (!bounds_.contains(position)) continue;

        const auto* object = stage1::find_object(spawn.object);
        // Construct SetFrame selects a different original atlas rectangle
        // whose dimensions and hotspot can differ from the first image.
        // Preserve explicit SetSize overrides before falling back to the
        // source frame dimensions, then the original Sprite defaults.
        const auto* frame=stage1::source_building_frame_geometry(
            spawn.object,0,static_cast<std::uint32_t>(
                std::max(0,spawn.frame)));
        float object_width = static_cast<float>(spawn.width);
        float object_height = static_cast<float>(spawn.height);
        if (object_width <= 0.0f && frame) object_width = frame->width;
        if (object_height <= 0.0f && frame) object_height = frame->height;
        if (object_width <= 0.0f && object) object_width = object->width;
        if (object_height <= 0.0f && object) object_height = object->height;
        if (object_width <= 0.0f || object_height <= 0.0f) {
            for (const auto& instance : stage1::kInstances) {
                if (instance.object != spawn.object ||
                    instance.w <= 0.0f || instance.h <= 0.0f) continue;
                if (object_width <= 0.0f) object_width = instance.w;
                if (object_height <= 0.0f) object_height = instance.h;
                break;
            }
        }

        const auto runtime_id = (std::uint64_t{1} << 63) |
            (static_cast<std::uint64_t>(placements_.size()) + 1);
        placements_.push_back({
            layer, spawn.object, 0, position.x, position.y, 0.0f,
            object_width, object_height,
            static_cast<float>(spawn.angle),
            static_cast<float>(spawn.opacity), 1.0f,
            frame ? frame->origin_x :
                (object ? object->origin_x : 0.0f),
            frame ? frame->origin_y :
                (object ? object->origin_y : 0.0f), 0,
            static_cast<std::uint32_t>(std::max(0, spawn.frame))
        });
        auto& placed = placements_.back();
        placed.runtime_uid = runtime_id;
        placed.instance_id = static_cast<std::uint64_t>(placements_.size());
        // Construct Sprite Set Instance Variable command 88 is executed
        // by the source location runtime. Only expose original loot
        // category 15 when the original object 218 has received it.
        if (spawn.object == 218) {
            const double c = spawn.instance_variables[0].as_number();
            if (std::isfinite(c) && c >= 1.0 && c <= 65535.0 &&
                std::floor(c) == c)
                placed.source_loot_category =
                    static_cast<std::uint16_t>(c);
        }
        WorldFeatureKind kind{};
        if (const auto* source_layer = stage1::find_layer(layout_index, layer);
            source_layer && classify_source_layer(source_layer->name, kind)) {
            features_.push_back({
                kind, position, 0, spawn.object, layer, spawn.object,
                object_width, object_height,
                static_cast<float>(spawn.angle), 0
            });
        }
        ++added;
    }
    source_location_spawn_count_ = added;
    return true;
}

bool WorldSystem::create_source_door_companions() {
    if (!initialized_) return false;
    const auto* layout=stage1::find_layout("Map");
    if(!layout)return false;
    const auto layout_id=static_cast<std::uint16_t>(
        layout-stage1::kLayoutTable);
    const auto buildings=find_layer_index_by_name(layout_id,"Buildings");
    if(buildings==0xffff)return false;
    const std::size_t original_count=placements_.size();
    for(std::size_t i=0;i<original_count;++i) {
        // Indexed relookup is required: push_back may reallocate.
        const auto door=placements_[i];
        if(door.object!=349 || door.runtime_uid==0 ||
           door.linked_runtime_uid!=0)continue;
        const auto* point=world_source::source_door_point(
            door.source_animation,door.source_frame);
        if(!point || !(door.width>0.0f) || !(door.height>0.0f))
            continue; // Unsupported animations stay source-verified later.
        // The helper uses the frame-0 Imagepoint 1 that the original
        // close event selects, even for a door initially spawned open.
        (void)point;
        // Original UID link is per-instance, and frame 15 belongs to
        // source Solid object 348, not the door sprite sheet 349.
        const auto runtime_id=(std::uint64_t{1}<<63) |
            static_cast<std::uint64_t>(placements_.size()+1);
        placements_.push_back({
            buildings,world_source::door_solid348::kObject,0,
            door.x,door.y,0.0f,
            world_source::door_solid348::kWidth,
            world_source::door_solid348::kHeight,
            door.angle+world_source::door_solid348::kDoorAngleOffsetDegrees,
            door.source_frame==0?1.0f:0.0f,1.0f,
            world_source::door_solid348::kOriginX,
            world_source::door_solid348::kOriginY,
            0,world_source::door_solid348::kClosedFrame
        });
        auto& solid=placements_.back();
        solid.runtime_uid=runtime_id;
        solid.instance_id=static_cast<std::uint64_t>(placements_.size());
        solid.linked_runtime_uid=door.runtime_uid;
        solid.parent_runtime_uid=door.runtime_uid;
        solid.house_base_runtime_uid=door.house_base_runtime_uid;
        if(!sync_source_door_solid_placement(door,solid))return false;
        placements_[i].linked_runtime_uid=runtime_id;
        // Original close event 243854400993751 writes
        // 349.SetInstanceVariable(0, 348.UID) after spawning its Solid.
        if(placements_[i].source_frame==0)
            placements_[i].source_variable_0=runtime_id;
    }
    return true;
}

bool WorldSystem::load_source_bunker_spawns(bool map_fs) {
    if (!initialized_) return false;
    std::vector<WorldFeature> replacements;
    replacements.reserve(stage1::kBunkerSpawnSourceCount);
    constexpr float origin_x=2306.0f, origin_y=17464.0f, alternate_offset_x=16900.0f;
    for (const auto& source : stage1::kBunkerSpawnSource) {
        const WorldPoint pos{origin_x+source.x+(map_fs?alternate_offset_x:0.0f),
                             origin_y+source.y};
        if (!bounds_.contains(pos)) return false;
        const std::uint32_t source_value =
            (static_cast<std::uint32_t>(source.slot) << 16) | source.weight;
        replacements.push_back({WorldFeatureKind::Bunker,pos,source.uid,source.kind,
                             18,975,0,0,0,source_value});
    }
    features_.reserve(features_.size() + replacements.size());
    features_.erase(std::remove_if(features_.begin(), features_.end(),
        [](const WorldFeature& item) { return item.kind == WorldFeatureKind::Bunker; }),
        features_.end());
    features_.insert(features_.end(), replacements.begin(), replacements.end());
    return true;
}

bool WorldSystem::load_source_bridge_elements() {
    if (!initialized_) return false;
    std::vector<WorldFeature> replacements;
    replacements.reserve(stage1::kBridgeElementSourceCount);
    for (const auto& source : stage1::kBridgeElementSource) {
        const WorldPoint pos{static_cast<float>(source.x),static_cast<float>(source.y)};
        if (!bounds_.contains(pos)) return false;
        const std::uint32_t variant =
            (static_cast<std::uint32_t>(source.family) << 1) | (source.enabled?1u:0u);
        replacements.push_back({WorldFeatureKind::Bridge,pos,source.uid,variant,
                             41,source.object,0,0,0,source.element});
    }
    features_.reserve(features_.size() + replacements.size());
    features_.erase(std::remove_if(features_.begin(), features_.end(),
        [](const WorldFeature& item) { return item.kind == WorldFeatureKind::Bridge; }),
        features_.end());
    features_.insert(features_.end(), replacements.begin(), replacements.end());
    return true;
}

void WorldSystem::reset() noexcept {
    grid_.clear();
    map_markers_.clear();
    features_.clear();
    placements_.clear();
    original_sprite_states_.clear();
    source_location_spawn_count_ = 0;
    generation_phase_ = 0;
    generation_level_ = 0;
    generation_is_map_ = false;
    seed_ = 0;
    initialized_ = false;
}

bool WorldSystem::add_feature(const WorldFeature& f) {
    if (!initialized_ || !bounds_.contains(f.position)) return false;
    features_.push_back(f);
    return true;
}


std::size_t WorldSystem::expand_map_element(std::uint8_t var1, std::uint8_t var2,
                                            std::uint8_t var6, WorldPoint position,
                                            double var3) {
    if (!initialized_ || !bounds_.contains(position)) return 0;
    const auto* layout = stage1::find_layout("Map");
    if (!layout) return 0;
    const auto layout_index = static_cast<std::uint16_t>(layout - stage1::kLayoutTable);

    std::size_t added = 0;
    // Apply precisely one original source Spawn descriptor. The primary
    // original world table has some known stale/missing house branches;
    // root-only literal house Spawn entries may be safely appended below
    // without copying the building renderer/collision logic.
    const auto emit_source_rule=[&](
        const stage1::WorldElementSpawnSource& rule) {
        const std::uint16_t layer =
            find_layer_index_by_name(layout_index, rule.layer_name);
        if (layer == 0xffff) return;

        const auto runtime_id = (std::uint64_t{1} << 63) |
            (static_cast<std::uint64_t>(placements_.size()) + 1);
        // The source's System Create action doesn't carry a Set Size.
        // Runtime-created vegetation/buildings inherit their own Construct
        // Sprite's initial frame dimensions and hotspot; 0x0 placements
        // previously had no collision, and were poorly culled/rendered.
        const auto* source_object=stage1::find_object(rule.object);
        // ObjectDef width/height/origin are zero in the root snapshot's
        // metadata (stage1_data.hpp); reading them made every generated
        // house foundation 0x0. Original Spawn inherits the ACTUAL Sprite
        // animation0/frame0 dimensions and hotspot. In particular, child
        // roofs/walls/doors compute their named imagepoint offsets using
        // this parent's width and height. A zero-sized parent collapses
        // those offsets even though the renderer can draw frame pixels.
        const auto* initial_frame=stage1::source_building_frame_geometry(
            rule.object,0,0);
        placements_.push_back({
            layer, rule.object, rule.uid, position.x, position.y, 0.0f,
            initial_frame ? initial_frame->width :
                (source_object ? source_object->width : 0.0f),
            initial_frame ? initial_frame->height :
                (source_object ? source_object->height : 0.0f),
            0.0f, 1.0f, 1.0f,
            initial_frame ? initial_frame->origin_x :
                (source_object ? source_object->origin_x : 0.0f),
            initial_frame ? initial_frame->origin_y :
                (source_object ? source_object->origin_y : 0.0f),
            rule.image_point
        });
        placements_.back().runtime_uid = runtime_id;
        placements_.back().instance_id = static_cast<std::uint64_t>(placements_.size());
        // The source create_map_element action may directly Spawn Sprite218
        // on a non-house landmark. Only the EXACT source event UID and
        // SetInstanceVariable(0) grant a category and initial Trigger_spawn.
        if(rule.object==218){
            std::uint16_t category{};
            if(items::source_initial_marker_event_category(rule.uid,category)){
                placements_.back().source_variable_0=category;
                placements_.back().source_loot_category=category;
                placements_.back().source_initial_loot_trigger=
                    items::source_event_triggers_initial_loot(rule.uid);
            }
        }
        // Root event 8279941300293499 / child 3903561981802149:
        // when Mapgen508.var3=0, Sprite442.var0=choose(100,200).
        if(rule.object==world_source::kFurniturePump &&
           var1==3 && var2==2 && var3==0.0)
            placements_.back().source_variable_0=
                100u + 100u * source_random_bounded(rng_, 2u);

        // A root Construct map-element "house" is a SEQUENCE of Sprite
        // Spawn-another-object actions, not one Building-base texture.
        // Source UIDs 6055506806351951 etc. spawn matching roof/door,
        // interior/wall collider and shadow sprites at ORIGINAL imagepoints.
        // These per-frame imagepoints come from source frame[9], including
        // named w1/s1 and numeric 3, not guessed PNG texture rectangles.
        if(var1==7 || var1==4) {
        std::vector<std::pair<std::uint16_t,std::size_t>> spawned;
        spawned.emplace_back(rule.object,placements_.size()-1);
        for(const auto& action:world_source::kSourceHouseCompanionSpawns){
            // Source child condition is Mapgen508.var2 == variant.
            // The archived world element table contains a known stale
            // UID for house 312; the source event's actual variant/base
            // predicate is authoritative and avoids missing its roof.
            if(action.variant!=var2 || action.base_object!=rule.object)continue;
            // Category 4 scenery is distinct from category 7 houses;
            // its exact original event UID also preserves nested variant guards.
            if(var1==4 && action.event_uid!=rule.uid)continue;
            auto parent_index=placements_.size();
            for(auto it=spawned.rbegin();it!=spawned.rend();++it){
                if(it->first==action.parent_object){
                    parent_index=it->second;break;
                }
            }
            if(parent_index>=placements_.size())continue;
            const WorldPlacement parent=placements_[parent_index];
            const auto destination=find_layer_index_by_name(
                layout_index,action.layer);
            if(destination==0xffff)continue;
            const auto* child_obj=stage1::find_object(action.object);
            if(!child_obj)continue;
            const auto* child_frame=stage1::source_building_frame_geometry(
                action.object,action.animation,action.frame);
            float w=child_frame?child_frame->width:child_obj->width;
            float h=child_frame?child_frame->height:child_obj->height;
            float mirror_x=1.0f;
            // The original source immediately resizes wall-trigger 346
            // from base.ImagePointX/Y("w2")-("w1") (SetSize 103),
            // and shadow 347 from "s2"-"s1" (SetHeight 169).
            // Default 346 is 1x1: ignoring this creates an invisible
            // near-zero wall trigger and breaks roof transparency.
            const auto source_delta=[&](
                std::uint16_t object,const char* hi,const char* lo,
                bool horizontal,float& output) noexcept {
                if(object==0xffff || !hi || !lo || !hi[0] || !lo[0])
                    return false;
                for(auto it=spawned.rbegin();it!=spawned.rend();++it){
                    if(it->first!=object)continue;
                    const auto& base=placements_[it->second];
                    const auto* a=world_source::source_house_image_point(
                        object,base.source_animation,base.source_frame,hi,0);
                    const auto* b=world_source::source_house_image_point(
                        object,base.source_animation,base.source_frame,lo,0);
                    if(!a || !b)return false;
                    const float angle=base.angle*0.01745329251994329577f;
                    const float dx=base.width*(a->x-b->x)*
                        (base.scale_x<0.0f?-1.0f:1.0f);
                    const float dy=base.height*(a->y-b->y);
                    const float delta=horizontal?
                        dx*std::cos(angle)-dy*std::sin(angle):
                        dx*std::sin(angle)+dy*std::cos(angle);
                    if(!std::isfinite(delta)||std::fabs(delta)<0.001f)
                        return false;
                    output=delta;
                    return true;
                }
                return false;
            };
            float authored_width=0.0f,authored_height=0.0f;
            if(source_delta(action.width_parent,
                            action.width_hi,action.width_lo,
                            true,authored_width)){
                mirror_x=authored_width<0.0f?-1.0f:1.0f;
                w=std::fabs(authored_width);
            }
            if(source_delta(action.height_parent,
                            action.height_hi,action.height_lo,
                            false,authored_height))
                h=std::fabs(authored_height);
            if(!(w>0.0f) || !(h>0.0f))continue;
            float x=parent.x,y=parent.y;
            if(action.point_index!=0 ||
               (action.point_name && action.point_name[0])) {
                const auto* point=world_source::source_house_image_point(
                    parent.object,parent.source_animation,
                    parent.source_frame,action.point_name,
                    action.point_index);
                if (point) {
                    const float angle = parent.angle * 0.01745329251994329577f;
                    const float dx = parent.width * (point->x - point->origin_x) *
                        (parent.scale_x < 0.0f ? -1.0f : 1.0f);
                    const float dy = parent.height * (point->y - point->origin_y);
                    x += dx * std::cos(angle) - dy * std::sin(angle);
                    y += dx * std::sin(angle) + dy * std::cos(angle);
                } else if (action.point_name && action.point_name[0]) {
                    continue;
                }
                // Missing numeric points use the parent hotspot, as in loot placement.
            }
            if(!std::isfinite(x)||!std::isfinite(y))continue;
            const auto new_uid=(std::uint64_t{1}<<63) |
                static_cast<std::uint64_t>(placements_.size()+1);
            placements_.push_back({
                destination,action.object,action.event_uid,
                x,y,0.0f,w,h,
                action.angle_degrees==99999.0f ?
                    parent.angle : action.angle_degrees,
                action.opacity,mirror_x,
                child_frame?child_frame->origin_x:child_obj->origin_x,
                child_frame?child_frame->origin_y:child_obj->origin_y,
                0,action.frame,action.animation
            });
            auto& child=placements_.back();
            child.runtime_uid=new_uid;
            child.instance_id=static_cast<std::uint64_t>(placements_.size());
            child.parent_runtime_uid=parent.runtime_uid;
            child.house_base_runtime_uid=
                parent.house_base_runtime_uid!=0 ?
                parent.house_base_runtime_uid : parent.runtime_uid;
            // Original same-event SetInstanceVariable(88): 346 walls
            // receive numeric slots, while 347 shadows and 349 doors
            // record the UID of the original chosen parent Sprite.
            // Original object 218 receives the location loot category
            // directly from its source Spawn event, not a guessed roll.
            for(const auto& setting:world_source::kSourceHouseCompanionVariables){
                if(setting.event_uid!=action.event_uid ||
                   setting.base_object!=action.base_object ||
                   setting.variant!=action.variant ||
                   setting.sequence!=action.sequence ||
                   setting.child!=action.object)continue;
                std::uint64_t value=0;
                if(setting.kind==1 && setting.literal_value>=0)
                    value=static_cast<std::uint64_t>(
                        setting.literal_value);
                else if(setting.kind==2){
                    for(auto it=spawned.rbegin();it!=spawned.rend();++it)
                        if(it->first==setting.uid_object){
                            value=placements_[it->second].runtime_uid;
                            break;
                        }
                }
                if(setting.slot==0)child.source_variable_0=value;
                if(setting.slot==1)child.source_variable_1=value;
            }
            if(child.object==218 && child.source_variable_0>0 &&
               child.source_variable_0<=65535)
                child.source_loot_category=static_cast<std::uint16_t>(
                    child.source_variable_0);
            if(child.object==218 && child.source_loot_category!=0)
                child.source_initial_loot_trigger=
                    items::source_event_triggers_initial_loot(action.event_uid);
            // The visual door 349 reserves this link for its separately
            // spawned original Solid 348. Other house pieces remember
            // the original parent Sprite selected by command 73.
            if(child.object!=349)
                child.linked_runtime_uid=parent.runtime_uid;
            spawned.emplace_back(action.object,placements_.size()-1);
        }
        // Separate original var1=7 loot branch (4949969985883762).
        // Markers use the selected source furniture/foundation imagepoints,
        // even when no Trigger_spawn call followed SetInstanceVariable.
        for(const auto& action:world_source::kSourceHouseLootMarkers){
            if(var1!=7 || action.variant!=var2)continue;
            const auto found=std::find_if(spawned.rbegin(),spawned.rend(),
                [&](const auto& pair){return pair.first==action.parent;});
            if(found==spawned.rend())continue;
            const WorldPlacement parent=placements_[found->second];
            const auto destination=find_layer_index_by_name(layout_index,action.layer);
            if(destination==0xffff)continue;
            float x=parent.x,y=parent.y;
            const auto* point=world_source::source_house_image_point(
                parent.object,parent.source_animation,parent.source_frame,
                action.point_name,action.point_index);
            if(point){
                const float angle=parent.angle*0.01745329251994329577f;
                const float dx=parent.width*(point->x-point->origin_x)*
                    (parent.scale_x<0.0f?-1.0f:1.0f);
                const float dy=parent.height*(point->y-point->origin_y);
                x+=dx*std::cos(angle)-dy*std::sin(angle);
                y+=dx*std::sin(angle)+dy*std::cos(angle);
            }else if(action.point_name[0])continue;
            // Construct's absent numeric imagepoint falls back to the hotspot.
            if(action.offset_y)y+=parent.height*0.5f+15.0f;
            if(!std::isfinite(x)||!std::isfinite(y))continue;
            const auto new_uid=(std::uint64_t{1}<<63)|
                static_cast<std::uint64_t>(placements_.size()+1);
            placements_.push_back({
                destination,218,action.action_uid,x,y,0.0f,1.0f,1.0f,
                parent.angle,1.0f,1.0f,0.0f,1.0f,action.point_index
            });
            auto& marker=placements_.back();
            marker.runtime_uid=new_uid;
            marker.instance_id=static_cast<std::uint64_t>(placements_.size());
            marker.parent_runtime_uid=parent.runtime_uid;
            marker.house_base_runtime_uid=placements_[spawned.front().second].runtime_uid;
            marker.source_variable_0=action.category;
            marker.source_loot_category=action.category;
            marker.source_initial_loot_trigger=action.trigger_spawn;
        }
        }

        // Original create_map_element branches 5221960099677299,
        // 1639594650416124, 7109935635557623, 2584406677664733,
        // 2788700463962024, 5413722554242966 do NOT create a visible
        // tree alone. Each immediately creates obst_base object 345 on
        // zed_resp at Tree.ImagePoint("Imagepoint 1"), sets its frame 10,
        // and saves the companion UID in a tree instance variable.
        // Previously those hitboxes were absent, so original projectile
        // Hitting_obstacles branches had nothing to collide with.
        if(const auto* point=world_source::source_tree_impact_point(rule.object)){
            const auto hitbox_layer=
                find_layer_index_by_name(layout_index,"zed_resp");
            if(hitbox_layer!=0xffff){
                const std::size_t tree_index=placements_.size()-1;
                const auto tree=placements_[tree_index];
                const float angle=tree.angle*0.01745329251994329577f;
                const float dx=(point->point_x-point->origin_x)*tree.width;
                const float dy=(point->point_y-point->origin_y)*tree.height;
                const float bx=tree.x+dx*std::cos(angle)-dy*std::sin(angle);
                const float by=tree.y+dx*std::sin(angle)+dy*std::cos(angle);
                const std::uint64_t hitbox_uid=(std::uint64_t{1}<<63) |
                    (static_cast<std::uint64_t>(placements_.size())+1);
                placements_.push_back({
                    hitbox_layer,world_source::kOriginalTreeHitboxObject,0,
                    bx,by,0.0f,
                    world_source::kOriginalTreeHitboxWidth,
                    world_source::kOriginalTreeHitboxHeight,
                    tree.angle,1.0f,1.0f,
                    world_source::kOriginalTreeHitboxOriginX,
                    world_source::kOriginalTreeHitboxOriginY,
                    0,world_source::kOriginalTreeStandingHitboxFrame
                });
                auto& hitbox=placements_.back();
                hitbox.runtime_uid=hitbox_uid;
                hitbox.instance_id=static_cast<std::uint64_t>(placements_.size());
                hitbox.linked_runtime_uid=tree.runtime_uid;
                placements_[tree_index].linked_runtime_uid=hitbox_uid;
                // zed_resp is an invisible gameplay trigger layer; do
                // NOT also add every trunk hitbox as an NPC spawn feature.
            }
        }

        WorldFeatureKind kind{};
        if (classify_source_layer(rule.layer_name, kind)) {
            const std::uint32_t source_variant =
                (static_cast<std::uint32_t>(var1) << 16) |
                (static_cast<std::uint32_t>(var2) << 8) | var6;
            features_.push_back({kind, position, rule.uid, source_variant,
                                 layer, rule.object, 0.0f, 0.0f, 0.0f,
                                 rule.image_point});
        }
        ++added;
    };
    // Category 7 is owned by the authoritative root source, not the
    // older cached map-element table. Preserve original action order and
    // avoid creating stale cached buildings for removed variants.
    if(var1==7) {
        for(const auto& original:world_source::kSourceHousePrimarySpawns) {
            if(original.variant!=var2)continue;
            const stage1::WorldElementSpawnSource rule{
                original.event_uid,0,7,0,original.variant,255,255,
                original.object,original.layer,original.point_index
            };
            emit_source_rule(rule);
        }
    } else {
        for(const auto& rule:stage1::kWorldElementSpawnSource) {
            if(!compare_source_value(rule.var1_op,var1,rule.var1_value) ||
               !compare_source_value(rule.var2_op,var2,rule.var2_value) ||
               !compare_source_value(rule.var3_op,var3,rule.var3_value) ||
               !compare_source_value(rule.var6_op,var6,rule.var6_value))
                continue;
            emit_source_rule(rule);
        }
    }
    return added;
}

const WorldMapMarker* WorldSystem::map_marker(std::size_t i) const noexcept {
    return i < map_markers_.size() ? &map_markers_[i] : nullptr;
}

const WorldFeature* WorldSystem::feature(std::size_t i) const noexcept {
    return i < features_.size() ? &features_[i] : nullptr;
}

std::size_t WorldSystem::feature_count(WorldFeatureKind kind) const noexcept {
    return static_cast<std::size_t>(std::count_if(features_.begin(),features_.end(),
        [kind](const WorldFeature& item){return item.kind==kind;}));
}

bool WorldSystem::set_placement_layer(
    std::size_t index,std::uint16_t layer) noexcept {
    if(index>=placements_.size() || !initialized_)return false;
    const auto* map=stage1::find_layout("Map");
    if(!map)return false;
    const auto map_index=static_cast<std::uint16_t>(
        map-stage1::kLayoutTable);
    if(!stage1::find_layer(map_index,layer))return false;
    placements_[index].layer=layer;
    return true;
}

bool WorldSystem::set_source_pump_water(
    std::size_t index,std::uint64_t water) noexcept {
    if(!initialized_ || index>=placements_.size() ||
       water>world_source::kPumpMaxWater)return false;
    auto& p=placements_[index];
    if(p.object!=world_source::kFurniturePump ||
       p.layer!=world_source::kFurnitureLayer)return false;
    p.source_variable_0=water;
    return true;
}

bool WorldSystem::add_source_constructed_bed(
    float x,float y,std::uint8_t bear_pi) noexcept {
    if(!generation_complete() || !generation_is_map_ ||
       !bounds_.contains({x,y}) || bear_pi>1 ||
       original_sprite_states_.size()!=placements_.size() ||
       placements_.size()>=100000)return false;
    const auto* map=stage1::find_layout("Map");
    if(!map)return false;
    const auto layout_index=static_cast<std::uint16_t>(map-stage1::kLayoutTable);
    const auto layer=find_layer_index_by_name(layout_index,"items_on_ground");
    if(layer==0xffff)return false;
    const auto frames=stage1::animations_for_object(world_source::kFurnitureBed);
    if(!frames.data || frames.size==0 || frames.data[0].frames<2)return false;
    // Original object951 frame0: 37x21 with (0.5,0.5) hotspot.
    const std::uint64_t uid=(std::uint64_t{1}<<63) |
        static_cast<std::uint64_t>(placements_.size()+1);
    placements_.push_back({layer,world_source::kFurnitureBed,0,
        x,y,0.0f,37.0f,21.0f,0.0f,1.0f,1.0f,0.5f,0.5f,0,0});
    auto& p=placements_.back();
    p.runtime_uid=uid;
    p.instance_id=placements_.size();
    p.source_variable_0=bear_pi;
    p.source_constructed_bed=true;
    original_sprite_states_.push_back({uid,p.object,0,0,1.0f,p.layer,bear_pi});
    return true;
}

bool WorldSystem::dismantle_source_bed(std::size_t index) noexcept {
    if(!generation_complete() || index>=placements_.size())return false;
    const auto& p=placements_[index];
    if(p.object!=world_source::kFurnitureBed ||
       p.layer!=world_source::kFurnitureLayer || p.opacity<=0.0f ||
       p.source_variable_0>1)return false;
    // Preserve stable runtime UIDs of doors, buildings and all other beds.
    return set_placement_sprite_state(index,0,0,0.0f);
}

std::vector<SourceConstructedBed> WorldSystem::capture_source_constructed_beds() const {
    std::vector<SourceConstructedBed> result;
    if(!generation_complete())return result;
    for(const auto& p:placements_)if(p.source_constructed_bed){
        result.push_back({p.runtime_uid,p.x,p.y,
            static_cast<std::uint8_t>(p.source_variable_0),p.opacity>0.0f});
    }
    return result;
}

bool WorldSystem::restore_source_constructed_beds(
    const std::vector<SourceConstructedBed>& beds) noexcept {
    if(!generation_complete() || !generation_is_map_ ||
       beds.size()>256 ||
       original_sprite_states_.size()!=placements_.size() ||
       placements_.size()!=authored_placement_count_)return false;
    // Validate all source-created bed records before changing active world.
    for(std::size_t i=0;i<beds.size();++i){
        const auto& b=beds[i];
        const auto id=(std::uint64_t{1}<<63) |
            static_cast<std::uint64_t>(authored_placement_count_+i+1);
        if(b.runtime_uid!=id || b.material_variant>1 ||
           !bounds_.contains({b.x,b.y}))return false;
    }
    for(const auto& b:beds){
        if(!add_source_constructed_bed(b.x,b.y,b.material_variant))return false;
        if(!b.active && !dismantle_source_bed(placements_.size()-1))return false;
    }
    return true;
}

bool WorldSystem::set_linked_door_solid_present(
    std::size_t door_index,bool present) noexcept {
    if(door_index>=placements_.size())return false;
    const auto& door=placements_[door_index];
    if(door.object!=349 || door.linked_runtime_uid==0)return false;
    const std::uint64_t uid=door.linked_runtime_uid;
    if((uid&(std::uint64_t{1}<<63))==0)return false;
    const std::uint64_t slot=uid&~(std::uint64_t{1}<<63);
    if(slot==0 || slot>placements_.size())return false;
    auto& solid=placements_[static_cast<std::size_t>(slot-1)];
    if(solid.object!=world_source::door_solid348::kObject ||
       solid.runtime_uid!=uid ||
       solid.linked_runtime_uid!=door.runtime_uid)return false;
    solid.opacity=present?1.0f:0.0f;
    return true;
}

bool WorldSystem::set_placement_sprite_state(
    std::size_t index, std::uint16_t animation, std::uint32_t frame,
    float opacity) noexcept {
    if (index >= placements_.size() || !std::isfinite(opacity) ||
        opacity < 0.0f || opacity > 1.0f) return false;
    auto& placement = placements_[index];
    // Both native tests and the 3DS renderer share the source Construct
    // animation table. The generated T3X atlas is a build-time artifact and
    // is deliberately NOT a dependency of the native WorldSystem.
    const auto animations = stage1::animations_for_object(placement.object);
    if (animation >= animations.size || !animations.data ||
        frame >= animations.data[animation].frames) return false;
    const bool frame_changed=placement.source_animation!=animation ||
        placement.source_frame!=frame;
    apply_source_building_frame_geometry(placement,animation,frame);
    placement.source_animation = animation;
    placement.source_frame = frame;
    placement.opacity = opacity;
    // A roof-driven door opacity update is not a door frame transition.
    // Do not respawn a collider during the original 0.01-second wait.
    if(frame_changed && placement.object==349 && placement.linked_runtime_uid!=0) {
        // Original open event destroys the linked 348 Solid, while close
        // event creates frame 15 at the authored imagepoint. A stable
        // parked native placement preserves source UID/save identity;
        // transparent 348 has no collision and cannot be rendered.
        const std::uint64_t linked=placement.linked_runtime_uid;
        const auto companion_index=static_cast<std::size_t>(
            (linked&~(std::uint64_t{1}<<63))-1);
        if((linked&(std::uint64_t{1}<<63))!=0 &&
           companion_index<placements_.size()){
            auto& candidate=placements_[companion_index];
            if(candidate.runtime_uid==linked &&
               candidate.object==world_source::door_solid348::kObject &&
               candidate.linked_runtime_uid==placement.runtime_uid){
                (void)sync_source_door_solid_placement(
                    placement,candidate);
                // Construct destroys the selected 348 when opening but
                // does not clear 349.var0. On closing its new 348 UID is
                // written to var0. The parked native 348 has a stable
                // per-door identity across transitions.
                if(frame==0)placement.source_variable_0=linked;
            }
        }
    }
    return true;
}

std::vector<WorldSpriteState> WorldSystem::capture_sprite_states() const {
    std::vector<WorldSpriteState> result;
    // Until generation has completed there is no stable authored baseline.
    if (!generation_complete() ||
        original_sprite_states_.size() != placements_.size()) return result;
    for (std::size_t i=0;i<placements_.size();++i) {
        const auto& p=placements_[i];
        if(p.source_constructed_bed)continue; // full dynamic records in save
        const auto& base=original_sprite_states_[i];
        if (p.source_animation==base.animation &&
            p.source_frame==base.frame && p.opacity==base.opacity &&
            p.layer==base.layer &&
            p.source_variable_0==base.source_variable_0) continue;
        result.push_back({p.runtime_uid,p.object,p.source_animation,
                          p.source_frame,p.opacity,p.layer,p.source_variable_0});
    }
    return result;
}

bool WorldSystem::set_sprite_state_by_runtime_uid(
    std::uint64_t uid, std::uint16_t animation,
    std::uint32_t frame, float opacity) noexcept {
    if (!generation_complete() || uid==0) return false;
    for (std::size_t i=0;i<placements_.size();++i)
        if (placements_[i].runtime_uid==uid)
            return set_placement_sprite_state(i,animation,frame,opacity);
    return false;
}

bool WorldSystem::restore_sprite_states(
    const std::vector<WorldSpriteState>& states) noexcept {
    if (!generation_complete() ||
        original_sprite_states_.size()!=placements_.size()) return false;
    // Fully validate every requested state BEFORE changing any placement.
    // Reject unknown/duplicate runtime IDs, mismatched original object
    // types and unsupported atlas frames; do not corrupt the loaded world.
    for (std::size_t j=0;j<states.size();++j) {
        const auto& state=states[j];
        if (state.runtime_uid==0 || !std::isfinite(state.opacity) ||
            state.opacity<0.0f || state.opacity>1.0f) return false;
        for (std::size_t k=0;k<j;++k)
            if (states[k].runtime_uid==state.runtime_uid) return false;
        const auto it=std::find_if(placements_.begin(),placements_.end(),
            [&](const WorldPlacement& p){return p.runtime_uid==state.runtime_uid;});
        if (it==placements_.end() || it->object!=state.object) return false;
        const auto frames=stage1::animations_for_object(state.object);
        if (!frames.data || state.animation>=frames.size ||
            state.frame>=frames.data[state.animation].frames) return false;
        if(state.object==world_source::kFurniturePump &&
           state.source_variable_0!=~std::uint64_t{0} &&
           state.source_variable_0>world_source::kPumpMaxWater)return false;
        if(state.layer!=0xffff){
            const auto* map=stage1::find_layout("Map");
            if(!map)return false;
            const auto layout=static_cast<std::uint16_t>(map-stage1::kLayoutTable);
            if(!stage1::find_layer(layout,state.layer))return false;
        }
    }
    // An empty snapshot resets all visual states to the source-generated
    // baseline. This is needed for New Game and save-slot replacement.
    for(std::size_t i=0;i<placements_.size();++i) {
        const auto& baseline=original_sprite_states_[i];
        auto& placed=placements_[i];
        apply_source_building_frame_geometry(placed,baseline.animation,baseline.frame);
        placed.source_animation=baseline.animation;
        placed.source_frame=baseline.frame;
        placed.opacity=baseline.opacity;
        placed.layer=baseline.layer;
        placed.source_variable_0=baseline.source_variable_0;
    }
    for(const auto& state:states)
        for(auto& p:placements_)
            if(p.runtime_uid==state.runtime_uid) {
                apply_source_building_frame_geometry(p,state.animation,state.frame);
                p.source_animation=state.animation;
                p.source_frame=state.frame;
                p.opacity=state.opacity;
                if(state.layer!=0xffff)p.layer=state.layer;
                if(state.source_variable_0!=~std::uint64_t{0})
                    p.source_variable_0=state.source_variable_0;
                break;
            }
    // Companion visibility is derived from its linked source door frame.
    // This also repairs pre-companion v1 saves that only stored object 349.
    for(const auto& door:placements_) {
        if(door.object!=349 || !door.linked_runtime_uid)continue;
        const std::uint64_t linked=door.linked_runtime_uid;
        if((linked&(std::uint64_t{1}<<63))==0)continue;
        const auto index=static_cast<std::size_t>(
            (linked&~(std::uint64_t{1}<<63))-1);
        if(index>=placements_.size())continue;
        auto& solid=placements_[index];
        if(solid.runtime_uid==linked &&
           solid.object==world_source::door_solid348::kObject &&
           solid.linked_runtime_uid==door.runtime_uid)
            (void)sync_source_door_solid_placement(door,solid);
    }
    return true;
}

const WorldPlacement* WorldSystem::placement(std::size_t i) const noexcept {
    return i < placements_.size() ? &placements_[i] : nullptr;
}

const WorldPlacement* WorldSystem::placement_by_id(
    std::uint32_t id) const noexcept {
    if (id == 0 || static_cast<std::size_t>(id)>placements_.size()) return nullptr;
    const auto& object = placements_[static_cast<std::size_t>(id)-1];
    return object.instance_id==id ? &object : nullptr;
}

const WorldPlacement* WorldSystem::placement_by_instance_id(
    std::uint64_t id) const noexcept {
    if (id == 0 || id > placements_.size() ||
        id > static_cast<std::uint64_t>(UINT32_MAX)) return nullptr;
    return placement_by_id(static_cast<std::uint32_t>(id));
}

bool WorldTravel::begin(std::uint32_t seed, std::uint8_t level) noexcept {
    cancel();
    try {
        auto candidate = std::make_unique<WorldSystem>();
        if (!candidate->begin_source_generation(seed, "Map", level)) return false;
        destination_ = std::move(candidate);
        return true;
    } catch (const std::bad_alloc&) {
        return false;
    }
}

bool WorldTravel::advance() noexcept {
    if (!destination_) return false;
    try {
        if (destination_->advance_source_generation()) return true;
    } catch (const std::bad_alloc&) {
        // The active map still belongs to the caller.
    }
    cancel();
    return false;
}

} // namespace minidayz
