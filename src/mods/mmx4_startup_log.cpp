#include "mmx4_startup_log.h"
#include "mmx4_diagnostic_paths.h"
#include "execution_profile.h"
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <string>
#include <thread>
#include <fcntl.h>
#include <sys/stat.h>
#ifndef MMX4_STARTUP_BUILD_ID
#define MMX4_STARTUP_BUILD_ID "standalone"
#endif
#ifdef _WIN32
#include <io.h>
#define FD_DUP _dup
#define FD_DUP2 _dup2
#define FD_CLOSE _close
#define FD_READ _read
#define FD_WRITE _write
#else
#include <unistd.h>
#define FD_DUP dup
#define FD_DUP2 dup2
#define FD_CLOSE close
#define FD_READ read
#define FD_WRITE write
#endif

namespace {
void write_all(int descriptor,const char *data,size_t bytes) {
    while(bytes && descriptor>=0) {
        auto written=FD_WRITE(descriptor,data,(unsigned)bytes);
        if(written<0 && errno==EINTR)continue;
        if(written<=0)break;
        data+=written;bytes-=(size_t)written;
    }
}
struct StartupLog {
    int original=-1,file=-1,reader=-1;
    bool direct=false;
    std::string path;
    std::thread worker;
    ~StartupLog() {end();}
    void end() {
        if(direct) {std::fflush(stderr);return;}
        if(original>=0) {
            std::fflush(stderr);
            FD_DUP2(original,2); // close the last pipe writer, then drain it
            if(worker.joinable())worker.join();
            FD_CLOSE(original);original=-1;
        }
        if(reader>=0) {FD_CLOSE(reader);reader=-1;}
        if(file>=0) {FD_CLOSE(file);file=-1;}
    }
    int begin(const char *root) {
        if(worker.joinable() || direct)return 1;
        try {
            auto application=root?std::filesystem::u8path(root):mmx4_diagnostic_application_directory();
            auto directory=application/"saves"/"diagnostics";
            std::filesystem::create_directories(directory);
            auto stamp=std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
#ifdef _WIN32
            unsigned pid=GetCurrentProcessId();
#else
            unsigned pid=(unsigned)getpid();
#endif
            auto destination=directory/("startup-"+std::to_string(stamp)+"-"+std::to_string(pid)+".log");
            original=FD_DUP(2);
#ifdef _WIN32
            file=_wopen(destination.c_str(),_O_WRONLY|_O_CREAT|_O_EXCL|_O_BINARY,_S_IREAD|_S_IWRITE);
#else
            file=open(destination.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0666);
#endif
            if(file<0)throw std::runtime_error("cannot open startup log");
            path=destination.u8string();
            if(original<0) {
                // A Windows GUI launch may have no stderr. Give its FILE a
                // real destination instead of dropping all startup errors.
                FD_CLOSE(file);file=-1;
#ifdef _WIN32
                if(!_wfreopen(destination.c_str(),L"ab",stderr))
#else
                if(!std::freopen(destination.c_str(),"ab",stderr))
#endif
                    throw std::runtime_error("cannot attach startup log");
                std::setvbuf(stderr,nullptr,_IONBF,0);direct=true;
            }else {
                int pipe_fds[2];
#ifdef _WIN32
                if(_pipe(pipe_fds,65536,_O_BINARY|_O_NOINHERIT))
#else
                if(pipe(pipe_fds))
#endif
                    throw std::runtime_error("cannot create startup log pipe");
                reader=pipe_fds[0];
                int result=FD_DUP2(pipe_fds[1],2);FD_CLOSE(pipe_fds[1]);
                if(result<0)throw std::runtime_error("cannot attach startup log pipe");
                worker=std::thread([this] {
                    char buffer[4096];
                    for(;;) {
                        auto bytes=FD_READ(reader,buffer,sizeof buffer);
                        if(bytes<0 && errno==EINTR)continue;
                        if(bytes<=0)break;
                        write_all(file,buffer,(size_t)bytes);
                        write_all(original,buffer,(size_t)bytes);
                    }
                });
            }
            std::ofstream pointer(application/"startup-log-location.txt",std::ios::trunc);
            pointer<<path<<'\n';
            std::fprintf(stderr,"mmx4.startup: log=%s pid=%u execution=%s coop=%s\n",
                         path.c_str(),pid,PSX_EXECUTION_ID,MMX4_STARTUP_BUILD_ID);
            std::fflush(stderr);
            return 1;
        }catch(const std::exception& error) {
            end();
            std::fprintf(stderr,"mmx4.startup: logging unavailable: %s\n",error.what());
            return 0;
        }
    }
};
StartupLog& startup() {static StartupLog log;return log;}
}
extern "C" int mmx4_startup_log_begin(const char *root) {return startup().begin(root);}
extern "C" const char *mmx4_startup_log_path(void) {return startup().path.c_str();}
extern "C" void mmx4_startup_log_end(void) {startup().end();}
#ifndef MMX4_STARTUP_LOG_TEST
#include "mod_plugins.h"
PSX_MOD_CONSTRUCTOR(mmx4_register_startup_log) {mmx4_startup_log_begin(nullptr);}
#endif
