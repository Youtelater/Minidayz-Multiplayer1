#include "minidayz/world.hpp"
#include "minidayz/stage1_data_access.hpp"
#include "minidayz/generated/world_source_data.hpp"

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
    if (equals(name, "bg") || equals(name, "snow") || equals(name, "sand") ||
        equals(name, "swamp") || equals(name, "ground_enviroment") ||
        equals(name, "Obstacle_enviroment") || equals(name, "Water") ||
        equals(name, "Forests") || equals(name, "Trees")) {
        kind = WorldFeatureKind::Terrain;
        return true;
    }
    return false;
}

bool place_poi_count(WorldGrid& grid, std::uint8_t count, std::uint16_t code,
                     std::mt19937& rng) {
    if (count == 0) return true;
    if (grid.width() < 3 || grid.height() < 3) return false;
    std::uniform_int_distribution<int> xdist(1, static_cast<int>(grid.width()) - 1);
    std::uniform_int_distribution<int> ydist(1, static_cast<int>(grid.height()) - 1);
    const int max_attempts = static_cast<int>(grid.width()) *
                             static_cast<int>(grid.height()) * 64;
    for (std::uint8_t placed = 0; placed < count; ++placed) {
        bool found = false;
        for (int attempt = 0; attempt < max_attempts; ++attempt) {
            const int x = xdist(rng);
            const int y = ydist(rng);
            if (!grid.has_empty_3x3(x, y)) continue;
            if (!grid.set(x, y, code)) return false;
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
    p.map_size_y = (level == 4) ? 33 : 16;
    switch (level) {
    case 0: p.villages=20; p.cities=3; p.hospitals=1; p.firestations=1;
            p.gas_stations=1; p.roadside_cafes=1; return true;
    case 1: p.villages=15; p.militaries=2; p.cities=5; p.hospitals=2;
            p.firestations=2; p.chaoshi=1; p.gas_stations=2;
            p.lite_shop=1; p.roadside_cafes=2; return true;
    case 2: p.villages=6; p.militaries=4; p.cities=10; p.hospitals=3;
            p.firestations=3; p.chaoshi=1; p.gas_stations=3;
            p.roadside_cafes=3; return true;
    case 3: p.villages=3; p.militaries=5; p.cities=11; p.hospitals=4;
            p.firestations=2; p.chaoshi=1; p.gas_stations=4;
            p.roadside_cafes=4; return true;
    case 4: p.villages=4; p.militaries=8; p.cities=6; p.hospitals=4;
            p.firestations=4; p.chaoshi=1; p.gas_stations=4;
            p.lite_shop=2; p.roadside_cafes=4; return true;
    default: return false;
    }
}

bool WorldSystem::initialize(std::uint32_t seed, WorldBounds bounds) noexcept {
    if (!std::isfinite(bounds.width) || !std::isfinite(bounds.height) ||
        bounds.width <= 0.0f || bounds.height <= 0.0f) return false;
    bounds_ = bounds;
    seed_ = seed;
    grid_.clear();
    features_.clear();
    placements_.clear();
    initialized_ = true;
    return true;
}

bool WorldSystem::generate_from_source(std::uint32_t seed, std::string_view name,
                                       std::uint8_t level) {
    if (!initialize(seed) || !load_source_layout(name) ||
        !generate_procedural_grid(level, seed)) {
        reset();
        return false;
    }
    if (name == "Map" &&
        (!load_source_bunker_spawns(false) || !load_source_bridge_elements())) {
        reset();
        return false;
    }
    return true;
}

bool WorldSystem::generate_procedural_grid(std::uint8_t level, std::uint32_t seed) {
    if (!initialized_) return false;
    WorldGenerationProfile profile{};
    if (!source_world_profile(level, profile) ||
        !grid_.resize(profile.map_size_x, profile.map_size_y)) return false;

    seed_ = seed;
    std::mt19937 rng(seed);
    // These are the exact location codes written by generate_array_locations.
    // Gas stations, shops and roadside cafes are separate source counters and
    // are not inserted into this map-marker Array.
    return place_poi_count(grid_, profile.villages, 9, rng) &&
           place_poi_count(grid_, profile.militaries, 5, rng) &&
           place_poi_count(grid_, profile.cities, 6, rng) &&
           place_poi_count(grid_, profile.hospitals, 11, rng) &&
           place_poi_count(grid_, profile.firestations, 15, rng) &&
           place_poi_count(grid_, profile.chaoshi, 103, rng);
}

bool WorldSystem::load_source_layout(std::string_view name) {
    const auto* layout = stage1::find_layout(name);
    if (!layout) return false;
    const auto index = static_cast<std::uint16_t>(layout - stage1::kLayoutTable);
    const auto range = stage1::instances_for_layout(index);
    bounds_ = {layout->width, layout->height};
    features_.clear();
    placements_.clear();
    placements_.reserve(range.size);
    features_.reserve(range.size + stage1::kBunkerSpawnSourceCount +
                      stage1::kBridgeElementSourceCount);

    for (std::size_t i = 0; i < range.size; ++i) {
        const auto& src = range.data[i];
        placements_.push_back({
            src.layer, src.object, src.uid, src.x, src.y, src.z, src.w, src.h,
            src.angle, src.opacity, src.scale_x, src.ox, src.oy
        });
        const auto* layer = stage1::find_layer(index, src.layer);
        WorldFeatureKind kind{};
        if (!layer || !classify_source_layer(layer->name, kind)) continue;
        // Preserve original coordinates, including negative off-layout art.
        features_.push_back({kind,{src.x,src.y},src.uid,src.object,src.layer,
                             src.object,src.w,src.h,src.angle,0});
    }
    initialized_ = true;
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
    features_.clear();
    placements_.clear();
    seed_ = 0;
    initialized_ = false;
}

bool WorldSystem::add_feature(const WorldFeature& f) {
    if (!initialized_ || !bounds_.contains(f.position)) return false;
    features_.push_back(f);
    return true;
}

const WorldFeature* WorldSystem::feature(std::size_t i) const noexcept {
    return i < features_.size() ? &features_[i] : nullptr;
}

std::size_t WorldSystem::feature_count(WorldFeatureKind kind) const noexcept {
    return static_cast<std::size_t>(std::count_if(features_.begin(),features_.end(),
        [kind](const WorldFeature& item){return item.kind==kind;}));
}

const WorldPlacement* WorldSystem::placement(std::size_t i) const noexcept {
    return i < placements_.size() ? &placements_[i] : nullptr;
}

} // namespace minidayz
