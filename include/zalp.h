#ifndef ZNS_ZALP_HPP
#define ZNS_ZALP_HPP

#include <assert.h>
#include <libzbd/zbd.h>
#include <stdlib.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <unordered_map>

#include "../libcuckoo/cuckoohash_map.hh"
#include "datastructure.h"
#include "parameter.h"

#define cluster_gc_num 16

namespace zns {
class ZALP {
    typedef std::list<FRAME_ID>::iterator LIST_ITER;

   public:
    ZALP(std::string out);

    ~ZALP();

    void is_correct();

    void get_candidate(std::list<int> *);

    void get_candidate_cflru(std::list<int> *);

    bool get_frame(FRAME_ID *rtframe);

    bool is_evict();

    void set_dirty(FRAME_ID frame_id);

    void update_dirty(std::list<int> *);

    void dirty_to_clean(std::list<int> *);

    void push(FRAME_ID id, bool is_dirty);

    void update(FRAME_ID id, bool is_dirty);

    void print_list();

   private:
    std::string output;
    std::list<FRAME_ID> *lList;
    ListNode worksSizePtr;
    std::list<FRAME_ID> *fList;
    std::list<FRAME_ID> *cList;
    std::list<FRAME_ID> *dList;

    libcuckoo::cuckoohash_map<int, LIST_ITER> lMap;
    libcuckoo::cuckoohash_map<int, LIST_ITER> cMap;
    libcuckoo::cuckoohash_map<int, LIST_ITER> dMap;
};

class LRU {
   public:
    LRU();

    ~LRU();

    int get_victim();

    void push(int id);

    void update(int id);

   private:
    std::list<int> *lList;
    std::unordered_map<int, std::list<int>::iterator> *lMap;
};
}  // namespace zns

#endif  // ZNS_ZALP_HPP