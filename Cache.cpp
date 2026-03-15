#include "Cache.h"

Cache::Split Cache::split_address(uint32_t mem_address) const {
    uint32_t tag{mem_address >> (m_B + m_S)};
    uint32_t index{(mem_address >> m_B) & ((1U << (m_C-m_B-m_S)) - 1)};
    uint32_t block_offset{mem_address & ((1U << m_B) - 1)};

    return Split{tag, index, block_offset};
}


Cache::Entry Cache::cache_access(const uint32_t mem_address, const bool RW, bool update) {
    Split split = split_address(mem_address);

    if (RW) {
        ++stats.writes;
    } else {
        ++stats.reads;
    }

    // hit
    for (auto& entry: sets[split.index]) {
        if (entry.tag == split.tag) {
            ++stats.hits;
            if (RW) {
                switch (write_policy) {
                    case WTWNA:
                        write_back(entry, RW, mem_address);
                        break;
                    case WBWA:
                        entry.dirty = true;
                        break;
                }
            }

            if (eviction_policy == MFU) {
                entry.counter++;
            }

            return entry;
        }
    }

    //miss
    if (!update) {
        ++stats.misses;
        Entry e = Entry{split.tag, false, 0};
        if (m_upper_level.has_value()) {
            auto &upper_level_cache = m_upper_level.value().get();
            e = upper_level_cache.cache_access(mem_address, RW);
        }

        cache_repair(e, RW, mem_address);
        return e; // went all the way to memory
    }
}

void Cache::write_back(Entry &victim, const bool RW, const uint32_t mem_address) {
    Split split = split_address(mem_address);

    // write back/write through
    int32_t idx{-1};
    Cache *upper_level_cache{nullptr};
    Set *s{nullptr};
    if (m_upper_level.has_value()) {
        upper_level_cache = &m_upper_level.value().get();
        s = &upper_level_cache->sets[split.index];
        for (int x = 0; x < s->size(); x++) {
            if (s->at(x).tag == victim.tag) {
                idx = x;
            }
        }
    }


    switch (write_policy) {
        case WBWA:
            if (upper_level_cache != nullptr) {
                if (idx != -1) {
                    // if idx found, simply update dirty bit
                    s->at(idx).dirty = true;
                } else {
                    // force update to the next cache level
                    upper_level_cache->cache_repair(victim, RW, mem_address);
                }
            }
            break;

        case WTWNA:
            if (upper_level_cache != nullptr) {
                // update if it's already in the cache
                // upper_level_cache->write_back(victim, RW, mem_address);
                upper_level_cache->cache_access(mem_address, RW);
            }
            break;
    }
}

void Cache::cache_repair(Entry& new_entry, const bool RW, const uint32_t mem_address) {
    Split split = split_address(mem_address);
    Set& s = sets[split.index];

    //evict an entry
    if (sets.size() == m_num_ways) {
        Entry &victim = s[0];
        s.pop_front();

        write_back(victim, RW, mem_address);
        ++stats.writes;
    }

    //add new_entry
    switch (eviction_policy) {
        case LRU:
            s.push_back(new_entry);
            break;
        case FIFO:
            s.push_back(new_entry);
            break;
        case MFU:
            s.push_back(new_entry);
            ++new_entry.counter;
            std::ranges::sort(s, [](const
                                    Entry &a, const Entry &b) {
                return a.counter < b.counter;
            });
            break;
    }
}
