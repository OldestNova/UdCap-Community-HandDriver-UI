#include "SteamVRInstallation.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <chrono>
#include <thread>
#include <algorithm>
#include <mutex>
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#else
#include <cstdlib>
#endif

namespace SteamVRInstallation {
namespace fs = std::filesystem;
std::string utf8(const fs::path &path) {
    const auto s=path.u8string(); return {s.begin(),s.end()};
}
namespace {
bool file(const fs::path &p) { std::error_code e; return fs::is_regular_file(p,e); }
std::string driverName(const fs::path &p) {
    try { std::ifstream in(p/"driver.vrdrivermanifest"); return nlohmann::json::parse(in).value("name",""); }
    catch(...) { return {}; }
}
bool samePath(const fs::path &a,const fs::path &b) {
    std::error_code e; if(fs::equivalent(a,b,e)) return true;
    auto x=a.lexically_normal().generic_wstring(), y=b.lexically_normal().generic_wstring();
    while(x.size()>1 && x.back()==L'/') x.pop_back();
    while(y.size()>1 && y.back()==L'/') y.pop_back();
#ifdef _WIN32
    return CompareStringOrdinal(x.c_str(),int(x.size()),y.c_str(),int(y.size()),TRUE)==CSTR_EQUAL;
#else
    return x==y;
#endif
}
#ifdef _WIN32
fs::path env(const wchar_t *key) {
    wchar_t text[32768]{}; const auto n=GetEnvironmentVariableW(key,text,32768);
    return n && n<32768?fs::path(text):fs::path{};
}
std::wstring quote(const std::wstring &s) {
    std::wstring result=L"\""; int slashes=0;
    for(wchar_t c:s) {
        if(c==L'\\') {++slashes;continue;}
        result.append(c==L'"'?slashes*2+1:slashes,L'\\'); slashes=0; result+=c;
    }
    result.append(slashes*2,L'\\'); result+=L'"'; return result;
}
#endif
}
Status inspect(const fs::path &registry,const fs::path &bundle,const fs::path &fallback) {
    Status s; s.registryFile=registry; s.bundle=bundle;
    std::vector<fs::path> runtimes;
    try {
        if(file(registry)) {
            std::ifstream in(registry); const auto j=nlohmann::json::parse(in);
            if(j.contains("runtime") && j["runtime"].is_array())
                for(const auto &r:j["runtime"]) if(r.is_string()) runtimes.push_back(fs::u8path(r.get<std::string>()));
            if(j.contains("external_drivers") && j["external_drivers"].is_array())
                for(const auto &r:j["external_drivers"]) if(r.is_string()) {
                    auto path=fs::u8path(r.get<std::string>());
                    const auto name=driverName(path);
                    s.officialDriverPresent |= name=="udcap";
                    // Also allow unregistering this application's path if its DLL was removed.
                    if(name=="udcapc" || samePath(path,bundle))
                        if(std::none_of(s.installedPaths.begin(),s.installedPaths.end(),[&](const auto &p){return samePath(p,path);}))
                            s.installedPaths.push_back(path);
                }
        }
    } catch(const std::exception &e) { s.error=e.what(); }
    if(!fallback.empty()) runtimes.push_back(fallback);
#ifdef _WIN32
    constexpr auto toolPath="bin/win64/vrpathreg.exe";
    constexpr auto driverPath="bin/win64/driver_udcapc.dll";
#else
    constexpr auto toolPath="bin/linux64/vrpathreg";
    constexpr auto driverPath="bin/linux64/driver_udcapc.so";
#endif
    for(const auto &r:runtimes) if(file(r/toolPath)) {s.tool=r/toolPath;break;}
    s.bundleAvailable=driverName(bundle)=="udcapc" && file(bundle/driverPath) &&
        file(bundle/"resources/input/udcap_profile.json");
    return s;
}
Status detect() {
#ifdef _WIN32
    wchar_t exe[32768]{}; const auto length=GetModuleFileNameW(nullptr,exe,32768);
    if(!length || length>=32768) throw std::runtime_error("Cannot locate application");
    auto registry=env(L"VR_PATHREG_OVERRIDE");
    if(registry.empty()) registry=env(L"LOCALAPPDATA")/"openvr/openvrpaths.vrpath";
    auto fallback=env(L"VR_OVERRIDE");
    if(fallback.empty()) for(auto view:{RRF_SUBKEY_WOW6464KEY,RRF_SUBKEY_WOW6432KEY}) {
        wchar_t value[32768]{}; DWORD bytes=sizeof(value);
        if(RegGetValueW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Steam App 250820",
            L"InstallLocation",RRF_RT_REG_SZ|view,nullptr,value,&bytes)==ERROR_SUCCESS) {fallback=value;break;}
    }
    auto base=fs::path(exe).parent_path();
    auto bundle=base/"steamvr/udcapc";
    // Multi-config CMake builds keep the bundle next to the configuration directories.
    if(!fs::exists(bundle) && fs::exists(base.parent_path()/"steamvr/udcapc")) bundle=base.parent_path()/"steamvr/udcapc";
    return inspect(registry,bundle,fallback);
#else
    Status s; s.error="Driver installation from this application is currently supported on Windows."; return s;
#endif
}
CommandResult run(const fs::path &executable,const std::vector<std::string> &args) {
#ifdef _WIN32
    // Explicit executable and individually quoted arguments; no command shell.
    SECURITY_ATTRIBUTES sa{sizeof(sa),nullptr,TRUE}; HANDLE read=nullptr,write=nullptr;
    if(!CreatePipe(&read,&write,&sa,0)) return {-1,"Cannot create process pipe"};
    SetHandleInformation(read,HANDLE_FLAG_INHERIT,0);
    std::wstring command=quote(executable.wstring());
    for(const auto &arg:args) command+=L" "+quote(fs::u8path(arg).wstring());
    STARTUPINFOW startup{}; startup.cb=sizeof(startup); startup.dwFlags=STARTF_USESTDHANDLES;
    startup.hStdOutput=write; startup.hStdError=write;
    startup.hStdInput=CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_EXISTING,0,nullptr);
    PROCESS_INFORMATION process{};
    const bool started=CreateProcessW(executable.c_str(),command.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&process);
    const auto startError=GetLastError(); CloseHandle(write);
    if(startup.hStdInput!=INVALID_HANDLE_VALUE) CloseHandle(startup.hStdInput);
    if(!started) {CloseHandle(read);return {-1,"CreateProcess failed: "+std::to_string(startError)};}
    CloseHandle(process.hThread); CommandResult result;
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(15);
    bool done=false;
    do {
        DWORD available=0;
        while(PeekNamedPipe(read,nullptr,0,nullptr,&available,nullptr) && available) {
            char buffer[4096]; DWORD count=0;
            if(!ReadFile(read,buffer,std::min<DWORD>(available,sizeof(buffer)),&count,nullptr) || !count) break;
            if(result.output.size()<65536) result.output.append(buffer,std::min<std::size_t>(count,65536-result.output.size()));
        }
        if(done) break;
        done=WaitForSingleObject(process.hProcess,20)==WAIT_OBJECT_0;
        if(!done && std::chrono::steady_clock::now()>deadline) {
            TerminateProcess(process.hProcess,1); WaitForSingleObject(process.hProcess,1000);
            result.output="vrpathreg timed out"; break;
        }
    } while(true);
    if(done) {DWORD code;GetExitCodeProcess(process.hProcess,&code);result.exitCode=int(code);}
    CloseHandle(read);CloseHandle(process.hProcess);return result;
#else
    return {-1,"Automatic registration is only available on Windows"};
#endif
}
Status change(const Status &previous,bool install,const Runner &runner) {
    static std::mutex registrationMutex;
    std::lock_guard guard(registrationMutex);
    auto s=inspect(previous.registryFile,previous.bundle,previous.tool.parent_path().parent_path().parent_path());
    if(!s.error.empty()) return s;
    if(s.tool.empty()) {s.error="SteamVR was not found";return s;}
    if(install && !s.bundleAvailable) {s.error="The udcapc driver bundle is incomplete";return s;}
    const auto query=runner(s.tool,{"finddriver","udcapc"});
    if(query.exitCode!=0 && query.exitCode!=1) {s.error="Cannot query udcapc: "+query.output;return s;}
    if(install) {
        if(!s.installedPaths.empty() || query.exitCode==0) {
            if(s.installedPaths.size()==1 && samePath(s.installedPaths[0],s.bundle)) return s;
            s.error="udcapc is already registered elsewhere. Uninstall it before installing this copy.";return s;
        }
        const auto result=runner(s.tool,{"adddriver",utf8(s.bundle)});
        if(result.exitCode!=0) {s.error="Install failed: "+result.output;return s;}
    } else {
        if(s.installedPaths.empty() && query.exitCode==0) {s.error="Cannot resolve the registered udcapc path";return s;}
        for(const auto &path:s.installedPaths) {
            const auto result=runner(s.tool,{"removedriver",utf8(path)});
            if(result.exitCode!=0) {s.error="Uninstall failed: "+result.output;return s;}
        }
    }
    auto after=inspect(s.registryFile,s.bundle,s.tool.parent_path().parent_path().parent_path());
    const auto verify=runner(s.tool,{"finddriver","udcapc"});
    if((install && (verify.exitCode!=0 || after.installedPaths.empty())) ||
        (!install && (verify.exitCode!=1 || !after.installedPaths.empty())))
        after.error="SteamVR driver registration could not be verified";
    return after;
}
}
