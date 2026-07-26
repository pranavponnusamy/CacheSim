#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>

#include "Cache.h"
#include <unordered_set>

int main() {
    uint64_t init_val = 0;
    //
    for (int x = 8; x>0; x/=2) {
        init_val |= (1 << (x-1));
    }

    // std::cout << init_val ;

    std::unordered_set<int64_t> uset {};
    std::ifstream input("/Users/pranavponnusamy/CLionProjects/CacheSim/short_linpack.trace");
    if (!input) {
        std::cerr << "Failed to open trace file\n";
        return 1;
    }

    uint64_t C = 15, B = 7, S = 0;
    Cache l2{17, 7, 1, Cache::FIFO , std::nullopt};
    Cache l1{15, 8, 0,Cache::FIFO,  l2};

    // l1.replacer = new FIFO();
    // l2.replacer = new FIFO();

    std::string line{};
    int counter = 0;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream iss(line);

        char access{};
        uint64_t addr{};

        if (!(iss >> access >> std::hex >> addr)) {
            std::cerr << line << '\n';
            continue;
        }

        if (access == 'R') {
            l1.cache_access(addr, false, true);  // read
        } else if (access == 'W') {
            l1.cache_access(addr, true, true);   // write
        }
        // uset.insert(addr);
    }

    // std::cout << uset.size() << std::endl;
    std::cout << l1.stats.misses << std::endl;
    std::cout << l2.stats.misses << std::endl;
    return 0;
}