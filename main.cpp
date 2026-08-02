#include <iostream>
#include <string>
#include <vector>

#include "Cache.h"
#include "Trace.h"

int main() {
    const std::string trace_path{
    "/Users/pranavponnusamy/CLionProjects/CacheSim/short_linpack.trace"};

    Trace trace{trace_path};

    const std::vector<std::pair<Cache::Eviction_Policy, std::string>> eviction_policies{
        {Cache::FIFO, "FIFO"},
        {Cache::LRU, "LRU"},
        {Cache::LFU, "LFU"},
        {Cache::PLRU, "PLRU"},
        {Cache::Belady, "Belady"}
    };
    const std::vector<std::pair<Cache::Prefetch_Policy, std::string>> prefetch_policies{
        {Cache::STRIDED, "STRIDED"},
        {Cache::MARKOV, "MARKOV"}
    };
    const std::vector<std::pair<Cache::Admission_Policy, std::string>> admission_policies{
        {Cache::ADMIT_ALL, "ADMIT_ALL"},
        {Cache::REJECT_PREFETCH, "REJECT_PREFETCH"},
        {Cache::TINYLFU, "TINYLFU"}
    };

    std::cout << "eviction,prefetch,admission,l1_misses,l2_misses,l1_hits,l2_hits\n";

    for (const auto& [eviction_policy, eviction_name] : eviction_policies) {
        for (const auto& [prefetch_policy, prefetch_name] : prefetch_policies) {
            for (const auto& [admission_policy, admission_name] : admission_policies) {
                Cache l2{18, 6, 3, eviction_policy, prefetch_policy,
                          std::nullopt, &trace, admission_policy};
                Cache l1{14, 6, 2, eviction_policy, prefetch_policy,
                          l2, &trace, admission_policy};

                l1.run(trace_path);

                std::cout << eviction_name << ','
                          << prefetch_name << ','
                          << admission_name << ','
                          << l1.stats.misses << ','
                          << l2.stats.misses << ','
                          << l1.stats.hits << ','
                          << l2.stats.hits << '\n';
            }
        }
    }

    return 0;
}