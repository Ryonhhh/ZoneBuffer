#ifndef ZNS_ZALPLRU_HPP
#define ZNS_ZALPLRU_HPP

#include <cstdlib>
#include <iostream>
#include <list>
#include <memory>
#include <unordered_map>

#include "common.h"
#include "parameter.h"

namespace zns {
class LRUCache {
   private:
    int cap;
    std::list<std::pair<FRAME_ID, Frame::sptr>> cache;
    std::unordered_map<FRAME_ID,
                       std::list<std::pair<FRAME_ID, Frame::sptr>>::iterator>
        map;

   public:
    LRUCache(int capacity);

    Frame::sptr get(FRAME_ID key);

    void put(FRAME_ID key, Frame::sptr value);
};

}  // namespace zns

#endif  // ZNS_ZALPLRU