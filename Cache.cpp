//
// Created by pranavponnusamy on 3/14/26.
//


#include "Cache.h"


Cache::Entry Cache::cache_access(const uint32_t mem_address, const bool RW, const std::optional<std::reference_wrapper<Cache>> upper_level) {
    uint32_t tag {mem_address >> (m_B + m_S)};
    uint32_t index {(mem_address >> m_B) & ((1U << m_S) - 1)};
    uint32_t block_offset {mem_address & ((1U << m_B) - 1)};

    if (RW) {
        ++stats.reads;
    } else {
        ++stats.writes;
    }

    // For read
    for (auto entry: sets[index]) {
        if (entry.tag == tag) {
            ++stats.hits;
        } else {
            if (upper_level.has_value()) {
                Entry e = upper_level.value().get().cache_access(mem_address, RW, std::nullopt);
                // we need to evict
            }
            ++stats.misses;
        }
    }



}
