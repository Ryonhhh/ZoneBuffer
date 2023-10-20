#ifndef ZNS_ZALP
#define ZNS_ZALP

#include <assert.h>
#include <stdlib.h>
#include "/home/wht/libzbd/include/libzbd/zbd.h"

#include <cstdlib>
#include <iostream>
#include <list>
#include <memory>
#include <unordered_map>


#include "parameter.h"
namespace zns {
class ZALP {
 public:
  ZALP();

  ~ZALP();

  int get_victim();

  void push(int id);

  void update(int id);

 private:
  std::list<int> *lru_list;
  std::unordered_map<int, std::list<int>::iterator> *lru_map;
};
}  // namespace zns

#endif  // ZNS_ZALP