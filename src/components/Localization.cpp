#include "Localization.h"

#include <clocale>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <libintl.h>
#if defined(_WIN32)
#include <Windows.h>
#endif

namespace {
constexpr const char *domain = "udcap-community-driver-ui";

std::filesystem::path executableDirectory(const char *argv0) {
#if defined(_WIN32)
    char buffer[32768]{};
    const auto length = GetModuleFileNameA(nullptr, buffer, sizeof(buffer));
    if (length > 0 && length < sizeof(buffer))
        return std::filesystem::path(buffer).parent_path();
#elif defined(__linux__)
    std::error_code error;
    const auto path = std::filesystem::read_symlink("/proc/self/exe", error);
    if (!error) return path.parent_path();
#endif
    return std::filesystem::absolute(argv0 ? argv0 : "UdCapCommunityDriverUI").parent_path();
}
}

void initializeUiLocalization(const std::string &language, const char *argv0) {
    const bool overrideLanguage = language == "en" || language == "zh_CN";
    if (overrideLanguage) {
#if defined(_WIN32)
        _putenv_s("LANGUAGE", language.c_str());
#else
        setenv("LANGUAGE", language.c_str(), 1);
#endif
    }
    const char *locale = std::setlocale(LC_ALL, "");
    if (overrideLanguage && (!locale || std::strcmp(locale, "C") == 0 ||
                             std::strcmp(locale, "POSIX") == 0)) {
#if defined(_WIN32)
        std::setlocale(LC_ALL, ".UTF-8");
#else
        if (!std::setlocale(LC_ALL, "C.UTF-8"))
            std::setlocale(LC_ALL, "en_US.UTF-8");
#endif
    }
    const char *overrideDir = std::getenv("UDCAP_LOCALE_DIR");
    const auto localeDir = overrideDir && *overrideDir
        ? std::filesystem::path(overrideDir)
        : executableDirectory(argv0) / "locale";
    const auto nativePath = localeDir.string();
    bindtextdomain(domain, nativePath.c_str());
    bind_textdomain_codeset(domain, "UTF-8");
    textdomain(domain);
}
