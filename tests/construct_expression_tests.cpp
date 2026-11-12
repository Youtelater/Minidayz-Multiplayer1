#include "minidayz/construct_expression.hpp"

#include <cassert>
#include <cmath>
#include <map>
#include <random>
#include <string>

using namespace minidayz::construct;

namespace {
struct TestState {
    std::map<std::string, Value> variables;
    double array_value{7};
    std::size_t system_calls{};
    double instance_variable{33};
};

bool read_variable(void* user, std::string_view name, Value& result) {
    auto& state = *static_cast<TestState*>(user);
    const auto found = state.variables.find(std::string(name));
    if (found == state.variables.end()) return false;
    result = found->second;
    return true;
}

bool system_expression(void* user, std::uint16_t id, const Value* args,
                       std::size_t count, Value& result) {
    auto& state = *static_cast<TestState*>(user);
    ++state.system_calls;
    if (id != 82 || count != 2) return false;
    result = args[1];
    return true;
}

bool read_instance_variable(void* user, std::uint16_t object, bool behavior,
                            std::string_view, std::uint16_t variable, Value& result) {
    auto& state = *static_cast<TestState*>(user);
    if (object != 514 || behavior || variable != 2) return false;
    result = Value::from_number(state.instance_variable);
    return true;
}

bool object_expression(void* user, std::uint16_t object, std::uint16_t id,
                       bool behavior, std::string_view, const Value* args,
                       std::size_t count, Value& result) {
    auto& state = *static_cast<TestState*>(user);
    if (object != 525 || id != 222 || behavior || count != 2 ||
        args[0].as_number() != 0 || args[1].as_number() != 0) return false;
    result = Value::from_number(state.array_value);
    return true;
}
}

int main() {
    TestState state;
    state.variables.emplace("hotspotX", Value::from_number(100));
    EvaluationContext context{&state, read_variable, system_expression, object_expression, read_instance_variable};
    Value result;

    assert(evaluate_expression("[4,[23,\"hotspotX\"],[0,237]]", context, result));
    assert(result.as_number() == 337);

    assert(evaluate_expression("[5,[0,9],[0,4]]", context, result));
    assert(result.as_number() == 5);

    assert(evaluate_expression("[19,82,[[0,2],[0,4]]]", context, result));
    assert(result.as_number() == 4 && state.system_calls == 1);

    assert(evaluate_expression("[20,525,222,false,null,[[0,0],[0,0]]]", context, result));
    assert(result.as_number() == 7);

    assert(evaluate_expression("[21,514,false,null,2]", context, result));
    assert(result.as_number() == 33);

    assert(evaluate_expression("[4,[0,\"north\"],[0,\" road\"]]", context, result));
    assert(result.kind == Value::Kind::String && result.string == "north road");

    std::mt19937 source_rng(1234);
    SourceSystemExpressionState source_system{&source_rng, 4};
    const Value floor_argument[] = {Value::from_number(3.9)};
    assert(evaluate_source_system_expression(&source_system, 80, floor_argument, 1, result));
    assert(result.as_number() == 3);
    assert(evaluate_source_system_expression(&source_system, 150, nullptr, 0, result));
    assert(result.as_number() == 4);
    const Value choose_arguments[] = {Value::from_number(2), Value::from_number(8)};
    assert(evaluate_source_system_expression(&source_system, 82, choose_arguments, 2, result));
    assert(result.as_number() == 2 || result.as_number() == 8);
    const Value random_argument[] = {Value::from_number(12)};
    assert(evaluate_source_system_expression(&source_system, 95, random_argument, 1, result));
    assert(result.as_number() >= 0 && result.as_number() < 12);
    assert(evaluate_source_system_expression(&source_system, 204, floor_argument, 1, result));
    assert(result.as_number() == 3);

    EvaluationError error{};
    assert(!evaluate_expression("[99,0]", context, result, &error));
    assert(error == EvaluationError::UnsupportedOpcode);
    return 0;
}
