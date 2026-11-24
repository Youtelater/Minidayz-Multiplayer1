#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <memory>
#include <string_view>
#include <vector>
#include "minidayz/generated/location_template_source.hpp"

namespace minidayz {

enum class WorldFeatureKind : std::uint8_t {
    Terrain, Road, Building, Bunker, Bridge, SpawnPoint, Obstacle, Water
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

// Source-authored sprite state of one distinct object, addressed by its
// stable runtime identity rather than the reused Construct event UID.
// The snapshot is also used for door/tree/roof state replay after generating
// the same world seed; it deliberately contains no invented gameplay rule.
struct WorldSpriteState {
    std::uint64_t runtime_uid{};
    std::uint16_t object{};
    std::uint16_t animation{};
    std::uint32_t frame{};
    float opacity{};
    // Original House_transparenting moves individual roofs to Buildings_bg.
    // Keep this layer state in the versioned world save. 0xffff is the
    // legacy v1 snapshot sentinel: retain generated source layer.
    std::uint16_t layer{0xffff};
    // Original source variable0 for fixtures; old v1/v2 save sentinel.
    std::uint64_t source_variable_0{~std::uint64_t{0}};
};

// Exact Construct 573.SpawnAnotherObject(951,items_on_ground,0) record.
struct SourceConstructedBed {
    std::uint64_t runtime_uid{};
    float x{},y{};
    std::uint8_t material_variant{}; // bear_pi 0=Deerskin 714, 1=Bear Hide 784
    bool active{true};
};

struct WorldMapMarker {
    std::uint8_t cell_x{};
    std::uint8_t cell_y{};
    std::uint16_t source_code{};
    std::uint16_t marker_type{};
};
// Sparse original Tilemap230 SetTile result for one Mapgen514 location.
struct WorldScatterTile { std::uint16_t local_index{}, original_frame{}; };
struct WorldScatterPatch {
    std::size_t marker_index{};
    std::vector<WorldScatterTile> tiles{};
    std::int16_t offset_x{-500},offset_y{-500};
};
// Independently authored Tilemap230 for bridge/tutorial/menu events.
// Raw source frame descriptors retain the original flip bits and RLE cells.
struct WorldSpecialTerrain {
    float x{},y{},width{},height{};
    std::uint16_t columns{},rows{},layer{};
    std::uint64_t source_action_uid{};
    std::vector<std::uint32_t> cells{};
    std::uint32_t writes{};
    bool empty() const noexcept {return cells.empty();}
};
struct WorldGroundPatch {
    std::size_t marker_index{};
    std::uint16_t source_object{};
    std::int16_t offset_x{-500},offset_y{-500};
    std::array<std::int16_t,17*17> cells{};
};

struct WorldPlacement {
    std::uint16_t layer{};
    std::uint16_t object{};
    std::uint64_t source_uid{};
    float x{}, y{}, z{}, width{}, height{}, angle{}, opacity{}, scale_x{}, origin_x{}, origin_y{};
    std::uint8_t image_point{};
    // Original Construct Set animation frame actions on created world sprites.
    std::uint32_t source_frame{};
    // Dynamic animation/frame selected by the original Construct events.
    // Separate from object identity: different instances may have different
    // door, foliage or damage states without changing the original atlas.
    std::uint16_t source_animation{};
    // Original LOOT_respawn instance variable 0, when a source event
    // actually creates and configures a category marker.
    std::uint16_t source_loot_category{};
    std::uint64_t runtime_uid{};
    // Original source world events link trees to obst_base (345) and
    // automatic doors (349) to their Solid companion (348). This runtime link is rebuilt
    // deterministically during world generation, not guessed by distance.
    std::uint64_t linked_runtime_uid{};
    // One-based, deterministic index within this world generation.
    // Fast lookup for local interactions; runtime_uid is the globally
    // distinguished original/static or generated world object identity.
    std::uint64_t instance_id{};
    // Exact root Construct SpawnAnotherObject parent, distinct from
    // linked_runtime_uid: a door's latter points to companion Solid 348,
    // while its authored parent is exterior roof 321/322/323/324/333.
    // Retain both relationships without clobbering source tree hitboxes.
    std::uint64_t parent_runtime_uid{};
    // Original nested house children (especially door 349 var1) retain
    // the source foundation UID even when SpawnAnotherObject used roof
    // 1049 as their immediate parent. Distinct from linked Solid 348.
    std::uint64_t house_base_runtime_uid{};
    // The original house companion SetInstanceVariable(88) uses slots
    // 0 and 1 for wall solidity settings, selected parent UIDs and
    // original object-218 loot marker categories. Preserve exact
    // source data without inferring extra items, sounds or loot tables.
    std::uint64_t source_variable_0{};
    std::uint64_t source_variable_1{};
    // Source Trigger_spawn was called immediately after setting this marker.
    bool source_initial_loot_trigger{};
    bool source_constructed_bed{};
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

// Source-backed dispatch descriptors extracted from the original generate_location function.
inline constexpr std::size_t source_location_template_count() noexcept {
    return stage1::kLocationTemplateSourceCount;
}
inline const stage1::LocationTemplateSource* source_location_template(std::size_t index) noexcept {
    return index < stage1::kLocationTemplateSourceCount ? &stage1::kLocationTemplateSource[index] : nullptr;
}
inline const stage1::LocationTemplateSource* find_source_location_template(std::uint16_t tile_code) noexcept {
    for (const auto& entry : stage1::kLocationTemplateSource) {
        for (std::uint8_t i = 0; i < entry.tile_code_count; ++i)
            if (entry.tile_codes[i] == tile_code) return &entry;
    }
    return nullptr;
}

class WorldSystem {
public:
    static constexpr float kSourceMapWidth = 36850.0f;
    static constexpr float kSourceMapHeight = 20000.0f;

    bool initialize(std::uint32_t seed, WorldBounds bounds = {}) noexcept;
    bool generate_from_source(std::uint32_t seed, std::string_view layout_name = "Map",
                              std::uint8_t level = 0, std::uint8_t perk_dao = 0);
    bool begin_source_generation(std::uint32_t seed, std::string_view layout_name = "Map",
                                 std::uint8_t level = 0, std::uint8_t perk_dao = 0);
    bool advance_source_generation();
    std::uint8_t generation_phase() const noexcept { return generation_phase_; }
    bool generation_complete() const noexcept { return generation_phase_ == 6; }
    bool generate_procedural_grid(std::uint8_t level, std::uint32_t seed);
    bool build_map_markers(std::uint8_t level, std::uint32_t seed);
    std::size_t expand_map_element(std::uint8_t var1, std::uint8_t var2,
                                   std::uint8_t var6, WorldPoint position, double var3 = 0.0);
    bool load_source_layout(std::string_view layout_name);
    bool load_source_bunker_spawns(bool map_fs = false);
    bool load_source_bridge_elements();
    // Source Doors_auto creates/destroys companion Sprite 348 for each
    // door 349. Reserve stable source-compatible linked placements when
    // world generation completes, before snapshot/save indices are fixed.
    bool create_source_door_companions();
    void reset() noexcept;

    bool add_feature(const WorldFeature& feature);
    const WorldFeature* feature(std::size_t index) const noexcept;
    std::size_t feature_count() const noexcept { return features_.size(); }
    std::size_t feature_count(WorldFeatureKind kind) const noexcept;

    const WorldPlacement* placement(std::size_t index) const noexcept;
    const WorldPlacement* placement_by_instance_id(std::uint64_t id) const noexcept;
    const WorldPlacement* placement_by_id(std::uint32_t id) const noexcept;
    // Apply an event-resolved original sprite frame to ONE existing instance.
    // Reject nonexistent atlas frames rather than drawing a guessed image.
    // Does not itself infer the triggering event, audio, collision or drops.
    bool set_placement_sprite_state(std::size_t index,
                                    std::uint16_t animation,
                                    std::uint32_t frame,
                                    float opacity) noexcept;
    // Original door-open sequence destroys object 348 BEFORE Wait(0.01)
    // and door frame changes. Close creates its 348 AFTER the wait.
    // Keep the stable native companion ID while changing effective
    // source object presence, collision and rendering immediately.
    bool set_linked_door_solid_present(std::size_t door_index,
                                       bool present) noexcept;
    // Original House_transparenting event 9848716569903957 moves the
    // specific roof sprite to Buildings_bg on leaving its overlap area.
    // Sprite state and source UID remain unchanged.
    bool set_placement_layer(std::size_t index,
                             std::uint16_t layer) noexcept;
    bool set_source_pump_water(std::size_t index,std::uint64_t water) noexcept;
    // Original collision-on-796 transition: frame1 and instance bool1
    // must change together, exactly once per source box instance.
    bool open_source_bunker_lootbox(std::size_t index) noexcept;
    // Original car trunk Sprite241 is destroyed on first search even if
    // its 50% loot roll fails; keep a stable save-state identity.
    bool mark_source_car_trunk_searched(std::size_t index) noexcept;
    bool add_source_constructed_bed(float x,float y,std::uint8_t bear_pi) noexcept;
    bool dismantle_source_bed(std::size_t index) noexcept;
    std::vector<SourceConstructedBed> capture_source_constructed_beds() const;
    bool restore_source_constructed_beds(
        const std::vector<SourceConstructedBed>& beds) noexcept;
    // Capture only sprite states different from the generated original.
    // No 3DS rendering resources are needed; replay validates every frame.
    std::vector<WorldSpriteState> capture_sprite_states() const;
    bool restore_sprite_states(const std::vector<WorldSpriteState>& states) noexcept;
    bool set_sprite_state_by_runtime_uid(std::uint64_t uid,
                                         std::uint16_t animation,
                                         std::uint32_t frame,
                                         float opacity) noexcept;
    std::size_t placement_count() const noexcept { return placements_.size(); }

    std::size_t source_location_spawn_count() const noexcept { return source_location_spawn_count_; }
    const WorldGrid& grid() const noexcept { return grid_; }
    const WorldMapMarker* map_marker(std::size_t index) const noexcept;
    std::size_t map_marker_count() const noexcept { return map_markers_.size(); }
    // The world generator, never camera visibility, determines source tiles.
    const std::vector<WorldScatterPatch>& source_scatter_patches() const noexcept {
        return source_scatter_patches_;
    }
    const std::vector<WorldGroundPatch>& source_ground_patches() const noexcept {
        return source_ground_patches_;
    }
    int source_ground_frame_at(float x,float y) const noexcept;
    const WorldSpecialTerrain& source_special_terrain() const noexcept {
        return source_special_terrain_;
    }
    // Only the source onBridge state may execute bridge-specific writes.
    // Reload_locations_aftersave has its own original tilemap origin.
    bool run_source_bridge_terrain(bool on_bridge,bool after_save);
    const std::vector<WorldScatterPatch>& source_pond_patches() const noexcept {
        return source_pond_patches_;
    }
    bool source_pond_contains(float x,float y,bool solid_only) const noexcept;
    std::uint8_t generation_level() const noexcept { return generation_level_; }
    std::uint8_t generation_perk_dao() const noexcept { return generation_perk_dao_; }

    const WorldBounds& bounds() const noexcept { return bounds_; }
    std::uint32_t seed() const noexcept { return seed_; }
    // Original location-generation RNG stream continues into the
    // initial Trigger_spawn event family; never seed a guessed second stream.
    std::mt19937& source_random() noexcept { return rng_; }
    bool initialized() const noexcept { return initialized_; }

private:
    WorldBounds bounds_{};
    std::uint32_t seed_{};
    // One continuous random stream for the original generation pipeline:
    // POI placement consumes it before map-marker selection.
    std::mt19937 rng_{};
    WorldGrid grid_{};
    std::vector<WorldMapMarker> map_markers_{};
    std::vector<WorldScatterPatch> source_scatter_patches_{};
    std::vector<WorldGroundPatch> source_ground_patches_{};
    WorldSpecialTerrain source_special_terrain_{};
    bool source_layout_is_map_{};
    std::vector<WorldScatterPatch> source_pond_patches_{};
    std::vector<WorldFeature> features_{};
    std::vector<WorldPlacement> placements_{};
    std::vector<WorldSpriteState> original_sprite_states_{};
    std::size_t authored_placement_count_{};

    std::size_t source_location_spawn_count_{};
    std::uint8_t generation_phase_{};
    std::uint8_t generation_level_{};
    std::uint8_t generation_perk_dao_{};
    bool generation_is_map_{};
    // Source generate_mapgen_toelements stores proposals before crash,
    // bunker, radiation and road-vehicle functions consume randomness.
    // Reload_locations_start creates the actual location elements later.
    struct SourceLocationProposal {
        std::size_t marker_index{};
        std::uint8_t type{},variant{},extra{};
        WorldPoint position{};
        double state{};
    };
    std::vector<SourceLocationProposal> source_location_proposals_{};
    bool generate_source_location_proposals(std::uint8_t level);
    bool materialize_source_locations();
    bool build_source_location_terrain(std::size_t marker_index);
    bool generate_source_crash_sites();
    bool generate_source_road_vehicles();
    bool generate_source_radiation_sites();
    bool initialized_{};
};

// Hold the destination until all generation phases succeed. The caller can
// discard or retry a failed load while continuing to use its active map.
class WorldTravel {
public:
    bool begin(std::uint32_t seed, std::uint8_t level,
               std::uint8_t perk_dao = 0) noexcept;
    bool advance() noexcept;
    WorldSystem* destination() const noexcept { return destination_.get(); }
    void cancel() noexcept { destination_.reset(); }

private:
    std::unique_ptr<WorldSystem> destination_;
};

} // namespace minidayz
