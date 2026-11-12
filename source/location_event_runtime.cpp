#include "minidayz/location_event_runtime.hpp"
#include "minidayz/generated/location_event_source.hpp"

#include <cmath>
#include <string_view>

namespace minidayz::construct {

bool SourceLocationEventRuntime::initialize(std::uint8_t level,
                                            const Array& map_array,
                                            std::mt19937& rng) {
    level_ = level;
    rng_ = &rng;
    system_state_ = {&rng, 0};
    map_array_ = map_array;
    if (!location_array_.set_size(75, 8, 1)) return false;
    mapgen_variables_ = {};
    zed_variables_ = {};
    function_parameters_ = {};
    variables_.clear();
    spawns_.clear();
    last_spawn_index_ = static_cast<std::size_t>(-1);
    zed_frame_ = 0;
    set_variable("CurrentLevel", Value::from_number(level));
    set_variable("post_points", Value::from_number(0));

    expression_context_ = {
        this, read_variable, call_system, call_object, read_instance_variable
    };
    event_callbacks_ = {this, evaluate_condition, execute_action};
    initialized_ = true;
    return true;
}

void SourceLocationEventRuntime::set_mapgen(std::uint16_t tile_code,
                                            std::uint8_t grid_x,
                                            std::uint8_t grid_y,
                                            double world_x, double world_y,
                                            double cell_width,
                                            double cell_height) {
    tile_code_ = tile_code;
    grid_x_ = grid_x;
    grid_y_ = grid_y;
    mapgen_x_ = world_x;
    mapgen_y_ = world_y;
    cell_width_ = cell_width > 0.0 ? cell_width : 1.0;
    cell_height_ = cell_height > 0.0 ? cell_height : 1.0;

    mapgen_variables_[0] = Value::from_number(tile_code);
    mapgen_variables_[1] = Value::from_number(grid_y);
    mapgen_variables_[2] = Value::from_number(static_cast<double>(grid_x) + 1.0);
    function_parameters_[0] = Value::from_number(tile_code);

    set_variable("hotspotX", Value::from_number(world_x));
    set_variable("hotspotY", Value::from_number(world_y));
    set_variable("TilemapspotX", Value::from_number(grid_x));
    set_variable("TilemapspotY", Value::from_number(grid_y));
    set_variable("loopzounter", Value::from_number(0));
    set_variable("PlantX", Value::from_number(0));
    set_variable("PlantY", Value::from_number(0));
    last_spawn_index_ = static_cast<std::size_t>(-1);
}

bool SourceLocationEventRuntime::execute_branch(std::uint64_t branch_uid,
                                                bool& matched) {
    if (!initialized_) return false;
    return execute_location_event_branch(branch_uid, event_callbacks_, matched);
}

bool SourceLocationEventRuntime::read_variable(void* user,
                                               std::string_view name,
                                               Value& result) {
    result = static_cast<SourceLocationEventRuntime*>(user)->variable(name);
    return true;
}

bool SourceLocationEventRuntime::call_system(void* user, std::uint16_t id,
                                             const Value* arguments,
                                             std::size_t count, Value& result) {
    auto& self = *static_cast<SourceLocationEventRuntime*>(user);
    return evaluate_source_system_expression(&self.system_state_, id,
                                              arguments, count, result);
}

bool SourceLocationEventRuntime::call_object(void* user, std::uint16_t object,
                                             std::uint16_t id, bool behavior,
                                             std::string_view,
                                             const Value* arguments,
                                             std::size_t count, Value& result) {
    auto& self = *static_cast<SourceLocationEventRuntime*>(user);
    if (behavior) return false;

    if ((object == 518 || object == 525) && id == 222) {
        return (object == 518 ? self.map_array_ : self.location_array_)
            .evaluate_expression(id, arguments, count, result);
    }
    if (object == 514 && count == 0 && id == 65) {
        result = Value::from_number(self.mapgen_x_);
        return true;
    }
    if (object == 514 && count == 0 && id == 66) {
        result = Value::from_number(self.mapgen_y_);
        return true;
    }
    if (object == 189 && id == 93 && count == 1) {
        const auto index = static_cast<int>(arguments[0].as_number());
        if (index < 0 || static_cast<std::size_t>(index) >= self.function_parameters_.size())
            return false;
        result = self.function_parameters_[static_cast<std::size_t>(index)];
        return true;
    }
    if (object == 1066 && count == 1 && id == 220) {
        result = Value::from_number(std::floor(arguments[0].as_number() / self.cell_width_));
        return true;
    }
    if (object == 1066 && count == 1 && id == 221) {
        result = Value::from_number(std::floor(arguments[0].as_number() / self.cell_height_));
        return true;
    }
    if (object == 915 && count == 0 && (id == 65 || id == 66)) {
        const auto* spawn = self.last_spawn(915);
        result = Value::from_number(spawn ? (id == 65 ? spawn->x : spawn->y)
                                           : (id == 65 ? self.mapgen_x_ : self.mapgen_y_));
        return true;
    }
    return false;
}

bool SourceLocationEventRuntime::read_instance_variable(
    void* user, std::uint16_t object, bool behavior, std::string_view,
    std::uint16_t index, Value& result) {
    auto& self = *static_cast<SourceLocationEventRuntime*>(user);
    if (behavior) return false;
    if (object == 514 && index < self.mapgen_variables_.size()) {
        result = self.mapgen_variables_[index];
        return true;
    }
    if (object == 216 && index < self.zed_variables_.size()) {
        result = self.zed_variables_[index];
        return true;
    }
    return false;
}

bool SourceLocationEventRuntime::evaluate_parameter(std::uint32_t first,
                                                    std::uint16_t count,
                                                    std::size_t index,
                                                    Value& result) {
    if (index >= count ||
        first + index >= stage1::kLocationEventParameterSourceCount) return false;
    return evaluate_location_event_parameter(
        stage1::kLocationEventParameterSource[first + index],
        expression_context_, result);
}

bool SourceLocationEventRuntime::compare_values(const Value& lhs, int operation,
                                                const Value& rhs) const {
    const bool strings = lhs.kind == Value::Kind::String ||
                         rhs.kind == Value::Kind::String;
    const double left_number = lhs.as_number();
    const double right_number = rhs.as_number();
    const std::string left_string = lhs.as_string();
    const std::string right_string = rhs.as_string();
    switch (operation) {
    case 0: return strings ? left_string == right_string : left_number == right_number;
    case 1: return strings ? left_string != right_string : left_number != right_number;
    case 2: return strings ? left_string < right_string : left_number < right_number;
    case 3: return strings ? left_string <= right_string : left_number <= right_number;
    case 4: return strings ? left_string > right_string : left_number > right_number;
    case 5: return strings ? left_string >= right_string : left_number >= right_number;
    default: return false;
    }
}

Value SourceLocationEventRuntime::variable(std::string_view name) const {
    for (const auto& entry : variables_)
        if (entry.first == name) return entry.second;
    return Value::from_number(0);
}

void SourceLocationEventRuntime::set_variable(std::string_view name,
                                              const Value& value) {
    for (auto& entry : variables_) {
        if (entry.first == name) {
            entry.second = value;
            return;
        }
    }
    variables_.emplace_back(std::string(name), value);
}

SourceLocationSpawn* SourceLocationEventRuntime::last_spawn(std::uint16_t object) noexcept {
    if (last_spawn_index_ >= spawns_.size()) return nullptr;
    auto& spawn = spawns_[last_spawn_index_];
    if (object != 0 && spawn.object != object) return nullptr;
    return &spawn;
}

bool SourceLocationEventRuntime::evaluate_condition(
    void* user, const stage1::LocationEventConditionSource& condition,
    int loop_index, bool, bool& matched, int& repeat_count) {
    auto& self = *static_cast<SourceLocationEventRuntime*>(user);
    self.system_state_.loop_index = loop_index;
    Value p[4];
    for (std::size_t i = 0; i < condition.parameter_count && i < 4; ++i)
        if (!self.evaluate_parameter(condition.first_parameter,
                                     condition.parameter_count, i, p[i])) return false;

    if (condition.object_index == -1 && condition.command_id == 51) {
        if (condition.parameter_count != 3) return false;
        matched = self.compare_values(p[0], static_cast<int>(p[1].as_number()), p[2]);
        return true;
    }
    if (condition.object_index == -1 && condition.command_id == 123) {
        if (condition.parameter_count != 1) return false;
        repeat_count = static_cast<int>(p[0].as_number());
        matched = repeat_count > 0;
        return true;
    }
    if (condition.object_index == -1 && condition.command_id == 46) {
        if (condition.parameter_count != 3 || p[0].kind != Value::Kind::String) return false;
        matched = self.compare_values(self.variable(p[0].as_string()),
                                      static_cast<int>(p[1].as_number()), p[2]);
        return true;
    }
    if (condition.object_index == 514 && condition.command_id == 71) {
        if (condition.parameter_count != 3) return false;
        const int index = static_cast<int>(p[0].as_number());
        if (index < 0 || static_cast<std::size_t>(index) >= self.mapgen_variables_.size())
            return false;
        matched = self.compare_values(self.mapgen_variables_[static_cast<std::size_t>(index)],
                                      static_cast<int>(p[1].as_number()), p[2]);
        return true;
    }
    if (condition.object_index == 216 && condition.command_id == 64) {
        if (condition.parameter_count != 2) return false;
        matched = self.compare_values(Value::from_number(self.zed_frame_),
                                      static_cast<int>(p[0].as_number()), p[1]);
        return true;
    }
    if (condition.object_index == 518 && condition.command_id == 203) {
        if (condition.parameter_count != 4) return false;
        const auto value = self.map_array_.at(
            static_cast<int>(p[0].as_number()),
            static_cast<int>(p[1].as_number()));
        matched = self.compare_values(value, static_cast<int>(p[2].as_number()), p[3]);
        return true;
    }
    return false;
}

bool SourceLocationEventRuntime::execute_action(
    void* user, const stage1::LocationEventActionSource& action,
    int loop_index) {
    auto& self = *static_cast<SourceLocationEventRuntime*>(user);
    self.system_state_.loop_index = loop_index;
    Value p[4];
    for (std::size_t i = 0; i < action.parameter_count && i < 4; ++i)
        if (!self.evaluate_parameter(action.first_parameter,
                                     action.parameter_count, i, p[i])) return false;

    if (action.object_index == -1 && (action.command_id == 41 ||
                                      action.command_id == 122)) {
        if (action.parameter_count != 2 || p[0].kind != Value::Kind::String) return false;
        self.set_variable(p[0].as_string(), p[1]);
        return true;
    }
    if (action.object_index == -1 && action.command_id == 215) {
        if (action.parameter_count != 2 || p[0].kind != Value::Kind::String) return false;
        self.set_variable(p[0].as_string(),
            Value::from_number(self.variable(p[0].as_string()).as_number() + p[1].as_number()));
        return true;
    }
    if (action.object_index == 525 &&
        (action.command_id == 199 || action.command_id == 210)) {
        return self.location_array_.execute_action(action.command_id, p,
                                                    action.parameter_count);
    }
    if (action.object_index == -1 && action.command_id == 94) {
        if (action.parameter_count != 4 || p[1].kind != Value::Kind::String) return false;
        SourceLocationSpawn spawn{};
        spawn.object = static_cast<std::uint16_t>(p[0].as_number());
        spawn.layer = p[1].as_string();
        spawn.x = p[2].as_number();
        spawn.y = p[3].as_number();
        self.spawns_.push_back(std::move(spawn));
        self.last_spawn_index_ = self.spawns_.size() - 1;
        return true;
    }
    if (action.object_index == 514 && action.command_id == 73) {
        if (action.parameter_count != 3 || p[1].kind != Value::Kind::String) return false;
        SourceLocationSpawn spawn{};
        spawn.object = static_cast<std::uint16_t>(p[0].as_number());
        spawn.layer = p[1].as_string();
        spawn.x = self.mapgen_x_;
        spawn.y = self.mapgen_y_;
        self.spawns_.push_back(std::move(spawn));
        self.last_spawn_index_ = self.spawns_.size() - 1;
        return true;
    }
    if (action.object_index == 216 && action.command_id == 72) {
        if (action.parameter_count != 1) return false;
        self.zed_frame_ = static_cast<int>(p[0].as_number());
        if (auto* spawn = self.last_spawn(216)) spawn->frame = self.zed_frame_;
        return true;
    }
    if (action.object_index == 216 && action.command_id == 88) {
        if (action.parameter_count != 2) return false;
        const int index = static_cast<int>(p[0].as_number());
        if (index < 0 || static_cast<std::size_t>(index) >= self.zed_variables_.size())
            return false;
        self.zed_variables_[static_cast<std::size_t>(index)] = p[1];
        if (auto* spawn = self.last_spawn(216))
            spawn->instance_variables[static_cast<std::size_t>(index)] = p[1];
        return true;
    }
    if (action.object_index == 404 && action.command_id == 103) {
        if (action.parameter_count != 2) return false;
        if (auto* spawn = self.last_spawn()) {
            spawn->width = p[0].as_number();
            spawn->height = p[1].as_number();
        }
        return true;
    }
    if (action.object_index == 915 && action.command_id == 47) {
        if (action.parameter_count != 2) return false;
        if (auto* spawn = self.last_spawn(915)) {
            spawn->x = p[0].as_number();
            spawn->y = p[1].as_number();
        }
        return true;
    }
    if (action.object_index == 915 && action.command_id == 72) {
        if (action.parameter_count != 1) return false;
        if (auto* spawn = self.last_spawn(915))
            spawn->frame = static_cast<int>(p[0].as_number());
        return true;
    }
    return false;
}

} // namespace minidayz::construct
