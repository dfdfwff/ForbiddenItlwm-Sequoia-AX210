#pragma once
// Which macOS builds AirDrop (AWDL) may run on. The AWDL interface subclasses Apple's IO80211 classes
// and relies on their virtual tables and on a few native data layouts, so it may only start on a
// build where those were verified. Elsewhere the driver behaves like plain AirportItlwm: normal
// Wi-Fi works and nothing AWDL-related starts. -itlawdlanybuild overrides this, for testers who
// accept the risk of a kernel panic.
//
// How each build was verified: 25G241 on hardware; 25G332 and 25G342 offline, from Apple's Kernel
// Debug Kits (IOSkywalkFamily identical, IO80211Family's AWDL, interface, buffer and registration
// code instruction-identical, all virtual table slots and all kernel imports matching).
#include <stddef.h>
#include <string.h>

namespace ItlAWDL {

struct VerifiedBuild {
    const char *osBuild;   // kern.osversion
    const char *kernel;    // the xnu build tag inside the kernel's version string
};

constexpr VerifiedBuild kVerifiedBuilds[] = {
    {"25G241", "xnu-12377.161.15.700.19~2/RELEASE_X86_64"},   // macOS 26.7.1
    {"25G332", "xnu-12377.161.15.701.48~2/RELEASE_X86_64"},   // macOS 26.7.2
    {"25G342", "xnu-12377.161.15.701.48~2/RELEASE_X86_64"},   // macOS 26.7.3
};

enum class BuildCheck { Verified, UnknownBuild, UnknownKernel };

inline bool containsText(const char *haystack, const char *needle) {
    if (!haystack || !needle || !*needle) return false;
    const size_t n = strlen(needle);
    for (const char *p = haystack; *p; ++p)
        if (!strncmp(p, needle, n)) return true;
    return false;
}

// kernelVersion is the kernel's `version` string, which is always set. osBuild is `osversion`,
// which may still be empty early in boot; then the kernel alone decides. Both must match when set.
inline BuildCheck checkBuild(const char *kernelVersion, const char *osBuild) {
    bool kernelKnown = false;
    for (const auto &b : kVerifiedBuilds)
        if (containsText(kernelVersion, b.kernel)) kernelKnown = true;
    if (!kernelKnown) return BuildCheck::UnknownKernel;
    if (!osBuild || !*osBuild) return BuildCheck::Verified;
    for (const auto &b : kVerifiedBuilds)
        if (!strcmp(osBuild, b.osBuild) && containsText(kernelVersion, b.kernel)) return BuildCheck::Verified;
    return BuildCheck::UnknownBuild;
}

inline const char *buildCheckName(BuildCheck c) {
    switch (c) {
        case BuildCheck::Verified: return "verified";
        case BuildCheck::UnknownBuild: return "unknown-build";
        case BuildCheck::UnknownKernel: return "unknown-kernel";
    }
    return "unknown";
}

}
