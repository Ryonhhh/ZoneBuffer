#ifndef ZNS_ZALP_HPP
#define ZNS_ZALP_HPP

#include <assert.h>
#include <stdlib.h>

#include <cstdlib>
#include <iostream>
#include <list>
#include <memory>
#include <unordered_map>

#include "/home/wht/libzbd/include/libzbd/zbd.h"
#include "parameter.h"

#define cluster_gc_num 16

namespace zns {
class ZALP {
   public:
    ZALP();

    ~ZALP();

    void get_victim(std::list<int> *);

    FRAME_ID get_free_frame(); 

    int set_dirty(FRAME_ID frame_id, int cf);  // cluster flag

    void push(FRAME_ID id);

    void update(FRAME_ID id);

   private:
    std::list<FRAME_ID> *lru_list;
    std::list<FRAME_ID> *free_list;
    std::list<FRAME_ID> *dirty_cluster_list[CLUSTER_NUM];

    std::unordered_map<FRAME_ID, std::list<FRAME_ID>::iterator> *lru_map;
    std::unordered_map<FRAME_ID, std::list<FRAME_ID>::iterator> *free_map;
    std::unordered_map<FRAME_ID, std::list<FRAME_ID>::iterator> *dirty_map;
};
}  // namespace zns

#endif  // ZNS_ZALP_HPP