#pragma once

#include "minidayz/construct_array.hpp"
#include "minidayz/construct_event_interpreter.hpp"

#include <array>
#include <cstdint>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace minidayz::construct {

struct SourceLocationSpawn {
    std::uint16_t object{};
    std::string layer{};
    double x{};
    double y{};
    double width{};
    double height{};
    int frame{};
    std::array<Value, 16> instance_variables{};
};

// Runtime state for the original generate_location event sheet. The map
// Array (object 518) is supplied by the caller; the location Array (525)
// starts with the original layout dimensions and is retained across calls.
class SourceLocationEventRuntime {
public:
    bool initialize(std::uint8_t level, const Array& map_array,
                    std::mt19937& rng);
    void set_mapgen(std::uint16_t tile_code, std::uint8_t grid_x,
                    std::uint8_t grid_y, double world_x, double world_y,
                    double cell_width, double cell_height);
    bool execute_branch(std::uint64_t branch_uid, bool& matched);

    const Array& map_array() const noexcept { return map_array_; }
    const Array& location_array() const noexcept { return location_array_; }
    const std::vector<SourceLocationSpawn>& spawns() const noexcept { return spawns_; }

private:
    static bool read_variable(void*, std::string_view, Value&);
    static bool call_system(void*, std::uint16_t, const Value*, std::size_t, Value&);
    static bool call_object(void*, std::uint16_t, std::uint16_t, bool,
                            std::string_view, const Value*, std::size_t, Value&);
    static bool read_instance_variable(void*, std::uint16_t, bool,
                                       std::string_view, std::uint16_t, Value&);
    static bool evaluate_condition(void*, const stage1::LocationEventConditionSource&,
                                   int, bool, bool&, int&);
    static bool execute_action(void*, const stage1::LocationEventActionSource&, int);

    bool evaluate_parameter(std::uint32_t first, std::uint16_t count,
                            std::size_t index, Value& result);
    bool compare_values(const Value& lhs, int operation, const Value& rhs) const;
    Value variable(std::string_view name) const;
    void set_variable(std::string_view name, const Value& value);
    SourceLocationSpawn* last_spawn(std::uint16_t object = 0) noexcept;

    std::uint8_t level_{};
    std::uint16_t tile_code_{};
    std::uint8_t grid_x_{}, grid_y_{};
    double mapgen_x_{}, mapgen_y_{}, cell_width_{1.0}, cell_height_{1.0};
    std::mt19937* rng_{};
    SourceSystemExpressionState system_state_{};
    EvaluationContext expression_context_{};
    EventCallbacks event_callbacks_{};
    Array map_array_{};
    Array location_array_{};
    std::array<Value, 16> mapgen_variables_{};
    std::array<Value, 16> zed_variables_{};
    std::array<Value, 4> function_parameters_{};
    std::vector<std::pair<std::string, Value>> variables_{};
    std::vector<SourceLocationSpawn> spawns_{};
    std::size_t last_spawn_index_{static_cast<std::size_t>(-1)};
    std::size_t spawn_start_index_{};
    int zed_frame_{};
    bool initialized_{};
};

} // namespace minidayz::construct
