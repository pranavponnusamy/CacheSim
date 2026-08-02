#include "Cache.h"

Cache::Split Cache::split_address(uint64_t mem_address) const {
    uint64_t tag{mem_address >> (m_C - m_S)};
    uint64_t index{(mem_address >> m_B) & ((1U << (m_C - m_B - m_S)) - 1)};
    uint64_t block_offset{mem_address & ((1U << m_B) - 1)};

    return Split{tag, index, block_offset};
}

void Cache::do_prefetch(const uint64_t mem_address) {
    std::vector<uint64_t> prefetched_addrs;
    prefetcher->generate_prefetch_addresses(mem_address, prefetched_addrs);

    for (const auto addr: prefetched_addrs) {
        cache_access(addr, false, false, true);
    }
}

Entry Cache::cache_access(const uint64_t mem_address, const bool RW,
                          const bool prefetch, const bool is_prefetch) {
    admission_policy->record_access(mem_address);
    Split split = split_address(mem_address);

    // hit
    for (auto it = sets[split.index].begin(); it != sets[split.index].end();
         ++it) {
        if (it->tag == split.tag) {
            if (RW) {
                switch (write_policy) {
                    case WTWNA:
                        stats.writes++;
                        stats.hits++;
                        if (m_upper_level.has_value()) {
                            m_upper_level.value().get().cache_access(
                                mem_address, true, false, is_prefetch);
                        }
                        break;
                    case WBWA:
                        stats.writes++;
                        stats.hits++;
                        it->dirty = true; // FIX: Update the actual entry in the cache
                        break;
                }
            } else {
                ++stats.reads;
                ++stats.hits;
            }

            Entry entry = *it;
            replacer->access(sets[split.index], split.tag, split.index);
            return entry;
        }
    }

    // miss
    Entry e{mem_address, split.tag, false, 0};
    if (write_policy == WBWA) {
        ++stats.misses;

        // only check above if we need to read
        if (!RW) {
            ++stats.reads;
        } else {
            ++stats.writes;
            e.dirty = true;
        }

        if (m_upper_level.has_value()) {
            auto &upper_level_cache = m_upper_level.value().get();
            upper_level_cache.cache_access(mem_address,
                                           false, false,
                                           is_prefetch); // Read from upper level
        }

        cache_repair(e, RW, mem_address, is_prefetch);
    } else {
        // if read
        if (!RW) {
            ++stats.reads;
            ++stats.misses;

            if (m_upper_level.has_value()) {
                auto &upper_level_cache = m_upper_level.value().get();
                upper_level_cache.cache_access(mem_address, false, false,
                                               is_prefetch);
            }

            cache_repair(e, RW, mem_address, is_prefetch);
        } else {
            ++stats.writes;
            ++stats.misses;

            if (m_upper_level.has_value()) {
                auto &upper_level_cache = m_upper_level.value().get();
                upper_level_cache.cache_access(mem_address, true);
            }
        }
    }

    if (prefetch && (!RW || write_policy == WBWA)) {
        do_prefetch(mem_address);
    }
    return e;
}

void Cache::write_back(Entry &victim, const bool RW,
                       const uint64_t mem_address) {
    (void)RW;
    // write back/write through
    switch (write_policy) {
        case WBWA: {
            if (m_upper_level.has_value()) {
                auto &upper_level_cache = m_upper_level.value().get();
                if (victim.dirty) {
                    // write to the upper level
                    upper_level_cache.cache_access(mem_address, true);
                }
            }
        }
        break;

        case WTWNA:
            break;
    }
}

void Cache::cache_repair(Entry &new_entry, const bool RW,
                         const uint64_t mem_address, const bool prefetch) {
    Split split = split_address(mem_address);
    Set &s = sets[split.index];
    Entry victim{};
    const Entry* victim_ptr = nullptr;
    if (s.size() == m_num_ways) {
        victim = replacer->peek(s, split.index, m_demand_index);
        victim_ptr = &victim;
    }

    if (!admission_policy->admit(new_entry, victim_ptr, prefetch)) {
        return;
    }

    // evict an entry
    if (s.size() == m_num_ways) {
        Entry victim = replacer->evict(s, split.index, m_demand_index);

        uint64_t victim_address{(victim.tag << (m_C - m_S)) | (split.index << m_B)};

        // no need to write back on eviction on WTNBA because cache is already
        // consistent
        if (write_policy == WBWA && victim.dirty) {
            write_back(victim, RW, victim_address);
        }
    }

    replacer->insert(s, new_entry, split.index);
}

void Cache::run(const std::string &trace_path) {
    std::ifstream input(trace_path);
    if (!input) {
        std::cerr << "Failed to open trace file: " << trace_path << '\n';
        return;
    }
    std::string line;
    uint64_t demand_index = 0;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        std::istringstream iss(line);
        char access{};
        uint64_t addr{};
        if (!(iss >> access >> std::hex >> addr)) {
            std::cerr << line << '\n';
            continue;
        }
        m_demand_index = demand_index;
        cache_access(addr, access == 'W', true);
        ++demand_index;
    }
}
