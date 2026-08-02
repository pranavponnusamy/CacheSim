#ifndef CACHESIM_CACHE_H
#define CACHESIM_CACHE_H
#include <cstdint>
#include <deque>
#include <vector>
#include <optional>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <CacheTypes.h>
#include <Replacement_Policy.h>
#include "Admission_Policy.h"
#include "Prefetcher_Policy.h"

#define PREFETCH_DEGREE 1
#define PREFETCH_STRIDE 1


class Replacement_Policy;
class AdmissionPolicy;
class Prefetcher;
class Trace;

class Cache {
    enum Write_Policy {
        WBWA,
        WTWNA
    };


    struct Stats {
        uint64_t reads{};
        uint64_t writes{};

        uint64_t hits{};
        uint64_t misses{};
    };

    struct Split {
        uint64_t tag;
        uint64_t index;
        uint64_t block_offset;
    };

public:
    enum Admission_Policy {
        ADMIT_ALL,
        REJECT_PREFETCH,
        TINYLFU
    };

    enum Prefetch_Policy {
        STRIDED,
        MARKOV //might do this???
    };

    enum Eviction_Policy {
        FIFO,
        LRU,
        LFU,
        PLRU,
        ARC,
        Belady,
    };
    std::unique_ptr<Replacement_Policy> replacer;
    std::unique_ptr<AdmissionPolicy> admission_policy;
    std::unique_ptr<Prefetcher> prefetcher;

    // C = 2^C total cache capacity (bytes)
    // B = 2^B block size (bytes)
    // S = 2^S sets Associativity

    uint64_t m_C{};
    uint64_t m_B{};
    uint64_t m_S{};

    uint64_t m_num_ways{};
    uint64_t m_num_sets{};

    uint64_t m_demand_index{};

    Stats stats{};

    std::vector<Set> sets;


    Eviction_Policy eviction_policy{LRU};
    Write_Policy write_policy{WBWA};
    Prefetch_Policy prefetch_policy{STRIDED};
    const std::optional<std::reference_wrapper<Cache> > m_upper_level;
    const Trace* m_trace{nullptr};

    Cache(uint64_t C, uint64_t B, uint64_t S, Eviction_Policy r_policy,
          Prefetch_Policy p_policy,
          std::optional<std::reference_wrapper<Cache> > upper_level,
          const Trace* trace = nullptr,
          Admission_Policy a_policy = ADMIT_ALL) : m_C{C},
        m_B{B}, m_S{S}, m_num_ways{1U << S}, m_num_sets{1U << (C - B - S)}, eviction_policy(r_policy), m_upper_level(upper_level), m_trace(trace) {
        sets = std::vector<Set>(m_num_sets, Set(0));

        switch (r_policy) {
            case FIFO:
                replacer = std::make_unique<::FIFO>();
                break;
            case LRU:
                replacer = std::make_unique<::LRU>();
                break;
            case LFU:
                replacer = std::make_unique<::LFU>();
                break;
            case PLRU:
                replacer = std::make_unique<::PLRU>(m_num_sets, m_num_ways);
                break;
            case ARC:
                break;
            case Belady:
                replacer = std::make_unique<::Belady>(m_trace);
                break;
        }

        switch (p_policy) {
            case STRIDED:
                prefetcher = std::make_unique<Strided_Prefetcher>(
                    PREFETCH_STRIDE, PREFETCH_DEGREE, m_B);
                break;
            case MARKOV:
                prefetcher = std::make_unique<Markov_Prefetcher>(
                    PREFETCH_DEGREE, m_B);
                break;
        }

        switch (a_policy) {
            case ADMIT_ALL:
                admission_policy = std::make_unique<AdmitAll>();
                break;
            case REJECT_PREFETCH:
                admission_policy = std::make_unique<RejectPrefetch>();
                break;
            case TINYLFU:
                admission_policy = std::make_unique<TinyLFU>(
                    m_B, m_num_sets * m_num_ways);
                break;
        }
    }

    Split split_address(uint64_t mem_address) const;

    void do_prefetch(uint64_t mem_address);

    Entry cache_access(uint64_t mem_address, bool RW, bool prefetch = false,
                       bool is_prefetch = false);

    void write_back(Entry &victim, bool RW, uint64_t mem_address);

    void cache_repair(Entry &new_entry, bool RW, uint64_t mem_address,
                      bool prefetch = false);

    // Drive this cache from a trace file. Each line is "<R|W> <hex_addr>".
    // Lines that fail to parse are skipped (with a diagnostic to stderr).
    void run(const std::string& trace_path);

    friend Replacement_Policy;
};

#endif //CACHESIM_CACHE_H
