#include <iostream>
#include "Cache.h"
#include <vector>

int main() {
    uint32_t C = 20, B=5, S=3;
    Cache l2 {C, B, S, std::nullopt};
    Cache l1 {C, B, S, l2};

    std::vector<uint32_t> s {0x1000000, 0x1000000};

    for (auto mem_address: s) {
        l1.cache_access(mem_address, 1);
    }

    return 0;

}