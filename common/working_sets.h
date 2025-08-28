#ifndef WORKING_SETS_H
#define WORKING_SETS_H

#include "memory_types.h"
#include <utility>
#include <vector>
#include <string>

/**
 * @brief Working set sizes for cache-aware testing
 *
 * Contains a comprehensive set of working set sizes that span
 * the entire cache hierarchy for detailed cache performance analysis.
 */
class WorkingSetSizes {
public:
    std::vector<size_t> sizes;              ///< Working set sizes in bytes
    std::vector<std::string> descriptions;  ///< Human-readable descriptions

    /**
     * @brief Constructor that initializes cache-aware test sizes
     * @param cache_info Reference to cache information structure
     */
    explicit WorkingSetSizes(const CacheInfo& cache_info);

    /**
     * @brief Get working set sizes adjusted for thread count
     * @param cache_info Reference to cache information structure
     * @param num_threads Number of threads to use
     * @return Pair of sizes and descriptions adjusted for thread count
     */
    static std::pair<std::vector<size_t>, std::vector<std::string>> 
    get_thread_aware_sizes(const CacheInfo& cache_info, size_t num_threads);
};

#endif  // WORKING_SETS_H