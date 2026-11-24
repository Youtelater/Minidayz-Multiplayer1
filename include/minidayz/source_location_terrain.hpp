#pragma once
#include "minidayz/generated/location_terrain_program.hpp"
#include "minidayz/world.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <random>

namespace minidayz::world_source {
// Native projection of the original independently picked ground family,
// water and ground_enviroment Tilemap instances. Coordinates are local
// tile indices, not normalized Array518 coordinates.
struct SourceLocationTerrain {
    static constexpr int ground_side=17,water_side=17,scatter_side=35;
    const WorldGrid& grid;
    std::mt19937& rng;
    int col,row,marker,level,perk;
    double TilemapspotX{},TilemapspotY{},x{},y{};
    int ground_object{},ground_x{-500},ground_y{-500};
    int scatter_x{-500},scatter_y{-500};
    int water_x{-500},water_y{-500};
    bool scatter_created{},water_created{},valid{true};
    std::array<std::int16_t,ground_side*ground_side> ground{};
    std::array<std::int16_t,water_side*water_side> water{};
    std::array<std::int16_t,scatter_side*scatter_side> scatter{};
    SourceLocationTerrain(const WorldGrid& g,std::mt19937& random,
                          int cx,int cy,int type,int lv,int pk) noexcept
        :grid(g),rng(random),col(cx),row(cy),marker(type),level(lv),perk(pk) {
        ground.fill(-1);water.fill(-1);scatter.fill(-1);
    }
    double random(double hi) noexcept {
        return static_cast<double>(rng())/4294967296.0*hi;
    }
    double random(double lo,double hi) noexcept {return lo+random(hi-lo);}
    double choose(std::initializer_list<double> values) noexcept {
        if(!values.size()){valid=false;return 0;}
        const auto n=static_cast<std::size_t>(random(static_cast<double>(values.size())));
        return values.begin()[n];
    }
    int array_at(double cx,double cy) const noexcept {
        return grid.at(static_cast<int>(cx),static_cast<int>(cy));
    }
    template<std::size_t N>
    static int read(const std::array<std::int16_t,N>& a,int side,int tx,int ty) noexcept {
        if(tx<0||ty<0||tx>=side||ty>=side)return -1;
        return a[static_cast<std::size_t>(ty)*side+tx];
    }
    int tile_at(int object,double tx,double ty) const noexcept {
        const int ix=static_cast<int>(tx),iy=static_cast<int>(ty);
        if(object==1066)return read(ground,ground_side,ix,iy);
        if(object==208)return read(water,water_side,ix,iy);
        if(object==230)return read(scatter,scatter_side,ix,iy);
        return -1;
    }
    void create(int object,int ox,int oy) noexcept {
        if(object==208){
            water_created=true;water_x=ox;water_y=oy;water.fill(-1);
        }else if(object==230){
            scatter_created=true;scatter_x=ox;scatter_y=oy;scatter.fill(-1);
        }else{
            // Original Sprite514.var6 is assigned the last created
            // ground instance UID. The projected program has validated
            // every corresponding source UID assignment.
            ground_object=object;ground_x=ox;ground_y=oy;ground.fill(6);
        }
    }
    template<std::size_t N>
    static void write(std::array<std::int16_t,N>& a,int side,
                      int tx,int ty,int w,int h,int frame) noexcept {
        for(int j=0;j<h;++j)for(int i=0;i<w;++i){
            const int cx=tx+i,cy=ty+j;
            if(cx>=0&&cy>=0&&cx<side&&cy<side)
                a[static_cast<std::size_t>(cy)*side+cx]=static_cast<std::int16_t>(frame);
        }
    }
    void paint(int object,std::initializer_list<double> params) noexcept {
        if(params.size()!=3&&params.size()!=5){valid=false;return;}
        // C++ list-initialization evaluates x, y, width, height and tile
        // expressions in original order, including every random draw.
        const auto* p=params.begin();
        const int tx=static_cast<int>(p[0]),ty=static_cast<int>(p[1]);
        const bool range=params.size()==5;
        const int w=range?static_cast<int>(p[2]):1,h=range?static_cast<int>(p[3]):1;
        const int frame=static_cast<int>(p[range?4:2]);
        if(frame<0||frame>255||w<=0||h<=0){valid=false;return;}
        if(object==1066){
            if(ground_object)write(ground,ground_side,tx,ty,w,h,frame);
        }else if(object==208){
            if(water_created)write(water,water_side,tx,ty,w,h,frame);
        }else if(object==230){
            if(scatter_created)write(scatter,scatter_side,tx,ty,w,h,frame);
        }else valid=false;
    }
    bool execute() noexcept {
        execute_source_location_terrain(*this);
        return valid;
    }
};
}
