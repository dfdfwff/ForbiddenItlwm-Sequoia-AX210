// Host test for the macOS build allow-list (AWDLBuildGuard.hpp).
#include <AWDLBuildGuard.hpp>
#include <assert.h>
#include <stdio.h>
using namespace ItlAWDL;
int main() {
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
