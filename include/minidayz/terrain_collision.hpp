#pragma once
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include <utility>
#include "minidayz/terrain_surface.hpp"
#include "minidayz/source_tilemap_solid_collision.hpp"

namespace minidayz::terrain {

struct SolidTerrainMap {
    float x{}, y{};
    std::uint32_t tile_size{}, columns{}, rows{};
    std::uint16_t object{}, layer{};
    std::vector<std::uint32_t> cells{};
    bool occupied(float px, float py) const noexcept {
        if (!std::isfinite(px)||!std::isfinite(py)||
            tile_size==0||columns==0||rows==0) return false;
        const float dx=px-x,dy=py-y;
        if(dx<0.0f||dy<0.0f||
           dx>=tile_size*static_cast<float>(columns)||
           dy>=tile_size*static_cast<float>(rows))return false;
        const auto col=static_cast<std::uint32_t>(dx/tile_size);
        const auto row=static_cast<std::uint32_t>(dy/tile_size);
        const auto index=static_cast<std::size_t>(row)*columns+col;
        return index<cells.size() && cells[index]!=0xffffffffu;
    }

    bool blocks(float px,float py) const noexcept {
        if(!occupied(px,py))return false;
        const float fx=(px-x)/static_cast<float>(tile_size);
        const float fy=(py-y)/static_cast<float>(tile_size);
        const auto col=static_cast<std::uint32_t>(fx);
        const auto row=static_cast<std::uint32_t>(fy);
        const auto offset=static_cast<std::size_t>(row)*columns+col;
        if(offset>=cells.size())return false;
        // Preserve exact Construct per-tile Solid polygons, including
        // shore/forest diagonal openings and original H/V/D flip flags.
        // A plain full-tile occupied() test wrongly blocked transparent
        // source polygon gaps.
        return source_solid_tile_contains(
            object,cells[offset],fx-static_cast<float>(col),
            fy-static_cast<float>(row));
    }
};

class SolidTerrain {
public:
    bool load(const char* file_path) noexcept {
        maps_.clear();
        if(!file_path)return false;
        std::FILE* file=std::fopen(file_path,"rb");
        if(!file)return false;
        char magic[4]{};
        std::uint32_t count=0;
        bool ok=std::fread(magic,1,4,file)==4&&
                std::memcmp(magic,"MDT1",4)==0&&
                std::fread(&count,4,1,file)==1&&
                count<=32;
        for(std::uint32_t i=0;i<count&&ok;++i){
            SolidTerrainMap map{};
            ok=std::fread(&map.object,2,1,file)==1&&
               std::fread(&map.layer,2,1,file)==1&&
               std::fread(&map.x,4,1,file)==1&&
               std::fread(&map.y,4,1,file)==1&&
               std::fread(&map.tile_size,4,1,file)==1&&
               std::fread(&map.columns,4,1,file)==1&&
               std::fread(&map.rows,4,1,file)==1;
            if(!ok||!std::isfinite(map.x)||!std::isfinite(map.y)||
               map.tile_size<8||map.tile_size>256||
               map.columns==0||map.rows==0||
               static_cast<std::uint64_t>(map.columns)*map.rows>2000000){
                ok=false;break;
            }
            // Preserve the original Solid behavior of tilemap_water (208)
            // and tilemap_forest (211); sand/ground decorations are non-solid.
            const bool relevant=(map.object==208||map.object==211||
                                 map.object==418)&&map.layer<44;
            const std::size_t cells=static_cast<std::size_t>(map.columns)*map.rows;
            if(relevant){
                map.cells.resize(cells);
                ok=std::fread(map.cells.data(),4,cells,file)==cells;
                if(ok)maps_.push_back(std::move(map));
            }else{
                ok=std::fseek(file,static_cast<long>(cells*4),SEEK_CUR)==0;
            }
        }
        std::fclose(file);
        if(!ok)maps_.clear();
        return ok;
    }

    bool blocked(float x,float y) const noexcept {
        for(const auto& map:maps_)
            if(detail_blocks_player(map.object) && map.blocks(x,y))
                return true;
        return false;
    }

    FootstepSurface surface_at(float x,float y) const noexcept {
        // Sand and forest may overlap; water takes precedence if present.
        FootstepSurface surface=FootstepSurface::ground;
        for(const auto& map:maps_) {
            if(!map.occupied(x,y))continue;
            if(map.object==208)return FootstepSurface::shallow_water;
            if(map.object==211)surface=FootstepSurface::forest;
            if(map.object==418 && surface==FootstepSurface::ground)
                surface=FootstepSurface::sand;
        }
        return surface;
    }

private:
    std::vector<SolidTerrainMap> maps_{};
};

} // namespace minidayz::terrain
