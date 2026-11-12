#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>
namespace minidayz {
struct LocationExpressionValue {
 enum class Type : std::uint8_t { Number, String, Boolean, Null };
 Type type{Type::Null}; double number{}; bool boolean{}; std::string string{};
 static LocationExpressionValue numeric(double v){LocationExpressionValue x;x.type=Type::Number;x.number=v;return x;}
 static LocationExpressionValue text(std::string v){LocationExpressionValue x;x.type=Type::String;x.string=std::move(v);return x;}
 double as_number() const noexcept;
};
struct LocationExpressionContext {
 std::function<bool(std::string_view,LocationExpressionValue&)> variable;
 std::function<bool(std::uint32_t,const std::vector<LocationExpressionValue>&,LocationExpressionValue&)> function;
 std::function<bool(const std::vector<LocationExpressionValue>&,LocationExpressionValue&)> object;
 std::function<bool(std::uint32_t,std::string_view,LocationExpressionValue&)> special;
};
class LocationExpressionEvaluator {
public:
 explicit LocationExpressionEvaluator(const LocationExpressionContext& c):context_(c){}
 bool evaluate(std::string_view json,LocationExpressionValue& out) const;
private: const LocationExpressionContext& context_;
};
}
