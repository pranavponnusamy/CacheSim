#include "Cache.h"

#include <algorithm>
#include <unistd.h>

Cache::Split Cache::split_address(uint32_t mem_address) const {
  uint32_t tag{mem_address >> (m_B + m_S)};
  uint32_t index{(mem_address >> m_B) & ((1U << m_S) - 1)};
  uint32_t block_offset{mem_address & ((1U << m_B) - 1)};

  return Split{tag, index, block_offset};
}

Cache::Entry Cache::cache_access(const uint32_t mem_address, const bool RW) {
  Split split = split_address(mem_address);

  // hit
  for (auto it = sets[split.index].begin(); it != sets[split.index].end();
       ++it) {
    if (it->tag == split.tag) {
      Entry entry = *it;
      if (RW) {
        switch (write_policy) {
        case WTWNA:
          write_back(entry, RW, mem_address);
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
        if (eviction_policy == MFU) {
          entry.counter++;
        }
      }

      //update the queue
      if (eviction_policy == LRU) {
        sets[split.index].erase(it);
        sets[split.index].push_back(entry);
      } else if (eviction_policy == MFU) {
        sets[split.index].erase(it);
        sets[split.index].push_back(entry);
        std::ranges::sort(sets[split.index], [](const Entry &a,
                                                const Entry &b) {
          return a.counter >
                 b.counter;
        });
      } else {
        //fifo
        *it = entry;
      }

      return entry;
    }
  }

  // miss
  if (write_policy == WBWA) {
    if (RW) {
      ++stats.writes;
    } else {
      ++stats.reads;
    }

    Entry e{split.tag, false, 0};
    ++stats.misses;

    // only check above if we need to read
    if (!RW) {
      if (m_upper_level.has_value()) {
        auto &upper_level_cache = m_upper_level.value().get();
        auto temp = upper_level_cache.cache_access(
            mem_address, false); // Read from upper level
                                 // e.dirty = temp.dirty;
      }
    } else {
      e.dirty = true;
    }

    cache_repair(e, RW, mem_address);
    return e; // went all the way to memory
  } else {
    //if read
    if (!RW) {
      ++stats.reads;
      ++stats.misses;

      Entry e{split.tag, false, 0};

      if (m_upper_level.has_value()) {
        auto &upper_level_cache = m_upper_level.value().get();
        upper_level_cache.cache_access(mem_address, RW);
      }

      cache_repair(e, RW, mem_address);
      return e; // went all the way to memory
    } else {
      Entry e{split.tag, false, 0};
      write_back(e, RW, mem_address);
      return e; // value is pretty much discarded
    }
  }
}

void Cache::write_back(Entry &victim, const bool RW,
                       const uint32_t mem_address) {
  // write back/write through
  switch (write_policy) {
  case WBWA: {
    if (m_upper_level.has_value()) {
      auto &upper_level_cache = m_upper_level.value().get();
      if (victim.dirty) {
        //write to the upper level
        upper_level_cache.cache_access(mem_address, true);
      }
    }
  } break;

  case WTWNA:
    std::optional<std::reference_wrapper<Cache>> curr = *this;
    while (curr.has_value()) {
      Split curr_split{curr->get().split_address(mem_address)};
      for (Entry &e : curr->get().sets[curr_split.index]) {
        if (e.tag == curr_split.tag) {
          if (curr->get().eviction_policy == MFU) {
            ++e.counter;
            std::ranges::sort(curr->get().sets[curr_split.index],
                              [](const Entry &a, const Entry &b) {
                                return a.counter > b.counter;
                              });
          }
          ++curr->get().stats.hits;
          ++curr->get().stats.writes;
        }
      }
      curr = curr.value().get().m_upper_level;
    }
    break;
  }
}

void Cache::cache_repair(Entry &new_entry, const bool RW,
                         const uint32_t mem_address) {
  Split split = split_address(mem_address);
  Set &s = sets[split.index];

  // evict an entry
  if (s.size() == m_num_ways) {
    Entry victim = s[0];
    s.pop_front();

    uint32_t victim_address{(victim.tag << (m_B + m_S)) | (split.index << m_B)};

    // no need to write back on eviction on WTNBA because cache is already consistent
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
  case MFU:
    s.push_back(new_entry);
    ++new_entry.counter;
    std::ranges::sort(s, [](const Entry &a, const Entry &b) {
      return a.counter > b.counter;
    });
    break;
  }
}
