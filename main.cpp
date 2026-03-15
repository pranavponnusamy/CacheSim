#include <iostream>
#include "Cache.h"
#include <vector>

int main() {
    uint32_t C = 15, B=15, S=0;
    Cache l2 {C+1, B, S, std::nullopt};
    Cache l1 {C, B, S, l2};

    std::vector<uint32_t> s {0x1000000, 0x2000000, 0x1000000};
    std::vector<uint32_t> r {0, 0, 1};

    for (int x = 0; x<s.size(); x++) {
        l1.cache_access(s[x], r[x]);
    }

    return 0;

}
