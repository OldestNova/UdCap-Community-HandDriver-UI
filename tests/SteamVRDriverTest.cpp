// Exercise the actual driver with an in-process OpenVR host, without SteamVR,
// a headset, receivers, sockets, or changes to the user's SteamVR configuration.
#include "../steamvr/driver_udcap.cpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <vector>

using namespace vr;
using Json = nlohmann::json;
namespace fs = std::filesystem;
void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
struct Properties final : IVRProperties {
    std::map<std::pair<PropertyContainerHandle_t, ETrackedDeviceProperty>, std::string> strings;
    std::map<std::pair<PropertyContainerHandle_t, ETrackedDeviceProperty>, std::pair<PropertyTypeTag_t,std::vector<char>>> values;
    ETrackedPropertyError ReadPropertyBatch(PropertyContainerHandle_t h, PropertyRead_t *p, uint32_t n) override {
        for (uint32_t i=0;i<n;++i) {
            const auto found=values.find({h,p[i].prop});
            if(found==values.end()) {p[i].eError=TrackedProp_UnknownProperty;continue;}
            const auto &[tag,bytes]=found->second;p[i].unTag=tag;p[i].unRequiredBufferSize=static_cast<uint32_t>(bytes.size());
            if(p[i].unBufferSize<bytes.size()) {p[i].eError=TrackedProp_BufferTooSmall;continue;}
            std::memcpy(p[i].pvBuffer,bytes.data(),bytes.size());p[i].eError=TrackedProp_Success;
        }
        return TrackedProp_Success;
    }
    ETrackedPropertyError WritePropertyBatch(PropertyContainerHandle_t h, PropertyWrite_t *p, uint32_t n) override {
        for (uint32_t i=0;i<n;++i) {
            if (p[i].unTag==k_unStringPropertyTag) strings[{h,p[i].prop}]=static_cast<char *>(p[i].pvBuffer);
            const auto bytes=static_cast<char *>(p[i].pvBuffer);
            values[{h,p[i].prop}]={p[i].unTag,std::vector<char>(bytes,bytes+p[i].unBufferSize)};
            p[i].eError=TrackedProp_Success;
        }
        return TrackedProp_Success;
    }
    const char *GetPropErrorNameFromEnum(ETrackedPropertyError) override { return "test"; }
    PropertyContainerHandle_t TrackedDeviceToPropertyContainer(TrackedDeviceIndex_t i) override { return i+100; }
};
struct Input final : IVRDriverInput {
    struct Skeleton {std::string component,path; EVRSkeletalMotionRange range; std::array<VRBoneTransform_t,31> bones;};
    VRInputComponentHandle_t next=1;
    std::map<VRInputComponentHandle_t,std::pair<std::string,std::string>> skeletons;
    std::vector<Skeleton> updates;
    std::map<VRInputComponentHandle_t,std::string> poseNames;
    std::map<std::pair<PropertyContainerHandle_t,std::string>,VRInputComponentHandle_t> poseHandles;
    std::map<VRInputComponentHandle_t,HmdMatrix34_t> poseUpdates;
    std::map<std::pair<PropertyContainerHandle_t,std::string>,VRInputComponentHandle_t> hapticHandles;
    EVRInputError createError=VRInputError_None, updateError=VRInputError_None;
    EVRInputError poseCreateError=VRInputError_None, poseUpdateError=VRInputError_None;
    EVRInputError hapticCreateError=VRInputError_None;
    EVRInputError CreateBooleanComponent(PropertyContainerHandle_t,const char *,VRInputComponentHandle_t *h) override { *h=next++;return VRInputError_None; }
    EVRInputError UpdateBooleanComponent(VRInputComponentHandle_t,bool,double) override {return VRInputError_None;}
    EVRInputError CreateScalarComponent(PropertyContainerHandle_t,const char *,VRInputComponentHandle_t *h,EVRScalarType,EVRScalarUnits) override { *h=next++;return VRInputError_None; }
    EVRInputError UpdateScalarComponent(VRInputComponentHandle_t,float,double) override {return VRInputError_None;}
    EVRInputError CreateHapticComponent(PropertyContainerHandle_t container,const char *name,VRInputComponentHandle_t *h) override {
        if(hapticCreateError!=VRInputError_None) return hapticCreateError;
        *h=next++;hapticHandles[{container,name}]=*h;return VRInputError_None;
    }
    EVRInputError CreateSkeletonComponent(PropertyContainerHandle_t,const char *name,const char *path,const char *base,
        EVRSkeletalTrackingLevel level,const VRBoneTransform_t *,uint32_t,VRInputComponentHandle_t *h) override {
        if(createError!=VRInputError_None) return createError;
        require(std::string(base)=="/pose/raw", "wrong skeleton origin");
        require(level!=VRSkeletalTracking_Estimated,"glove must expose measured tracking");
        *h=next++;skeletons[*h]={name,path};return VRInputError_None;
    }
    EVRInputError UpdateSkeletonComponent(VRInputComponentHandle_t h,EVRSkeletalMotionRange range,const VRBoneTransform_t *bones,uint32_t n) override {
        require(skeletons.count(h)==1,"unknown skeletal handle");require(n==31,"not a 31 bone skeleton");
        Skeleton s{skeletons[h].first,skeletons[h].second,range,{}};
        std::copy_n(bones,31,s.bones.begin());updates.push_back(s);return updateError;
    }
    EVRInputError CreatePoseComponent(PropertyContainerHandle_t container,const char *name,VRInputComponentHandle_t *h) override {
        if(poseCreateError!=VRInputError_None) return poseCreateError;
        *h=next++;poseNames[*h]=name;poseHandles[{container,name}]=*h;return VRInputError_None;
    }
    EVRInputError UpdatePoseComponent(VRInputComponentHandle_t h,const HmdMatrix34_t *pose,double) override {
        require(poseNames.count(h)==1,"unknown render pose handle");poseUpdates[h]=*pose;return poseUpdateError;
    }
    EVRInputError CreateEyeTrackingComponent(PropertyContainerHandle_t,const char *,VRInputComponentHandle_t *) override {return VRInputError_None;}
    EVRInputError UpdateEyeTrackingComponent(VRInputComponentHandle_t,const VREyeTrackingData_t *,double) override {return VRInputError_None;}
};
struct DriverLog final : IVRDriverLog {std::vector<std::string> messages;void Log(const char *s) override {messages.emplace_back(s);} };
struct Settings final : IVRSettings {
    std::string trackerRole="TrackerRole_Handed,TrackedControllerRole_RightHand";
    const char *GetSettingsErrorNameFromEnum(EVRSettingsError) override {return "test";}
    void SetBool(const char *,const char *,bool,EVRSettingsError *) override {}
    void SetInt32(const char *,const char *,int32_t,EVRSettingsError *) override {}
    void SetFloat(const char *,const char *,float,EVRSettingsError *) override {}
    void SetString(const char *,const char *,const char *,EVRSettingsError *) override {}
    bool GetBool(const char *,const char *,EVRSettingsError *) override {return false;}
    int32_t GetInt32(const char *,const char *,EVRSettingsError *) override {return 0;}
    float GetFloat(const char *,const char *,EVRSettingsError *) override {return 0;}
    void GetString(const char *section,const char *key,char *out,uint32_t size,EVRSettingsError *error) override {
        require(std::string(section)=="trackers","wrong Tracker settings section");
        require(std::string(key)=="/devices/htc/vive_trackerLHR-TEST","wrong registered Tracker key");
        require(size>trackerRole.size(),"short Tracker role buffer");
        std::memcpy(out,trackerRole.c_str(),trackerRole.size()+1);if(error) *error=VRSettingsError_None;
    }
    void RemoveSection(const char *,EVRSettingsError *) override {}
    void RemoveKeyInSection(const char *,const char *,EVRSettingsError *) override {}
};
struct Host final : IVRServerDriverHost {
    DriverPose_t pose{};
    bool TrackedDeviceAdded(const char *,ETrackedDeviceClass,ITrackedDeviceServerDriver *) override {return true;}
    void TrackedDevicePoseUpdated(uint32_t,const DriverPose_t &p,uint32_t) override {pose=p;}
    void VsyncEvent(double) override {}
    void VendorSpecificEvent(uint32_t,EVREventType,const VREvent_Data_t &,double) override {}
    bool IsExiting() override {return false;}
    bool PollNextEvent(VREvent_t *,uint32_t) override {return false;}
    void GetRawTrackedDevicePoses(float,TrackedDevicePose_t *p,uint32_t n) override {std::fill_n(p,n,TrackedDevicePose_t{});}
    void RequestRestart(const char *,const char *,const char *,const char *) override {}
    uint32_t GetFrameTimings(Compositor_FrameTiming *,uint32_t) override {return 0;}
    void SetDisplayEyeToHead(uint32_t,const HmdMatrix34_t &,const HmdMatrix34_t &) override {}
    void SetDisplayProjectionRaw(uint32_t,const HmdRect2_t &,const HmdRect2_t &) override {}
    void SetRecommendedRenderTargetSize(uint32_t,uint32_t,uint32_t) override {}
};
struct Context final : IVRDriverContext {
    Properties properties; Input input; DriverLog log; Host host; Settings settings;
    void *GetGenericInterface(const char *s, EVRInitError *e) override {
        if(e) *e=VRInitError_None;
        if(std::strcmp(s,IVRProperties_Version)==0) return &properties;
        if(std::strcmp(s,IVRDriverInput_Version)==0) return &input;
        if(std::strcmp(s,IVRDriverLog_Version)==0) return &log;
        if(std::strcmp(s,IVRServerDriverHost_Version)==0) return &host;
        if(std::strcmp(s,IVRSettings_Version)==0) return &settings;
        if(e) *e=VRInitError_Init_InterfaceNotFound;return nullptr;
    }
    DriverHandle_t GetDriverHandle() override {return 1;}
};
Json read(const fs::path &path) {std::ifstream f(path);return Json::parse(f);}
void resources(const fs::path &root, const Context &ctx) {
    const auto base=root/"resources";
    const auto profile=read(base/"input/udcap_profile.json");
    const auto binding=read(base/"input/warudo_bindings.json");
    require(profile["controller_type"]==binding["controller_type"],"binding identity mismatch");
    require(read(base/"input/udcap_remapping.json")["to_controller_type"]==profile["controller_type"],"remapping identity mismatch");
    require(profile["input_source"]["/output/haptic"]["type"]=="vibration","haptic output missing from input profile");
    const auto remapping=read(base/"input/udcap_remapping.json");
    bool hapticMapped=false;
    for(const auto &entry:remapping["layouts"][0]["autoremappings"])
        hapticMapped |= entry.value("from",std::string{})=="/user/hand/right/output/haptic" &&
                        entry.value("to",std::string{})=="/user/hand/right/output/haptic";
    require(hapticMapped,"Knuckles haptic output is not remapped");
    for(const auto &entry:profile["default_bindings"]) require(fs::is_regular_file(base/"input"/entry["binding_url"].get<std::string>()),"default binding not bundled");
    require(profile["default_bindings"][0]["app_key"]=="steam.overlay.2079120","missing Warudo overlay binding");
    const auto &skeletons=binding["bindings"]["/actions/default"]["skeleton"];
    require(skeletons.size()==2,"Warudo must bind both hands");
    for(int hand=0;hand<2;++hand) {
        const std::string side=hand==0?"left":"right";
        require(skeletons[hand]["path"]=="/user/hand/"+side+"/input/skeleton/"+side,"crossed skeleton binding");
        require(skeletons[hand]["output"]=="/actions/default/in/skeleton"+side+"hand","wrong Warudo action");
        require(profile["input_source"]["/input/skeleton/"+side]["side"]==side,"profile hand mismatch");
        const auto model="udcapc_hand_"+side;
        const auto folder=base/"rendermodels"/model;
        const auto render=read(folder/(model+".json"));
        require(render["components"].size()==16,"hand must animate palm and all fifteen measured joints");
        for(int bone:handRenderBones) {
            const auto &component=render["components"]["bone_"+std::to_string(bone)];
            require(fs::is_regular_file(folder/component["filename"].get<std::string>()),"moving hand mesh missing");
            require(component["motion"]["type"]=="pose_component","hand mesh is static");
            require(component["motion"]["pose_path"]==handRenderPosePath(bone),"render pose path mismatch");
            require(profile["input_source"][handRenderPosePath(bone)]["type"]=="pose","render input missing from profile");
        }
    }
    for(const auto &[key,value]:ctx.properties.strings) {
        if(value.rfind("{udcapc}/",0)==0)
            require(fs::is_regular_file(base/value.substr(9)),"registered resource missing from bundle");
    }
}
float distance(const HmdVector4_t &a,const HmdVector4_t &b) {
    float d=0;for(int i=0;i<3;++i) d+=(a.v[i]-b.v[i])*(a.v[i]-b.v[i]);return std::sqrt(d);
}
bool nearlyEqual(float a,float b) {return std::abs(a-b)<0.00001f;}
void skeletonGeometry() {
    std::array<std::array<VRBoneTransform_t,31>,2> curled{};
    for(int hand=0;hand<2;++hand) {
        SteamVRBridgePacket p;
        const auto openLocal=makeBones(p,hand);
        const auto open=handModelSpace(openLocal);
        require(open[10].position.v[2]<-.15f && open[10].position.v[2]>-.25f,"neutral hand scale/forward direction is wrong");
        require(distance(open[15].position,open[12].position)>distance(open[25].position,open[22].position),"all finger lengths are identical");
        // Official Core flexion of -60 degrees -> left +Z, right -Z.
        for(int source=3;source<15;++source) p.bones[source]={0,0,hand==0?.5f:-.5f,.8660254f};
        auto local=makeBones(p,hand);const auto model=handModelSpace(local);curled[hand]=model;
        for(int tip:{10,15,20,25}) {
            require(distance(open[tip].position,model[tip].position)>.065f,"curl twists finger instead of bending it");
            require(model[tip].position.v[2]>open[tip].position.v[2]+.05f,"fist does not shorten forward finger reach");
            require((hand==0?1:-1)*(model[tip].position.v[0]-open[tip].position.v[0])>.015f,"finger curls out of palm");
        }
        constexpr std::array<int,5> distal{4,9,14,19,24};
        for(int f=0;f<5;++f) {
            require(distance(local[26+f].position,model[distal[f]].position)<.000001f,"auxiliary IK target is not the distal knuckle");
            const auto &a=local[26+f].orientation;const auto &b=model[distal[f]].orientation;
            require(nearlyEqual(a.w,b.w)&&nearlyEqual(a.x,b.x)&&nearlyEqual(a.y,b.y)&&nearlyEqual(a.z,b.z),"auxiliary IK rotation lost");
        }
        for(int source=0;source<15;++source) {
            p=SteamVRBridgePacket{};
            if(source==0) p.bones[source]={-.3826834f,0,0,.9238795f};
            else if(source<3) p.bones[source]={0,hand==0?-.5f:.5f,0,.8660254f};
            else p.bones[source]={0,0,hand==0?.5f:-.5f,.8660254f};
            const auto bent=handModelSpace(makeBones(p,hand));
            const int tip=source<3?5:10+5*(source/3-1);
            require(distance(open[tip].position,bent[tip].position)>.014f,"one measured joint lost its motion or angle");
            if(source==0) require(bent[5].position.v[1]<open[5].position.v[1]-.03f,"thumb opposition moves away from fingers");
            // Bone's own joint position is fixed; its descendants move.
            require(distance(open[measuredHandBones[source]].position,bent[measuredHandBones[source]].position)<.000001f,"rotation changed the joint origin");
        }
        p=SteamVRBridgePacket{};
        p.bones[3]={0,hand==0?.173648f:-.173648f,0,.98480775f};
        p.bones[12]={0,hand==0?-.173648f:.173648f,0,.98480775f};
        const auto spread=handModelSpace(makeBones(p,hand));
        require(spread[10].position.v[1]-spread[25].position.v[1]>
                open[10].position.v[1]-open[25].position.v[1]+.035f,"splay moves fingers together");
        p.bones[3]={0,0,0,0}; p.bones[0].x=std::numeric_limits<float>::quiet_NaN();
        for(const auto &bone:makeBones(p,hand)) {
            const auto &q=bone.orientation;
            require(nearlyEqual(q.w*q.w+q.x*q.x+q.y*q.y+q.z*q.z,1),"invalid/nonunit skeletal rotation");
        }
    }
    for(int b=0;b<31;++b) {
        const auto &l=curled[0][b].position;const auto &r=curled[1][b].position;
        require(nearlyEqual(l.v[0],-r.v[0])&&nearlyEqual(l.v[1],r.v[1])&&nearlyEqual(l.v[2],r.v[2]),"left/right hand geometry not mirrored");
    }
}
void nativeThumbGeometry() {
    std::array<VRBoneTransform_t, 2> closedRoots{};
    for (int hand=0; hand<2; ++hand) {
        SteamVRBridgePacket p;
        p.hasNativeThumb=1;
        p.thumbFlexion[1]=15.0f/65.0f;
        p.thumbFlexion[2]=15.0f/65.0f;
        const auto open=handModelSpace(makeBones(p,hand));
        const auto openRoot=makeBones(p,hand)[2].orientation;
        p.thumbFlexion[0]=1;
        const auto closedLocal=makeBones(p,hand);
        const auto closed=handModelSpace(closedLocal);
        closedRoots[hand]=closedLocal[2];
        const auto closedRoot=closedLocal[2].orientation;
        const float dot=std::abs(openRoot.w*closedRoot.w+openRoot.x*closedRoot.x+
                                 openRoot.y*closedRoot.y+openRoot.z*closedRoot.z);
        require(2*std::acos(std::clamp(dot,0.0f,1.0f))>.60f,
                "native thumb root did not traverse official open-to-closed pose");
        require(distance(open[5].position,closed[5].position)>.025f,
                "native thumb root barely moves the fingertip");
        p.thumbFlexion[1]=1;p.thumbFlexion[2]=1;
        const auto full=handModelSpace(makeBones(p,hand));
        require(distance(closed[5].position,full[5].position)>.02f,
                "native thumb middle/distal flexion was lost");
        p.thumbFlexion[0]=0;
        p.thumbSplay=-.25f;
        const auto negative=handModelSpace(makeBones(p,hand));
        p.thumbSplay=.1875f;
        const auto positive=handModelSpace(makeBones(p,hand));
        require(distance(negative[5].position,positive[5].position)>.02f,
                "native thumb side-to-side splay was lost");
        const auto plain=makeBones(p,hand)[2].orientation;
        p.thumbOffsets[0]={0,.173648f,0,.98480775f};
        const auto offset=makeBones(p,hand)[2].orientation;
        const float offsetDot=std::abs(offset.w*plain.w+offset.x*plain.x+
                                       offset.y*plain.y+offset.z*plain.z);
        require(offsetDot<.999f,"native thumb lost user bone offset");
    }
    require(nearlyEqual(closedRoots[0].position.v[0],-closedRoots[1].position.v[0]) &&
            nearlyEqual(closedRoots[0].position.v[1],closedRoots[1].position.v[1]) &&
            nearlyEqual(closedRoots[0].position.v[2],closedRoots[1].position.v[2]),
            "native thumb root position is not mirrored");
}
int main(int argc,char **argv) try {
    require(argc==2,"expected driver bundle directory");
    skeletonGeometry();
    nativeThumbGeometry();
    Context ctx; VRDriverContext()=&ctx;
    for(int hand=0;hand<2;++hand) {
        Controller device(hand); const auto before=ctx.input.updates.size();
        require(device.Activate(hand)==VRInitError_None,"activation failed");
        require(ctx.input.updates.size()==before+2,"both ranges must initialize before receiving data");
        require(ctx.input.updates[before].range==VRSkeletalMotionRange_WithController &&
            ctx.input.updates[before+1].range==VRSkeletalMotionRange_WithoutController,"missing skeletal range");
        require(ctx.input.updates.back().path==(hand==0?"/skeleton/hand/left":"/skeleton/hand/right"),"wrong skeletal side");
        const auto hapticHandle=ctx.input.hapticHandles.at({static_cast<PropertyContainerHandle_t>(hand+100),"/output/haptic"});
        VREvent_t hapticEvent{};
        hapticEvent.eventType=VREvent_Input_HapticVibration;
        hapticEvent.data.hapticVibration.containerHandle=hand+100;
        hapticEvent.data.hapticVibration.componentHandle=hapticHandle;
        hapticEvent.data.hapticVibration.fDurationSeconds=.2f;
        hapticEvent.data.hapticVibration.fFrequency=150;
        hapticEvent.data.hapticVibration.fAmplitude=.75f;
        SteamVRHapticPacket hapticPacket{};
        require(!device.hapticPacket(hapticEvent,hapticPacket),"disconnected glove accepted haptic output");
        device.RunFrame();require(!ctx.host.pose.deviceIsConnected,"initialization must not fake a connected glove");
        require(ctx.input.updates.size()==before+4,"idle device lost skeletal stream");
        SteamVRBridgePacket p; p.connected=1;p.hand=hand;p.bones[3]={0,0,.6f,.8f};device.SetPacket(p);device.RunFrame();
        require(ctx.host.pose.deviceIsConnected,"fresh glove not connected");
        require(device.hapticPacket(hapticEvent,hapticPacket) && hapticPacket.hand==hand &&
                nearlyEqual(hapticPacket.durationSeconds,.2f) && nearlyEqual(hapticPacket.amplitude,.75f),
                "SteamVR haptic event was not routed to the correct glove");
        hapticEvent.data.hapticVibration.componentHandle=hapticHandle+1;
        require(!device.hapticPacket(hapticEvent,hapticPacket),"foreign haptic handle accepted");
        hapticEvent.data.hapticVibration.componentHandle=hapticHandle;
        hapticEvent.data.hapticVibration.fAmplitude=0;
        require(!device.hapticPacket(hapticEvent,hapticPacket),"zero-amplitude haptic event accepted");
        hapticEvent.data.hapticVibration.fAmplitude=.75f;
        hapticEvent.data.hapticVibration.fFrequency=0;
        require(!device.hapticPacket(hapticEvent,hapticPacket),"zero-frequency haptic event accepted");
        hapticEvent.data.hapticVibration.fFrequency=150;
        require(!ctx.host.pose.poseIsValid,"must not fake a Tracker pose");
        require(std::abs(ctx.input.updates.back().bones[7].orientation.z)>.4f,"measured finger bend not updated");
        const auto expected=handModelSpace(ctx.input.updates.back().bones);
        for(int bone:handRenderBones) {
            const auto handle=ctx.input.poseHandles.at({static_cast<PropertyContainerHandle_t>(hand+100),handRenderPosePath(bone)});
            const auto &actual=ctx.input.poseUpdates.at(handle);
            const auto expectedMatrix=boneMatrix(expected[bone]);
            for(int row=0;row<3;++row) for(int col=0;col<4;++col)
                require(nearlyEqual(actual.m[row][col],expectedMatrix.m[row][col]),"rendered joint differs from skeletal input");
        }
        p.connected=0;device.SetPacket(p);device.RunFrame();require(!ctx.host.pose.deviceIsConnected,"disconnect not reported");
        require(!device.hapticPacket(hapticEvent,hapticPacket),"disconnected glove accepted haptics");
        const auto size=ctx.input.updates.size();device.Deactivate();device.RunFrame();
        require(ctx.input.updates.size()==size,"deactivated handle still used");
    }
    resources(argv[1],ctx);
    const auto tracker=VRProperties()->TrackedDeviceToPropertyContainer(7);
    VRProperties()->SetInt32Property(tracker,Prop_DeviceClass_Int32,TrackedDeviceClass_GenericTracker);
    VRProperties()->SetStringProperty(tracker,Prop_SerialNumber_String,"LHR-TEST");
    VRProperties()->SetStringProperty(tracker,Prop_RegisteredDeviceType_String,"htc/vive_trackerLHR-TEST");
    require(trackerIsHand(7,1,"") && !trackerIsHand(7,0,""),"composite right Tracker role lost");
    ctx.settings.trackerRole="TrackerRole_Handed,TrackedControllerRole_LeftHand";
    require(trackerIsHand(7,0,"") && !trackerIsHand(7,1,""),"composite left Tracker role lost");
    ctx.settings.trackerRole="TrackerRole_Waist";
    VRProperties()->SetInt32Property(tracker,Prop_ControllerRoleHint_Int32,TrackedControllerRole_LeftHand);
    require(!trackerIsHand(7,0,""),"body Tracker incorrectly selected as a hand");
    Controller failing(0);ctx.input.createError=VRInputError_InvalidParam;
    ctx.input.hapticCreateError=VRInputError_InvalidParam;
    require(failing.Activate(3)==VRInitError_Driver_Failed,"haptic creation failure silently accepted");
    ctx.input.hapticCreateError=VRInputError_None;
    require(failing.Activate(3)==VRInitError_Driver_Failed,"creation failure silently accepted");
    ctx.input.createError=VRInputError_None;ctx.input.updateError=VRInputError_InvalidBoneCount;
    require(failing.Activate(3)==VRInitError_Driver_Failed,"initial pose failure silently accepted");
    ctx.input.updateError=VRInputError_None;ctx.input.poseCreateError=VRInputError_InvalidParam;
    require(failing.Activate(3)==VRInitError_Driver_Failed,"render component failure silently accepted");
    ctx.input.poseCreateError=VRInputError_None;ctx.input.poseUpdateError=VRInputError_InvalidHandle;
    require(failing.Activate(3)==VRInitError_Driver_Failed,"render update failure silently accepted");
    require(ctx.log.messages.back().find("VRInputError=")!=std::string::npos,"missing actionable skeletal error");
    CleanupDriverContext();std::cout<<"SteamVR 31-bone geometry, all 15 joint motions, mirrored curl/splay, IK targets, animated render poses, activation and Warudo bindings passed\n";
    return 0;
} catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
