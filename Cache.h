#ifndef CACHESIM_CACHE_H
#define CACHESIM_CACHE_H
#include <cstdint>
#include <deque>
#include <vector>
#include <optional>
#include <algorithm>

class Cache {

    enum Write_Policy {
        WBWA,
        WTWNA
    };

    enum Eviction_Policy {
        FIFO,
        LRU,
        MFU
    };

    struct Stats {
        uint32_t reads {};
        uint32_t writes {};

        uint32_t hits {};
        int32_t misses {};
    };

    struct Entry {
        uint32_t tag {};
        // bool valid {false};
        bool dirty {false};
        uint32_t counter {};
    };

    struct Split {
        uint32_t tag;
        uint32_t index;
        uint32_t block_offset;
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

    using Set = std::deque<Entry>;
    std::vector<Set> sets;

    Eviction_Policy eviction_policy {};
    Write_Policy write_policy{WBWA};
    const std::optional<std::reference_wrapper<Cache>> m_upper_level;

    Cache(uint32_t C, uint32_t B, uint32_t S, std::optional<std::reference_wrapper<Cache>> upper_level): m_C{C}, m_B{B}, m_S{S}, m_num_ways{1U << (C-B-S)}, m_num_sets{1U<<S}, m_upper_level(upper_level){
     sets = std::vector<Set>(m_num_sets, Set(0));
    }

    Split split_address (const uint32_t mem_address) const;

    Entry cache_access(uint32_t mem_address, bool RW);

    void write_back(Entry &victim, bool RW, uint32_t mem_address);

    void cache_repair(Entry new_entry, bool RW, uint32_t mem_address) ;
};

#endif //CACHESIM_CACHE_H