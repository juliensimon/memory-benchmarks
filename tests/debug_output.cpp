#include "test_framework.h"
#include "common/output_formatter.h"
#include "common/memory_types.h"
#include <iostream>

int main() {
    OutputFormatter formatter(OutputFormat::MARKDOWN);
    
    std::vector<TestResult> results;
    TestResult result = {};
    result.test_name = "sequential_read";
    result.pattern_name = "sequential_read";
    result.working_set_desc = "1GB";
    result.num_threads = 8;
    result.stats.bandwidth_gbps = 45.67;
    result.stats.latency_ns = 12.34;
    result.stats.bytes_processed = 1000000000;
    result.stats.time_seconds = 1.0;
    
    results.push_back(result);
    
    MemorySpecs mem_specs = {};
    mem_specs.theoretical_bandwidth_gbps = 50.0;
    
    std::string output = formatter.format_test_results(results, mem_specs);
    std::cout << "Output:
" << output << std::endl;
    
    return 0;
}
