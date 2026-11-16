#pragma once

#include <cstdint>

namespace minidayz::items {

// Construct item instance variable 6 is numeric, not an integer percentage.
// Source wear events use 0.2, 0.25, 0.4, and 0.5 condition decrements.
// Store hundredths exactly so repeated events cannot round away fractional
// damage or break equipment early. These values must be persisted by a
// future source-event wear integration; this class does not fire any events.
struct SourceFractionalWearSnapshot {
    std::uint16_t condition_hundredths{10000};
};

class SourceFractionalWear {
public:
    bool restore(SourceFractionalWearSnapshot snapshot) noexcept {
        if (snapshot.condition_hundredths > 10000) return false;
        condition_hundredths_ = snapshot.condition_hundredths;
        return true;
    }
    SourceFractionalWearSnapshot snapshot() const noexcept {
        return {condition_hundredths_};
    }
    bool initialize(std::uint8_t condition) noexcept {
        if (condition > 100) return false;
        condition_hundredths_ = static_cast<std::uint16_t>(condition * 100u);
        return true;
    }
    bool wear(std::uint16_t loss_hundredths) noexcept {
        if (loss_hundredths == 0 || condition_hundredths_ == 0)
            return false;
        condition_hundredths_ =
            loss_hundredths >= condition_hundredths_
                ? 0u
                : static_cast<std::uint16_t>(
                    condition_hundredths_ - loss_hundredths);
        return true;
    }
    bool broken() const noexcept { return condition_hundredths_ == 0; }
    float exact_condition() const noexcept {
        return static_cast<float>(condition_hundredths_) / 100.0f;
    }
    // The old native byte field cannot represent fractional conditions.
    // Rounding UP is safe for a compatibility display: wear only reaches
    // zero when the exact original value is zero, never prematurely.
    std::uint8_t compatible_condition() const noexcept {
        return static_cast<std::uint8_t>(
            (static_cast<unsigned>(condition_hundredths_) + 99u) / 100u);
    }

private:
    std::uint16_t condition_hundredths_{10000};
};

} // namespace minidayz::items
