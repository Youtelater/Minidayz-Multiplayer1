#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace minidayz::construct {

struct Value {
    enum class Kind : std::uint8_t { Number, String };
    Kind kind{Kind::Number};
    double number{};
    std::string string{};

    static Value from_number(double value);
    static Value from_string(std::string value);
    double as_number() const noexcept;
    std::string as_string() const;
};

using VariableReader = bool (*)(void* user, std::string_view name, Value& result);
using SystemExpression = bool (*)(void* user, std::uint16_t expression_id,
                                  const Value* arguments, std::size_t argument_count,
                                  Value& result);
using ObjectExpression = bool (*)(void* user, std::uint16_t object_index,
                                  std::uint16_t expression_id, bool is_behavior,
                                  std::string_view behavior_name,
                                  const Value* arguments, std::size_t argument_count,
                                  Value& result);

struct EvaluationContext {
    void* user{};
    VariableReader read_variable{};
    SystemExpression call_system{};
    ObjectExpression call_object{};
};

enum class EvaluationError : std::uint8_t {
    None,
    InvalidSyntax,
    UnsupportedOpcode,
    MissingCallback,
    CallbackFailed,
    RecursionLimit,
    TrailingInput
};

// Evaluates the serialized expression arrays emitted by Construct 2 exports.
// System and object ACEs are delegated to callbacks so their numeric IDs can
// be mapped against the exact source project/runtime instead of guessed here.
bool evaluate_expression(std::string_view source, EvaluationContext& context,
                         Value& result, EvaluationError* error = nullptr);

} // namespace minidayz::construct
