#include "minidayz/generated/npc_spawn_call_source.hpp"
#include "minidayz/generated/npc_spawn_function_source.hpp"
#include "minidayz/generated/game_events_callsite_source.hpp"
#include "minidayz/npc_system.hpp"
#include "minidayz/world.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

int main() {
    using namespace minidayz::stage1;
    static_assert(kNpcSpawnCallSourceCount == 162);
    static_assert(kGameEventSourceCount > 0);
    static_assert(kGameEventActionSourceCount > 0);
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
    for (const auto& call : kNpcSpawnCallSource) {
        bool found_event = false;
        bool found_action = false;
        for (std::size_t i = 0; i < kGameEventSourceCount; ++i) {
            const auto& event = kGameEventSource[i];
            if (event.event_uid != call.event_uid) continue;
            found_event = true;
            for (std::size_t j = 0; j < event.action_count; ++j) {
                const auto& action = kGameEventActionSource[event.first_action + j];
                if (action.action_uid == call.action_uid &&
                    action.object_index == 189 && action.command_id == 69) {
                    found_action = true;
                    break;
                }
            }
        }
        assert(found_event && found_action);
    }

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

    // Exercise the actual world-generation -> source zed_resp placements ->
    // native actor path, rather than only testing isolated spawn_at calls.
    minidayz::WorldSystem world;
    assert(world.generate_from_source(0x4e504321u, "Map", 0));
    std::size_t source_markers = 0;
    for (std::size_t i = 0; i < world.placement_count(); ++i) {
        const auto* placement = world.placement(i);
        if (placement && placement->object == 216) ++source_markers;
    }
    assert(source_markers > 0);
    minidayz::NpcSystem integrated;
    assert(integrated.initialize(world));
    assert(integrated.count() == source_markers);
    bool has_valid_skin = false;
    for (std::size_t i = 0; i < integrated.count(); ++i) {
        const auto* actor = integrated.actor(i);
        assert(actor && actor->alive && actor->eyes_object == 256);
        assert(actor->skin_id >= 1 && actor->skin_id <= 6);
        assert(actor->source_object == expected_bodies[actor->skin_id - 1]);
        has_valid_skin = true;
    }
    assert(has_valid_skin);
    return 0;
}
