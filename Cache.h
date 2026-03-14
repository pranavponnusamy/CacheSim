//
// Created by pranavponnusamy on 3/13/26.
//

#ifndef CACHESIM_CACHE_H
#define CACHESIM_CACHE_H
#include <cstdint>
#include <vector>
#include <optional>

class Cache {

    enum Policy {
        WBWA,
        WTWNA
    };

    enum Eviction_Policy {
        FIFO,
        LRU,
        MRU
    };

    struct Stats {
        uint32_t reads {};
        uint32_t writes {};

        uint32_t hits {};
        int32_t misses {};
    };

    struct Entry {
        uint32_t tag {};
        bool valid {false};
        bool dirty {false};
    };

public:

    // C = 2^C total cache capacity (bytes)
    // B = 2^B block size (bytes)
    // S = 2^S sets Associativity

    uint32_t m_C {};
    uint32_t m_B {};
    uint32_t m_S {};

    uint32_t m_num_ways{};
    uint32_t m_num_sets{};

    Stats stats {};

    using set = std::vector<Entry>;
    std::vector<set> sets;

    Cache(uint32_t C, uint32_t B, uint32_t S): m_C{C}, m_B{B}, m_S{S}, m_num_ways{1U<<S}, m_num_sets{1U << (C-B-S)}{
        sets = std::vector<set>(m_num_sets, set(m_num_ways));
    }
    Entry cache_access(uint32_t mem_address,  bool RW, std::optional<std::reference_wrapper<Cache>> upper_level);

};

#endif //CACHESIM_CACHE_H