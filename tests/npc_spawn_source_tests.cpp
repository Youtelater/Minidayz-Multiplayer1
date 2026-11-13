#include "minidayz/generated/npc_spawn_call_source.hpp"
#include "minidayz/generated/npc_spawn_function_source.hpp"
#include "minidayz/npc_system.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

int main() {
    using namespace minidayz::stage1;
    static_assert(kNpcSpawnCallSourceCount == 162);
    static_assert(kNpcFunctionDefinitionEventUid == 180375540292947ULL);
    static_assert(kNpcFunctionEventSourceCount == 399);
    static_assert(kNpcFunctionConditionSourceCount > 0);
    static_assert(kNpcFunctionActionSourceCount > 0);
    static_assert(kNpcFunctionParameterSourceCount > 0);

    bool has_nested_function_call = false;
    for (const auto& action : kNpcFunctionActionSource) {
        if (action.object_index == 189 && action.command_id == 69)
            has_nested_function_call = true;
        assert(action.raw_call != nullptr);
    }
    assert(has_nested_function_call);
    for (const auto& event : kNpcFunctionEventSource) {
        assert(event.event_uid != 0);
        assert(event.depth < 64);
        assert(event.parent_index == 0xffffffffu || event.parent_index < kNpcFunctionEventSourceCount);
    }

    std::size_t three_argument_calls = 0;
    std::size_t four_argument_calls = 0;
    for (const auto& call : kNpcSpawnCallSource) {
        assert(call.event_sheet != nullptr);
        assert(std::strcmp(call.event_sheet, "Game_events") == 0);
        assert(call.event_uid != 0);
        assert(call.action_uid != 0);
        assert(call.argument_count == 3 || call.argument_count == 4);
        for (std::size_t i = 0; i < call.argument_count; ++i) {
            assert(call.arguments[i] != nullptr);
            assert(call.arguments[i][0] != '\0');
        }
        for (std::size_t i = call.argument_count; i < 4; ++i)
            assert(call.arguments[i] == nullptr);
        if (call.argument_count == 3) ++three_argument_calls;
        else ++four_argument_calls;
    }
    assert(three_argument_calls + four_argument_calls == 162);

    // Source Spawn_NPC Skin_id branches 1..6 select these body object IDs.
    constexpr std::uint16_t expected_bodies[] = {600, 206, 582, 583, 586, 587};
    minidayz::NpcSystem npcs;
    for (std::uint8_t skin = 1; skin <= 6; ++skin) {
        assert(npcs.spawn_at(100.0f, 200.0f, 0.0f, skin, skin));
        const auto* actor = npcs.actor(npcs.count() - 1);
        assert(actor && actor->alive);
        assert(actor->skin_id == skin);
        assert(actor->source_object == expected_bodies[skin - 1]);
        assert(actor->eyes_object == 256);
        assert(actor->x == 100.0f && actor->y == 200.0f);
    }
    assert(!npcs.spawn_at(0.0f, 0.0f, -1.0f, 1));
    assert(!npcs.spawn_at(0.0f, 0.0f, 1.0f, 7));
    assert(!npcs.spawn_at(NAN, 0.0f, 1.0f, 1));

    assert(npcs.spawn_at(500.0f, 600.0f, 25.0f, 0, 99));
    const auto* random_skin = npcs.actor(npcs.count() - 1);
    assert(random_skin && random_skin->skin_id >= 1 && random_skin->skin_id <= 6);
    assert(std::fabs(random_skin->x - 500.0f) <= 25.0f);
    assert(std::fabs(random_skin->y - 600.0f) <= 25.0f);
    return 0;
}
