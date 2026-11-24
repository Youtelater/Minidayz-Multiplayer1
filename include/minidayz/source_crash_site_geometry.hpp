#pragma once
#include <cstdint>
#include <cstddef>
#include <random>
#include <vector>

namespace minidayz::world_source {
// Root data.deobfuscated.js Sprite frames, source Game_events:
// generate_helicrashes (924737797383123),
// create_map_location 52/1/10/2/8 (427158362727888,
// 897613305483386,147519896698226),
// roadside hammer crashes (904312442718003 et al.).
struct CrashSpriteGeometry {
    std::uint16_t object{};
    std::uint32_t frame{};
    float width{},height{},origin_x{},origin_y{};
};
struct CrashImagePoint {
    std::uint8_t index{};
    float x{},y{}; // Original normalized imagepoint coordinates.
};
inline constexpr CrashSpriteGeometry kHeliAnchor{344,0,50.0f,50.0f,0.5f,0.5f};
inline constexpr CrashSpriteGeometry kHammerAnchor{434,0,1.0f,1.0f,1.0f,1.0f};
inline constexpr CrashSpriteGeometry kHeliCrash{246,0,240.0f,186.0f,
    0.71666664f,0.46774194f};
inline constexpr CrashSpriteGeometry kHeliSolid{348,19,240.0f,186.0f,
    0.71666664f,0.46774194f};
inline constexpr CrashSpriteGeometry kHammerCrashStanding{441,0,
    103.0f,56.0f,0.49514562f,0.5f};
inline constexpr CrashSpriteGeometry kHammerCrashFallen{441,1,
    71.0f,87.0f,0.49295774f,0.50574714f};
inline constexpr CrashSpriteGeometry kHammerSolid{348,26,1.0f,1.0f,0.0f,0.0f};
inline constexpr CrashSpriteGeometry kHeliCounter{405,0,4.0f,4.0f,0.5f,0.5f};
inline constexpr CrashSpriteGeometry kHammerCounter{990,0,4.0f,4.0f,0.5f,0.5f};
inline constexpr CrashSpriteGeometry kCrashLoot{218,0,1.0f,1.0f,0.0f,1.0f};
inline constexpr CrashSpriteGeometry kCrashZed{216,13,15.0f,15.0f,0.6f,0.6f};

inline constexpr CrashImagePoint kHeliLootPoints[3] = {
    {3,0.50833333f,0.93010753f},
    {4,0.78333336f,0.85483873f},
    {5,0.98750001f,0.66666669f}
};
inline constexpr CrashImagePoint kHammerStandingLootPoints[3] = {
    {3,0.88349515f,1.25f},
    {2,0.44660193f,1.28571427f},
    {1,-0.05825243f,1.14285719f}
};
inline constexpr CrashImagePoint kHammerFallenLootPoints[3] = {
    {3,1.12676060f,0.87356323f},
    {2,1.05633807f,0.33333334f},
    {1,-0.19718310f,0.52873564f}
};
inline constexpr CrashImagePoint kHammerStandingWallPoints[2] = {
    {1,0.07766990f,0.42857143f},
    {2,0.89320385f,0.875f}
};
inline constexpr CrashImagePoint kHammerFallenWallPoints[2] = {
    {1,0.08450704f,0.13793103f},
    {2,0.67605633f,0.91954023f}
};

// Root PickRandom 116073859892425 / 311546240563344 happens BEFORE
// the Sprite514.var9==0 guard in the same event. While repeats after
// a rejected pick, consuming another random draw from the full set.
// Stop defensively if a malformed native map has no unused candidate;
// the browser's while event would otherwise never terminate.
inline std::size_t source_pick_unused_crash_marker(
    const std::vector<std::size_t>& candidates,
    std::vector<bool>& used,std::mt19937& rng) noexcept {
    bool available=false;
    for(const auto i:candidates)
        if(i<used.size() && !used[i]){available=true;break;}
    if(!available)return used.size();
    for(;;){
        const auto index=static_cast<std::size_t>(
            (static_cast<std::uint64_t>(rng())*candidates.size())>>32);
        const auto picked=candidates[index];
        if(picked<used.size() && !used[picked]){
            used[picked]=true;
            return picked;
        }
    }
}
constexpr std::uint8_t source_helicrash_quota(std::uint8_t level) noexcept {
    return level==0?0:level==1?2:level==2?3:level==3?4:5;
}
constexpr std::uint8_t source_hammercrash_quota(std::uint8_t level) noexcept {
    return level==0?1:level==1?3:level==2?5:level==3?6:8;
}
} // namespace minidayz::world_source
