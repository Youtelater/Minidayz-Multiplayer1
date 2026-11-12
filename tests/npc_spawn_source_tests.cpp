#include "minidayz/generated/npc_spawn_call_source.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

int main() {
    using namespace minidayz::stage1;
    static_assert(kNpcSpawnCallSourceCount == 162);
    static_assert(kNpcSpawnFunctionEventUid == 180375540292947ULL);

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
    return 0;
}
