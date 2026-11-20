#include "minidayz/mp_worldseed_random.hpp"
#include <cassert>
#include <cstdint>
#include <cmath>
int main(){
    minidayz::worldseed::MpWorldRandom a(123456789u);
    constexpr std::uint32_t expected[]{
        1107202814u,4169434471u,3372958138u,
        885470128u,1301683845u,3208624240u};
    for(auto value:expected)assert(a.next_u32()==value);
    minidayz::worldseed::MpWorldRandom b(123456789u);
    assert(std::fabs(b.next_unit()-0.2577907438389957)<1.0e-15);
    minidayz::worldseed::MpWorldRandom same(123456789u);
    assert(same.next_u32()==expected[0]);
    minidayz::worldseed::MpWorldRandom different(123456788u);
    assert(different.next_u32()!=expected[0]);
    return 0;
}
