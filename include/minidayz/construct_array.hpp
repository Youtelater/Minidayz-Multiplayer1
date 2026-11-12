#pragma once

#include "minidayz/construct_expression.hpp"

#include <cstddef>
#include <vector>

namespace minidayz::construct {

// Construct 2 Array semantics: zero-based 3D storage, out-of-range reads
// return numeric zero, out-of-range writes are ignored, and resizing retains
// values at coordinates that remain inside the new dimensions.
class Array {
public:
    bool set_size(int width, int height, int depth);
    void clear() noexcept;

    Value at(int x, int y = 0, int z = 0) const;
    bool evaluate_expression(std::uint16_t expression_id, const Value* arguments,
                             std::size_t argument_count, Value& result) const;
    bool execute_action(std::uint16_t action_id, const Value* arguments,
                        std::size_t argument_count);
    bool set_at_x(int x, Value value);
    bool set_at_xy(int x, int y, Value value);
    bool set_at_xyz(int x, int y, int z, Value value);

    std::size_t width() const noexcept { return width_; }
    std::size_t height() const noexcept { return height_; }
    std::size_t depth() const noexcept { return depth_; }

private:
    static constexpr std::size_t kMaxElements = 1u << 20;
    std::size_t offset(std::size_t x, std::size_t y, std::size_t z) const noexcept;

    std::size_t width_{};
    std::size_t height_{};
    std::size_t depth_{};
    std::vector<Value> values_{};
};

} // namespace minidayz::construct
