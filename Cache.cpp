#include "Cache.h"

Cache::Split Cache::split_address(uint64_t mem_address) const {
  uint64_t tag{mem_address >> (m_C - m_S)};
  uint64_t index{(mem_address >> m_B) & ((1U << (m_C - m_B - m_S)) - 1)};
  uint64_t block_offset{mem_address & ((1U << m_B) - 1)};

  return Split{tag, index, block_offset};
}

void Cache::do_prefetch(uint64_t mem_address) {
  switch (prefetch_policy) {
  case STRIDED:
    for (int i = 0; i < prefetch_degree; i++) {
      uint64_t incremented_address =
          mem_address + (((i + 1) * prefectch_stride) << m_B);
      cache_access(incremented_address, false);
    }
    break;

  case MARKOV:
    break;
  }
}

Cache::Entry Cache::cache_access(const uint64_t mem_address, const bool RW,
                                 const bool prefetch) {
  Split split = split_address(mem_address);

  // hit
  for (auto it = sets[split.index].begin(); it != sets[split.index].end();
       ++it) {
    if (it->tag == split.tag) {
      Entry entry = *it;
      if (RW) {
        switch (write_policy) {
        case WTWNA:
          stats.writes++;
          stats.hits++;
          if (m_upper_level.has_value()) {
            m_upper_level.value().get().cache_access(mem_address, true);
          }
          break;
        case WBWA:
          stats.writes++;
          stats.hits++;
          entry.dirty = true;
          break;
        }
      } else {
        ++stats.reads;
        ++stats.hits;
        if (eviction_policy == LFU) {
          entry.counter++;
        }
      }

      switch (eviction_policy) {
      case LRU:
        sets[split.index].erase(it); // linear scan is expensive
        sets[split.index].push_back(entry);
        break;

      case LFU:
        *it = entry;
        std::ranges::sort(sets[split.index],
                          [](const Entry &a, const Entry &b) {
                            return a.counter > b.counter;
                          });
        break;

      default:
        *it = entry;
        break;
      }

      return entry;
    }
  }

  // miss
  Entry e{split.tag, false, 0};
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
                                     false); // Read from upper level
    }

    cache_repair(e, RW, mem_address);
  } else {
    // if read
    if (!RW) {
      ++stats.reads;
      ++stats.misses;

      if (m_upper_level.has_value()) {
        auto &upper_level_cache = m_upper_level.value().get();
        upper_level_cache.cache_access(mem_address, false);
      }

      cache_repair(e, RW, mem_address);
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
  } break;

  case WTWNA:
    break;
  }
}

void Cache::cache_repair(Entry &new_entry, const bool RW,
                         const uint64_t mem_address) {
  Split split = split_address(mem_address);
  Set &s = sets[split.index];

  // evict an entry
  if (s.size() == m_num_ways) {
    Entry victim = s[0];
    s.pop_front();

    uint64_t victim_address{(victim.tag << (m_C - m_S)) | (split.index << m_B)};

    // no need to write back on eviction on WTNBA because cache is already
    // consistent
    if (write_policy == WBWA && victim.dirty) {
      write_back(victim, RW, victim_address);
    }
  }

  // add new_entry
  switch (eviction_policy) {
  case LRU:
    s.push_back(new_entry);
    break;
  case FIFO:
    s.push_back(new_entry);
    break;
  case LFU:
    ++new_entry.counter;
    s.push_back(new_entry);
    std::ranges::sort(s, [](const Entry &a, const Entry &b) {
      return a.counter > b.counter;
    });
    break;
  }
}
