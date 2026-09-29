# 宇叠社区驱动 (UI)
基于[UdCap 宇叠动作捕捉手套社区驱动核心](https://github.com/OldestNova/UdCap-Community-HandDriver-Core)的用户 GUI。

## 当前功能支持状态
 - [x] UdCap 宇叠动作捕捉手套社区驱动核心
 - [x] VMC 协议支持
 - [x] OSC 协议支持
 - [x] 广播协议支持
 - [x] SteamVR 控制器驱动（需单独构建并注册）
 - [x] 简易模式(消费者版)界面
 - [x] 高级模式(企业版)界面：按序列号自动配对或手动组合多组接收器，每组独立配置 VMC 和 OSC，在偏好设置中统一配置青瞳，并指定一组输出到 SteamVR（多组实机验证待完成）
 - [x] 控制器支持
 - [x] OSC 操作接口，可以通过 OSC 控制本客户端
 - [x] 3D 双手手指预览
 - [x] 关节微调
 - [x] 控制器标定和配置

该库跟随核心库功能更新，为社区驱动核心的第一方用户界面实现。

高级模式会扫描所有接收器，默认把序列号仅末尾 `L`/`R` 不同的接收器自动组成一组，也可在创建手套组弹窗中手动指定左右接收器；配组关系按序列号保存并在下次启动时恢复。手套或手套组详情提供各自的控制器和手部设置弹窗；每组独立设置 VMC 和 VRChat OSC 的目标地址。青瞳 UDP 在偏好设置页统一配置，将所有已连接手套发往同一个目标，同组左右手共用 `DeviceID`，通过 `DeviceName` 区分手套。固件页自动列出检测到的所有手套，偏好设置直接显示为页面。SteamVR 总开关位于偏好设置页，每组详情的「作为 SteamVR 手套」按钮指定唯一的输出组；Tracker 位置和旋转偏移在单手套详情中设置。

在「手部设置」中可以为左右手分别选择标定算法：默认（0，当前使用原始 V1）、原始 V1（1）或社区 V1（20，自适应标定）。选择会立即生效并保存；高级模式按接收器序列号分别保存。

选择「社区 V1」后，Core 在首次标定基础上持续跟踪零点漂移和传感器范围变化，无需定时点击重新标定。日常偶尔充分伸直、握拳和张开手指即可，不要求固定顺序，也支持连续不停顿的动作。长时间静止时不会通过衰减边界缩小量程；仅保持未知中间姿势无法提供可靠的自动校准依据。默认和原始 V1 不启用此功能。

简单模式与高级模式的预览、OSC 服务、OptiTrack、手套首选项和 SteamVR 偏移分别保存。切换模式后窗口会自动关闭并重新打开；首次启动时迁移旧版共享设置，此后互不覆盖。Core 首选项分别保存在配置目录的 `core/consumer` 和 `core/enterprise`。

## 介绍
社区实现的宇叠动作捕捉手套驱动的用户界面，一致化消费者版驱动和企业版驱动。理论上以原生方式支持 Windows macOS Linux 平台。以 C++ 20 实现，尽可能静态链接。

提供 VMC、VRChat OSC、SteamVR、UdCap 广播 协议的支持。使用 GTK4 和 GTKmm 编写用户界面。

## 依赖和编译
### 依赖
 * udcap-community-handdriver-core（可由 submodule、同级目录或 `UDCAP_CORE_SOURCE_DIR` 指定）
 * pkg-config（Windows 由 vcpkg 的 pkgconf 提供）
 * assimp
 * OpenGL
 * gtkmm-4
 * Boost.Asio（UI 网络组件）
 * rapidjson
 * platformdirs (CPM 自动下载)
 * threepp (CPM 自动下载)
 * oscpp (CPM 自动下载)
 * OpenVR SDK（构建 SteamVR 驱动时需要；UI 可选用其客户端库）
 * gettext 的 `msgfmt`（构建多语言 UI 时需要）

### 编译
Windows 传入 vcpkg toolchain 后，会自动识别 UI 根目录的 `vcpkg.json`，安装原本由 CMake 查找的 gtkmm、rapidjson、pkgconf 和 gettext 工具；3D 预览所需的 assimp 与 OpenVR 分别位于可选 feature。OpenGL 来自系统 SDK，Core 所需的第三方库、threepp、oscpp、platformdirs 等仍由 CPM 获取。

Linux/macOS：

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

UI 构建需要 CMake 3.29 或更新版本和支持 C++20 的编译器。

SteamVR 的 OpenVR 依赖位于可选的 `steamvr` manifest feature；设置 `BUILD_STEAMVR_DRIVER=ON` 时会自动启用。仅需要 OpenVR 客户端时，可传入 `-DVCPKG_MANIFEST_FEATURES=steamvr`。

如果出现 CMake 新版本不兼容旧版本依赖，使用 `CMAKE_POLICY_VERSION_MINIMUM=3.5` 兼容当前依赖中较旧的 CMake 项目。

Core 源码位于其他位置时，在首次配置命令后追加 `-DUDCAP_CORE_SOURCE_DIR=D:/path/to/UdCap-Community-HandDriver-Core`。该路径须指向包含 `CMakeLists.txt` 和 `src/UdCapV1Core.h` 的 Core 根目录；UI 会通过 `add_subdirectory` 在自己的构建目录中编译 Core。未指定时依次查找 UI 仓库中的 submodule 和同级的 `UdCap-Community-HandDriver-Core`。

### 界面语言

界面支持 English 和简体中文，默认跟随系统语言。可在「偏好设置 → 语言」中选择语言，重启应用后生效。构建时 `msgfmt` 将 `po/zh_CN.po` 编译并复制到可执行文件旁的 `locale/zh_CN/LC_MESSAGES/`；分发程序时请保留该目录。英文使用源码文本。新增界面文案时用 `_()` 标记；静态字符串表用 `N_()` 标记，并更新 `po/udcap-community-driver-ui.pot` 与 `po/zh_CN.po`。

## 说明
宇叠为上海宇叠智能科技有限公司，UdCap 是其动作捕捉手套产品的名称。

其余库内容以 MIT 协议开源。
