// Host test for the macOS build allow-list (AWDLBuildGuard.hpp).
#include <AWDLBuildGuard.hpp>
#include <assert.h>
#include <stdio.h>
using namespace ItlAWDL;
int main() {
#ifdef ITL_AWDL_GUARD_SEQUOIA
    const char *s1 = "Darwin Kernel Version 24.6.0: Sun Aug 23 20:30:56 PDT 2026; root:xnu-11417.140.69.712.69~1/RELEASE_X86_64";
    const char *s3 = "Darwin Kernel Version 24.6.0: Sun Oct 11 23:36:58 PDT 2026; root:xnu-11417.140.69.713.47~2/RELEASE_X86_64";
    const char *sx = "Darwin Kernel Version 24.6.0: root:xnu-11417.140.69.712.80~3/RELEASE_X86_64";   // e.g. 15.8.1
    assert(checkBuild(s1, "24H23") == BuildCheck::Verified);
    assert(checkBuild(s3, nullptr) == BuildCheck::Verified);
    assert(checkBuild(sx, nullptr) == BuildCheck::Expected);
    assert(checkBuild("root:xnu-11417.140.69.712.80~3/DEVELOPMENT_X86_64", nullptr) == BuildCheck::UnknownKernel);
    assert(checkBuild("root:xnu-11417.121.6~2/RELEASE_X86_64", nullptr) == BuildCheck::UnknownKernel);   // older Sequoia
    assert(checkBuild("root:xnu-12377.161.15.700.19~2/RELEASE_X86_64", "25G241") == BuildCheck::UnknownKernel);  // Tahoe kernel
    puts("macOS build allow-list (Sequoia): passed (offline only)");
    return 0;
#endif
    const char *k1 = "Darwin Kernel Version 25.6.0: Tue Aug 18 17:46:40 PDT 2026; root:xnu-12377.161.15.700.19~2/RELEASE_X86_64";
    const char *k2 = "Darwin Kernel Version 25.6.0: Mon Oct 12 00:33:09 PDT 2026; root:xnu-12377.161.15.701.48~2/RELEASE_X86_64";
    const char *kx = "Darwin Kernel Version 25.7.0: root:xnu-12377.200.1~1/RELEASE_X86_64";
    assert(checkBuild(k1, "25G241") == BuildCheck::Verified);
    assert(checkBuild(k2, "25G332") == BuildCheck::Verified);
    assert(checkBuild(k2, "25G342") == BuildCheck::Verified);
    assert(checkBuild(k1, "") == BuildCheck::Verified);          // osversion not set yet
    assert(checkBuild(k2, nullptr) == BuildCheck::Verified);
    assert(checkBuild(k1, "25G332") == BuildCheck::UnknownBuild); // build and kernel must agree
    assert(checkBuild(k2, "25G350") == BuildCheck::UnknownBuild); // a later build on the same kernel
    assert(checkBuild(kx, "25H10") == BuildCheck::UnknownKernel);
    assert(checkBuild(kx, "") == BuildCheck::UnknownKernel);
    assert(checkBuild(nullptr, "25G241") == BuildCheck::UnknownKernel);
    assert(!containsText("abc", "") && containsText("abc", "bc") && !containsText("abc", "cd"));
    // The development kernel of a verified build doesn't count.
    assert(checkBuild("root:xnu-12377.161.15.700.19~2/DEVELOPMENT_X86_64", "25G241") == BuildCheck::UnknownKernel);
    puts("macOS build allow-list: passed (offline only)");
}
