#include "components/VMCSender.h"
#include "components/OSCSender.h"
#include "components/QTSender.h"
#include "components/QingTongPose.h"
#include "components/OptiTrackSender.h"
#include "components/VRChatPackets.h"
#include <HandOffsetProjection.h>
#include <filesystem>
#include <oscpp/server.hpp>
#include <nng/protocol/pubsub0/sub.h>
#include <rapidjson/document.h>
#include <iostream>
#include <set>
#include <cmath>

using boost::asio::ip::udp;
using Clock=std::chrono::steady_clock;
void require(bool condition,const char *message) { if(!condition) throw std::runtime_error(message); }
void inject(const std::shared_ptr<PortAccessor> &port,CommandType command,const std::vector<uint8_t> &bytes) {
    std::vector<uint8_t> packet{170,85,1,static_cast<uint8_t>(command),0,static_cast<uint8_t>(bytes.size())};
    for(auto b:bytes) packet.push_back(b^129);
    uint8_t sum=0; for(std::size_t i=2;i<packet.size();++i) sum+=packet[i];
    packet.push_back(sum); port->injectReceivedData(packet);
}
void raw(const std::shared_ptr<PortAccessor> &port,int value) {
    std::vector<uint8_t> bytes;
    for(int i=0;i<19;++i) {
        int v=i<12 ? value : (i==15 ? 2000 : (i==18 ? 0 : 1000));
        bytes.push_back(v>>8); bytes.push_back(v&255);
    }
    inject(port,CMD_DATA,bytes);
}
std::array<double,28> officialAngles(bool left) {
    // Installed official AR1_linear_AG DLL, uniform input 20% / 70%.
    auto a=left ? std::array<double,28>{-15,-19.8,-4,15,-19.2,-24,-16.2,0,-18.4,-23,-18.4,0,-25.76,-32.2,-25.68,0,-26.72,-33.4,-21.4,0,-7.4,0,0,-1}
                : std::array<double,28>{-52.5,-27.3,-39,15,-67.2,-84,-60.875,0,-75.76,-94.7,-74.16,0,-80,-100,-80,0,-74.24,-92.8,-70.712,0,-28.4,0,0,-1};
    a[27]=1;
    return a;
}
std::array<BoneQuaternion,15> joints(const HandQuaternion &p) {
    std::array<BoneQuaternion,15> out; std::size_t i=0;
    for(const auto &f:{p.thumbFinger,p.indexFinger,p.middleFinger,p.ringFinger,p.littleFinger})
        for(const auto &q:{f.proximal,f.intermediate,f.distal}) out[i++]=q;
    return out;
}
bool same(const BoneQuaternion &a,const BoneQuaternion &b) {
    return std::abs(a.x-b.x)<.0001 && std::abs(a.y-b.y)<.0001 && std::abs(a.z-b.z)<.0001 && std::abs(a.w-b.w)<.0001;
}
struct Glove {
    std::shared_ptr<PortAccessor> port=std::make_shared<PortAccessor>();
    std::shared_ptr<UdCapV1Core> core;
    Glove(const std::string &serial) {
        port->bindSerialCallbacks({[]{return true;},[]{},[](int){},[](const uint8_t *,std::size_t n){return n;}});
        core=std::make_shared<UdCapV1Core>(port);
        std::vector<uint8_t> linked{1}; linked.insert(linked.end(),serial.begin(),serial.end());
        inject(port,CMD_LINK_STATE,linked); raw(port,200);
        core->runCalibration(UDCAP_V1_DEVICE_CALI_TYPE_HAND);
        raw(port,1200); core->captureCalibrationData(UDCAP_V1_HAND_CALI_TYPE_FIST);
        raw(port,200); core->captureCalibrationData(UDCAP_V1_HAND_CALI_TYPE_ADDUCTION);
        raw(port,1200); core->captureCalibrationData(UDCAP_V1_HAND_CALI_TYPE_PROTRACT);
        core->completeCalibration(UDCAP_V1_DEVICE_CALI_TYPE_HAND);
        inject(port,CMD_BATTERY,{0x09,0x60,static_cast<uint8_t>(-45)}); // 2400: level 4
    }
};
std::vector<char> receive(udp::socket &socket) {
    std::vector<char> data(8192);
    boost::system::error_code ec;
    auto n=socket.receive(boost::asio::buffer(data),0,ec);
    if(ec==boost::asio::error::would_block || ec==boost::asio::error::try_again) return {};
    if(ec) throw boost::system::system_error(ec);
    data.resize(n); return data;
}

int main(int argc,char **argv) try {
    const bool adjusted=argc>1 && std::string(argv[1])=="--adjusted";
    boost::asio::io_context io;
    udp::socket vmcRx(io,{udp::v4(),0}),oscRx(io,{udp::v4(),0}),qtRx(io,{udp::v4(),0});
    vmcRx.non_blocking(true); oscRx.non_blocking(true); qtRx.non_blocking(true);
    Glove left("ACSTDE4FG540004L"),right("ACSTDE4FG540004R");
    if(adjusted) for(const auto &core:{left.core,right.core}) {
        auto curves=defaultHandResponse();
        const double side=core->getTarget()==UD_TARGET_LEFT_HAND?-1:1;
        for(std::size_t i=0;i<curves.size();++i) {
            if(i!=11) {curves[i].outputMin-=3;curves[i].outputMax+=2;curves[i].balance=.3;}
            curves[i].offset=side*4;
        }
        core->setHandResponse(curves);
        const BoneRotation o{2,float(side*3),float(side*4)};
        const FingerRotation finger{o,o,o};
        core->setHandOffset(HandRotation{finger,finger,finger,finger,finger});
        const auto path=std::filesystem::temp_directory_path()/
            ("udcap-pose-pref-"+core->getUDCapSerial()+"-"+std::to_string(Clock::now().time_since_epoch().count())+".json");
        require(core->savePref(path.string()),"Cannot save response settings");
        core->setHandResponse(defaultHandResponse());core->setHandOffset(HandRotation{});
        require(core->loadPref(path.string()),"Cannot restore response settings");std::filesystem::remove(path);
        require(core->getHandResponse()[2].balance==.3 && core->getHandResponse()[20].offset==side*4 &&
            core->getHandOffset().littleFinger.distal.z==side*4,"Response/offset persistence failed");
    }
    VMCSender vmc("127.0.0.1",vmcRx.local_endpoint().port());
    OSCSender osc("127.0.0.1",oscRx.local_endpoint().port());
    QTSender qt("127.0.0.1",qtRx.local_endpoint().port());
    vmc.add(left.core,UD_TARGET_LEFT_HAND); vmc.add(right.core,UD_TARGET_RIGHT_HAND);
    osc.add(left.core); osc.add(right.core);
    const auto lfd=qt.add(left.core,"receiver-left",3); qt.add(right.core,"receiver-right",3);
    const char *endpoint=adjusted?"ipc:///tmp/udcap-community-output-adjusted":"ipc:///tmp/udcap-community-output-audit";
    OptiTrackSender opti(endpoint);
    // Discovery names deliberately disagree with glove names: Core is authoritative.
    opti.add(left.core,"UDXST400049R"); opti.add(right.core,"UDXST400049L");
    nng_socket subscriber{};
    require(nng_sub0_open(&subscriber)==0,"NNG subscriber open failed");
    require(nng_setopt(subscriber,NNG_OPT_SUB_SUBSCRIBE,"",0)==0,"NNG subscribe failed");
    require(nng_dial(subscriber,endpoint,nullptr,NNG_FLAG_NONBLOCK)==0,"NNG dial failed");
    std::map<std::string,BoneQuaternion> expectedVmc;
    std::map<std::string,bool> expectedOsc;
    for(const bool isLeft:{true,false}) {
        const auto core=isLeft?left.core:right.core;
        const auto target=isLeft?UD_TARGET_LEFT_HAND:UD_TARGET_RIGHT_HAND;
        const auto angles=applyHandResponse(officialAngles(isLeft),core->getHandResponse());
        auto pose=joints(UdCapV1Core::officialHandPose(angles,target,core->getHandOffset(), {.3f,.3f,1.2f}));
        forEachVRChatPacket(anglesWithHandOffset(angles,target,core->getHandOffset()),target,[&](const void *data,std::size_t size) {
            OSCPP::Server::Bundle bundle=OSCPP::Server::Packet(data,size);auto messages=bundle.packets();
            while(!messages.atEnd()) {OSCPP::Server::Message m=messages.next();expectedOsc[m.address()]=m.args().tag()=='T';}
        });
        std::size_t i=0;
        for(const auto *finger:{"Thumb","Index","Middle","Ring","Little"})
            for(const auto *joint:{"Proximal","Intermediate","Distal"})
                expectedVmc[std::string(isLeft?"Left":"Right")+finger+joint]=pose[i++];
    }
    std::set<std::string> bones,parameters,qtNames,optiNames,blendNames;
    bool applied=false;
    const auto end=Clock::now()+std::chrono::seconds(3);
    while(Clock::now()<end && (bones.size()!=30 || parameters.size()!=120 || qtNames.size()!=2 || optiNames.size()!=2 || !applied)) {
        raw(left.port,400); raw(right.port,900);
        std::this_thread::sleep_for(std::chrono::milliseconds(12));
        for(int pass=0;pass<3;++pass) {
            auto &socket=pass==0 ? vmcRx : pass==1 ? oscRx : qtRx;
            for(auto data=receive(socket); !data.empty(); data=receive(socket)) {
                if(pass==2) {
                    rapidjson::Document doc; doc.Parse(data.data(),data.size());
                    require(!doc.HasParseError() && doc["DeviceID"].GetUint()==3,"Invalid QingTong JSON/device id");
                    require(doc["CalibrationStatus"].GetInt()==3 && doc["Battery"].GetInt()==4,"QingTong startup metadata lost");
                    require(doc["Bones"].Size()==16,"QingTong missing joints");
                    const std::string name=doc["DeviceName"].GetString();
                    const auto core=name.back()=='L'?left.core:right.core;
                    const auto expected=joints(poseForQingTong(applyHandResponse(officialAngles(name.back()=='L'),core->getHandResponse()),core->getTarget(),core->getHandOffset()));
                    for(std::size_t i=0;i<15;++i) {
                        const auto &q=doc["Bones"][static_cast<rapidjson::SizeType>(i)];
                        const std::size_t source=i<12?i+3:i-12;
                        require(same({q[0].GetFloat(),q[1].GetFloat(),q[2].GetFloat(),q[3].GetFloat()},expected[source]),"QingTong bone order/pose differs from official");
                    }
                    for(const auto &q:doc["Bones"].GetArray()) {
                        double norm=0; for(const auto &x:q.GetArray()) norm+=x.GetDouble()*x.GetDouble();
                        require(std::abs(norm-1)<.0001,"QingTong invalid quaternion");
                    }
                    qtNames.insert(doc["DeviceName"].GetString());
                    continue;
                }
                const OSCPP::Server::Packet packet(data.data(),data.size());
                require(packet.isBundle(),"Missing OSC bundle on wire");
                const OSCPP::Server::Bundle bundle=packet;
                require(bundle.time()==1,"Non-immediate OSC timetag");
                auto messages=bundle.packets();
                while(!messages.atEnd()) {
                    const OSCPP::Server::Message message=messages.next();
                    auto args=message.args();
                    if(pass==0 && std::string(message.address())=="/VMC/Ext/Bone/Pos") {
                        const std::string name=args.string();
                        for(int i=0;i<3;++i) require(args.float32()==0,"Unexpected VMC joint position");
                        const BoneQuaternion q{args.float32(),args.float32(),args.float32(),args.float32()};
                        require(same(q,expectedVmc.at(name)),"VMC bone side/pose differs from official");
                        bones.insert(name);
                    } else if(pass==0 && std::string(message.address())=="/VMC/Ext/Blend/Val") {
                        blendNames.insert(args.string());
                        const float value=args.float32();
                        require(std::isfinite(value)&&value>=0&&value<=1,"VMC BlendShape out of range");
                    } else if(pass==0 && std::string(message.address())=="/VMC/Ext/Blend/Apply") {
                        applied=true;
                    } else if(pass==1) {
                        require(args.size()==1 && (args.tag()=='T'||args.tag()=='F'),"Invalid VRChat parameter type");
                        require(expectedOsc.at(message.address())==(args.tag()=='T'),"VRChat omitted response curves or bone offsets");
                        parameters.insert(message.address());
                    }
                }
            }
        }
        while(true) {
            udcap::optitrack::Frame frame{}; std::size_t length=sizeof(frame);
            const int error=nng_recv(subscriber,&frame,&length,NNG_FLAG_NONBLOCK);
            if(error==NNG_EAGAIN) break;
            require(error==0 && length==sizeof(frame),"Invalid OptiTrack IPC frame");
            require(frame.frameMagic==udcap::optitrack::magic && frame.frameVersion==udcap::optitrack::version,"OptiTrack version mismatch");
            require(frame.hand==(std::string(frame.serial).back()=='L'?1:2),"OptiTrack used receiver side instead of glove side");
            require(frame.batteryLevel==4 && frame.rssiDbm==-45 && frame.batteryRaw==2400,"OptiTrack lost telemetry");
            std::size_t index=0;
            for(const auto *finger:{"Thumb","Index","Middle","Ring","Little"})
                for(const auto *joint:{"Proximal","Intermediate","Distal"}) {
                    const auto &q=frame.joints[index++];
                    require(same({q.x,q.y,q.z,q.w},expectedVmc.at(std::string(frame.hand==1?"Left":"Right")+finger+joint)),"OptiTrack changed Core local bone order/pose");
                }
            optiNames.insert(frame.serial);
        }
    }
    nng_close(subscriber);
    require(bones.size()==30 && parameters.size()==120 && qtNames.size()==2 && optiNames.size()==2,"Output stream incomplete");
    for(const auto *side:{"Left","Right"}) for(const auto *name:{"JoyX_Positive","JoyX_Negative","JoyY_Positive","JoyY_Negative","Button_A","Button_B","Button_Joy","Button_Menu"})
        require(blendNames.contains(std::string(side)+name),"Official VMC BlendShape missing");
    // Removal during active incoming poses must not deadlock on its callback lock.
    std::atomic_bool feed{true};
    std::thread producer([&]{while(feed) {raw(left.port,600);std::this_thread::sleep_for(std::chrono::milliseconds(1));}});
    qt.remove(lfd); feed=false; producer.join();
    std::cout<<(adjusted?"Custom curves and offsets: ":"Default pose: ")<<"Serial -> Core -> UDP/IPC passed: 30 VMC bones, 120 OSC parameters, 2 QingTong gloves, 2 OptiTrack gloves with battery/RSSI\n";
    return 0;
} catch(const std::exception &error) { std::cerr<<error.what()<<'\n'; return 1; }
