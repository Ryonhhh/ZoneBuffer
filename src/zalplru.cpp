#include "../include/zalplru.h"

namespace zns {
LRUCache::LRUCache(int capacity) { this->cap = capacity; }

Frame::sptr LRUCache::get(FRAME_ID key) {
    auto it = map.find(key);
    if (it == map.end()) return NULL;
    std::pair<FRAME_ID, Frame::sptr> kv = *map[key];
    cache.erase(map[key]);
    cache.push_front(kv);
    map[key] = cache.begin();
    return kv.second;
}

void LRUCache::put(FRAME_ID key, Frame::sptr value) {
    auto it = map.find(key);
    if (it == map.end()) {
        if (cache.size() == cap) {
            auto lastPair = cache.back();
            int lastKey = lastPair.first;
            map.erase(lastKey);
            cache.pop_back();
        }
        cache.push_front(std::make_pair(key, value));
        map[key] = cache.begin();
    } else {
        cache.erase(map[key]);
        cache.push_front(std::make_pair(key, value));
        map[key] = cache.begin();
    }
}
}  // namespace zns