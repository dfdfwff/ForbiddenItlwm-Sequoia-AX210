// Selects the native declarations for the macOS version this kext is built for.
#pragma once
#if __IO80211_TARGET == __MAC_15_2
#include "macOS15/IO80211ManagementBuffer.h"
#else
#include "macOS26/IO80211ManagementBuffer.h"
#endif
