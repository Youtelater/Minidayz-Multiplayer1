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
    std::uint32_t source_value{};
};

struct WorldMapMarker {
    std::uint8_t cell_x{};
    std::uint8_t cell_y{};
    std::uint16_t source_code{};
    std::uint16_t marker_type{};
};

struct WorldPlacement {
    std::uint16_t layer{};
    std::uint16_t object{};
    std::uint64_t source_uid{};
    float x{}, y{}, z{}, width{}, height{}, angle{}, opacity{}, scale_x{}, origin_x{}, origin_y{};
};

// The source game uses a zero-based 2D Array for POI placement. Out-of-range
// reads return zero, matching the Construct Array behavior used by the map events.
class WorldGrid {
public:
    bool resize(std::uint8_t width, std::uint8_t height);
    void clear() noexcept;
    std::uint16_t at(int x, int y) const noexcept;
    bool set(int x, int y, std::uint16_t value) noexcept;
    bool has_empty_3x3(int x, int y) const noexcept;
    std::uint8_t width() const noexcept { return width_; }
    std::uint8_t height() const noexcept { return height_; }

private:
    std::uint8_t width_{};
    std::uint8_t height_{};
    std::vector<std::uint16_t> cells_{};
};

struct WorldGenerationProfile {
    std::uint8_t map_size_x{16};
    std::uint8_t map_size_y{16}; // active source loop range
    std::uint8_t grid_height{16}; // allocated Array height
    std::uint8_t second_half_start_y{};
    std::uint8_t villages{};
    std::uint8_t militaries{};
    std::uint8_t cities{};
    std::uint8_t hospitals{};
    std::uint8_t firestations{};
    std::uint8_t chaoshi{};
    std::uint8_t gas_stations{};
    std::uint8_t lite_shop{};
    std::uint8_t roadside_cafes{};
    std::uint8_t second_half_villages{};
    std::uint8_t second_half_militaries{};
    std::uint8_t second_half_cities{};
    std::uint8_t second_half_hospitals{};
    std::uint8_t second_half_firestations{};
    std::uint8_t second_half_chaoshi{};
};

bool source_world_profile(std::uint8_t level, WorldGenerationProfile& profile) noexcept;

class WorldSystem {
public:
    static constexpr float kSourceMapWidth = 36850.0f;
    static constexpr float kSourceMapHeight = 20000.0f;

    bool initialize(std::uint32_t seed, WorldBounds bounds = {}) noexcept;
    bool generate_from_source(std::uint32_t seed, std::string_view layout_name = "Map",
                              std::uint8_t level = 0);
    bool generate_procedural_grid(std::uint8_t level, std::uint32_t seed);
    bool build_map_markers(std::uint8_t level, std::uint32_t seed);
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
    const WorldGrid& grid() const noexcept { return grid_; }
    const WorldMapMarker* map_marker(std::size_t index) const noexcept;
    std::size_t map_marker_count() const noexcept { return map_markers_.size(); }

    const WorldBounds& bounds() const noexcept { return bounds_; }
    std::uint32_t seed() const noexcept { return seed_; }
    bool initialized() const noexcept { return initialized_; }

private:
    WorldBounds bounds_{};
    std::uint32_t seed_{};
    WorldGrid grid_{};
    std::vector<WorldMapMarker> map_markers_{};
    std::vector<WorldFeature> features_{};
    std::vector<WorldPlacement> placements_{};
    bool initialized_{};
};

} // namespace minidayz
