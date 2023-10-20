#ifndef ADB_PROJECT_BUFFER_MANAGER_HPP
#define ADB_PROJECT_BUFFER_MANAGER_HPP

#include <list>

#include "common.h"
#include "zalp.h"
#include "zController.h"

#define DEF_BUF_SIZE 1024

using std::list;

namespace zns {
class BufferManager {
 public:
  typedef std::shared_ptr<BufferManager> sptr;

  BufferManager();

  ~BufferManager();

  Frame::sptr read_page(PAGE_ID page_id);

  void write_page(PAGE_ID page_id, const Frame::sptr &frame);

  int fix_page(PAGE_ID page_id);

  int fix_page(bool is_write, PAGE_ID page_id);

  void fix_new_page(const Frame::sptr &frame);

  int get_free_frames_num();

  int get_io_count();

  int get_hit_count();

 private:
  Frame buffer[DEF_BUF_SIZE]{};
  ZNSController *zdsm;
  int free_frames_num;
  // Hash Table
  int frame_to_page[DEF_BUF_SIZE]{};
  list<BCB> page_to_frame[DEF_BUF_SIZE];
  // victim strategy
  ZALP *strategy;

  int hit_count;

  static int hash_func(PAGE_ID page_id);

  int select_victim();

  void clean_buffer();

  int get_page_id(int frame_id);

  void set_page_id(int frame_id, PAGE_ID page_id);

  void insert_bcb(PAGE_ID page_id, int frame_id);

  BCB *get_bcb(PAGE_ID page_id);

  void set_dirty(int frame_id);

  void unset_dirty(int frame_id);

  void inc_hit_count();
};
}  // namespace zns

#endif  // ADB_PROJECT_BUFFER_MANAGER_HPP