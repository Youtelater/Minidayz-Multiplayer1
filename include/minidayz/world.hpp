#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace minidayz {

enum class WorldFeatureKind : std::uint8_t {
    Terrain, Road, Building, Bunker, Bridge, SpawnPoint
};

struct WorldPoint {
    float x{};
    float y{};
};

struct WorldBounds {
    float width{36850.0f};
    float height{20000.0f};

    bool contains(WorldPoint point) const noexcept;
    WorldPoint clamp(WorldPoint point) const noexcept;
};

// A source feature keeps the original Construct instance identity and geometry.
// source_uid is 64-bit because Construct UIDs exceed 32-bit range.
struct WorldFeature {
    WorldFeatureKind kind{WorldFeatureKind::Terrain};
    WorldPoint position{};
    std::uint64_t source_uid{};
    std::uint32_t variant{};
    std::uint16_t source_layer{};
    std::uint16_t source_object{};
    float width{};
    float height{};
    float angle{};
};

struct WorldPlacement {
    std::uint16_t layer{};
    std::uint16_t object{};
    std::uint64_t source_uid{};
    float x{}, y{}, z{}, width{}, height{}, angle{}, opacity{}, scale_x{}, origin_x{}, origin_y{};
};

class WorldSystem {
public:
    static constexpr float kSourceMapWidth = 36850.0f;
    static constexpr float kSourceMapHeight = 20000.0f;

    bool initialize(std::uint32_t seed, WorldBounds bounds = {}) noexcept;
    // Initializes the world from the authoritative extracted Construct layout.
    // This loads source placements and indexes features by their source layer;
    // it does not fabricate procedural generation rules.
    bool generate_from_source(std::uint32_t seed, std::string_view layout_name = "Map");
    bool load_source_layout(std::string_view layout_name);
    bool load_source_bunker_spawns(bool map_fs = false);
    bool load_source_bridge_elements();
    void reset() noexcept;

    bool add_feature(const WorldFeature& feature);
    const WorldFeature* feature(std::size_t index) const noexcept;
    std::size_t feature_count() const noexcept { return features_.size(); }
    std::size_t feature_count(WorldFeatureKind kind) const noexcept;

    const WorldPlacement* placement(std::size_t index) const noexcept;
    std::size_t placement_count() const noexcept { return placements_.size(); }

    const WorldBounds& bounds() const noexcept { return bounds_; }
    std::uint32_t seed() const noexcept { return seed_; }
    bool initialized() const noexcept { return initialized_; }

private:
    WorldBounds bounds_{};
    std::uint32_t seed_{};
    std::vector<WorldFeature> features_{};
    std::vector<WorldPlacement> placements_{};
    bool initialized_{};
};

} // namespace minidayz
