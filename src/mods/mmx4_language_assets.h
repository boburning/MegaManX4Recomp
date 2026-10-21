#pragma once
#include <cstdint>
#include <vector>
#include <map>
#include <stdexcept>
#include <algorithm>

namespace Mmx4Language {
inline uint32_t u32(const uint8_t* p) {
    return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;
}
inline void put32(std::vector<uint8_t>& v,size_t p,uint32_t w) {
    if(p>v.size() || v.size()-p<4)throw std::runtime_error("ARC write outside buffer");
    for(unsigned i=0;i<4;++i)v[p+i]=uint8_t(w>>(8*i));
}
struct Member {uint32_t type;std::vector<uint8_t> data;};
inline std::vector<Member> unpack(const std::vector<uint8_t>& data) {
    if(data.size()<2048 || data.size()%2048 || u32(data.data()+4)!=data.size())
        throw std::runtime_error("Unexpected X4 ARC layout");
    const uint32_t count=u32(data.data());
    if(!count || count>255)throw std::runtime_error("Invalid X4 ARC member count");
    size_t at=2048;std::vector<Member> result;
    for(unsigned i=0;i<count;++i) {
        const uint32_t type=u32(data.data()+8+i*8),size=u32(data.data()+12+i*8);
        if(!size || at>data.size() || size>data.size()-at)throw std::runtime_error("X4 ARC member outside file");
        result.push_back({type,{data.begin()+at,data.begin()+at+size}});
        at=(at+size+2047)&~size_t(2047);
    }
    if(at!=data.size())throw std::runtime_error("X4 ARC has unclaimed data");
    return result;
}
inline std::vector<uint8_t> pack(const std::vector<Member>& members,const std::vector<uint8_t>& header={}) {
    if(members.empty() || members.size()>255)throw std::runtime_error("Invalid derived ARC");
    if(!header.empty() && header.size()!=2048)throw std::runtime_error("Invalid original ARC header");
    std::vector<uint8_t> data=header.empty()?std::vector<uint8_t>(2048):header;
    put32(data,0,uint32_t(members.size()));
    for(size_t i=0;i<members.size();++i) {
        const auto& m=members[i];
        if(m.data.empty() || m.data.size()>1024*1024 || data.size()>4*1024*1024)
            throw std::runtime_error("Derived X4 ARC exceeds bounds");
        put32(data,8+i*8,m.type);put32(data,12+i*8,uint32_t(m.data.size()));
        data.insert(data.end(),m.data.begin(),m.data.end());data.resize((data.size()+2047)&~size_t(2047));
    }
    put32(data,4,uint32_t(data.size()));return data;
}
inline std::vector<uint8_t>& member(std::vector<Member>& members,uint32_t type) {
    for(auto& m:members)if(m.type==type)return m.data;
    throw std::runtime_error("Missing X4 language asset member");
}
inline std::vector<std::vector<uint8_t>> messages(const std::vector<uint8_t>& data) {
    if(data.size()<2)throw std::runtime_error("Missing X4 message table");
    const uint32_t first=data[0]|uint32_t(data[1])<<8;
    if(!first || first%2 || first>data.size())throw std::runtime_error("Invalid X4 message table");
    std::vector<std::vector<uint8_t>> result;
    for(unsigned i=0;i<first/2;++i) {
        const uint32_t at=data[i*2]|uint32_t(data[i*2+1])<<8;
        if(at<first || at%2 || at>=data.size())throw std::runtime_error("Invalid X4 message offset");
        uint32_t end=at;
        do {
            if(end+2>data.size())throw std::runtime_error("Unterminated X4 message");
            end+=2;
        } while(!(data[end-1]&0x80));
        result.emplace_back(data.begin()+at,data.begin()+end);
    }
    return result;
}
inline std::vector<uint8_t> message_bank(const std::vector<std::vector<uint8_t>>& records) {
    std::vector<uint8_t> data(records.size()*2);
    for(size_t i=0;i<records.size();++i) {
        if(data.size()>65535)throw std::runtime_error("X4 message offset overflow");
        data[i*2]=uint8_t(data.size());data[i*2+1]=uint8_t(data.size()>>8);
        data.insert(data.end(),records[i].begin(),records[i].end());
    }
    // The guest loader advances its next RAM destination by this member size.
    // Retain word alignment even when edited UTF-16 glyph counts are odd.
    data.resize((data.size()+3)&~size_t(3));
    if(data.size()>0xa000)throw std::runtime_error("Translated X4 message bank exceeds reserved capacity");
    return data;
}
} // namespace Mmx4Language
