#include "minidayz/world.hpp"
#include "minidayz/stage1_data_access.hpp"
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
 bounds_=bounds; seed_=seed; features_.clear(); placements_.clear(); initialized_=true; return true;
}
bool WorldSystem::load_source_layout(std::string_view name) {
 const auto* layout=stage1::find_layout(name);
 if(!layout) return false;
 const auto index=static_cast<std::uint16_t>(layout-stage1::kLayoutTable);
 const auto range=stage1::instances_for_layout(index);
 bounds_={layout->width,layout->height};
 features_.clear(); placements_.clear(); placements_.reserve(range.size);
 for(std::size_t i=0;i<range.size;++i) {
  const auto& src=range.data[i];
  placements_.push_back({src.layer,src.object,src.uid,src.x,src.y,src.z,src.w,src.h,
                         src.angle,src.opacity,src.scale_x,src.ox,src.oy});
 }
 initialized_=true;
 return true;
}
void WorldSystem::reset() noexcept {
 features_.clear(); placements_.clear(); seed_=0; initialized_=false;
}
bool WorldSystem::add_feature(const WorldFeature& f) {
 if(!initialized_||!bounds_.contains(f.position)) return false;
 features_.push_back(f); return true;
}
const WorldFeature* WorldSystem::feature(std::size_t i) const noexcept {
 return i<features_.size()?&features_[i]:nullptr;
}
const WorldPlacement* WorldSystem::placement(std::size_t i) const noexcept {
 return i<placements_.size()?&placements_[i]:nullptr;
}
} // namespace minidayz
