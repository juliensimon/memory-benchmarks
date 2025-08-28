#include "working_sets.h"
#include "constants.h"
#include <algorithm>

using namespace BenchmarkConstants;

WorkingSetSizes::WorkingSetSizes(const CacheInfo& cache_info) {
    const size_t min_size = MIN_WORKING_SET_SIZE;
    const size_t max_size = MAX_WORKING_SET_SIZE;

    // L1 cache working sets (per-core)
    std::vector<size_t> l1_sizes = {
        cache_info.l1_data_size / 8,
        cache_info.l1_data_size / 4,
        cache_info.l1_data_size / 2,
        cache_info.l1_data_size
    };

    std::vector<std::string> l1_descriptions = {
        "1/8 L1 cache",
        "1/4 L1 cache", 
        "1/2 L1 cache",
        "Full L1 cache"
    };

    // L2 cache working sets (per-core on Apple Silicon)
    std::vector<size_t> l2_sizes = {
        cache_info.l2_size / 8,
        cache_info.l2_size / 4,
        cache_info.l2_size / 2,
        cache_info.l2_size
    };

    std::vector<std::string> l2_descriptions = {
        "1/8 L2 cache",
        "1/4 L2 cache",
        "1/2 L2 cache", 
        "Full L2 cache"
    };

    // L3/SLC cache working sets (shared)
    std::vector<size_t> l3_sizes = {
        cache_info.l3_size / 8,
        cache_info.l3_size / 4,
        cache_info.l3_size / 2,
        cache_info.l3_size
    };

    std::vector<std::string> l3_descriptions = {
        "1/8 SLC",
        "1/4 SLC",
        "1/2 SLC",
        "Full SLC"
    };

    // Beyond cache sizes
    std::vector<size_t> beyond_cache_sizes = {
        cache_info.l3_size * WORKING_SET_MULTIPLIERS[0],  // 2x
        cache_info.l3_size * WORKING_SET_MULTIPLIERS[1],  // 4x
        cache_info.l3_size * WORKING_SET_MULTIPLIERS[2],  // 8x
        STANDARD_WORKING_SETS[0],  // 64MB
        STANDARD_WORKING_SETS[1],  // 128MB
        STANDARD_WORKING_SETS[2],  // 256MB
        STANDARD_WORKING_SETS[3],  // 512MB
        STANDARD_WORKING_SETS[4],  // 1GB
        STANDARD_WORKING_SETS[5],  // 2GB
        STANDARD_WORKING_SETS[6]   // 4GB
    };

    std::vector<std::string> beyond_descriptions = {
        "2x SLC", "4x SLC", "8x SLC",
        "64MB", "128MB", "256MB", "512MB",
        "1GB", "2GB", "4GB"
    };

    // Combine all sizes and descriptions
    sizes.insert(sizes.end(), l1_sizes.begin(), l1_sizes.end());
    descriptions.insert(descriptions.end(), l1_descriptions.begin(), l1_descriptions.end());
    
    sizes.insert(sizes.end(), l2_sizes.begin(), l2_sizes.end());
    descriptions.insert(descriptions.end(), l2_descriptions.begin(), l2_descriptions.end());
    
    sizes.insert(sizes.end(), l3_sizes.begin(), l3_sizes.end());
    descriptions.insert(descriptions.end(), l3_descriptions.begin(), l3_descriptions.end());
    
    sizes.insert(sizes.end(), beyond_cache_sizes.begin(), beyond_cache_sizes.end());
    descriptions.insert(descriptions.end(), beyond_descriptions.begin(), beyond_descriptions.end());

    // Filter out sizes that are too small or too large
    auto it_size = sizes.begin();
    auto it_desc = descriptions.begin();
    
    while (it_size != sizes.end()) {
        if (*it_size < min_size || *it_size > max_size) {
            it_size = sizes.erase(it_size);
            it_desc = descriptions.erase(it_desc);
        } else {
            ++it_size;
            ++it_desc;
        }
    }
}

std::pair<std::vector<size_t>, std::vector<std::string>> 
WorkingSetSizes::get_thread_aware_sizes(const CacheInfo& cache_info, size_t num_threads) {
    const size_t min_size = MIN_WORKING_SET_SIZE;
    const size_t max_size = MAX_WORKING_SET_SIZE;

    std::vector<size_t> sizes;
    std::vector<std::string> descriptions;

    // L1 cache working sets - L1 is per-core, give full cache to each thread
    size_t l1_per_thread = cache_info.l1_data_size;
    if(l1_per_thread / WORKING_SET_FRACTIONS[1] >= min_size) {
        sizes.push_back(l1_per_thread / 4);
        descriptions.push_back("1/4 L1 per thread");
    }
    if(l1_per_thread / WORKING_SET_FRACTIONS[2] >= min_size) {
        sizes.push_back(l1_per_thread / 2);
        descriptions.push_back("1/2 L1 per thread");
    }
    if(l1_per_thread >= min_size) {
        sizes.push_back(l1_per_thread);
        descriptions.push_back("L1 per thread");
    }

    // L2 cache working sets - L2 is per-core on Apple Silicon, give full cache to each thread
    size_t l2_per_thread = cache_info.l2_size;
    if(l2_per_thread / WORKING_SET_FRACTIONS[1] >= min_size) {
        sizes.push_back(l2_per_thread / 4);
        descriptions.push_back("1/4 L2 per thread");
    }
    if(l2_per_thread / WORKING_SET_FRACTIONS[2] >= min_size) {
        sizes.push_back(l2_per_thread / 2);
        descriptions.push_back("1/2 L2 per thread");
    }
    if(l2_per_thread >= min_size) {
        sizes.push_back(l2_per_thread);
        descriptions.push_back("L2 per thread");
    }

    // SLC/L3 cache working sets - shared cache, split across threads
    size_t l3_per_thread = cache_info.l3_size / num_threads;
    if(l3_per_thread / WORKING_SET_FRACTIONS[1] >= min_size) {
        sizes.push_back(l3_per_thread / 4);
        descriptions.push_back("1/4 SLC per thread");
    }
    if(l3_per_thread / WORKING_SET_FRACTIONS[2] >= min_size) {
        sizes.push_back(l3_per_thread / 2);
        descriptions.push_back("1/2 SLC per thread");
    }
    if(l3_per_thread >= min_size) {
        sizes.push_back(l3_per_thread);
        descriptions.push_back("SLC per thread");
    }

    // Beyond cache - multiples of L3/SLC
    std::vector<size_t> beyond_cache_sizes = {
        cache_info.l3_size * WORKING_SET_MULTIPLIERS[0],  // 2x
        cache_info.l3_size * WORKING_SET_MULTIPLIERS[1],  // 4x
        STANDARD_WORKING_SETS[0],  // 64MB
        STANDARD_WORKING_SETS[2],  // 256MB
        STANDARD_WORKING_SETS[4],  // 1GB
        STANDARD_WORKING_SETS[5],  // 2GB
        STANDARD_WORKING_SETS[6]   // 4GB
    };

    for(size_t size : beyond_cache_sizes) {
        if(size >= min_size && size <= max_size) {
            sizes.push_back(size);
            if(size == cache_info.l3_size * 2) {
                descriptions.push_back("2x SLC");
            } else if(size == cache_info.l3_size * 4) {
                descriptions.push_back("4x SLC");
            } else if(size == 64 * 1024 * 1024) {
                descriptions.push_back("64MB");
            } else if(size == 256 * 1024 * 1024) {
                descriptions.push_back("256MB");
            } else if(size == 1024 * 1024 * 1024) {
                descriptions.push_back("1GB");
            } else if(size == 2048ULL * 1024 * 1024) {
                descriptions.push_back("2GB");
            } else if(size == 4096ULL * 1024 * 1024) {
                descriptions.push_back("4GB");
            }
        }
    }

    return {sizes, descriptions};
}