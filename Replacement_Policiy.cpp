#include <Replacement_Policiy.h>
#include <Cache.h>

void FIFO::access(Set &s, uint64_t tag) {
    return;
}

void FIFO::insert(Set &s, uint64_t tag) {
    s.push_back({tag});
}

Entry FIFO::evict(Set &s) {
    Entry front = s.front();
    s.pop_front();

    return front;
}

void LRU::access(Set &s, uint64_t tag) {
    Entry temp;
    for (int x = 0; x<s.size(); x++) {
        if (s[x].tag == tag) {
            temp = s[x];
            s.erase(s.begin()+x);
            break;
        }
    }

    s.push_back(temp);
}

Entry LRU::evict(Set &s) {
    Entry temp = s.front();
    s.pop_front();
    return temp;
}

void LRU::insert(Set &s, uint64_t tag) {
    s.push_back({tag});
}


