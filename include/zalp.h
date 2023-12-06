#ifndef ZNS_ZALP_HPP
#define ZNS_ZALP_HPP

#include <assert.h>
#include <stdlib.h>

#include <cstdlib>
#include <iostream>
#include <list>
#include <memory>
#include <unordered_map>
#include <fstream>

#include "/home/wht/libzbd/include/libzbd/zbd.h"
#include "parameter.h"

#define cluster_gc_num 16

namespace zns {
class ZALP {
   public:
    ZALP(std::string out);

    ~ZALP();

    void get_candidate(std::list<int> *);

    bool get_frame(FRAME_ID *rtframe); 

    bool is_evict();

    void set_dirty(FRAME_ID frame_id);

    void update_dirty(std::list<int> *);

    void push(FRAME_ID id, bool is_dirty);

    void update(FRAME_ID id, bool is_dirty);

    bool lfind(std::list<FRAME_ID> *list, FRAME_ID frame_id);

    void print_list();

   private:
    std::string output;
    std::list<FRAME_ID> *lru_list;
    std::list<FRAME_ID> *free_list;
    std::list<FRAME_ID> *clean_list;
    std::list<FRAME_ID> *dirty_list;

    std::unordered_map<FRAME_ID, std::list<FRAME_ID>::iterator> *lru_map;
    std::unordered_map<FRAME_ID, std::list<FRAME_ID>::iterator> *clean_map;
    std::unordered_map<FRAME_ID, std::list<FRAME_ID>::iterator> *dirty_map;
};

class LRU {
 public:
  LRU();

  ~LRU();

  int get_victim();

  void push(int id);

  void update(int id);

 private:
  std::list<int> *lru_list;
  std::unordered_map<int, std::list<int>::iterator> *lru_map;
};
}  // namespace zns

#endif  // ZNS_ZALP_HPP