# Sender integration test

This isolated test builds Core with `USE_MANUAL_SERIAL_DRIVER`, injects two
calibrated virtual gloves and decodes the real VMC/VRChat OSC/QingTong UDP and
OptiTrack IPC outputs. It never scans physical devices. Its endpoint is
`ipc:///tmp/udcap-community-output-audit`, separate from the application.

Run in a Visual Studio developer terminal with CMake >= 3.29:

```powershell
cmake -S tests/output-integration -B build-output-audit -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_PREFIX_PATH=D:/Program/vcpkg/installed/x64-windows
cmake --build build-output-audit
$env:PATH="D:\Program\vcpkg\installed\x64-windows\bin;"+$env:PATH
ctest --test-dir build-output-audit --output-on-failure
```

The test reuses existing CPM sources under `build/_deps` and the CPM module
under `build-compile-repro/cmake`. Override `UI_DEPS`, `CPM_CMAKE_PATH` and
`UDCAP_CORE_SOURCE_DIR` if these are elsewhere. nng and RapidJSON must already
be installed; this configure does not install/remove vcpkg packages.

The fixed AR1 reference poses were generated from the official v0.1.8.6 DLL.
`UdCapOfficialPoseTest` independently checks its Euler tuples, preview basis
conversions and OSC parameter quantization.
