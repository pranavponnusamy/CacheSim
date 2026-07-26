#include <Replacement_Policiy.h>
#include <Cache.h>

#define MAX_WAYS 16;

void FIFO::access(Set &s, uint64_t tag, uint64_t index) {
}

void FIFO::insert(Set &s, const Entry& e, uint64_t index) {
    s.push_back(e);
}

Entry FIFO::evict(Set &s, uint64_t index) {
    Entry front = s.front();
    s.pop_front();

    return front;
}

void LRU::access(Set &s, uint64_t tag, uint64_t index) {
    Entry temp;
    for (int x = 0; x < s.size(); x++) {
        if (s[x].tag == tag) {
            temp = s[x];
            s.erase(s.begin() + x);
            break;
        }
    }

    s.push_back(temp);
}

Entry LRU::evict(Set &s, uint64_t index) {
    Entry temp = s.front();
    s.pop_front();
    return temp;
}

void LRU::insert(Set &s, const Entry& e, uint64_t index) {
    s.push_back(e);
}

void LFU::access(Set &s, uint64_t tag, uint64_t index) {
    ++this->m[index][tag]; //just need to increment
}

void LFU::insert(Set &s, const Entry& e, uint64_t index) {
    m[index][e.tag] = 1;
    s.push_back(e);
}

Entry LFU::evict(Set &s, uint64_t index) {
    int least_idx = 0;

    //ensures that we are evicting in FIFO order in case of ties
    for (int x = 0; x < s.size(); x++) {
        if (m[index][s[x].tag] < m[index][s[least_idx].tag]) {
            least_idx = x;
        }
    }

    auto temp = s[least_idx];
    m[index].erase(s[least_idx].tag);
    s.erase(s.begin() + least_idx);

    return temp;
}

//this makes it such that all the nodes are pointing to the left
PLRU::PLRU(uint64_t num_sets, uint64_t num_ways) : trees(num_sets), m_ways(num_ways) {
}

//note that index is actually set index
void PLRU::access(Set &s, uint64_t tag, uint64_t index) {
    int way = -1;
    for (int x = 0; x < s.size(); x++) {
        if (s[x].tag == tag) {
            way = x;
        }
    }

    uint64_t node = (m_ways - 1) + way;

    while (node > 0) {
        uint64_t parent = (node - 1) / 2;
        bool cameFromLeft = (node == 2 * parent + 1);
        trees[index][parent] = cameFromLeft ? 1 : 0;
        node = parent;
    }
}

void PLRU::insert(Set &s, const Entry& e, uint64_t index) {
    if (s.size() != m_ways) {
        s.push_back(e);
    } else {
        int node = 0;

        while (node < (int) (m_ways - 1)) {
            node = trees[index][node] ? (node * 2 + 2) : (node * 2 + 1);
        }
        int way = node - (m_ways - 1);

        s[way] = e;
    }

    access(s, e.tag, index);
}

Entry PLRU::evict(Set &s, uint64_t index) {
    int node = 0;

    while (node < (int) (m_ways - 1)) {
        node = trees[index][node] ? (node * 2 + 2) : (node * 2 + 1);
    }
    int way = node - (m_ways-1);

    Entry victim = s[way];

    return victim;
}
