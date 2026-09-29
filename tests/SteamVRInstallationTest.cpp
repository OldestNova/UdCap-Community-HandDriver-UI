#include "components/SteamVRInstallation.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <chrono>
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif
namespace fs=std::filesystem;
using namespace SteamVRInstallation;
void require(bool ok,const char *message) {if(!ok) throw std::runtime_error(message);}
void put(const fs::path &p,const std::string &text) {fs::create_directories(p.parent_path());std::ofstream(p)<<text;}
int main(int argc,char **argv) try {
    if(argc>1 && std::string(argv[1])=="--probe-args") {for(int i=2;i<argc;++i) std::cout<<argv[i]<<'\n';return 0;}
    if(argc>1 && std::string(argv[1])=="--detect") {
        const auto s=detect();std::cout<<utf8(s.tool)<<'\n'<<utf8(s.bundle)<<'\n'<<s.bundleAvailable<<' '<<s.installedPaths.size()<<' '<<s.officialDriverPresent<<'\n'<<s.error<<'\n';return s.error.empty()?0:1;
    }
    const auto root=fs::temp_directory_path()/("udcapc-install-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(root);
    struct Cleanup {fs::path p;~Cleanup(){std::error_code e;fs::remove_all(p,e);}} cleanup{root};
    const auto runtime=root/"Steam VR",bundle=root/"Application with spaces/steamvr/udcapc",official=root/"official/udcap";
    const auto registry=root/"openvrpaths.vrpath";
    put(runtime/"bin/win64/vrpathreg.exe","");
    put(bundle/"driver.vrdrivermanifest",R"({"name":"udcapc"})");
    put(bundle/"bin/win64/driver_udcapc.dll","");put(bundle/"resources/input/udcap_profile.json","{}");
    put(official/"driver.vrdrivermanifest",R"({"name":"udcap"})");
    nlohmann::json config{{"runtime",{utf8(runtime)}},{"external_drivers",{utf8(official)}}};
    const auto save=[&](){put(registry,config.dump());};save();
    std::vector<std::string> commands;
    const Runner fake=[&](const fs::path &exe,const std::vector<std::string> &args)->CommandResult {
        require(exe==runtime/"bin/win64/vrpathreg.exe","Wrong vrpathreg executable");
        require(args.size()==2,"Broken argument boundary");commands.push_back(args[0]);
        if(args[0]=="finddriver") {
            require(args[1]=="udcapc","Query touched official driver");
            return {config["external_drivers"].size()>1?0:1,""};
        }
        require(args[1]!=utf8(official),"Touched official registration");
        if(args[0]=="adddriver") config["external_drivers"].push_back(args[1]);
        else if(args[0]=="removedriver") {
            auto &paths=config["external_drivers"];
            for(auto i=paths.begin();i!=paths.end();) {if(*i==args[1]) i=paths.erase(i);else ++i;}
        } else throw std::runtime_error("Unexpected command");
        save();return {0,""};
    };
    auto s=inspect(registry,bundle);
    require(s.bundleAvailable && s.officialDriverPresent && s.installedPaths.empty(),"Detection failed");
    s=change(s,true,fake);require(s.error.empty() && s.installedPaths.size()==1,"Install failed");
    require(commands.front()=="finddriver","Did not query before install");
    s=change(s,true,fake);require(s.error.empty() && config["external_drivers"].size()==2,"Duplicate registration");
    const auto other=root/"Another build";put(other/"driver.vrdrivermanifest",R"({"name":"udcapc"})");
    config["external_drivers"][1]=utf8(other);save();
    s=change(s,true,fake);require(!s.error.empty(),"Conflicting install accepted");
    s=change(s,false,fake);require(s.error.empty() && s.installedPaths.empty() && config["external_drivers"].size()==1,"Uninstall changed official driver");
    const Runner failure=[](const fs::path &,const std::vector<std::string> &)->CommandResult{return {-1,"launch failed"};};
    require(!change(s,true,failure).error.empty(),"Process error hidden");
#ifdef _WIN32
    wchar_t self[32768]{};GetModuleFileNameW(nullptr,self,32768);
    auto child=run(self,{"--probe-args",R"(C:\Directory with spaces\)",R"(quote"and & $(text))"});
    std::erase(child.output,'\r');
    require(child.exitCode==0 && child.output=="C:\\Directory with spaces\\\nquote\"and & $(text)\n","Native process argument quoting failed");
#endif
    std::cout<<"SteamVR detection, registration, conflict, uninstall, failure and argument tests passed\n";
} catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
