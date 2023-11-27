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

    void get_candidate(std::list<int> *);

    FRAME_ID get_frame(); 

    bool is_evict();

    void set_dirty(FRAME_ID frame_id);

    void update_dirty(std::list<int> *);

    void push(FRAME_ID id, bool is_dirty);

    void update(FRAME_ID id, bool is_dirty);

    void print_list();

   private:
    std::list<FRAME_ID> *lru_list;
    std::list<FRAME_ID> *free_list;
    std::list<FRAME_ID> *clean_list;
    std::list<FRAME_ID> *dirty_list;

    std::unordered_map<FRAME_ID, std::list<FRAME_ID>::iterator> *lru_map;
    std::unordered_map<FRAME_ID, std::list<FRAME_ID>::iterator> *clean_map;
    std::unordered_map<FRAME_ID, std::list<FRAME_ID>::iterator> *dirty_map;
};
}  // namespace zns

#endif  // ZNS_ZALP_HPP