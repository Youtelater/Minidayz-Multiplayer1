#include "minidayz/world.hpp"
#include <algorithm>
#include <cmath>
namespace minidayz {
bool WorldBounds::contains(WorldPoint p) const noexcept {
 return std::isfinite(p.x)&&std::isfinite(p.y)&&p.x>=0.0f&&p.y>=0.0f&&p.x<=width&&p.y<=height;
}
WorldPoint WorldBounds::clamp(WorldPoint p) const noexcept {
 if(!std::isfinite(p.x)) p.x=0.0f;
 if(!std::isfinite(p.y)) p.y=0.0f;
 return {std::clamp(p.x,0.0f,std::max(0.0f,width)),std::clamp(p.y,0.0f,std::max(0.0f,height))};
}
bool WorldSystem::initialize(std::uint32_t seed,WorldBounds bounds) noexcept {
 if(!std::isfinite(bounds.width)||!std::isfinite(bounds.height)||bounds.width<=0.0f||bounds.height<=0.0f) return false;
 bounds_=bounds; seed_=seed; features_.clear(); initialized_=true; return true;
}
void WorldSystem::reset() noexcept { features_.clear(); seed_=0; initialized_=false; }
bool WorldSystem::add_feature(const WorldFeature& f) {
 if(!initialized_||!bounds_.contains(f.position)) return false;
 features_.push_back(f); return true;
}
const WorldFeature* WorldSystem::feature(std::size_t i) const noexcept {
 return i<features_.size()?&features_[i]:nullptr;
}
} // namespace minidayz
