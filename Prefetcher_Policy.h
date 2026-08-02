//
// Created by Pranav Ponnusamy on 7/28/26.
//

#ifndef CACHESIM_PREFETCHER_POLICY_H
#define CACHESIM_PREFETCHER_POLICY_H
#include <cstdint>
#include <unordered_map>
#include <vector>


class Prefetcher {
public:
    virtual ~Prefetcher() = default;
    virtual void generate_prefetch_addresses(uint64_t mem_addr, std::vector<uint64_t>& generated_addresses) = 0;
};

class Strided_Prefetcher: public Prefetcher {
private:
    int stride, degree;
    std::uint64_t block_bits;
public:
    Strided_Prefetcher(int prefetch_stride, int prefetch_degree,
                       std::uint64_t block_bits);

    void generate_prefetch_addresses(uint64_t mem_addr,  std::vector<uint64_t>& generated_addresses) override;
    void update_config(int prefetch_stride, int prefetch_degree);
};

// Learns first-order address transitions (block[i] -> block[i+1]) and
// prefetches the most frequently observed successor blocks of the current
// access.
class Markov_Prefetcher: public Prefetcher {
private:
    std::unordered_map<uint64_t, std::unordered_map<uint64_t, uint64_t>> transitions;
    uint64_t last_block{};
    bool has_last{false};
    int degree;
    std::uint64_t block_bits;
public:
    Markov_Prefetcher(int prefetch_degree, std::uint64_t block_bits);

    void generate_prefetch_addresses(uint64_t mem_addr, std::vector<uint64_t>& generated_addresses) override;
};

#endif //CACHESIM_PREFETCHER_POLICY_H
