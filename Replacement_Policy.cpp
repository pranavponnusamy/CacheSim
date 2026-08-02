#include <Replacement_Policy.h>


void FIFO::access(Set &s, uint64_t tag, uint64_t index) {
    (void)s;
    (void)tag;
    (void)index;
}

void FIFO::insert(Set &s, const Entry& e, uint64_t index) {
    (void)index;
    s.push_back(e);
}

Entry FIFO::evict(Set &s, uint64_t index, uint64_t now) {
    (void)index;
    (void)now;
    Entry front = s.front();
    s.pop_front();

    return front;
}

Entry FIFO::peek(const Set& s, uint64_t index, uint64_t now) const {
    (void)index;
    (void)now;
    return s.front();
}

void LRU::access(Set &s, uint64_t tag, uint64_t index) {
    (void)index;
    Entry temp;
    for (std::size_t x = 0; x < s.size(); x++) {
        if (s[x].tag == tag) {
            temp = s[x];
            s.erase(s.begin() + x);
            break;
        }
    }

    s.push_back(temp);
}

Entry LRU::evict(Set &s, uint64_t index, uint64_t now) {
    (void)index;
    (void)now;
    Entry temp = s.front();
    s.pop_front();
    return temp;
}

Entry LRU::peek(const Set& s, uint64_t index, uint64_t now) const {
    (void)index;
    (void)now;
    return s.front();
}

void LRU::insert(Set &s, const Entry& e, uint64_t index) {
    (void)index;
    s.push_back(e);
}

void LFU::access(Set &s, uint64_t tag, uint64_t index) {
    (void)s;
    ++m[index][tag];
}

void LFU::insert(Set &s, const Entry& e, uint64_t index) {
    m[index][e.tag] = 1;
    s.push_back(e);
}

Entry LFU::evict(Set &s, uint64_t index, uint64_t now) {
    Entry victim = peek(s, index, now);
    for (auto it = s.begin(); it != s.end(); ++it) {
        if (it->tag == victim.tag) {
            m[index].erase(it->tag);
            s.erase(it);
            break;
        }
    }
    return victim;
}

Entry LFU::peek(const Set& s, uint64_t index, uint64_t now) const {
    (void)now;
    int least_idx = 0;
    for (int x = 1; x < static_cast<int>(s.size()); ++x) {
        if (m.at(index).at(s[x].tag) < m.at(index).at(s[least_idx].tag)) {
            least_idx = x;
        }
    }
    return s[least_idx];
}

//this makes it such that all the nodes are pointing to the left
PLRU::PLRU(uint64_t num_sets, uint64_t num_ways) : trees(num_sets), m_ways(num_ways) {
    for (auto& tree : trees) {
        tree.resize(num_ways > 0 ? num_ways - 1 : 0, false);
    }
}

//note that index is actually set index
void PLRU::access(Set &s, uint64_t tag, uint64_t index) {
    std::size_t way = s.size();
    for (std::size_t x = 0; x < s.size(); ++x) {
        if (s[x].tag == tag) {
            way = x;
            break;
        }
    }

    if (way == s.size()) {
        return;
    }

    std::size_t node = (m_ways - 1) + way;

    while (node > 0) {
        std::size_t parent = (node - 1) / 2;
        bool cameFromLeft = (node == 2 * parent + 1);
        trees[index][parent] = cameFromLeft ? 1 : 0;
        node = parent;
    }
}

void PLRU::insert(Set &s, const Entry& e, uint64_t index) {
    s.push_back(e);
    access(s, e.tag, index);
}

Entry PLRU::evict(Set &s, uint64_t index, uint64_t now) {
    Entry victim = peek(s, index, now);
    std::size_t node = 0;

    while (node < m_ways - 1) {
        node = trees[index][node] ? (node * 2 + 2) : (node * 2 + 1);
    }
    const std::size_t way = node - (m_ways - 1);
    s.erase(s.begin() + way);
    return victim;
}

Entry PLRU::peek(const Set& s, uint64_t index, uint64_t now) const {
    (void)now;
    std::size_t node = 0;

    while (node < m_ways - 1) {
        node = trees[index][node] ? (node * 2 + 2) : (node * 2 + 1);
    }
    std::size_t way = node - (m_ways - 1);

    return s[way];
}

Belady::Belady(const Trace* trace)
    : m_trace{trace} {
}

void Belady::access(Set &s, uint64_t tag, uint64_t index) {
    (void)s;
    (void)tag;
    (void)index;
}

void Belady::insert(Set &s, const Entry &e, uint64_t index) {
    (void)index;
    s.push_back(e);
}

Entry Belady::evict(Set &s, uint64_t index, uint64_t now) {
    Entry victim = peek(s, index, now);
    for (auto it = s.begin(); it != s.end(); ++it) {
        if (it->tag == victim.tag) {
            s.erase(it);
            break;
        }
    }
    return victim;
}

Entry Belady::peek(const Set &s, uint64_t index, uint64_t now) const {
    (void)index;

    int victim_idx = 0;
    uint64_t victim_next = 0;

    for (int i = 0; i < static_cast<int>(s.size()); ++i) {
        const auto uses = m_trace->next_uses(s[i].addr);
        uint64_t next = std::numeric_limits<uint64_t>::max();

        auto it = std::upper_bound(
            uses.begin(), uses.end(), static_cast<int>(now));
        if (it != uses.end()) {
            next = static_cast<uint64_t>(*it);
        }

        if (i == 0 || next > victim_next) {
            victim_idx = i;
            victim_next = next;
        }
    }

    return s[victim_idx];
}
