#ifndef CACHESIM_REPLACEMENT_POLICIY_H
#define CACHESIM_REPLACEMENT_POLICIY_H
#include <unordered_map>
#include <CacheTypes.h>

class Replacement_Policy {
public:
    virtual ~Replacement_Policy() = default;

    virtual void access(Set& s, uint64_t tag) = 0;
    virtual void insert(Set& s, uint64_t tag) = 0;
    virtual Entry evict(Set& s) = 0;
};

class FIFO: public Replacement_Policy {
public:
    void access(Set& s, uint64_t tag) override;
    void insert(Set& s, uint64_t tag) override;
    Entry evict(Set& s) override;
};

class LRU: public Replacement_Policy {
    std::unordered_map<uint64_t, int> m {};
public:
    void access(Set& s, uint64_t tag);
    void insert(Set& s, uint64_t tag);
    Entry evict(Set& s);
};

class LFU: public Replacement_Policy {
    void access(Set& s, uint64_t tag);
    void insert(Set& s, uint64_t tag);
    Entry evict(Set& s);
};

#endif //CACHESIM_REPLACEMENT_POLICIY_H