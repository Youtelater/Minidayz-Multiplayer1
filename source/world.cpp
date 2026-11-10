#include "minidayz/world.hpp"
#include "minidayz/stage1_data_access.hpp"
#include "minidayz/generated/world_source_data.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

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

bool WorldSystem::initialize(std::uint32_t seed, WorldBounds bounds) noexcept {
    if (!std::isfinite(bounds.width) || !std::isfinite(bounds.height) ||
        bounds.width <= 0.0f || bounds.height <= 0.0f) {
        return false;
    }

    bounds_ = bounds;
    seed_ = seed;
    features_.clear();
    placements_.clear();
    initialized_ = true;
    return true;
}

bool WorldSystem::generate_from_source(std::uint32_t seed, std::string_view name) {
    if (!initialize(seed)) return false;
    if (!load_source_layout(name)) {
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

bool WorldSystem::load_source_layout(std::string_view name) {
    const auto* layout = stage1::find_layout(name);
    if (!layout) return false;

    const auto index = static_cast<std::uint16_t>(layout - stage1::kLayoutTable);
    const auto range = stage1::instances_for_layout(index);

    bounds_ = {layout->width, layout->height};
    features_.clear();
    placements_.clear();
    placements_.reserve(range.size);
    features_.reserve(range.size);

    for (std::size_t i = 0; i < range.size; ++i) {
        const auto& src = range.data[i];
        placements_.push_back({
            src.layer, src.object, src.uid, src.x, src.y, src.z, src.w, src.h,
            src.angle, src.opacity, src.scale_x, src.ox, src.oy
        });

        const auto* layer = stage1::find_layer(index, src.layer);
        WorldFeatureKind kind{};
        if (!layer || !classify_source_layer(layer->name, kind)) continue;

        // Source coordinates are deliberately preserved verbatim. Some
        // Construct instances use negative coordinates for off-layout art.
        features_.push_back({
            kind,
            {src.x, src.y},
            src.uid,
            src.object,
            src.layer,
            src.object,
            src.w,
            src.h,
            src.angle
        });
    }

    initialized_ = true;
    return true;
}


bool WorldSystem::load_source_bunker_spawns(bool map_fs) {
    if (!initialized_) return false;

    features_.erase(std::remove_if(features_.begin(), features_.end(),
        [](const WorldFeature& item) { return item.kind == WorldFeatureKind::Bunker; }),
        features_.end());

    constexpr float kBunkerOriginX = 2306.0f;
    constexpr float kBunkerOriginY = 17464.0f;
    constexpr float kAlternateMapOffsetX = 16900.0f;

    for (const auto& source : stage1::kBunkerSpawnSource) {
        const WorldPoint position{
            kBunkerOriginX + static_cast<float>(source.x) +
                (map_fs ? kAlternateMapOffsetX : 0.0f),
            kBunkerOriginY + static_cast<float>(source.y)
        };
        if (!bounds_.contains(position)) return false;

        features_.push_back({
            WorldFeatureKind::Bunker,
            position,
            source.uid,
            source.kind,
            source.layer,
            975, // Construct object created by Bunker_spawn.
            0.0f,
            0.0f,
            0.0f
        });
    }
    return true;
}

bool WorldSystem::load_source_bridge_elements() {
    if (!initialized_) return false;

    features_.erase(std::remove_if(features_.begin(), features_.end(),
        [](const WorldFeature& item) { return item.kind == WorldFeatureKind::Bridge; }),
        features_.end());

    for (const auto& source : stage1::kBridgeElementSource) {
        const WorldPoint position{
            static_cast<float>(source.x),
            static_cast<float>(source.y)
        };
        if (!bounds_.contains(position)) return false;

        // Pack the source function's family, element, and optional enabled
        // argument without losing the original event-call UID.
        const std::uint32_t variant =
            (static_cast<std::uint32_t>(source.family) << 16) |
            (static_cast<std::uint32_t>(source.element) << 1) |
            (source.enabled ? 1u : 0u);

        features_.push_back({
            WorldFeatureKind::Bridge,
            position,
            source.uid,
            variant,
            0,
            0,
            0.0f,
            0.0f,
            0.0f
        });
    }
    return true;
}

void WorldSystem::reset() noexcept {
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
    return static_cast<std::size_t>(std::count_if(
        features_.begin(), features_.end(),
        [kind](const WorldFeature& item) { return item.kind == kind; }));
}

const WorldPlacement* WorldSystem::placement(std::size_t i) const noexcept {
    return i < placements_.size() ? &placements_[i] : nullptr;
}

} // namespace minidayz
