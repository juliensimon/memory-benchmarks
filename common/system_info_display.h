#ifndef SYSTEM_INFO_DISPLAY_H
#define SYSTEM_INFO_DISPLAY_H

#include "platform_interface.h"
#include "output_formatter.h"
#include <memory>

/**
 * @brief Centralized system information display utility
 * 
 * This class eliminates duplication between --info flag output and 
 * benchmark run system information display by providing a unified
 * interface for formatting and displaying system details.
 */
class SystemInfoDisplay {
public:
    /**
     * @brief Display comprehensive system information
     * 
     * @param platform Platform interface for system detection
     * @param format Output format to use
     * @param show_build_info Whether to include build information
     * @param affinity_type CPU affinity type for core-specific info
     */
    static void print_system_info(
        const std::unique_ptr<PlatformInterface>& platform,
        OutputFormat format = OutputFormat::MARKDOWN,
        bool show_build_info = false,
        CPUAffinityType affinity_type = CPUAffinityType::DEFAULT
    );

    /**
     * @brief Display system information with cached data
     * 
     * @param cached_info Pre-cached system information
     * @param platform Platform interface for additional queries
     * @param format Output format to use  
     * @param affinity_type CPU affinity type for core-specific info
     */
    static void print_cached_system_info(
        const SystemInfo& cached_info,
        const std::unique_ptr<PlatformInterface>& platform,
        OutputFormat format = OutputFormat::MARKDOWN,
        CPUAffinityType affinity_type = CPUAffinityType::DEFAULT
    );

private:
    /**
     * @brief Display macOS heterogeneous architecture information
     */
    static void print_macos_heterogeneous_info(
        const std::unique_ptr<PlatformInterface>& platform,
        const SystemInfo& base_info,
        OutputFormatter& formatter,
        bool show_build_info
    );

    /**
     * @brief Display build information
     */
    static void print_build_info(const std::unique_ptr<PlatformInterface>& platform);
};

#endif  // SYSTEM_INFO_DISPLAY_H