#ifndef MEMORY_TYPES_H
#define MEMORY_TYPES_H

#include <cstddef>
#include <string>
#include <vector>

/**
 * @brief Cache information structure
 *
 * Contains detailed information about the CPU cache hierarchy
 * including sizes, associativity, and line sizes for all cache levels.
 */
struct CacheInfo {
    size_t l1_data_size;         ///< L1 data cache size in bytes (per core)
    size_t l1_instruction_size;  ///< L1 instruction cache size in bytes (per core)
    size_t l2_size;              ///< L2 cache size in bytes (per core)
    size_t l3_size;              ///< L3 cache size in bytes (shared)
    size_t l1d_assoc;            ///< L1 data cache associativity
    size_t l1i_assoc;            ///< L1 instruction cache associativity
    size_t l2_assoc;             ///< L2 cache associativity
    size_t l3_assoc;             ///< L3 cache associativity
    size_t l1_line_size;         ///< L1 cache line size in bytes
    size_t l2_line_size;         ///< L2 cache line size in bytes
    size_t l3_line_size;         ///< L3 cache line size in bytes
};

/**
 * @brief Memory specifications structure
 *
 * Contains detailed information about system memory including
 * type, speed, width, and theoretical bandwidth calculations.
 */
struct MemorySpecs {
    std::string type;                   ///< Memory type (DDR3, DDR4, DDR5, LPDDR4, LPDDR5)
    size_t speed_mtps;                  ///< Memory speed in MT/s
    size_t data_width_bits;             ///< Data width in bits
    size_t total_width_bits;            ///< Total width including ECC in bits
    size_t total_size_gb;               ///< Total memory size in GB
    size_t num_channels;                ///< Number of memory channels
    double theoretical_bandwidth_gbps;  ///< Theoretical bandwidth in GB/s
    bool is_virtualized;                ///< Whether the system is virtualized
    bool data_width_detected;           ///< Whether data width was detected from system
    bool total_width_detected;          ///< Whether total width was detected from system
    bool num_channels_detected;         ///< Whether number of channels was detected from system
    bool is_unified_memory;             ///< Whether using unified memory architecture (Apple Silicon)
    std::string architecture;           ///< Memory architecture description
};

/**
 * @brief System information structure
 *
 * Contains comprehensive system information including RAM, CPU,
 * memory specifications, and cache details.
 */
struct SystemInfo {
    size_t total_ram_gb;       ///< Total RAM in GB
    size_t available_ram_gb;   ///< Available RAM in GB
    size_t cpu_cores;          ///< Number of physical CPU cores
    size_t cpu_threads;        ///< Number of logical CPU threads
    size_t cache_line_size;    ///< Cache line size in bytes
    std::string cpu_name;      ///< CPU name/model
    MemorySpecs memory_specs;  ///< Memory specifications
    CacheInfo cache_info;      ///< Cache information
};

/**
 * @brief CPU affinity types for heterogeneous architectures
 */
enum class CPUAffinityType {
    DEFAULT,    ///< No specific affinity
    P_CORES,    ///< Performance cores only (Apple Silicon)
    E_CORES     ///< Efficiency cores only (Apple Silicon)
};

/**
 * @brief Cache line size constants
 * 
 * Centralized cache line size definitions to eliminate duplication
 * across platform implementations and test code.
 */
namespace CacheConstants {
    constexpr size_t DEFAULT_CACHE_LINE_SIZE = 64;      ///< Standard x86/ARM cache line size
    constexpr size_t APPLE_CACHE_LINE_SIZE = 128;       ///< Apple Silicon typical cache line size
    constexpr size_t MAX_CACHE_LINE_SIZE = 1024;        ///< Maximum reasonable cache line size
    constexpr size_t MIN_CACHE_LINE_SIZE = 32;          ///< Minimum reasonable cache line size
}

#endif  // MEMORY_TYPES_H