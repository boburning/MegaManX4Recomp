#include "mmx4_startup_log.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#ifdef _WIN32
#include <io.h>
#define FD_CLOSE _close
#else
#include <unistd.h>
#define FD_CLOSE close
#endif
#define CHECK(value) do {if(!(value)) {std::printf("line%d: %s\n",__LINE__,#value);return 1;}} while(0)
int main(int argc,char **argv) {
    CHECK(argc>=2);
    if(argc>2) {std::fflush(stderr);FD_CLOSE(2);}
    CHECK(mmx4_startup_log_begin(argv[1]));
    std::string expected="startup failure fixture\n";
    for(unsigned i=0;i<5;++i)expected+=std::string(4096,(char)('a'+i))+"\n";
    CHECK(std::fwrite(expected.data(),1,expected.size(),stderr)==expected.size());
    std::string path=mmx4_startup_log_path();mmx4_startup_log_end();
    std::ifstream input(std::filesystem::u8path(path),std::ios::binary);
    std::string contents((std::istreambuf_iterator<char>(input)),{});
    CHECK(contents.find(expected)!=std::string::npos);
    CHECK(contents.find("mmx4.startup:")!=std::string::npos);
    std::ifstream pointer(std::filesystem::u8path(argv[1])/"startup-log-location.txt");
    std::string pointed;std::getline(pointer,pointed);CHECK(pointed==path);
    return 0;
}
