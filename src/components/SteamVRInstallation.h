#pragma once
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace SteamVRInstallation {
struct CommandResult { int exitCode = -1; std::string output; };
using Runner = std::function<CommandResult(const std::filesystem::path &, const std::vector<std::string> &)>;
struct Status {
    std::filesystem::path registryFile, tool, bundle;
    std::vector<std::filesystem::path> installedPaths;
    bool bundleAvailable = false;
    bool officialDriverPresent = false;
    std::string error;
};
std::string utf8(const std::filesystem::path &path);
// Public inputs allow tests to use an isolated registry and fake vrpathreg.
Status inspect(const std::filesystem::path &registryFile, const std::filesystem::path &bundle,
               const std::filesystem::path &fallbackRuntime = {});
Status detect();
CommandResult run(const std::filesystem::path &executable, const std::vector<std::string> &arguments);
// Re-reads registration before changing it, queries finddriver, and verifies afterwards.
Status change(const Status &status, bool install, const Runner &runner = run);
}
