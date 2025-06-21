# 宇叠社区驱动 (UI)
基于[UdCap 宇叠动作捕捉手套社区驱动核心](https://github.com/OldestNova/UdCap-Community-HandDriver-Core)的用户 GUI。

## 当前功能支持状态
 - [x] UdCap 宇叠动作捕捉手套社区驱动核心 中的所有功能
 - [x] VMC 协议支持
 - [x] OSC 协议支持
 - [ ] 广播协议支持(将会实现为组播以避免广播风暴并提供对应客户端以兼容)
 - [ ] SteamVR 协议支持
 - [x] 简易模式(消费者版)界面
 - [ ] 高级模式(企业版)界面
 - [x] 控制器支持
 - [ ] 3D 预览
 - [ ] 关节微调
 - [ ] 控制器标定和配置

该库跟随核心库功能更新，为社区驱动核心的第一方用户界面实现。

## 介绍
社区实现的宇叠动作捕捉手套驱动的用户界面，一致化消费者版驱动和企业版驱动。理论上以原生方式支持 Windows macOS Linux 平台。以 C++ 20 实现，尽可能静态链接。

提供 VMC、VRChat OSC、SteamVR、UdCap 广播 协议的支持。使用 GTK4 和 GTKmm 编写用户界面。

## 依赖和编译
### 依赖
 * udcap-community-handdriver-core (已由 submodule 引入)
 * pkg-config
 * assimp
 * OpenGL
 * gtkmm-4
 * rapidjson
 * platformdirs (CPM 自动下载)
 * threepp (CPM 自动下载)
 * oscpp (CPM 自动下载)

### 编译
Windows 需要依赖 vcpkg 提供依赖，Linux 和 macOS 需要依赖系统包管理器提供依赖。

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

如使用 vcpkg 请根据 vcpkg 文档设置 `CMAKE_TOOLCHAIN_FILE` 变量并提前安装 `pkg-config` `assimp` `gtkmm` `opengl`。

## 说明
宇叠为上海宇叠智能科技有限公司，UdCap 是其动作捕捉手套产品的名称。

其余库内容以 MIT 协议开源。
