#ifndef CACHESIM_REPLACEMENT_POLICIY_H
#define CACHESIM_REPLACEMENT_POLICIY_H
#include <unordered_map>
#include <vector>
#include <CacheTypes.h>
#include "Trace.h"
#include <algorithm>
#include <limits>


static constexpr int MAX_WAYS = 16;

class Replacement_Policy {
public:
    virtual ~Replacement_Policy() = default;

    virtual void access(Set& s, uint64_t tag, uint64_t index) = 0;
    virtual void insert(Set& s, const Entry& e, uint64_t index) = 0;
    virtual Entry peek(const Set& s, uint64_t index, uint64_t now) const = 0;
    virtual Entry evict(Set& s, uint64_t index, uint64_t now) = 0;
};

class FIFO: public Replacement_Policy {
public:
    void access(Set& s, uint64_t tag, uint64_t index) override;
    void insert(Set& s, const Entry& e, uint64_t index) override;
    Entry evict(Set& s, uint64_t index, uint64_t now) override;
    Entry peek(const Set& s, uint64_t index, uint64_t now) const override;
};

class LRU: public Replacement_Policy {
public:
    void access(Set& s, uint64_t tag, uint64_t index) override;
    void insert(Set& s, const Entry& e, uint64_t index) override;
    Entry evict(Set& s, uint64_t index, uint64_t now) override;
    Entry peek(const Set& s, uint64_t index, uint64_t now) const override;
};

class LFU: public Replacement_Policy {
private:
    std::unordered_map<uint64_t, std::unordered_map<uint64_t, uint64_t>> m;
public:
    void access(Set& s, uint64_t tag, uint64_t index) override;
    void insert(Set& s, const Entry& e, uint64_t index) override;
    Entry evict(Set& s, uint64_t index, uint64_t now) override;
    Entry peek(const Set& s, uint64_t index, uint64_t now) const override;
};

//this needs to be tree based
class PLRU: public Replacement_Policy {
private:
    std::vector<std::vector<bool>> trees;
    uint64_t m_ways;
public:
    PLRU () = delete;
    PLRU(uint64_t num_sets, uint64_t num_ways);

    void access(Set& s, uint64_t tag, uint64_t index) override;
    void insert(Set& s, const Entry& e, uint64_t index) override;
    Entry evict(Set& s, uint64_t index, uint64_t now) override;
    Entry peek(const Set& s, uint64_t index, uint64_t now) const override;
};


class Belady: public Replacement_Policy {
private:
    const Trace* m_trace;
public:
    Belady() = delete;
    explicit Belady(const Trace* trace);

    void access(Set& s, uint64_t tag, uint64_t index) override;
    void insert(Set& s, const Entry& e, uint64_t index) override;
    Entry evict(Set& s, uint64_t index, uint64_t now) override;
    Entry peek(const Set& s, uint64_t index, uint64_t now) const override;
};
#endif //CACHESIM_REPLACEMENT_POLICIY_H