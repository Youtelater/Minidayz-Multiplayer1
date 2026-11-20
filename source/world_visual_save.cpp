#include "minidayz/world_visual_save.hpp"
#include <array>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <vector>

namespace minidayz::persistence {
namespace {
constexpr std::uint32_t kMagic=0x5357444du; // "MDWS" little-endian
constexpr std::uint32_t kVersion=1u;
constexpr std::uint32_t kMaxChanges=100000u;
constexpr std::uint32_t kFnvBasis=2166136261u;
constexpr std::uint32_t kFnvPrime=16777619u;

void pack32(unsigned char* b, std::uint32_t n) noexcept {
    for (int i=0;i<4;++i) b[i]=static_cast<unsigned char>(n>>(i*8));
}
std::uint32_t unpack32(const unsigned char* b) noexcept {
    return static_cast<std::uint32_t>(b[0]) |
        (static_cast<std::uint32_t>(b[1])<<8) |
        (static_cast<std::uint32_t>(b[2])<<16) |
        (static_cast<std::uint32_t>(b[3])<<24);
}
void pack64(unsigned char* b, std::uint64_t n) noexcept {
    pack32(b,static_cast<std::uint32_t>(n));
    pack32(b+4,static_cast<std::uint32_t>(n>>32));
}
std::uint64_t unpack64(const unsigned char* b) noexcept {
    return static_cast<std::uint64_t>(unpack32(b)) |
        (static_cast<std::uint64_t>(unpack32(b+4))<<32);
}
bool write32(std::FILE* f,std::uint32_t n) noexcept {
    unsigned char bytes[4]{};
    pack32(bytes,n);
    return std::fwrite(bytes,1,sizeof(bytes),f)==sizeof(bytes);
}
bool read32(std::FILE* f,std::uint32_t& n) noexcept {
    unsigned char bytes[4]{};
    if(std::fread(bytes,1,sizeof(bytes),f)!=sizeof(bytes))return false;
    n=unpack32(bytes);
    return true;
}
std::uint32_t extend_hash(std::uint32_t hash,
                          const unsigned char* bytes,
                          std::size_t count) noexcept {
    for(std::size_t i=0;i<count;++i)
        hash=(hash^bytes[i])*kFnvPrime;
    return hash;
}
std::array<unsigned char,20> encode(const WorldSpriteState& item) noexcept {
    std::array<unsigned char,20> b{};
    pack64(b.data(),item.runtime_uid);
    pack32(b.data()+8,static_cast<std::uint32_t>(item.object) |
                        (static_cast<std::uint32_t>(item.animation)<<16));
    pack32(b.data()+12,item.frame);
    static_assert(sizeof(float)==4,"World visual save requires IEEE-754 float32");
    std::uint32_t opacity_bits{};
    std::memcpy(&opacity_bits,&item.opacity,4);
    pack32(b.data()+16,opacity_bits);
    return b;
}
WorldSpriteState decode(const std::array<unsigned char,20>& b) noexcept {
    WorldSpriteState item{};
    item.runtime_uid=unpack64(b.data());
    const auto combined=unpack32(b.data()+8);
    item.object=static_cast<std::uint16_t>(combined & 0xffffu);
    item.animation=static_cast<std::uint16_t>(combined>>16);
    item.frame=unpack32(b.data()+12);
    const auto bits=unpack32(b.data()+16);
    std::memcpy(&item.opacity,&bits,4);
    return item;
}
}

bool write_world_visual_state(const WorldSystem& world, std::uint8_t level,
                              const char* target,const char* temporary) noexcept {
    if (!world.generation_complete() || !target || !temporary ||
        !target[0] || !temporary[0] || std::strcmp(target,temporary)==0)
        return false;
    const auto changes=world.capture_sprite_states();
    if(changes.size()>kMaxChanges || changes.size()>world.placement_count())
        return false;
    auto* file=std::fopen(temporary,"wb");
    if(!file)return false;
    bool ok=write32(file,kMagic) && write32(file,kVersion) &&
        write32(file,world.seed()) && write32(file,level) &&
        write32(file,static_cast<std::uint32_t>(changes.size()));
    std::uint32_t checksum=kFnvBasis;
    for(const auto& state:changes) {
        const auto row=encode(state);
        checksum=extend_hash(checksum,row.data(),row.size());
        if(ok)ok=std::fwrite(row.data(),1,row.size(),file)==row.size();
    }
    if(ok)ok=write32(file,checksum);
    if(std::fclose(file)!=0)ok=false;
    if(!ok || std::rename(temporary,target)!=0) {
        std::remove(temporary);
        return false;
    }
    return true;
}

bool read_world_visual_state(WorldSystem& world,std::uint8_t level,
                             const char* path) noexcept {
    if(!world.generation_complete() || !path || !path[0]) return false;
    auto* file=std::fopen(path,"rb");
    if(!file)return false;
    std::uint32_t magic{},version{},seed{},saved_level{},count{};
    bool ok=read32(file,magic)&&read32(file,version)&&
        read32(file,seed)&&read32(file,saved_level)&&read32(file,count);
    if(!ok || magic!=kMagic || version!=kVersion || seed!=world.seed() ||
       saved_level!=level || count>kMaxChanges ||
       count>world.placement_count()) {
        std::fclose(file);
        return false;
    }
    std::vector<WorldSpriteState> changes;
    changes.reserve(count);
    auto hash=kFnvBasis;
    for(std::uint32_t i=0;i<count;++i) {
        std::array<unsigned char,20> row{};
        if(std::fread(row.data(),1,row.size(),file)!=row.size()) {
            ok=false;
            break;
        }
        hash=extend_hash(hash,row.data(),row.size());
        changes.push_back(decode(row));
    }
    std::uint32_t expected{};
    if(ok)ok=read32(file,expected)&&expected==hash &&
              std::fgetc(file)==EOF && !std::ferror(file);
    if(std::fclose(file)!=0)ok=false;
    return ok && world.restore_sprite_states(changes);
}

} // namespace minidayz::persistence
