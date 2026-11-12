#include "minidayz/construct_array.hpp"

#include <algorithm>
#include <utility>

namespace minidayz::construct {

std::size_t Array::offset(std::size_t x, std::size_t y, std::size_t z) const noexcept {
    return x + width_ * (y + height_ * z);
}

bool Array::set_size(int width, int height, int depth) {
    if (width < 0 || height < 0 || depth < 0) return false;
    if (width == 0 || height == 0 || depth == 0) {
        width_ = height_ = depth_ = 0;
        values_.clear();
        return true;
    }

    const auto new_width = static_cast<std::size_t>(width);
    const auto new_height = static_cast<std::size_t>(height);
    const auto new_depth = static_cast<std::size_t>(depth);
    if (new_width > kMaxElements / new_height ||
        new_width * new_height > kMaxElements / new_depth) return false;
    const std::size_t new_count = new_width * new_height * new_depth;

    std::vector<Value> resized(new_count);
    const std::size_t copy_width = std::min(width_, new_width);
    const std::size_t copy_height = std::min(height_, new_height);
    const std::size_t copy_depth = std::min(depth_, new_depth);
    for (std::size_t z = 0; z < copy_depth; ++z) {
        for (std::size_t y = 0; y < copy_height; ++y) {
            for (std::size_t x = 0; x < copy_width; ++x) {
                resized[x + new_width * (y + new_height * z)] =
                    values_[offset(x, y, z)];
            }
        }
    }

    width_ = new_width;
    height_ = new_height;
    depth_ = new_depth;
    values_.swap(resized);
    return true;
}

void Array::clear() noexcept {
    for (auto& value : values_) value = Value::from_number(0);
}

Value Array::at(int x, int y, int z) const {
    if (x < 0 || y < 0 || z < 0 ||
        static_cast<std::size_t>(x) >= width_ ||
        static_cast<std::size_t>(y) >= height_ ||
        static_cast<std::size_t>(z) >= depth_) return Value::from_number(0);
    return values_[offset(static_cast<std::size_t>(x),
                          static_cast<std::size_t>(y),
                          static_cast<std::size_t>(z))];
}

bool Array::evaluate_expression(std::uint16_t expression_id, const Value* arguments,
                                  std::size_t argument_count, Value& result) const {
    // Construct Array expression ACE 222 is At(X[, Y[, Z]]).
    if (expression_id != 222 || !arguments || argument_count == 0 ||
        argument_count > 3) return false;
    const int x = static_cast<int>(arguments[0].as_number());
    const int y = argument_count > 1 ? static_cast<int>(arguments[1].as_number()) : 0;
    const int z = argument_count > 2 ? static_cast<int>(arguments[2].as_number()) : 0;
    result = at(x, y, z);
    return true;
}

bool Array::set_at_x(int x, Value value) {
    return set_at_xyz(x, 0, 0, std::move(value));
}

bool Array::set_at_xy(int x, int y, Value value) {
    return set_at_xyz(x, y, 0, std::move(value));
}

bool Array::set_at_xyz(int x, int y, int z, Value value) {
    if (x < 0 || y < 0 || z < 0 ||
        static_cast<std::size_t>(x) >= width_ ||
        static_cast<std::size_t>(y) >= height_ ||
        static_cast<std::size_t>(z) >= depth_) return false;
    values_[offset(static_cast<std::size_t>(x),
                   static_cast<std::size_t>(y),
                   static_cast<std::size_t>(z))] = std::move(value);
    return true;
}

} // namespace minidayz::construct
