#pragma once
#include "mmx4_language_assets.h"
#include <array>
#include <cstring>

namespace Mmx4Language {
// One header sector followed by the ten original, unmodified fan STR streams.
// Payload sectors retain the XA subheader and all 2328 following bytes. No
// host video player, subtitle overlay, transcoding, or executable is involved.
struct FmvExtent {uint64_t offset;uint32_t sectors;};
inline std::array<FmvExtent,10> fmv_pack(const uint8_t* data,uint64_t size) {
    if(!data || size<2336 || std::memcmp(data,"X4FMV001",8) || u32(data+8)!=10 || u32(data+12)!=2336)
        throw std::runtime_error("Unsupported X4 fan movie pack; import the original archive again");
    std::array<FmvExtent,10> streams{};uint64_t next=2336;
    for(unsigned i=0;i<streams.size();++i) {
        const auto* p=data+16+i*16;
        const uint64_t offset=uint64_t(u32(p+8))|(uint64_t(u32(p+12))<<32);
        const uint32_t count=u32(p+4);
        if(u32(p)!=i+1 || !count || count>40000 || offset!=next ||
           offset>size || uint64_t(count)*2336>size-offset)
            throw std::runtime_error("Invalid X4 fan movie extent");
        if(std::memcmp(data+offset,data+offset+4,4))
            throw std::runtime_error("Invalid X4 movie XA subheader");
        streams[i]={offset,count};next+=uint64_t(count)*2336;
    }
    if(next!=size)throw std::runtime_error("Trailing data in X4 fan movie pack");
    return streams;
}
} // namespace Mmx4Language
