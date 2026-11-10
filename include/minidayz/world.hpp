#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
namespace minidayz {
enum class WorldFeatureKind : std::uint8_t { Terrain, Road, Building, Bunker, Bridge, SpawnPoint };
struct WorldPoint { float x{}; float y{}; };
struct WorldBounds {
 float width{36850.0f}; float height{20000.0f};
 bool contains(WorldPoint point) const noexcept;
 WorldPoint clamp(WorldPoint point) const noexcept;
};
struct WorldFeature {
 WorldFeatureKind kind{WorldFeatureKind::Terrain};
 WorldPoint position{}; std::uint32_t source_uid{}; std::uint32_t variant{};
};
// Source-space registry foundation. It does not invent generation rules.
class WorldSystem {
public:
 static constexpr float kSourceMapWidth=36850.0f;
 static constexpr float kSourceMapHeight=20000.0f;
 bool initialize(std::uint32_t seed, WorldBounds bounds = {}) noexcept;
 void reset() noexcept;
 bool add_feature(const WorldFeature& feature);
 const WorldFeature* feature(std::size_t index) const noexcept;
 std::size_t feature_count() const noexcept { return features_.size(); }
 const WorldBounds& bounds() const noexcept { return bounds_; }
 std::uint32_t seed() const noexcept { return seed_; }
 bool initialized() const noexcept { return initialized_; }
private:
 WorldBounds bounds_{}; std::uint32_t seed_{};
 std::vector<WorldFeature> features_{}; bool initialized_{};
};
} // namespace minidayz
