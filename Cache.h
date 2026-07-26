#ifndef CACHESIM_CACHE_H
#define CACHESIM_CACHE_H
#include <cstdint>
#include <deque>
#include <vector>
#include <optional>
#include <algorithm>
#include <CacheTypes.h>
#include <Replacement_Policiy.h>

class Replacement_Policy;

class Cache {
    enum Write_Policy {
        WBWA,
        WTWNA
    };


    enum Admission_Policy {
        //we should do something here!
    };

    enum Prefetch_Policy {
        STRIDED,
        MARKOV //might do this???
    };

    struct Stats {
        uint64_t reads{};
        uint64_t writes{};

        uint64_t hits{};
        int64_t misses{};
    };

    struct Split {
        uint64_t tag;
        uint64_t index;
        uint64_t block_offset;
    };

public:

    enum Eviction_Policy {
        FIFO,
        LRU,
        LFU,
        PLRU,
        ARC,
        Belady,
    };
    Replacement_Policy *replacer;

    // C = 2^C total cache capacity (bytes)
    // B = 2^B block size (bytes)
    // S = 2^S sets Associativity

    uint64_t m_C{};
    uint64_t m_B{};
    uint64_t m_S{};

    uint64_t m_num_ways{};
    uint64_t m_num_sets{};

    Stats stats{};

    std::vector<Set> sets;


    Eviction_Policy eviction_policy{LRU};
    Write_Policy write_policy{WBWA};
    Prefetch_Policy prefetch_policy{STRIDED};
    const std::optional<std::reference_wrapper<Cache> > m_upper_level;

    uint64_t prefectch_stride{1};
    uint64_t prefetch_degree{1};

    Cache(uint64_t C, uint64_t B, uint64_t S, Eviction_Policy r_policy, std::optional<std::reference_wrapper<Cache> > upper_level) : m_C{C},
        m_B{B}, m_S{S}, m_num_ways{1U << S}, m_num_sets{1U << (C - B - S)}, m_upper_level(upper_level) {
        sets = std::vector<Set>(m_num_sets, Set(0));

        switch (r_policy) {
            case FIFO:
                replacer = new ::FIFO();
            break;
            case LRU:
                replacer = new ::LRU;
                break;
            case LFU:
                replacer = new ::LFU();
                break;
            case PLRU:
                replacer = new ::PLRU(m_num_sets, m_num_ways);
                break;
            case ARC:
                break;
            case Belady:
                break;
        }

    }

    Split split_address(uint64_t mem_address) const;

    void do_prefetch(uint64_t mem_address);

    Entry cache_access(uint64_t mem_address, bool RW, bool prefetch = false);

    void write_back(Entry &victim, bool RW, uint64_t mem_address);

    void cache_repair(Entry &new_entry, bool RW, uint64_t mem_address);

    friend Replacement_Policy;
};

#endif //CACHESIM_CACHE_H
