#ifndef CACHESIM_CACHETYPES_H
#define CACHESIM_CACHETYPES_H

#include <cstdint>
#include <deque>

struct Entry {
    uint64_t addr {};
    uint64_t tag {};
    uint64_t counter {};
    bool dirty {false};
    // bool valid {false};
};
using Set = std::deque<Entry>;

#endif //CACHESIM_CACHETYPES_H