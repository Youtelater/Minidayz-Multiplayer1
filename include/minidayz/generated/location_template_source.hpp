#pragma once
#include <cstddef>
#include <cstdint>
namespace minidayz::stage1 {
struct LocationTemplateSource {
    std::uint64_t event_uid;
    std::uint16_t tile_codes[24];
    std::uint8_t tile_code_count;
    std::uint16_t event_nodes;
    std::uint16_t array_write_actions;
    std::uint8_t system_create_actions;
};
inline constexpr std::size_t kLocationTemplateSourceCount = 62;
inline constexpr LocationTemplateSource kLocationTemplateSource[] = {
    {108325992016075ULL, {50,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,3,45,1},
    {549886041723769ULL, {51,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,3,45,1},
    {681214039176259ULL, {108,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,3,41,1},
    {540367043658427ULL, {109,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,3,41,1},
    {209769408222865ULL, {52,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,7,12,0},
    {3458349682606061ULL, {1,10,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 2,8,12,1},
    {9231486718159572ULL, {2,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 2,9,12,1},
    {285184237401461ULL, {23,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,60,0},
    {259096597311532ULL, {24,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,5,85,1},
    {529441923930050ULL, {25,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,158,0},
    {182257730581709ULL, {26,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,5,68,1},
    {203896123103257ULL, {27,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,73,0},
    {991723783043236ULL, {28,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,64,0},
    {552222308661835ULL, {29,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,53,0},
    {853783574848041ULL, {40,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,5,58,1},
    {750462581544539ULL, {41,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,5,58,1},
    {771900919213406ULL, {42,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,53,0},
    {820280212574969ULL, {43,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,48,0},
    {555977762724278ULL, {44,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,53,0},
    {762230503166806ULL, {45,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,53,0},
    {976948404987716ULL, {46,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,65,0},
    {676957321129644ULL, {47,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,53,0},
    {148893605846635ULL, {48,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,63,0},
    {209894270132125ULL, {49,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,52,0},
    {650491441075886ULL, {60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,53,0},
    {828632162116981ULL, {61,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,58,0},
    {330605429224417ULL, {62,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,53,0},
    {144206326135174ULL, {63,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,53,0},
    {314567262934603ULL, {64,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,53,0},
    {577097223999052ULL, {65,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,57,0},
    {268509954129827ULL, {66,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,74,0},
    {203564460568820ULL, {67,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,5,90,1},
    {9670184670359728ULL, {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,7,16,0},
    {6900587140270256ULL, {4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,84,1},
    {9017713591479232ULL, {5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,209,1},
    {6221187152452198ULL, {22,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,165,1},
    {233182247473362ULL, {70,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,10,201,1},
    {7398632301693316ULL, {6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,48,2},
    {2975870129601185ULL, {21,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,52,2},
    {6499244366010827ULL, {7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,3,28,1},
    {5581112549586124ULL, {9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,96,1},
    {3463332379885827ULL, {11,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,92,1},
    {673012591922620ULL, {102,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,342,1},
    {701528131755731ULL, {103,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,64,1},
    {654325871174469ULL, {104,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,327,1},
    {632373728603128ULL, {105,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,148,1},
    {151698771524205ULL, {106,110,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 2,5,176,1},
    {544591683564560ULL, {107,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,99,1},
    {5828017669220644ULL, {15,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,48,1},
    {9110672737066796ULL, {12,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,9,312,1},
    {7243610440090174ULL, {13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,5,64,1},
    {5748879959570351ULL, {14,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,17,685,5},
    {6049878429774374ULL, {16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,134,1},
    {5350540726438238ULL, {17,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,7,83,1},
    {731842808406228ULL, {30,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,3,44,0},
    {174261871477223ULL, {31,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,3,40,0},
    {558210106216281ULL, {32,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,52,1},
    {457184391566633ULL, {99,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,4,60,1},
    {271003505800440ULL, {100,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,2,24,0},
    {895341149396024ULL, {101,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,2,24,0},
    {718013605019961ULL, {23,24,25,26,27,28,29,40,41,42,43,44,45,46,47,48,49,60,61,62,63,64,65,66,0}, 24,2,0,0},
    {227650356880234ULL, {67,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, 1,2,0,0},
};
} // namespace minidayz::stage1
","script":"#!/usr/bin/env python3
"""Extract the original generate_location dispatch branches into a C++ source index.

The emitted records preserve Construct event UIDs, marker-code conditions, and
counts of the original Array write / System create actions. This is an index of
the source event tree, not an execution of those actions; command semantics
must be decoded before templates can be instantiated in the native world.
"""
import json
from pathlib import Path

SOURCE = Path("data/stage1/construct_project_parsed.json")
OUTPUT = Path("include/minidayz/generated/location_template_source.hpp")
FUNCTION_NAME = "generate_location"
FUNCTION_UID = 5017992314331459

def walk_events(items):
    for item in items:
        if not isinstance(item, list) or not item:
            continue
        if item[0] == 0:
            yield item
            yield from walk_events(item[7] if len(item) > 7 and isinstance(item[7], list) else [])

def count_branch(branch):
    nodes = writes = creates = 0
    stack = [branch]
    while stack:
        node = stack.pop()
        if not isinstance(node, list) or len(node) < 8 or node[0] != 0:
            continue
        nodes += 1
        for action in node[6] if isinstance(node[6], list) else []:
            if not isinstance(action, list) or len(action) < 2:
                continue
            if action[0] == 525 and action[1] == 199:
                writes += 1
            if action[0] == -1 and action[1] == 94:
                creates += 1
        stack.extend(node[7] if isinstance(node[7], list) else [])
    return nodes, writes, creates

def main():
    root = json.loads(SOURCE.read_text(encoding="utf-8"))
    project = root["project"]
    sheets = {name: entries for name, entries in project[6]}
    candidates = [n for n in walk_events(sheets["Game_events"])
                  if n[4] == FUNCTION_UID and FUNCTION_NAME in json.dumps(n[1], ensure_ascii=False)]
    if len(candidates) != 1:
        raise SystemExit(f"Expected one {FUNCTION_NAME} function event, found {len(candidates)}")
    function = candidates[0]
    event_roots = [x for x in function[7] if isinstance(x, list) and x and x[0] == 0]
    if len(event_roots) != 1:
        raise SystemExit(f"Expected one root event in {FUNCTION_NAME}, found {len(event_roots)}")
    branches = event_roots[0][7]
    rows = []
    for branch in branches:
        if not isinstance(branch, list) or not branch or branch[0] != 0:
            continue
        codes = []
        for condition in branch[5] if isinstance(branch[5], list) else []:
            params = condition[9] if len(condition) > 9 and isinstance(condition[9], list) else []
            last = params[-1] if params else None
            if (condition[0] == 514 and condition[1] == 71 and
                isinstance(last, list) and len(last) > 1 and last[0] == 7 and
                isinstance(last[1], list) and len(last[1]) > 1 and last[1][0] == 0):
                codes.append(int(last[1][1]))
        if not codes:
            raise SystemExit(f"Location branch UID {branch[4]} has no decoded tile-code condition")
        nodes, writes, creates = count_branch(branch)
        rows.append((branch[4], codes, nodes, writes, creates))
    if len(rows) != 62:
        raise SystemExit(f"Expected 62 original location branches, found {len(rows)}")
    lines = ["#pragma once", "#include <cstddef>", "#include <cstdint>",
             "namespace minidayz::stage1 {",
             "struct LocationTemplateSource { std::uint64_t event_uid; std::uint16_t tile_codes[24]; std::uint8_t tile_code_count; std::uint16_t event_nodes; std::uint16_t array_write_actions; std::uint8_t system_create_actions; };",
             f"inline constexpr std::size_t kLocationTemplateSourceCount = {len(rows)};",
             "inline constexpr LocationTemplateSource kLocationTemplateSource[] = {"]
    for uid, codes, nodes, writes, creates in rows:
        if len(codes) > 24:
            raise SystemExit(f"Branch {uid} has {len(codes)} tile codes; descriptor capacity is 24")
        padded = codes + [0] * (24 - len(codes))
        lines.append(f"    {{{uid}ULL, {{{', '.join(map(str, padded))}}}, {len(codes)}, {nodes}, {writes}, {creates}}},")
    lines += ["};", "} // namespace minidayz::stage1", ""]
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_text("\\n".join(lines), encoding="utf-8")
    print(f"Indexed {len(rows)} original generate_location branches -> {OUTPUT}")

if __name__ == "__main__":
    main()
