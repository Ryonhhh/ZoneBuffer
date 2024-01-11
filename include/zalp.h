#ifndef ZNS_ZALP_HPP
#define ZNS_ZALP_HPP

#include <assert.h>
#include <stdlib.h>

#include <cstdlib>
#include <iostream>
#include <list>
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <fstream>

#include <libzbd/zbd.h>
#include "parameter.h"
#include "datastructure.h"

#define cluster_gc_num 16

namespace zns {
class ZALP {
   public:
    ZALP(std::string out);

    ~ZALP();

    void get_candidate(std::list<int> *);

    void get_candidate_cflru(std::list<int> *);

    bool get_frame(FRAME_ID *rtframe); 

    bool is_evict();

    void set_dirty(FRAME_ID frame_id);

    void update_dirty(std::list<int> *);

    void update_dirty_readhot(std::list<int> *);

    void push(FRAME_ID id, bool is_dirty);

    void update(FRAME_ID id, bool is_dirty);

    void print_list();

   private:
    std::string output;
    List *lList;
    List *fList;
    List *cList;
    List *dList;

    HashTable *lMap;
    HashTable *cMap;
    HashTable *dMap;
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