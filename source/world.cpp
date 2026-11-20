#include "minidayz/world.hpp"
#include "minidayz/stage1_data_access.hpp"
#include "minidayz/generated/world_source_data.hpp"
#include "minidayz/location_event_runtime.hpp"

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


bool compare_source_value(std::uint8_t op, std::uint8_t actual,
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
    // generate_array_roads_1: connect the first two source road anchors in each row.
    for (int row = 0; row < grid.height(); ++row) {
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
    // Construct's random(1, Map_size_X/Y) includes both endpoints.
    // The event then writes at check_X - 1 / check_Y - 1.
    const std::uint32_t x_bound = grid.width();
    const std::uint32_t y_bound = static_cast<std::uint32_t>(max_y - min_y + 2);
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
    const std::uint32_t x_bound = grid.width();
    const std::uint32_t y_bound = static_cast<std::uint32_t>(max_y - min_y + 2);
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
        // Save the generated baseline for change-only snapshots. This runs
        // after ALL source location/bunker/bridge creation has completed.
        original_sprite_states_.clear();
        original_sprite_states_.reserve(placements_.size());
        for (const auto& p : placements_)
            original_sprite_states_.push_back({
                p.runtime_uid,p.object,p.source_animation,
                p.source_frame,p.opacity});
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
    // On split-map levels the original first-half event uses check_Y 1..15
    // and stores at Y-1, leaving grid rows 15 and 16 as the separator.
    const int first_max_y = static_cast<int>(profile.map_size_y) -
                            (profile.second_half_start_y ? 2 : 1);
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
        // The original second-half POI loop checks Y=18..32 and stores at Y-1,
        // so the last occupied row is 31; row 32 is the unused boundary.
        const int second_max_y = static_cast<int>(profile.grid_height) - 2;
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
            const WorldPoint hotspot{
                static_cast<float>((static_cast<double>(x) + 0.5) * cell_width),
                static_cast<float>((static_cast<double>(y) + 0.5) * cell_height)
            };

            // The root game's create_map_location reads Array 525 after
            // each generate_location() call. Its columns 0,1,2,3 and 6
            // are the original create_map_element var1, var2, x, y, var6.
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
                const double raw_extra=locations.at(static_cast<int>(entry),6).as_number();
                if (!std::isfinite(raw_type)||!std::isfinite(raw_variant)||
                    !std::isfinite(raw_x)||!std::isfinite(raw_y)||
                    !std::isfinite(raw_extra)) return false;
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
                               static_cast<float>(raw_y)});
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
        float object_width = static_cast<float>(spawn.width);
        float object_height = static_cast<float>(spawn.height);
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
            object_width, object_height, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0,
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
                object_width, object_height, 0.0f, 0
            });
        }
        ++added;
    }
    source_location_spawn_count_ = added;
    return true;
}

bool WorldSystem::load_source_bunker_spawns(bool map_fs) {
    if (!initialized_) return false;
    features_.erase(std::remove_if(features_.begin(), features_.end(),
        [](const WorldFeature& item) { return item.kind == WorldFeatureKind::Bunker; }),
        features_.end());
    constexpr float origin_x=2306.0f, origin_y=17464.0f, alternate_offset_x=16900.0f;
    for (const auto& source : stage1::kBunkerSpawnSource) {
        const WorldPoint pos{origin_x+source.x+(map_fs?alternate_offset_x:0.0f),
                             origin_y+source.y};
        if (!bounds_.contains(pos)) return false;
        const std::uint32_t source_value =
            (static_cast<std::uint32_t>(source.slot) << 16) | source.weight;
        features_.push_back({WorldFeatureKind::Bunker,pos,source.uid,source.kind,
                             18,975,0,0,0,source_value});
    }
    return true;
}

bool WorldSystem::load_source_bridge_elements() {
    if (!initialized_) return false;
    features_.erase(std::remove_if(features_.begin(), features_.end(),
        [](const WorldFeature& item) { return item.kind == WorldFeatureKind::Bridge; }),
        features_.end());
    for (const auto& source : stage1::kBridgeElementSource) {
        const WorldPoint pos{static_cast<float>(source.x),static_cast<float>(source.y)};
        if (!bounds_.contains(pos)) return false;
        const std::uint32_t variant =
            (static_cast<std::uint32_t>(source.family) << 1) | (source.enabled?1u:0u);
        features_.push_back({WorldFeatureKind::Bridge,pos,source.uid,variant,
                             41,source.object,0,0,0,source.element});
    }
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
                                            std::uint8_t var6, WorldPoint position) {
    if (!initialized_ || !bounds_.contains(position)) return 0;
    const auto* layout = stage1::find_layout("Map");
    if (!layout) return 0;
    const auto layout_index = static_cast<std::uint16_t>(layout - stage1::kLayoutTable);

    std::size_t added = 0;
    for (const auto& rule : stage1::kWorldElementSpawnSource) {
        if (!compare_source_value(rule.var1_op, var1, rule.var1_value) ||
            !compare_source_value(rule.var2_op, var2, rule.var2_value) ||
            !compare_source_value(rule.var6_op, var6, rule.var6_value)) continue;

        const std::uint16_t layer = find_layer_index_by_name(layout_index, rule.layer_name);
        if (layer == 0xffff) continue;

        const auto runtime_id = (std::uint64_t{1} << 63) |
            (static_cast<std::uint64_t>(placements_.size()) + 1);
        placements_.push_back({
            layer, rule.object, rule.uid, position.x, position.y, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, rule.image_point
        });
        placements_.back().runtime_uid = runtime_id;
        placements_.back().instance_id = static_cast<std::uint64_t>(placements_.size());

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
    placement.source_animation = animation;
    placement.source_frame = frame;
    placement.opacity = opacity;
    return true;
}

std::vector<WorldSpriteState> WorldSystem::capture_sprite_states() const {
    std::vector<WorldSpriteState> result;
    // Until generation has completed there is no stable authored baseline.
    if (!generation_complete() ||
        original_sprite_states_.size() != placements_.size()) return result;
    for (std::size_t i=0;i<placements_.size();++i) {
        const auto& p=placements_[i];
        const auto& base=original_sprite_states_[i];
        if (p.source_animation==base.animation &&
            p.source_frame==base.frame && p.opacity==base.opacity) continue;
        result.push_back({p.runtime_uid,p.object,p.source_animation,
                          p.source_frame,p.opacity});
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
    }
    // An empty snapshot resets all visual states to the source-generated
    // baseline. This is needed for New Game and save-slot replacement.
    for(std::size_t i=0;i<placements_.size();++i) {
        const auto& baseline=original_sprite_states_[i];
        auto& placed=placements_[i];
        placed.source_animation=baseline.animation;
        placed.source_frame=baseline.frame;
        placed.opacity=baseline.opacity;
    }
    for(const auto& state:states)
        for(auto& p:placements_)
            if(p.runtime_uid==state.runtime_uid) {
                p.source_animation=state.animation;
                p.source_frame=state.frame;
                p.opacity=state.opacity;
                break;
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

} // namespace minidayz
