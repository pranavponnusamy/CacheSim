//
// Created by Pranav Ponnusamy on 7/28/26.
//

#include "Prefetcher_Policy.h"

#include <algorithm>

Strided_Prefetcher::Strided_Prefetcher(int prefetch_stride,
                                                                             int prefetch_degree,
                                                                             std::uint64_t block_bits)
        : stride(prefetch_stride), degree(prefetch_degree),
            block_bits(block_bits) {}

void Strided_Prefetcher::generate_prefetch_addresses(uint64_t mem_addr, std::vector<uint64_t> &generated_addresses) {
    for (int i = 1; i <= degree; ++i) {
        const auto block_distance =
            static_cast<std::uint64_t>(i * stride) << block_bits;
        generated_addresses.push_back(mem_addr + block_distance);
    }
}

void Strided_Prefetcher::update_config(int prefetch_stride, int prefetch_degree) {
    this->stride = prefetch_stride;
    this->degree = prefetch_degree;
}

Markov_Prefetcher::Markov_Prefetcher(int prefetch_degree, std::uint64_t block_bits)
    : degree(prefetch_degree), block_bits(block_bits) {}

void Markov_Prefetcher::generate_prefetch_addresses(uint64_t mem_addr, std::vector<uint64_t> &generated_addresses) {
    const uint64_t block = mem_addr >> block_bits;

    if (has_last) {
        ++transitions[last_block][block];
    }
    last_block = block;
    has_last = true;

    const auto it = transitions.find(block);
    if (it == transitions.end()) {
        return;
    }

    std::vector<std::pair<uint64_t, uint64_t>> candidates(
        it->second.begin(), it->second.end());
    std::sort(candidates.begin(), candidates.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    const int count = std::min<int>(degree, static_cast<int>(candidates.size()));
    for (int i = 0; i < count; ++i) {
        generated_addresses.push_back(candidates[i].first << block_bits);
    }
}
