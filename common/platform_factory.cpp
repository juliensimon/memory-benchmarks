#include "platform_interface.h"
#include "errors.h"
#include <memory>

// Platform-specific includes
#ifdef __APPLE__
#include "../platforms/macos/macos_platform.h"
#elif defined(__linux__)
    #if defined(__x86_64__) || defined(__amd64__)
    #include "../platforms/intel_x64/intel_platform.h"
    #elif defined(__aarch64__)
    #include "../platforms/arm64/arm64_platform.h"
    #endif
#endif

std::unique_ptr<PlatformInterface> create_platform_interface() {
#ifdef __APPLE__
    return std::make_unique<MacOSPlatform>();
#elif defined(__linux__)
    #if defined(__x86_64__) || defined(__amd64__)
    return std::make_unique<IntelPlatform>();
    #elif defined(__aarch64__)
    return std::make_unique<ARM64Platform>();
    #else
    // Unsupported Linux architecture
    throw PlatformError("Unsupported Linux architecture. Only x86_64 and aarch64 are supported.");
    #endif
#else
    // Unsupported operating system
    throw PlatformError("Unsupported operating system. Only macOS and Linux are supported.");
#endif
}