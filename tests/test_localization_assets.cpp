#include "mod_plugins.h"
#include "mod_media.h"
#include "iso_reader.h"
#include "psx_sha256.h"
#include "mmx4_language_assets.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <cstring>
using namespace PSXRecompV4;
using namespace Mmx4Language;
namespace fs=std::filesystem;
using Call=void(*)(void);
static auto& activation() {static std::map<std::string,Call> c;return c;}
static auto& resetters() {static std::map<std::string,Call> c;return c;}
static auto& ticks() {static std::map<std::string,Call> c;return c;}
static std::shared_ptr<const std::vector<uint8_t>> donor_bytes;
static std::vector<uint8_t> catalog,ram(2*1024*1024),exe;
static std::map<uint32_t,uint8_t> aperture;
static std::map<std::string,std::vector<uint8_t>> replacements;
static std::string catalog_path;
static PS1::ISOReader original;
static unsigned options,appended;static uint32_t next_lba,next_guest;
static void check(bool ok,const std::string& text) {if(!ok)throw std::runtime_error(text);}
extern "C" {
int psx_mod_register_activation_plugin(const char* id,Call cb) {activation()[id]=cb;return 1;}
int psx_mod_register_session_reset_plugin(const char* id,Call cb) {resetters()[id]=cb;return 1;}
int psx_mod_register_vblank_plugin(const char* id,Call cb) {ticks()[id]=cb;return 1;}
int psx_mod_register_savestate_plugin(const char*,Call) {return 1;}
int psx_mod_register_function_entry_plugin(const char*,uint32_t,PSXModFunctionEntryCallback) {return 1;}
void psx_mod_counter_add(const char*,uint32_t) {}
void psx_fatal_halt(const char* why) {throw std::runtime_error(why);}
int psx_mod_option_value(const char*,const char*,const char* id,char* out,uint32_t n) {
    unsigned bit=!std::strcmp(id,"fmvs")?1:!std::strcmp(id,"voices")?2:4;
    std::snprintf(out,n,"%s",options&bit?"true":"false");return 1;
}
int psx_mod_current_resource_path(const char*,char* out,uint32_t n) {std::snprintf(out,n,"%s",catalog_path.c_str());return 1;}
int psx_mod_current_resource_bytes(const char* id,const uint8_t** out,uint64_t* size) {
    if(!std::strcmp(id,"catalog")) {*out=catalog.data();*size=catalog.size();return 1;}
    *out=donor_bytes->data();*size=donor_bytes->size();return 1;
}
int psx_mod_read_disc_file(const char* path,void* out,uint32_t capacity,uint32_t* size) {
    auto found=replacements.find(path);
    if(found!=replacements.end()) {
        *size=uint32_t(found->second.size());
        if(!out)return capacity==0;
        if(capacity<*size)return 0;
        std::memcpy(out,found->second.data(),*size);return 1;
    }
    PS1::ISOFileEntry file;
    if(!original.FindFile(path,file))return 0;
    *size=file.size;
    if(!out)return capacity==0;
    return capacity>=*size && original.ReadFile(path,static_cast<uint8_t*>(out),capacity)==*size;
}
int psx_mod_append_disc_extent(const char*,uint64_t offset,uint32_t count,uint32_t* lba) {
    check(offset<=donor_bytes->size() && uint64_t(count)*2336<=donor_bytes->size()-offset,"donor stream bounds");
    check(uint64_t(next_lba)+count+150<=450000,"native CD timeline capacity");
    *lba=next_lba;next_lba+=count;++appended;return 1;
}
int psx_mod_replace_disc_file(const char* path,const void* data,uint32_t size,uint32_t* lba) {
    const auto* p=static_cast<const uint8_t*>(data);replacements[path]={p,p+size};
    *lba=next_lba;next_lba+=(size+2047)/2048;return 1;
}
uint32_t psx_mod_alloc_guest_memory(uint32_t size,uint32_t alignment) {
    next_guest=(next_guest+alignment-1)&~(alignment-1);const auto p=next_guest;next_guest+=size;return p;
}
static uint8_t read(uint32_t address) {return address<0x80200000?ram[address&0x1fffff]:aperture[address];}
uint32_t psx_mod_read_word(uint32_t address) {
    uint32_t w=0;for(unsigned i=0;i<4;++i)w|=uint32_t(read(address+i))<<(i*8);return w;
}
void psx_mod_write_byte(uint32_t address,uint8_t v) {
    if(address<0x80200000)ram[address&0x1fffff]=v;else aperture[address]=v;
}
void psx_mod_write_word(uint32_t address,uint32_t w) {for(unsigned i=0;i<4;++i)psx_mod_write_byte(address+i,uint8_t(w>>(i*8)));}
}
static std::vector<uint8_t> read_file(const std::string& path) {
    uint32_t size=0;check(psx_mod_read_disc_file(path.c_str(),nullptr,0,&size),"file query "+path);
    std::vector<uint8_t> data(size);check(psx_mod_read_disc_file(path.c_str(),data.data(),size,&size),"file read "+path);return data;
}
int main(int argc,char** argv) {
    try {
        std::vector<Member> fixture{{9,{1,2,3}},{0x10001,std::vector<uint8_t>(32768,0)}};
        const auto built=pack(fixture);check(unpack(built)[0].data==fixture[0].data,"ARC round trip");
        std::vector<uint8_t> opaque_header(built.begin(),built.begin()+2048);opaque_header[2047]=0xa7;
        check(pack(fixture,opaque_header)[2047]==0xa7,"opaque original ARC header preserved");
        const std::vector<std::vector<uint8_t>> short_records{{0x4a,0,0x4c,0xa0},{0x4b,0xa0},{0x4c,0xa0}};
        const auto aligned_bank=message_bank(short_records);
        check(aligned_bank.size()%4==0 && messages(aligned_bank)==short_records,"edited native message banks retain word alignment");
        auto bad=built;bad[4]^=1;bool refused=false;
        try{unpack(bad);}catch(...){refused=true;}check(refused,"malformed ARC rejected");
        if(argc!=4) {std::cout<<"PASS: language archive contracts\n";return 0;}
        check(original.Open(argv[1]),"open owner US disc");catalog_path=argv[3];
        std::ifstream input(catalog_path,std::ios::binary);catalog={std::istreambuf_iterator<char>(input),{}};
        std::string error;
        check(load_mod_media(argv[2],"psx-mode2",589683488,
            "829185314c9040d74f66c5babdb08a6fa58239d299afae11fd396f4c243874d7",donor_bytes,&error),error);
        ModMode2Disc donor(donor_bytes->data(),donor_bytes->size());
        std::vector<uint8_t> jp_load;check(donor.read("ARC/LOAD.ARC",jp_load),"verified donor font atlas");
        auto jp=unpack(jp_load);
        const auto us_load=unpack(read_file("ARC/LOAD_U.ARC"));
        auto original_us=us_load;
        const auto us_logo=member(original_us,0x10008),us_font=member(original_us,0x10001);
        auto us_title=unpack(read_file("ARC/ST0E_U0.ARC"));
        std::vector<uint8_t> jp_title_bytes;check(donor.read("ARC/ST0E_00.ARC",jp_title_bytes),"Japanese title scene");
        auto jp_title=unpack(jp_title_bytes);
        exe=read_file("SLUS_005.61");
        const std::array<std::string,3> names{"ARC/PL00_U.ARC","ARC/PL01_U.ARC","ARC/PL02_U.ARC"};
        std::array<std::vector<std::vector<uint8_t>>,3> source_records;
        for(unsigned i=0;i<3;++i) {auto arc=unpack(read_file(names[i]));source_records[i]=messages(member(arc,0x15));}
        for(options=0;options<16;++options) {
            for(const auto& r:resetters())r.second();
            replacements.clear();aperture.clear();appended=0;next_lba=original.GetSectorCount();next_guest=0x8f100000;
            std::fill(ram.begin(),ram.end(),0);std::copy(exe.begin()+0x800,exe.end(),ram.begin()+0x10000);
            if(options&7)activation().at("mmx4.japanese-media")();
            if(options&8)activation().at("mmx4.translation")();
            for(const auto& cb:ticks())cb.second();
            check(appended==((options&1)?11u:0u)+((options&2)?6u:0u),"independent FMV/voice selection");
            auto load=unpack(read_file("ARC/LOAD_U.ARC"));
            check(member(load,0x10008)==((options&4)?member(jp,0x10008):us_logo),"independent Rockman title artwork");
            auto title=unpack(read_file("ARC/ST0E_U0.ARC"));
            for(uint32_t type:{0x10002u,0x12u,0xdu,0x10000u,0u,1u,0xbu})
                check(member(title,type)==member((options&4)?jp_title:us_title,type),"main title scene art and layout are independent");
            check(member(title,0x20000)==member((options&2)?jp_title:us_title,0x20000),"title audio belongs to voices option");
            const auto& font=member(load,0x10001);
            check((font==us_font)==!(options&8),"only translation changes native font atlas");
            for(unsigned i=0;i<3;++i) {
                auto arc=unpack(read_file(names[i]));auto records=messages(member(arc,0x15));
                check(records.size()==source_records[i].size(),"stable native message IDs");
                check(records.back()==source_records[i].back(),"non-text sentinel preserved");
                check((records[0]==source_records[i][0])==!(options&8),"Spanish applies independently of Japanese media");
                if(options&8)for(size_t n=0;n+1<records.size();++n)
                    check((records[n].back()&0xa0)==0xa0,"translated native message terminator");
            }
            // Publication is idempotent and preserves unrelated executable data.
            const auto before=ram;for(const auto& cb:ticks())cb.second();check(ram==before,"idempotent language tables");
            std::cout<<"PASS: owner-disc combination "<<options<<" streams="<<appended<<" archives="<<replacements.size()<<'\n';
        }
        std::cout<<"PASS: all 16 native asset combinations; actual guest playback remains a separate check\n";
        return 0;
    }catch(const std::exception& e) {std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
