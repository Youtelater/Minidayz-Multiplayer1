#pragma once
#include "minidayz/world.hpp"
#include "minidayz/generated/location_terrain_program.hpp"
#include <cmath>
#include <initializer_list>
#include <random>
#include <iterator>

namespace minidayz::world_source {
struct SpecialTerrainExecutor {
    WorldSpecialTerrain& map;
    std::mt19937& rng;
    bool valid{true};
    double random(double hi) noexcept {
        return static_cast<double>(rng())/4294967296.0*hi;
    }
    double random(double lo,double hi) noexcept {return lo+random(hi-lo);}
    double choose(std::initializer_list<double> values) noexcept {
        if(values.size()==0){valid=false;return 0;}
        return values.begin()[static_cast<std::size_t>(random(values.size()))];
    }
    void paint(int object,std::initializer_list<double> params) noexcept {
        if(object!=230||params.size()!=3){valid=false;return;}
        const auto* p=params.begin();
        const int x=static_cast<int>(std::floor(p[0]));
        const int y=static_cast<int>(std::floor(p[1]));
        const int tile=static_cast<int>(p[2]);
        if(x<0||y<0||x>=map.columns||y>=map.rows||tile<0||tile>71){
            valid=false;return;
        }
        map.cells[static_cast<std::size_t>(y)*map.columns+x]=static_cast<std::uint32_t>(tile);
        ++map.writes;
    }
};

// Dispatch explicitly from the corresponding source entrypoint. This
// does NOT run the nonterrain ancestors (zombies, bridge solids, etc.).
// In particular, normal Map generation never executes these four leaves.
inline bool build_special_terrain(SourceSpecialTerrainEvent event,
                                  std::mt19937& rng,WorldSpecialTerrain& out) {
    WorldSpecialTerrain m{};
    switch(event){
    case SourceSpecialTerrainEvent::BridgeLoad:
    case SourceSpecialTerrainEvent::BridgeReload:
        m.x=148.0f;
        m.y=event==SourceSpecialTerrainEvent::BridgeLoad?18391.0f:18351.0f;
        m.width=7838.0f;m.height=270.0f;m.columns=262;m.rows=9;
        m.source_action_uid=event==SourceSpecialTerrainEvent::BridgeLoad?
            895209268453600ULL:458228549101251ULL;
        m.cells.assign(static_cast<std::size_t>(m.columns)*m.rows,0xffffffffu);
        break;
    case SourceSpecialTerrainEvent::TutorialStart:
        m.x=kTutorialStartX;m.y=kTutorialStartY;
        m.width=kTutorialStartWidth;m.height=kTutorialStartHeight;
        m.columns=kTutorialStartColumns;m.rows=kTutorialStartRows;
        m.layer=kTutorialStartLayer;m.source_action_uid=506106248958134ULL;
        m.cells.assign(std::begin(kTutorialStartAuthored),std::end(kTutorialStartAuthored));
        break;
    case SourceSpecialTerrainEvent::MenuStart:
        m.x=kMenuStartX;m.y=kMenuStartY;
        m.width=kMenuStartWidth;m.height=kMenuStartHeight;
        m.columns=kMenuStartColumns;m.rows=kMenuStartRows;
        m.layer=kMenuStartLayer;m.source_action_uid=2084078328497493ULL;
        m.cells.assign(std::begin(kMenuStartAuthored),std::end(kMenuStartAuthored));
        break;
    default:return false;
    }
    SpecialTerrainExecutor c{m,rng};
    switch(event){
    case SourceSpecialTerrainEvent::BridgeLoad:execute_source_BridgeLoad(c);break;
    case SourceSpecialTerrainEvent::BridgeReload:execute_source_BridgeReload(c);break;
    case SourceSpecialTerrainEvent::TutorialStart:execute_source_TutorialStart(c);break;
    case SourceSpecialTerrainEvent::MenuStart:execute_source_MenuStart(c);break;
    }
    if(!c.valid)return false;
    out=std::move(m);
    return true;
}
}
