#pragma once
#include "minidayz/world.hpp"
#include "minidayz/stage1_data_access.hpp"
#include "minidayz/generated/world_source_extents.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace minidayz::terrain {

// GPU draw broadphase: native Map can have thousands of source objects,
// but a 400x240 3DS viewport sees a tiny fraction. The old renderer
// traversed every world placement EVERY FRAME even when far off screen.
class VisibleSourcePlacementIndex {
public:
    static constexpr float kCellSize=512.0f;
    void rebuild(const WorldSystem& world) {
        const auto& size=world.bounds();
        cols_=std::max(1,static_cast<int>(std::ceil(size.width/kCellSize)));
        rows_=std::max(1,static_cast<int>(std::ceil(size.height/kCellSize)));
        bins_.clear();
        bins_.resize(static_cast<std::size_t>(cols_)*rows_);
        const auto* layout=stage1::find_layout("Map");
        if(!layout)return;
        const auto layout_index=static_cast<std::uint16_t>(
            layout-stage1::kLayoutTable);
        for(std::size_t i=0;i<world.placement_count();++i){
            const auto* p=world.placement(i);
            // Include currently transparent source instances: original Construct
            // events can reveal an initially hidden roof/interior/object.
            // The draw loop still skips opacity <= 0; broadphase must not
            // permanently exclude sprites whose visibility can change.
            if(!p||p->layer==32||p->layer>=44)
                continue;
            const auto* object=stage1::find_object(p->object);
            if(!object||object->type!=18)continue;
            const auto* layer=stage1::find_layer(layout_index,p->layer);
            if(layer && (!layer->visible ||
                std::strstr(layer->name,"respawn")||
                std::strstr(layer->name,"_resp")||
                std::strstr(layer->name,"points")))continue;
            // Runtime-generated Construct sprites can have ZERO instance
            // dimensions. Their source ObjectDef still contains the original
            // sprite size. Without these bounds, buildings/trees disappear
            // when their hotspot is in the next 512px visibility bin.
            // Use the shared original game source table, not an optional
            // renderer-only atlas header (native world tests need this too).
            const float sprite_w=std::max(std::max(0.0f,p->width),
                                         std::max(0.0f,object->width));
            const float sprite_h=std::max(std::max(0.0f,p->height),
                                         std::max(0.0f,object->height));
            const float scale=std::max(1.0f,std::fabs(p->scale_x));
            // Generated world instances commonly have 0x0 dimensions;
            // ObjectDef itself also has zero size in the original export.
            // A fixed 30px broadphase then clips trees, houses and roofs
            // before their actual hotspots reach the camera boundary.
            // Use the most distant original frame corner across ALL sprite
            // animations, so changing the frame cannot lose visibility.
            const float source_radius=p->object<stage1::kWorldSourceExtentCount
                ? static_cast<float>(stage1::kWorldSourceMaxRadius[p->object])
                : 0.0f;
            const float extent=std::max(30.0f,
                std::max(std::hypot(sprite_w,sprite_h),source_radius)*scale);
            const int x0=std::clamp(static_cast<int>(
                std::floor((p->x-extent)/kCellSize)),0,cols_-1);
            const int x1=std::clamp(static_cast<int>(
                std::floor((p->x+extent)/kCellSize)),0,cols_-1);
            const int y0=std::clamp(static_cast<int>(
                std::floor((p->y-extent)/kCellSize)),0,rows_-1);
            const int y1=std::clamp(static_cast<int>(
                std::floor((p->y+extent)/kCellSize)),0,rows_-1);
            for(int y=y0;y<=y1;++y)
                for(int x=x0;x<=x1;++x)
                    bins_[static_cast<std::size_t>(y)*cols_+x].push_back(
                        static_cast<std::uint32_t>(i));
        }
    }
    void query(float x,float y,float w,float h,
               std::vector<std::uint32_t>& result) const {
        result.clear();
        if(bins_.empty()||w<=0||h<=0||!std::isfinite(x)||
           !std::isfinite(y))return;
        const int x0=std::clamp(static_cast<int>(
            std::floor(x/kCellSize)),0,cols_-1);
        const int x1=std::clamp(static_cast<int>(
            std::floor((x+w)/kCellSize)),0,cols_-1);
        const int y0=std::clamp(static_cast<int>(
            std::floor(y/kCellSize)),0,rows_-1);
        const int y1=std::clamp(static_cast<int>(
            std::floor((y+h)/kCellSize)),0,rows_-1);
        for(int cy=y0;cy<=y1;++cy)
            for(int cx=x0;cx<=x1;++cx){
                const auto& bin=bins_[static_cast<std::size_t>(cy)*cols_+cx];
                result.insert(result.end(),bin.begin(),bin.end());
            }
        // Restore original source creation order for overlapping sprites.
        std::sort(result.begin(),result.end());
        result.erase(std::unique(result.begin(),result.end()),result.end());
    }
private:
    int cols_{},rows_{};
    std::vector<std::vector<std::uint32_t>> bins_{};
};
} // namespace minidayz::terrain
