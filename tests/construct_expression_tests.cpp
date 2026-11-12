#include "minidayz/construct_expression.hpp"

#include <cassert>
#include <cmath>
#include <map>
#include <string>

using namespace minidayz::construct;

namespace {
struct TestState {
    std::map<std::string, Value> variables;
    double array_value{7};
    std::size_t system_calls{};
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
    EvaluationContext context{&state, read_variable, system_expression, object_expression};
    Value result;

    assert(evaluate_expression("[4,[23,\"hotspotX\"],[0,237]]", context, result));
    assert(result.as_number() == 337);

    assert(evaluate_expression("[5,[0,9],[0,4]]", context, result));
    assert(result.as_number() == 5);

    assert(evaluate_expression("[19,82,[[0,2],[0,4]]]", context, result));
    assert(result.as_number() == 4 && state.system_calls == 1);

    assert(evaluate_expression("[20,525,222,false,null,[[0,0],[0,0]]]", context, result));
    assert(result.as_number() == 7);

    assert(evaluate_expression("[4,[0,\"north\"],[0,\" road\"]]", context, result));
    assert(result.kind == Value::Kind::String && result.string == "north road");

    EvaluationError error{};
    assert(!evaluate_expression("[99,0]", context, result, &error));
    assert(error == EvaluationError::UnsupportedOpcode);
    return 0;
}
