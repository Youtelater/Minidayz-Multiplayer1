#include "minidayz/location_event_runtime.hpp"
#include "minidayz/world.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <random>

using namespace minidayz;

int main() {
    construct::Array map_array;
    assert(map_array.set_size(16, 16, 1));

    std::mt19937 rng(12345);
    construct::SourceLocationEventRuntime runtime;
    assert(runtime.initialize(0, map_array, rng));
    runtime.set_mapgen(50, 3, 4, 1000.0, 2000.0, 100.0, 100.0);

    const auto* location = find_source_location_template(50);
    assert(location);
    bool matched = false;
    assert(runtime.execute_branch(location->event_uid, matched));
    assert(matched);

    assert(runtime.location_array().height() == 6);
    const auto& spawns = runtime.spawns();
    const auto zed = std::find_if(spawns.begin(), spawns.end(),
        [](const construct::SourceLocationSpawn& spawn) {
            return spawn.object == 216 && spawn.layer == "zed_resp";
        });
    assert(zed != spawns.end());
    assert(std::fabs(zed->x - 1237.0) < 0.001);
    assert(std::fabs(zed->y - 2283.0) < 0.001);
    assert(zed->frame == 11);
    assert(zed->instance_variables[2].as_number() == 1.0);
    return 0;
}
