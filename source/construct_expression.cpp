#include "minidayz/construct_expression.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <utility>
#include <vector>

namespace minidayz::construct {
namespace {

class Parser {
public:
    Parser(std::string_view input, EvaluationContext& context)
        : input_(input), context_(context) {}

    bool run(Value& result, EvaluationError* error) {
        if (!expression(result, 0)) return fail(error, error_);
        whitespace();
        if (position_ != input_.size()) return fail(error, EvaluationError::TrailingInput);
        if (error) *error = EvaluationError::None;
        return true;
    }

private:
    static constexpr std::size_t kMaxDepth = 64;
    std::string_view input_;
    EvaluationContext& context_;
    std::size_t position_{};
    EvaluationError error_{EvaluationError::InvalidSyntax};

    static bool fail(EvaluationError* output, EvaluationError value) {
        if (output) *output = value;
        return false;
    }

    void whitespace() {
        while (position_ < input_.size()) {
            const char c = input_[position_];
            if (c != ' ' && c != '\n' && c != '\r' && c != '\t') break;
            ++position_;
        }
    }

    bool take(char expected) {
        whitespace();
        if (position_ >= input_.size() || input_[position_] != expected) return false;
        ++position_;
        return true;
    }

    bool integer(std::uint16_t& value) {
        whitespace();
        if (position_ >= input_.size()) return false;
        unsigned long parsed = 0;
        const std::size_t start = position_;
        while (position_ < input_.size() && input_[position_] >= '0' &&
               input_[position_] <= '9') {
            parsed = parsed * 10 + static_cast<unsigned long>(input_[position_] - '0');
            if (parsed > std::numeric_limits<std::uint16_t>::max()) return false;
            ++position_;
        }
        if (position_ == start) return false;
        value = static_cast<std::uint16_t>(parsed);
        return true;
    }

    bool string(std::string& value) {
        whitespace();
        if (position_ >= input_.size() || input_[position_] != '"') return false;
        ++position_;
        value.clear();
        while (position_ < input_.size()) {
            char c = input_[position_++];
            if (c == '"') return true;
            if (c != '\\') {
                value.push_back(c);
                continue;
            }
            if (position_ >= input_.size()) return false;
            c = input_[position_++];
            switch (c) {
            case '"': case '\\': case '/': value.push_back(c); break;
            case 'b': value.push_back('\b'); break;
            case 'f': value.push_back('\f'); break;
            case 'n': value.push_back('\n'); break;
            case 'r': value.push_back('\r'); break;
            case 't': value.push_back('\t'); break;
            case 'u': {
                if (position_ + 4 > input_.size()) return false;
                unsigned code = 0;
                for (unsigned i = 0; i < 4; ++i) {
                    const char h = input_[position_++];
                    code <<= 4;
                    if (h >= '0' && h <= '9') code |= static_cast<unsigned>(h - '0');
                    else if (h >= 'a' && h <= 'f') code |= static_cast<unsigned>(h - 'a' + 10);
                    else if (h >= 'A' && h <= 'F') code |= static_cast<unsigned>(h - 'A' + 10);
                    else return false;
                }
                if (code <= 0x7f) value.push_back(static_cast<char>(code));
                else if (code <= 0x7ff) {
                    value.push_back(static_cast<char>(0xc0 | (code >> 6)));
                    value.push_back(static_cast<char>(0x80 | (code & 0x3f)));
                } else {
                    value.push_back(static_cast<char>(0xe0 | (code >> 12)));
                    value.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3f)));
                    value.push_back(static_cast<char>(0x80 | (code & 0x3f)));
                }
                break;
            }
            default: return false;
            }
        }
        return false;
    }

    bool scalar(Value& value) {
        whitespace();
        if (position_ >= input_.size()) return false;
        if (input_[position_] == '"') {
            std::string parsed;
            if (!string(parsed)) return false;
            value = Value::from_string(std::move(parsed));
            return true;
        }
        if (input_.substr(position_, 4) == "true") {
            position_ += 4;
            value = Value::from_number(1);
            return true;
        }
        if (input_.substr(position_, 5) == "false") {
            position_ += 5;
            value = Value::from_number(0);
            return true;
        }
        if (input_.substr(position_, 4) == "null") {
            position_ += 4;
            value = Value::from_number(0);
            return true;
        }
        const std::size_t start = position_;
        while (position_ < input_.size()) {
            const char c = input_[position_];
            if ((c >= '0' && c <= '9') || c == '-' || c == '+' ||
                c == '.' || c == 'e' || c == 'E') ++position_;
            else break;
        }
        if (position_ == start) return false;
        const std::string token(input_.substr(start, position_ - start));
        char* end = nullptr;
        const double parsed = std::strtod(token.c_str(), &end);
        if (!end || end != token.c_str() + token.size()) return false;
        value = Value::from_number(parsed);
        return true;
    }

    bool skip_json_value() {
        whitespace();
        if (position_ >= input_.size()) return false;
        if (input_[position_] == '"') {
            std::string ignored;
            return string(ignored);
        }
        if (input_[position_] != '[' && input_[position_] != '{') {
            const std::size_t start = position_;
            while (position_ < input_.size()) {
                const char c = input_[position_];
                if (c == ',' || c == ']' || c == '}' ||
                    c == ' ' || c == '\n' || c == '\r' || c == '\t') break;
                ++position_;
            }
            return position_ > start;
        }

        std::vector<char> closing;
        bool in_string = false;
        bool escaped = false;
        while (position_ < input_.size()) {
            const char c = input_[position_++];
            if (in_string) {
                if (escaped) escaped = false;
                else if (c == '\\') escaped = true;
                else if (c == '"') in_string = false;
                continue;
            }
            if (c == '"') {
                in_string = true;
            } else if (c == '[') {
                closing.push_back(']');
            } else if (c == '{') {
                closing.push_back('}');
            } else if (c == ']' || c == '}') {
                if (closing.empty() || closing.back() != c) return false;
                closing.pop_back();
                if (closing.empty()) return true;
            }
        }
        return false;
    }

    bool expression_list(std::vector<Value>& values, std::size_t depth) {
        if (!take('[')) return false;
        whitespace();
        if (take(']')) return true;
        for (;;) {
            Value argument;
            if (!expression(argument, depth + 1)) return false;
            values.push_back(std::move(argument));
            whitespace();
            if (take(']')) return true;
            if (!take(',')) return false;
        }
    }

    bool expression(Value& result, std::size_t depth) {
        if (depth > kMaxDepth) {
            error_ = EvaluationError::RecursionLimit;
            return false;
        }
        if (!take('[')) return false;
        std::uint16_t opcode{};
        if (!integer(opcode) || !take(',')) return false;

        if (opcode == 0) {
            if (!scalar(result) || !take(']')) return false;
            return true;
        }

        if (opcode == 4 || opcode == 5 || opcode == 6 ||
            opcode == 7 || opcode == 17) {
            Value left, right;
            if (!expression(left, depth + 1) || !take(',') ||
                !expression(right, depth + 1) || !take(']')) return false;
            if (opcode == 4 && (left.kind == Value::Kind::String ||
                                right.kind == Value::Kind::String)) {
                result = Value::from_string(left.as_string() + right.as_string());
            } else if (opcode == 4) {
                result = Value::from_number(left.as_number() + right.as_number());
            } else if (opcode == 5) {
                result = Value::from_number(left.as_number() - right.as_number());
            } else if (opcode == 6) {
                result = Value::from_number(left.as_number() * right.as_number());
            } else if (opcode == 7) {
                result = Value::from_number(left.as_number() / right.as_number());
            } else {
                const bool equal = left.kind == Value::Kind::String ||
                                   right.kind == Value::Kind::String
                    ? left.as_string() == right.as_string()
                    : left.as_number() == right.as_number();
                result = Value::from_number(equal ? 1.0 : 0.0);
            }
            return true;
        }

        if (opcode == 18) {
            Value condition;
            if (!expression(condition, depth + 1) || !take(',')) return false;
            const bool choose_true = condition.kind == Value::Kind::String
                ? !condition.string.empty() : condition.as_number() != 0.0;
            if (choose_true) {
                if (!expression(result, depth + 1) || !take(',') ||
                    !skip_json_value() || !take(']')) return false;
            } else {
                if (!skip_json_value() || !take(',') ||
                    !expression(result, depth + 1) || !take(']')) return false;
            }
            return true;
        }

        if (opcode == 21) {
            std::uint16_t object_index{}, variable_index{};
            if (!integer(object_index) || !take(',')) return false;
            whitespace();
            bool is_behavior = false;
            if (input_.substr(position_, 4) == "true") {
                is_behavior = true;
                position_ += 4;
            } else if (input_.substr(position_, 5) == "false") {
                position_ += 5;
            } else return false;
            if (!take(',')) return false;
            whitespace();
            std::string behavior_name;
            if (input_.substr(position_, 4) == "null") position_ += 4;
            else if (!string(behavior_name)) return false;
            if (!take(',') || !integer(variable_index) || !take(']')) return false;
            if (!context_.read_instance_variable) {
                error_ = EvaluationError::MissingCallback;
                return false;
            }
            if (!context_.read_instance_variable(context_.user, object_index,
                                                 is_behavior, behavior_name,
                                                 variable_index, result)) {
                error_ = EvaluationError::CallbackFailed;
                return false;
            }
            return true;
        }

        if (opcode == 23) {
            std::string name;
            if (!string(name) || !take(']')) return false;
            if (!context_.read_variable) {
                error_ = EvaluationError::MissingCallback;
                return false;
            }
            if (!context_.read_variable(context_.user, name, result)) {
                error_ = EvaluationError::CallbackFailed;
                return false;
            }
            return true;
        }

        if (opcode == 19) {
            std::uint16_t expression_id{};
            if (!integer(expression_id)) return false;
            std::vector<Value> arguments;
            whitespace();
            if (take(',')) {
                if (!expression_list(arguments, depth + 1)) return false;
            }
            if (!take(']')) return false;
            if (!context_.call_system) {
                error_ = EvaluationError::MissingCallback;
                return false;
            }
            if (!context_.call_system(context_.user, expression_id,
                                      arguments.data(), arguments.size(), result)) {
                error_ = EvaluationError::CallbackFailed;
                return false;
            }
            return true;
        }

        if (opcode == 20) {
            std::uint16_t object_index{}, expression_id{};
            if (!integer(object_index) || !take(',') || !integer(expression_id) ||
                !take(',')) return false;
            whitespace();
            bool is_behavior = false;
            if (input_.substr(position_, 4) == "true") {
                is_behavior = true;
                position_ += 4;
            } else if (input_.substr(position_, 5) == "false") {
                position_ += 5;
            } else return false;
            if (!take(',')) return false;
            whitespace();
            std::string behavior_name;
            if (input_.substr(position_, 4) == "null") position_ += 4;
            else if (!string(behavior_name)) return false;
            std::vector<Value> arguments;
            whitespace();
            if (take(',')) {
                if (!expression_list(arguments, depth + 1)) return false;
            }
            if (!take(']')) return false;
            if (!context_.call_object) {
                error_ = EvaluationError::MissingCallback;
                return false;
            }
            if (!context_.call_object(context_.user, object_index, expression_id,
                                      is_behavior, behavior_name, arguments.data(),
                                      arguments.size(), result)) {
                error_ = EvaluationError::CallbackFailed;
                return false;
            }
            return true;
        }

        error_ = EvaluationError::UnsupportedOpcode;
        return false;
    }
};

} // namespace

Value Value::from_number(double value) {
    Value result;
    result.kind = Kind::Number;
    result.number = value;
    return result;
}

Value Value::from_string(std::string value) {
    Value result;
    result.kind = Kind::String;
    result.string = std::move(value);
    return result;
}

double Value::as_number() const noexcept {
    if (kind == Kind::Number) return number;
    char* end = nullptr;
    const double parsed = std::strtod(string.c_str(), &end);
    return end == string.c_str() ? 0.0 : parsed;
}

std::string Value::as_string() const {
    if (kind == Kind::String) return string;
    char buffer[64];
    const int written = std::snprintf(buffer, sizeof(buffer), "%.15g", number);
    return written > 0 ? std::string(buffer, static_cast<std::size_t>(written)) : std::string{};
}

bool evaluate_source_system_expression(void* user, std::uint16_t expression_id,
                                       const Value* arguments,
                                       std::size_t argument_count, Value& result) {
    auto* state = static_cast<SourceSystemExpressionState*>(user);
    if (!state) return false;

    switch (expression_id) {
    case 80: // floor(x)
        if (argument_count != 1 || !arguments) return false;
        result = Value::from_number(std::floor(arguments[0].as_number()));
        return true;
    case 82: { // choose(a, b, ...); a single choice is returned unchanged.
        if (!arguments || argument_count == 0 ||
            argument_count > std::numeric_limits<std::uint32_t>::max()) return false;
        if (argument_count == 1) {
            result = arguments[0];
            return true;
        }
        if (!state->rng) return false;
        const auto bound = static_cast<std::uint32_t>(argument_count);
        const std::uint32_t threshold = static_cast<std::uint32_t>(-bound) % bound;
        std::uint32_t draw{};
        do {
            draw = static_cast<std::uint32_t>((*state->rng)());
        } while (draw < threshold);
        result = arguments[draw % bound];
        return true;
    }
    case 95: { // random(x) / random(a, b)
        if (!state->rng || !arguments || argument_count == 0 || argument_count > 2)
            return false;
        const double unit = static_cast<double>((*state->rng)()) / 4294967296.0;
        if (argument_count == 1) {
            result = Value::from_number(unit * arguments[0].as_number());
        } else {
            const double lower = arguments[0].as_number();
            const double upper = arguments[1].as_number();
            result = Value::from_number(lower + unit * (upper - lower));
        }
        return true;
    }
    case 150: // loopindex
        if (argument_count != 0) return false;
        result = Value::from_number(static_cast<double>(state->loop_index));
        return true;
    case 204: // int(x)
        if (argument_count != 1 || !arguments) return false;
        result = Value::from_number(std::trunc(arguments[0].as_number()));
        return true;
    default:
        return false;
    }
}

bool evaluate_expression(std::string_view source, EvaluationContext& context,
                         Value& result, EvaluationError* error) {
    Parser parser(source, context);
    return parser.run(result, error);
}

} // namespace minidayz::construct
