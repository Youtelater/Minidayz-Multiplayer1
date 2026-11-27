#include "minidayz/source_special_box_actions.hpp"
#include <cassert>
#include <vector>
using namespace minidayz::items;
int main(){
    for(const auto& b:kSourceSpecialBoxBranches){
        std::vector<std::uint64_t> seen;
        assert(source_execute_special_box(b.category,[&](std::size_t n){
            assert(b.outcome>0 && static_cast<std::size_t>(b.outcome)<=n);
            return static_cast<std::size_t>(b.outcome-1);
        },[](std::string_view){return true;},[&](const auto& a){
            seen.push_back(a.uid);return true;
        },[](int,const auto&){return false;}));
        assert(seen.size()==b.count);
        for(std::size_t i=0;i<seen.size();++i)
            assert(seen[i]==kSourceSpecialBoxActions[b.first+i].uid);
        if(!b.unlock.empty()){
            int draws=0;seen.clear();
            assert(source_execute_special_box(b.category,[&](std::size_t n)->std::size_t{
                if(draws++==0)return static_cast<std::size_t>(b.outcome-1);
                assert(n==b.retry_count);return 0;
            },[](std::string_view){return false;},[&](const auto& a){
                seen.push_back(a.uid);return true;
            },[](int,const auto&){return false;}));
            assert(draws==2);
            for(const auto& fallback:kSourceSpecialBoxBranches)
                if(fallback.category==b.category&&fallback.outcome==1){
                    assert(seen.size()==fallback.count);
                    for(std::size_t i=0;i<seen.size();++i)
                        assert(seen[i]==kSourceSpecialBoxActions[fallback.first+i].uid);
                }
        }
    }
    std::size_t calls=0,zeros=0;
    assert(source_execute_special_box(1022,[](std::size_t n){return n-1;},
        [](std::string_view){return false;},[](const auto&){return false;},
        [&](int id,const auto& pool){
            assert(pool.condition==100);
            assert(id==kSourceSpecialBoxChoices[pool.first+pool.count-1]);
            ++calls;if(id==0)++zeros;return true;
        }));
    assert(calls==6 && zeros==3);
    assert(!source_execute_special_box(1022,[](std::size_t n){return n;},
        [](std::string_view){return false;},[](const auto&){return true;},
        [](int,const auto&){return true;}));
}
