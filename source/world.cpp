#include "minidayz/world.hpp"
#include "minidayz/source_mapgen_geometry.hpp"
#include "minidayz/generated/original_initial_building_loot.hpp"
#include "minidayz/stage1_data_access.hpp"
#include "minidayz/generated/world_source_data.hpp"
#include "minidayz/generated/terrain_scatter_source.hpp"
#include "minidayz/location_event_runtime.hpp"
#include "minidayz/generated/source_tree_impact_points.hpp"
#include "minidayz/generated/source_door_points.hpp"
#include "minidayz/generated/source_house_companions.hpp"
#include "minidayz/generated/source_car_wreck_trunks.hpp"
#include "minidayz/generated/source_family_members.hpp"
#include "minidayz/generated/source_building_frame_geometry.hpp"
#include "minidayz/source_furniture_interaction.hpp"
#include "minidayz/source_interior_container.hpp"
#include "minidayz/source_crash_site_geometry.hpp"
#include "minidayz/source_road_vehicle_geometry.hpp"
#include "minidayz/source_radiation_zone.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <random>
#include <initializer_list>

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
 
// Root generate_array_tomap (UID 1694439930581699) places gas stations,
// roadside cafes and lite shops AFTER Array518 roads become Sprite514
// markers. It uses Repeat(count), OR(type==12,type==13), PickRandom(514)
// and SetInstanceVariable(0). These are ROAD MARKER REPLACEMENTS, not
// Array518 3x3 placements; the raw road stays intact for its connections.
void replace_source_road_markers(
    std::vector<WorldMapMarker>& markers,std::mt19937& rng,
    std::uint8_t repetitions,bool second_half,
    std::uint16_t from_12,std::uint16_t from_13) {
    for(std::uint8_t n=0;n<repetitions;++n){
        std::vector<std::size_t> eligible;
        for(std::size_t i=0;i<markers.size();++i){
            const auto& m=markers[i];
            // Original first map tests Sprite514.var1 <= 15.
            // Its second-half counterpart tests var1 >= 17.
            if(second_half ? (m.cell_y<17) : (m.cell_y>15))continue;
            if(m.marker_type!=12 && m.marker_type!=13)continue;
            eligible.push_back(i);
        }
        // A Construct Pick Random Instance with an empty selection does
        // nothing; it does not fabricate a station or reject the world.
        if(eligible.empty())continue;
        auto& m=markers[eligible[source_random_bounded(
            rng,static_cast<std::uint32_t>(eligible.size()))]];
        m.marker_type=m.marker_type==12?from_12:from_13;
    }
}

// Original generate_array_tomap executes PickRandom(514) only among the
// instances selected by its own source conditions. Selection updates the
// picked 514.var0; the Array518 road/POI value remains unchanged.
void source_pick_marker(
    std::vector<WorldMapMarker>& markers,std::mt19937& rng,
    int min_row,int max_row,int min_col,int max_col,
    std::initializer_list<std::uint16_t> allowed,
    std::initializer_list<std::uint16_t> outputs,
    int repetitions=1) {
    if(!allowed.size()||!outputs.size()||repetitions<=0)return;
    for(int n=0;n<repetitions;++n){
        std::vector<std::size_t> matching;
        for(std::size_t i=0;i<markers.size();++i){
            const auto& m=markers[i];
            if(m.cell_y<min_row||m.cell_y>max_row||
               m.cell_x<min_col||m.cell_x>max_col)continue;
            if(std::find(allowed.begin(),allowed.end(),m.marker_type)==
               allowed.end())continue;
            matching.push_back(i);
        }
        if(matching.empty())continue; // Original PickRandom empty selection.
        auto& selected=markers[matching[source_random_bounded(
            rng,static_cast<std::uint32_t>(matching.size()))]];
        const auto chosen=source_random_bounded(
            rng,static_cast<std::uint32_t>(outputs.size()));
        selected.marker_type=outputs.begin()[chosen];
    }
}

// Root generate_array_tomap nodes 424890147621748 and
// 688300943103455: a 14x14 Repeat checks the four adjacent Mapgen
// instances for the ORIGINAL wilderness codes {1,2,3,8,10,52}, and
// multiplies the CENTER type by 1000 only if it is {1,2,3,8,10}.
// Keep in-place, nested-repeat order: multiplying one center affects
// the next neighbor's predicate, and prevents overlapping special sites.
void source_flag_interior_markers(
    std::vector<WorldMapMarker>& markers,int row_start,int columns){
    if(columns<3 || markers.size()%static_cast<std::size_t>(columns)!=0)
        return;
    const int rows=static_cast<int>(markers.size()/columns);
    const auto at=[&](int row,int col)->WorldMapMarker*{
        if(row<0||row>=rows||col<0||col>=columns)return nullptr;
        return &markers[static_cast<std::size_t>(row)*columns+col];
    };
    const auto wilderness=[](std::uint16_t code,bool include_52){
        switch(code){
        case 1:case 2:case 3:case 8:case 10:return true;
        case 52:return include_52;
        default:return false;
        }
    };
    for(int row=row_start;row<row_start+14;++row){
        for(int col=1;col<=14;++col){
            auto* center=at(row,col);
            auto* left=at(row,col-1);
            auto* right=at(row,col+1);
            auto* up=at(row-1,col);
            auto* down=at(row+1,col);
            if(!center||!left||!right||!up||!down||
               !wilderness(center->marker_type,false)||
               !wilderness(left->marker_type,true)||
               !wilderness(right->marker_type,true)||
               !wilderness(up->marker_type,true)||
               !wilderness(down->marker_type,true))continue;
            center->marker_type=static_cast<std::uint16_t>(
                center->marker_type*1000u);
        }
    }
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
    case 4: ok = !generation_is_map_ ||
        (generate_source_crash_sites() && load_source_bunker_spawns(false)
            && generate_source_radiation_sites()); break;
    case 5: ok = !generation_is_map_ ||
        (load_source_bridge_elements() && generate_source_road_vehicles()); break;
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
    source_scatter_patches_.clear();
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
    // Root generate_array_locations writes only village/military/city/
    // hospital/fire station/chaoshi POIs to Array518. The source creates
    // gas/cafe/shop locations later by picking road markers in
    // generate_array_tomap; writing their fake 99/100/101 footprints here
    // removed original roads and altered every later random choice.
    bool placed =
        place_poi_count(grid_, profile.villages, 9, rng_, first_min_y, first_max_y) &&
        place_poi_count(grid_, profile.militaries, 5, rng_, first_min_y, first_max_y) &&
        place_poi_count(grid_, profile.cities, 6, rng_, first_min_y, first_max_y) &&
        place_poi_count(grid_, profile.hospitals, 11, rng_, first_min_y, first_max_y) &&
        place_poi_count(grid_, profile.firestations, 15, rng_, first_min_y, first_max_y) &&
        place_poi_count(grid_, profile.chaoshi, 103, rng_, first_min_y, first_max_y);
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
            place_poi_count(grid_, profile.second_half_chaoshi, 103, rng_, second_min_y, second_max_y);
        if (!placed) return false;
    }

    generate_source_roads(grid_, level, profile.map_size_y);
    return true;
}

bool WorldSystem::build_map_markers(std::uint8_t level, std::uint32_t seed) {
    if (!initialized_ || grid_.width() == 0 || grid_.height() == 0) return false;
    map_markers_.clear();
    source_scatter_patches_.clear();
    map_markers_.reserve(static_cast<std::size_t>(grid_.width()) * grid_.height());
    (void)seed; // The source uses one global random stream across generation stages.
    WorldGenerationProfile profile{};
    if(!source_world_profile(level,profile))return false;
    for (std::uint8_t y = 0; y < grid_.height(); ++y) {
        for (std::uint8_t x = 0; x < grid_.width(); ++x) {
            const std::uint16_t source_code = grid_.at(x, y);
            map_markers_.push_back({
                x, y, source_code, source_marker_type(source_code, level, rng_)
            });
        }
    }
    // Root generate_array_tomap first runs these three functions on
    // first-half road markers, then repeats them in the second map half
    // at levels 4/5. The source repeats the FULL count in EACH half:
    // splitting one count across the halves silently halves locations.
    const int halves=profile.second_half_start_y?2:1;
    for(int half=0;half<halves;++half){
        const bool second_half=half==1;
        replace_source_road_markers(map_markers_,rng_,
            profile.gas_stations,second_half,30,31);
        replace_source_road_markers(map_markers_,rng_,
            profile.roadside_cafes,second_half,50,51);
        replace_source_road_markers(map_markers_,rng_,
            profile.lite_shop,second_half,108,109);
    }

    // Root generate_array_tomap branch 148779630202671 selects one
    // original wilderness marker and changes 514.var0 to 32 on maps
    // 1+, with an additional second-half selection at levels 4/5.
    if(level>=1)
        source_pick_marker(map_markers_,rng_,0,15,0,15,
            {2,8,1,10,3},{32});
    if(level>=4)
        source_pick_marker(map_markers_,rng_,17,32,0,15,
            {2,8,1,10,3},{32});

    // Root default sav_checked=0, not challenge-menu -2. The two
    // 105 branches use the same OR selection before interior marking.
    source_pick_marker(map_markers_,rng_,0,15,0,15,
        {2,8,1,10,3},{105});
    if(level>=4)
        source_pick_marker(map_markers_,rng_,17,32,0,15,
            {2,8,1,10,3},{105});

    // Root 14x14 interior tagging: first b_x=1..14, then 18..31.
    source_flag_interior_markers(map_markers_,1,grid_.width());
    if(level>=4)
        source_flag_interior_markers(map_markers_,18,grid_.width());

    // Root selected 514.var0 values 1000/2000/3000/8000/10000
    // are SOURCE TEMPORARY TYPES, not placed permanent scenery.
    // The ordered PickRandom branches below instantiate the authored
    // special map location variants before resetting every leftover.
    constexpr std::uint16_t chosen_source_types[]={
        23,24,25,26,27,28,29,40,42,43,44,45,46,47,48,49,
        60,61,62,63,64,66
    };
    const auto pick_source_special=[&](
        int first_row,int last_row,
        std::initializer_list<std::uint16_t> out) {
        source_pick_marker(map_markers_,rng_,
            first_row,last_row,1,14,
            {2000,8000,1000,10000,3000},out);
    };
    const auto choose_source_special=[&](int first_row,int last_row){
        std::vector<std::size_t> matching;
        for(std::size_t i=0;i<map_markers_.size();++i){
            const auto& m=map_markers_[i];
            if(m.cell_y<first_row||m.cell_y>last_row||
               m.cell_x<1||m.cell_x>14)continue;
            if(m.marker_type!=2000&&m.marker_type!=8000&&
               m.marker_type!=1000&&m.marker_type!=10000&&
               m.marker_type!=3000)continue;
            matching.push_back(i);
        }
        if(matching.empty())return;
        auto& m=map_markers_[matching[source_random_bounded(
            rng_,static_cast<std::uint32_t>(matching.size()))]];
        m.marker_type=choose_marker(chosen_source_types,rng_);
    };
    if(level>=4){
        pick_source_special(18,31,{104});
        pick_source_special(18,31,{102});
        pick_source_special(18,31,{106});
    }
    if(level>=5){
        pick_source_special(1,14,{102});
        pick_source_special(1,14,{110});
    }
    if(level>=2)pick_source_special(1,14,{107});
    if(level>=4)pick_source_special(18,31,{107});
    choose_source_special(1,14); // Source branch 871254110967877
    if(level>=5){
        choose_source_special(1,14);
        choose_source_special(18,31);
    }
    if(level>=4)pick_source_special(18,31,{65});
    // Source DLC conditional (dlc_2==1) produces 67. Normal new-game
    // initialization resets dlc_2 to zero; do not invent this marker.
    for(auto& marker:map_markers_){
        switch(marker.marker_type){
        case 1000:case 2000:case 3000:case 8000:case 10000:
            marker.marker_type=static_cast<std::uint16_t>(
                marker.marker_type/1000);
            break;
        default:break;
        }
    }

    // Original level-five return-to-wilderness branches convert six
    // first-half and four second-half markers to Sprite514 type 3.
    if(level>=5){
        source_pick_marker(map_markers_,rng_,0,15,0,15,
            {2,8,1,10},{3},6);
        source_pick_marker(map_markers_,rng_,17,32,0,15,
            {2,8,1,10},{3},4);
    }

    // Root generate_array_tomap military subtypes (UIDs
    // 234786497469423/972939048496615/200534481265776/
    // 957929406510728). Raw Array518 code 5 initially maps to
    // Sprite514 type22; the source THEN converts picked 22s to
    // types 5 and 70. Do not create additional military POIs.
    const auto military_pick=[&](int min_row,int max_row,int count,
                                 std::uint16_t result) {
        if(!count)return;
        for(int n=0;n<count;++n)
            source_pick_marker(map_markers_,rng_,min_row,max_row,0,15,
                {22},{result});
    };
    if(level==1)military_pick(0,15,1,5);
    if(level==2)military_pick(0,15,2,5);
    if(level==3){
        military_pick(0,15,1,70);
        military_pick(0,15,3,5);
    }
    if(level>=4){
        for(const auto bounds: {std::pair<int,int>{0,15},
                                std::pair<int,int>{17,32}}){
            military_pick(bounds.first,bounds.second,2,70);
            military_pick(bounds.first,bounds.second,3,5);
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
    source_scatter_patches_.clear();
    features_.clear();
    placements_.clear();
    original_sprite_states_.clear();
    authored_placement_count_=0;
    source_location_spawn_count_=0;
    // An authored Map layout is not a fully generated procedural world.
    // Replacing a layout must invalidate its old terrain and phase state.
    generation_phase_=0;
    generation_is_map_=false;
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


// Reproduce root-extracted Tilemap230 SetTile repetitions and weighted
// frame choices during each Mapgen514 event, in the shared world RNG stream.
// Camera-dependent minstd_rand hashes never existed in original gameplay.
// Source branches not yet decoded by the strict extractor are not invented.
bool WorldSystem::build_source_scatter_patch(std::size_t marker_index) {
    if(marker_index>=map_markers_.size() || generation_level_>=6)return false;
    constexpr std::size_t kSide=33, kCells=kSide*kSide;
    constexpr std::uint16_t kEmpty=0xffffu;
    const auto marker=map_markers_[marker_index];
    std::array<std::uint16_t,kCells> cells{};
    cells.fill(kEmpty);
    bool has_source_rule=false;
    for(const auto& rule:stage1::kTerrainScatterRules){
        if(rule.marker!=marker.marker_type ||
           !(rule.level_mask&(std::uint8_t{1}<<generation_level_)))continue;
        if(rule.repeat==0 || rule.x_min>=rule.x_max ||
           rule.y_min>=rule.y_max || rule.x_max>kSide ||
           rule.y_max>kSide || rule.tile_count==0 ||
           rule.tile_offset+rule.tile_count>
               stage1::kTerrainScatterChoiceCount)return false;
        has_source_rule=true;
        for(std::uint16_t n=0;n<rule.repeat;++n){
            const std::size_t x=rule.x_min+source_random_bounded(
                rng_,static_cast<std::uint32_t>(rule.x_max-rule.x_min));
            const std::size_t y=rule.y_min+source_random_bounded(
                rng_,static_cast<std::uint32_t>(rule.y_max-rule.y_min));
            const auto choice=source_random_bounded(
                rng_,static_cast<std::uint32_t>(rule.tile_count));
            cells[y*kSide+x]=stage1::kTerrainScatterChoices[
                rule.tile_offset+choice];
        }
    }
    if(!has_source_rule)return true;
    WorldScatterPatch patch{};
    patch.marker_index=marker_index;
    for(std::size_t i=0;i<kCells;++i)
        if(cells[i]!=kEmpty)
            patch.tiles.push_back({static_cast<std::uint16_t>(i),cells[i]});
    source_scatter_patches_.push_back(std::move(patch));
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
    source_scatter_patches_.clear();
    source_scatter_patches_.reserve(map_markers_.size());
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
            const std::size_t marker_index=
                static_cast<std::size_t>(y)*grid_.width()+x;
            if(!build_source_scatter_patch(marker_index))return false;
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
                {
                    // Preserve the original Sprite218 variable as well as
                    // the native category index. Otherwise the location
                    // loader's source SetInstanceVariable has no matching
                    // per-instance state after creating this marker.
                    placed.source_variable_0 = static_cast<std::uint64_t>(c);
                    placed.source_loot_category =
                        static_cast<std::uint16_t>(c);
                }
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

// Original generate_helicrashes is called after mapgen_toelements and
// before Reload_locations_start. It selects original Sprite514 markers,
// creates Sprite344/Sprite434, and the selected marker's create_map_location
// child event creates crash artwork, Solid348, and exact loot category
// markers at authored Sprite imagepoints. All structures are rebuildable
// from the same world seed (no guessed extra wrecks or category items).
bool WorldSystem::generate_source_crash_sites() {
    if(!initialized_||!generation_is_map_||map_markers_.empty())
        return false;
    const auto* layout=stage1::find_layout("Map");
    if(!layout)return false;
    const auto layout_id=static_cast<std::uint16_t>(layout-stage1::kLayoutTable);
    const auto buildings=find_layer_index_by_name(layout_id,"Buildings");
    const auto trees=find_layer_index_by_name(layout_id,"Trees");
    const auto zed=find_layer_index_by_name(layout_id,"zed_resp");
    const auto loot=find_layer_index_by_name(layout_id,"LOOT_respawn");
    if(buildings==0xffff||trees==0xffff||zed==0xffff||loot==0xffff)
        return false;
    const auto make=[&](const world_source::CrashSpriteGeometry& spec,
        std::uint16_t layer,std::uint64_t action_uid,float x,float y,
        std::uint64_t parent_uid=0)->std::size_t {
        const auto index=placements_.size();
        placements_.push_back({layer,spec.object,action_uid,x,y,0.0f,
            spec.width,spec.height,0.0f,1.0f,1.0f,
            spec.origin_x,spec.origin_y,0,spec.frame});
        auto& instance=placements_.back();
        instance.runtime_uid=(std::uint64_t{1}<<63) |
            static_cast<std::uint64_t>(index+1);
        instance.instance_id=index+1;
        instance.parent_runtime_uid=parent_uid;
        return index;
    };
    const auto point_position=[](const WorldPlacement& parent,
        world_source::CrashImagePoint point)->WorldPoint {
        return {parent.x+(point.x-parent.origin_x)*parent.width,
                parent.y+(point.y-parent.origin_y)*parent.height};
    };
    std::vector<bool> chosen(map_markers_.size(),false);
    const auto choose_site=[&](int first,int last,
        std::initializer_list<std::uint16_t> types)->std::size_t {
        std::vector<std::size_t> eligible;
        for(std::size_t i=0;i<map_markers_.size();++i){
            const auto& m=map_markers_[i];
            if(m.cell_y<first||m.cell_y>last||chosen[i])continue;
            if(std::find(types.begin(),types.end(),m.marker_type)==
               types.end())continue;
            eligible.push_back(i);
        }
        if(eligible.empty())return map_markers_.size();
        const auto picked=eligible[source_random_bounded(
            rng_,static_cast<std::uint32_t>(eligible.size()))];
        chosen[picked]=true; // Original Sprite514.var9 = 1
        return picked;
    };
    const auto spawn_markers=[&](const WorldPlacement& art,
        std::uint16_t category,
        const world_source::CrashImagePoint (&points)[3],
        const std::uint64_t (&action_uids)[3]) {
        for(std::size_t n=0;n<3;++n){
            const auto pos=point_position(art,points[n]);
            const auto idx=make(world_source::kCrashLoot,loot,
                action_uids[n],pos.x,pos.y,art.runtime_uid);
            auto& marker=placements_[idx];
            marker.image_point=points[n].index;
            marker.linked_runtime_uid=art.runtime_uid;
            marker.source_variable_0=category;
            marker.source_loot_category=category;
            // Root immediately calls Trigger_spawn(218.UID) after every
            // SetInstanceVariable(0,15/18), in this exact action order.
            marker.source_initial_loot_trigger=true;
        }
    };
    for(int half=0;half<(generation_level_>=4?2:1);++half){
        const int first=half==0?0:17;
        const int last=half==0?15:32;
        const auto heli_quota=world_source::source_helicrash_quota(
            generation_level_);
        const auto hammer_quota=world_source::source_hammercrash_quota(
            generation_level_);
        for(int n=0;n<heli_quota;++n){
            const auto i=choose_site(first,last,{1,2,8,10,52});
            if(i==map_markers_.size())break;
            const auto& marker=map_markers_[i];
            const auto hotspot=world_source::mapgen_hotspot(
                marker.cell_x,marker.cell_y);
            const float x=hotspot.x-500.0f,y=hotspot.y-500.0f;
            const auto anchor=make(world_source::kHeliAnchor,buildings,
                667245097137285ULL,x,y);
            auto& point=placements_[anchor];
            point.source_variable_0=1; // 344.var0 after first site reload
            // Original 344.var1 stores the selected 514 runtime UID.
            // Native Mapgen514 is represented by the deterministic
            // WorldMapMarker index, not a visible fake Sprite514.
            point.source_variable_1=static_cast<std::uint64_t>(i+1);
            const auto parent_uid=point.runtime_uid;
            const auto visual_uid=marker.marker_type==52
                ?453361015394042ULL
                :marker.marker_type==2||marker.marker_type==8
                    ?416326667732045ULL:439357571825697ULL;
            const auto model_idx=make(world_source::kHeliCrash,buildings,
                visual_uid,x,y,parent_uid);
            const auto model=placements_[model_idx];
            const auto solid_idx=make(world_source::kHeliSolid,zed,
                884735663000338ULL,x,y,model.runtime_uid);
            placements_[solid_idx].linked_runtime_uid=model.runtime_uid;
            placements_[model_idx].linked_runtime_uid=
                placements_[solid_idx].runtime_uid;
            const auto counter=make(world_source::kHeliCounter,zed,
                757293486782645ULL,x,y,model.runtime_uid);
            placements_[counter].width=300.0f;
            placements_[counter].height=300.0f;
            constexpr std::uint64_t loot_uids[]={
                239697520904115ULL,873380527087600ULL,
                571157701650261ULL};
            spawn_markers(model,15,world_source::kHeliLootPoints,
                loot_uids);
            const auto npc=make(world_source::kCrashZed,zed,
                567620764434094ULL,x+100.0f,y,parent_uid);
            // Object216 is an original special zombie spawn point.
            // Keep it in the world source spawn index as long as it is in
            // the Map bounds, without manufacturing extra zombies.
            if(bounds_.contains({x+100.0f,y}))
                features_.push_back({WorldFeatureKind::SpawnPoint,
                    {x+100.0f,y},567620764434094ULL,216,zed,216,
                    placements_[npc].width,placements_[npc].height,0.0f,0});
        }
        for(int n=0;n<hammer_quota;++n){
            const auto i=choose_site(first,last,{5,15,13});
            if(i==map_markers_.size())break;
            const auto& marker=map_markers_[i];
            const auto hotspot=world_source::mapgen_hotspot(
                marker.cell_x,marker.cell_y);
            const float x=hotspot.x-500.0f,y=hotspot.y-600.0f;
            const auto anchor=make(world_source::kHammerAnchor,buildings,
                marker.marker_type==13?770992642019829ULL:
                    515255034690416ULL,x,y);
            auto& point=placements_[anchor];
            point.source_variable_0=1;
            // Original 434.var2 remembers 514.UID; link as the native
            // parent marker index, not as an unrelated object variable.
            point.house_base_runtime_uid=static_cast<std::uint64_t>(i+1);
            const auto parent_uid=point.runtime_uid;
            const bool fallen=marker.marker_type==13;
            const auto& frame=fallen
                ?world_source::kHammerCrashFallen
                :world_source::kHammerCrashStanding;
            const auto model_idx=make(frame,trees,
                fallen?832344210313130ULL:672392095922871ULL,
                x,y,parent_uid);
            const auto model=placements_[model_idx];
            const auto counter=make(world_source::kHammerCounter,zed,
                525953418179817ULL,x,y,model.runtime_uid);
            placements_[counter].width=300.0f;
            placements_[counter].height=300.0f;
            constexpr std::uint64_t loot_uids[]={
                629751421654089ULL,370030648544993ULL,
                200882605382924ULL};
            if(fallen)
                spawn_markers(model,18,
                    world_source::kHammerFallenLootPoints,loot_uids);
            else
                spawn_markers(model,18,
                    world_source::kHammerStandingLootPoints,loot_uids);
            const auto& wall=fallen?
                world_source::kHammerFallenWallPoints:
                world_source::kHammerStandingWallPoints;
            const auto start=point_position(model,wall[0]);
            const auto end=point_position(model,wall[1]);
            const auto wall_index=make(world_source::kHammerSolid,zed,
                812180985531328ULL,start.x,start.y,model.runtime_uid);
            auto& obstacle=placements_[wall_index];
            // Original Solid348 SetSize is the difference of named
            // Sprite441 ImagePoint("w2") and ImagePoint("w1").
            obstacle.width=end.x-start.x;
            obstacle.height=end.y-start.y;
            obstacle.linked_runtime_uid=model.runtime_uid;
            obstacle.source_variable_0=model.runtime_uid;
            placements_[model_idx].linked_runtime_uid=obstacle.runtime_uid;
        }
    }
    return true;
}

// Root function "wuranqu" (UID316200290612230) follows
// generate_helicrashes and generate_bunker_entrance, BEFORE road vehicles.
// It spawns original Sprite1126 "duqu" zones on visible Rain layer44.
// Sprite514.var0 *= 1000 temporarily excludes a picked marker from later
// PickRandom calls. The final source event divides the marker by 1000.
// Keep original marker types unchanged after this function, including
// the minimap already generated BEFORE this source function.
bool WorldSystem::generate_source_radiation_sites(){
    if(!initialized_ || !generation_is_map_ || map_markers_.empty())
        return false;
    const auto* layout=stage1::find_layout("Map");
    if(!layout)return false;
    const auto layout_index=static_cast<std::uint16_t>(
        layout-stage1::kLayoutTable);
    const auto rain=find_layer_index_by_name(layout_index,"Rain");
    if(rain==0xffff)return false;
    const auto spawn=[&](std::size_t marker_index,
                         std::uint64_t action_uid) {
        const auto& marker=map_markers_[marker_index];
        const auto point=world_source::mapgen_hotspot(
            marker.cell_x,marker.cell_y);
        const auto placed_index=placements_.size();
        // Original Construct permits an object hotspot just outside the
        // layout: its 2000px radiation art still overlaps the playfield.
        // Never silently discard edge zones by bounds.contains(hotspot).
        placements_.push_back({rain,world_source::kOriginalRadioactiveSprite,
            action_uid,
            point.x+world_source::kOriginalRadioactiveOffsetX,
            point.y+world_source::kOriginalRadioactiveOffsetY,0.0f,
            world_source::kOriginalRadioactiveWidth,
            world_source::kOriginalRadioactiveHeight,
            0.0f,1.0f,1.0f,
            world_source::kOriginalRadioactiveOriginX,
            world_source::kOriginalRadioactiveOriginY,
            0,world_source::kOriginalRadioactiveFrame});
        auto& zone=placements_.back();
        zone.runtime_uid=(std::uint64_t{1}<<63) |
            static_cast<std::uint64_t>(placed_index+1);
        zone.instance_id=placed_index+1;
        // Mapgen514 is a transient original source marker, not an
        // invented physical Sprite514. Retain its exact Array index.
        zone.source_variable_0=static_cast<std::uint64_t>(marker_index+1);
    };
    // Root Event849848542281325 picks EACH original military subtype70,
    // independently of the repeated radiation-site selections.
    for(std::size_t i=0;i<map_markers_.size();++i)
        if(map_markers_[i].marker_type==70)
            spawn(i,world_source::kOriginalRadioactiveMilitarySpawnAction);

    const int count=world_source::original_radioactive_site_quota(
        generation_level_);
    std::vector<bool> consumed(map_markers_.size(),false);
    const auto add_source_zone=[&](int min_row,int max_row) {
        for(int repeat=0;repeat<count;++repeat){
            std::vector<std::size_t> eligible;
            for(std::size_t i=0;i<map_markers_.size();++i){
                const auto& marker=map_markers_[i];
                if(consumed[i]||marker.cell_y<min_row||
                   marker.cell_y>max_row||
                   !world_source::original_radioactive_site_type(
                       marker.marker_type))continue;
                eligible.push_back(i);
            }
            // Source System.PickRandom(514) does not create an object if
            // no marker is selected: it must never invent a radiation zone.
            if(eligible.empty())break;
            const auto selected=eligible[source_random_bounded(
                rng_,static_cast<std::uint32_t>(eligible.size()))];
            consumed[selected]=true;
            spawn(selected,world_source::kOriginalRadioactivePickedSpawnAction);
        }
    };
    if(generation_level_==3)
        add_source_zone(0,14); // Source Repeat(2), Sprite514.var1<15.
    else if(generation_level_>=4){
        // Source default perk_dao != 7: Repeat(3) in EACH half. The
        // separate perk_dao==7 Repeat(6) branch must be wired when
        // character perk state is passed through world creation.
        add_source_zone(0,14);
        add_source_zone(17,32);
    }
    return true;
}

// Root Car_respawn_uaz/volga/hammer/ka functions (UIDs
// 862359914595848 / 431593263530698 / 992596827703911 /
// 410051924834415) are called AFTER the original crash-site pass.
// These are DIFFERENT from category-6 searchable wrecks. Preserve
// original marker filtering, offsets, angles and visual body frames.
// Driving, fuel and runtime Pin attachments are separate vehicle work.
bool WorldSystem::generate_source_road_vehicles(){
    if(!initialized_||!generation_is_map_||map_markers_.empty())return false;
    const auto* layout=stage1::find_layout("Map");
    if(!layout)return false;
    const auto layout_index=static_cast<std::uint16_t>(
        layout-stage1::kLayoutTable);
    const auto buildings=find_layer_index_by_name(layout_index,"Buildings");
    if(buildings==0xffff)return false;
    const auto choose=[&](std::initializer_list<std::uint16_t> types,
                          int min_row,int max_row)->std::size_t{
        std::vector<std::size_t> eligible;
        for(std::size_t i=0;i<map_markers_.size();++i){
            const auto& marker=map_markers_[i];
            if(marker.cell_y<min_row||marker.cell_y>max_row)continue;
            if(std::find(types.begin(),types.end(),marker.marker_type)==
               types.end())continue;
            eligible.push_back(i);
        }
        // Source PickRandom(514) with no selected instance creates no car.
        if(eligible.empty())return map_markers_.size();
        return eligible[source_random_bounded(
            rng_,static_cast<std::uint32_t>(eligible.size()))];
    };
    const auto spawn=[&](const world_source::OriginalRoadVehicleFrame& spec,
        std::uint64_t event,float x,float y,float angle,
        std::uint64_t overlay_event) {
        const std::size_t body_index=placements_.size();
        placements_.push_back({buildings,spec.object,event,
            x,y,0.0f,spec.width,spec.height,angle,
            1.0f,1.0f,spec.origin_x,spec.origin_y});
        auto& body=placements_.back();
        body.runtime_uid=(std::uint64_t{1}<<63) |
            static_cast<std::uint64_t>(body_index+1);
        body.instance_id=body_index+1;
        if(spec.overlay!=0){
            const auto parent_uid=body.runtime_uid;
            const std::size_t overlay_index=placements_.size();
            placements_.push_back({buildings,spec.overlay,
                overlay_event,x,y,0.0f,
                spec.overlay_width,spec.overlay_height,angle,
                1.0f,1.0f,spec.overlay_origin_x,spec.overlay_origin_y});
            auto& overlay=placements_.back();
            overlay.runtime_uid=(std::uint64_t{1}<<63) |
                static_cast<std::uint64_t>(overlay_index+1);
            overlay.instance_id=overlay_index+1;
            overlay.parent_runtime_uid=parent_uid;
            // push_back may reallocate placements_: the previous body
            // reference is invalid. Link by stable index after the spawn.
            placements_[body_index].linked_runtime_uid=overlay.runtime_uid;
        }
    };
    const auto spawn_road=[&](
        const world_source::OriginalRoadVehicleFrame& spec,
        std::uint64_t vertical_event,
        std::uint64_t horizontal_event,
        std::uint64_t overlay_event){
        const auto index=choose({12,13},0,15);
        if(index==map_markers_.size())return;
        const auto& m=map_markers_[index];
        const auto pos=world_source::mapgen_hotspot(m.cell_x,m.cell_y);
        // Root System.Create: marker0==12 uses x+100+random(800),
        // y+400+random(200), angle choose(180,0).
        // marker0==13 uses x+400+random(200),
        // y+100+random(800), angle choose(90,270).
        const bool vertical=m.marker_type==12;
        const float x=pos.x+(vertical?100.0f:400.0f)+
            static_cast<float>(source_random_bounded(rng_,vertical?800:200));
        const float y=pos.y+(vertical?400.0f:100.0f)+
            static_cast<float>(source_random_bounded(rng_,vertical?200:800));
        const float angle=vertical?
            (source_random_bounded(rng_,2)==0?180.0f:0.0f):
            (source_random_bounded(rng_,2)==0?90.0f:270.0f);
        spawn(spec,vertical?vertical_event:horizontal_event,
              x,y,angle,overlay_event);
    };
    spawn_road(world_source::kRoadVehicleUaz,
        117612298080658ULL,812947475463361ULL,
        931809985139153ULL);
    spawn_road(world_source::kRoadVehicleVolga,
        375484843290557ULL,613501860905527ULL,
        262613965798799ULL);
    // Root hammer car function selects an original military marker70
    // only on the first 16 map rows. SetAngle(180) is its final action.
    const auto hammer=choose({70},0,15);
    if(hammer<map_markers_.size()){
        const auto& m=map_markers_[hammer];
        const auto p=world_source::mapgen_hotspot(m.cell_x,m.cell_y);
        spawn(world_source::kRoadVehicleHammer,
            579079383275854ULL,p.x+874.0f,p.y+855.0f,180.0f,0);
    }
    // Root KA car function selects marker104 (second-half rare site
    // in levels4/5); no first-half predicate appears in this function.
    const auto ka=choose({104},0,32);
    if(ka<map_markers_.size()){
        const auto& m=map_markers_[ka];
        const auto p=world_source::mapgen_hotspot(m.cell_x,m.cell_y);
        spawn(world_source::kRoadVehicleKa,
            117612298080658ULL,p.x+150.0f,p.y+500.0f,0.0f,0);
    }
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
    source_scatter_patches_.clear();
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
        // Original trees are NOT members of the building frame-geometry
        // table. Derive all SIX tree species' size and hotspot from their
        // actual source frame0/Imagepoint1 record. An ObjectDef with 0x0
        // editor dimensions otherwise collapses the 345 trunk offset to
        // the tree's sprite hotspot while the real PNG renders elsewhere.
        const auto* tree_frame=world_source::source_tree_impact_point(rule.object);
        // Original Sprite210 variants 7..10 use distinct standing frames
        // and frame-specific 348 Solids, NOT the six 345-trunk species.
        const auto* block_frame=(var1==1 && rule.object==210)
            ? world_source::source_tree_block_variant(var2) : nullptr;
        const std::size_t primary_index=placements_.size();
        placements_.push_back({
            layer, rule.object, rule.uid, position.x, position.y, 0.0f,
            block_frame ? block_frame->width :
            tree_frame ? tree_frame->width :
                initial_frame ? initial_frame->width :
                    (source_object ? source_object->width : 0.0f),
            block_frame ? block_frame->height :
            tree_frame ? tree_frame->height :
                initial_frame ? initial_frame->height :
                    (source_object ? source_object->height : 0.0f),
            0.0f, 1.0f, 1.0f,
            block_frame ? block_frame->origin_x :
            tree_frame ? tree_frame->origin_x :
                initial_frame ? initial_frame->origin_x :
                    (source_object ? source_object->origin_x : 0.0f),
            block_frame ? block_frame->origin_y :
            tree_frame ? tree_frame->origin_y :
                initial_frame ? initial_frame->origin_y :
                    (source_object ? source_object->origin_y : 0.0f),
            rule.image_point,
            block_frame ? block_frame->tree_frame : 0
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
        if(var1==7 || var1==4 || var1==6) {
        // Category 6 is the root original car-wreck scenery family.
        // Each Sprite213/212/487/... spawns an authored Sprite241 trunk
        // (and usually a 345/348 Solid). The old 4/7-only filter dropped
        // the ENTIRE car-trunk and collision companion chain.
        std::vector<std::pair<std::uint16_t,std::size_t>> spawned;
        spawned.emplace_back(rule.object,placements_.size()-1);
        for(const auto& action:world_source::kSourceHouseCompanionSpawns){
            // Source child condition is Mapgen508.var2 == variant.
            // The archived world element table contains a known stale
            // UID for house 312; the source event's actual variant/base
            // predicate is authoritative and avoids missing its roof.
            if(action.variant!=var2 || action.base_object!=rule.object)continue;
            // Category 4 has repeated subtype variants and needs its
            // exact source event UID. Category 6 car-wreck primaries come
            // from a parent var2 branch, but their Sprite241/345/348
            // SpawnAnotherObject actions occur in a NESTED child event:
            // that child's UID is necessarily different. Comparing
            // action.event_uid to the parent's cached rule.uid silently
            // omitted every original car/trunk companion. Category 6
            // source branches have a distinct (var2, base_object) pair,
            // so matching both is the original branch picker.
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

        // Root create_map_element branches 6033785432522481,
        // 7238212881044891, 9881460187187066, 1618691315170714
        // spawn 348 on zed_resp at 210.Imagepoint1. Each uses original
        // Solid frames 17/21/22/23; omitting them causes walkthroughs.
        if(block_frame){
            const auto dest=find_layer_index_by_name(layout_index,"zed_resp");
            if(dest!=0xffff){
                const std::size_t index=placements_.size()-1;
                const auto tree=placements_[index];
                const float angle=tree.angle*0.01745329251994329577f;
                const float dx=(block_frame->point_x-block_frame->origin_x)*
                    tree.width*(tree.scale_x<0.0f?-1.0f:1.0f);
                const float dy=(block_frame->point_y-block_frame->origin_y)*
                    tree.height;
                const float x=tree.x+dx*std::cos(angle)-dy*std::sin(angle);
                const float y=tree.y+dx*std::sin(angle)+dy*std::cos(angle);
                const std::uint64_t uid=(std::uint64_t{1}<<63) |
                    static_cast<std::uint64_t>(placements_.size()+1);
                placements_.push_back({
                    dest,348,rule.uid,x,y,0.0f,
                    block_frame->solid_width,block_frame->solid_height,
                    tree.angle,1.0f,1.0f,
                    block_frame->solid_origin_x,block_frame->solid_origin_y,
                    0,block_frame->solid_frame
                });
                auto& solid=placements_.back();
                solid.runtime_uid=uid;
                solid.instance_id=static_cast<std::uint64_t>(placements_.size());
                solid.linked_runtime_uid=tree.runtime_uid;
                solid.parent_runtime_uid=tree.runtime_uid;
                solid.source_variable_0=tree.runtime_uid; // 348.var0
                placements_[index].linked_runtime_uid=uid;
                placements_[index].source_variable_1=uid; // 210.var1
            }
        }

        // Category6 original Mapgen508.var2 car-wreck parent (body)
        // executes a nested child event when Mapgen508.var3 == 0,
        // and Body.SpawnAnotherObject(241) on Sunset_Dawn at Imagepoint1.
        // This action was dropped because the generic house extractor
        // reads only same-event companions; a nested event has its own UID.
        // Every variant/frame below is extracted from the original source,
        // not a guessed car count, position or randomized frame.
        if(var1==6 && var3==0.0) {
            for(const auto& car:world_source::kSourceCarWreckTrunks) {
                if(car.variant!=var2||car.body_object!=rule.object)
                    continue;
                const WorldPlacement base=placements_[primary_index];
                const auto dest=find_layer_index_by_name(
                    layout_index,car.layer);
                if(dest==0xffff)continue;
                const auto* point=world_source::source_house_image_point(
                    base.object,base.source_animation,base.source_frame,
                    "",car.image_point);
                if(!point)continue;
                const float rad=base.angle*0.01745329251994329577f;
                const float dx=base.width*(point->x-point->origin_x)*
                               (base.scale_x<0.0f?-1.0f:1.0f);
                const float dy=base.height*(point->y-point->origin_y);
                const float child_x=base.x+dx*std::cos(rad)-dy*std::sin(rad);
                const float child_y=base.y+dx*std::sin(rad)+dy*std::cos(rad);
                if(!std::isfinite(child_x)||!std::isfinite(child_y))
                    continue;
                const std::uint64_t uid=(std::uint64_t{1}<<63)|
                    static_cast<std::uint64_t>(placements_.size()+1);
                placements_.push_back({
                    dest,241,car.source_uid,child_x,child_y,0.0f,
                    car.width,car.height,base.angle,1.0f,1.0f,
                    car.origin_x,car.origin_y,car.image_point,car.frame
                });
                auto& trunk=placements_.back();
                trunk.runtime_uid=uid;
                trunk.instance_id=placements_.size();
                trunk.parent_runtime_uid=base.runtime_uid;
                trunk.house_base_runtime_uid=base.runtime_uid;
                trunk.linked_runtime_uid=base.runtime_uid;
                // Original same-child Body.SetInstanceVariable(1,241.UID)
                // ensures the car body retains its searchable trunk.
                placements_[primary_index].source_variable_1=uid;
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

bool WorldSystem::open_source_bunker_lootbox(std::size_t index) noexcept {
    if(!generation_complete() || index>=placements_.size())return false;
    const auto box=placements_[index];
    if(!world_source::source_bunker_lootbox_closed(box) ||
       box.source_variable_1!=0)return false;
    // Root event UID389950912936355: Sprite796.SetFrame(1) then
    // Fade.StartFade and Boolean instance variable1=true.
    // Guard both effects as one original per-instance state change.
    if(!set_placement_sprite_state(index,box.source_animation,
             world_source::kSourceBunkerLootboxOpenedFrame,box.opacity))
        return false;
    placements_[index].source_variable_1=1;
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

bool WorldSystem::mark_source_car_trunk_searched(
    std::size_t index) noexcept {
    if(index>=placements_.size())return false;
    auto& p=placements_[index];
    if(p.object!=241 || !p.runtime_uid || p.source_variable_0!=0)
        return false;
    // Root Game_events UID 1074032691412307 selects the linked
    // Family1041 car body and SetFrame(1) BEFORE the Sprite241 marker is
    // destroyed. Preserve BOTH effects; otherwise an opened wreck still
    // looks unopened after searching it (and after Continue).
    const auto parent_uid=p.parent_runtime_uid;
    if(!parent_uid)return false;
    const auto it=std::find_if(placements_.begin(),placements_.end(),
        [parent_uid](const WorldPlacement& candidate){
            return candidate.runtime_uid==parent_uid;});
    if(it==placements_.end() ||
       !stage1::source_family_contains(1041,it->object))return false;
    const auto parent_index=static_cast<std::size_t>(
        it-placements_.begin());
    const auto frames=stage1::animations_for_object(it->object);
    if(!frames.data||frames.size==0||frames.data[0].frames<=1)
        return false;
    if(!set_placement_sprite_state(parent_index,0,1,it->opacity))
        return false;
    // Original 241.Destroy is adapted as a permanently hidden stable
    // instance so the one-shot state replays from the world save sidecar.
    p.source_variable_0=1;
    p.opacity=0.0f;
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
        if(placed.object==world_source::kSourceBunkerLootbox)
            placed.source_variable_1=
                placed.source_frame==world_source::kSourceBunkerLootboxOpenedFrame?1u:0u;
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
                if(p.object==world_source::kSourceBunkerLootbox)
                    p.source_variable_1=
                        p.source_frame==world_source::kSourceBunkerLootboxOpenedFrame?1u:0u;
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
