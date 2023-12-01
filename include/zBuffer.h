#ifndef ADB_PROJECT_BUFFER_MANAGER_HPP
#define ADB_PROJECT_BUFFER_MANAGER_HPP

#include <list>

#include "common.h"
#include "zController.h"
#include "zalp.h"

using std::list;

namespace zns {
class BufferManager {
   public:
    typedef std::shared_ptr<BufferManager> sptr;

    BufferManager();

    ~BufferManager();

    Frame::sptr read_page(PAGE_ID page_id);

    void write_page(PAGE_ID page_id, const Frame::sptr &frame);

    FRAME_ID fix_page(PAGE_ID page_id);

    FRAME_ID fix_page(bool is_write, PAGE_ID page_id);

    void fix_new_page(PAGE_ID page_id, const Frame::sptr &frame);

    int get_free_frames_num();

    void hit_count_clear();

    int get_io_count();

    int get_hit_count();

   private:
    bool zalp_wc = 0;
    bool zalp = 1;
    bool cflru = 0;
    bool lru = 0;
    Frame buffer[DEF_BUF_SIZE]{};
    ZNSController *zdsm;
    int free_frames_num;
    // Hash Table
    PAGE_ID frame_to_page[DEF_BUF_SIZE]{};
    list<BCB> page_to_frame[DEF_BUF_SIZE];
    // victim strategy
    ZALP *strategy;
    LRU *lrus;

    int hit_count;

    static int hash_func(PAGE_ID page_id);

    int select_victim();

    void evict_victim();

    void clean_buffer();

    PAGE_ID get_page_id(FRAME_ID frame_id);

    void set_page_id(FRAME_ID frame_id, PAGE_ID page_id);

    void insert_bcb(PAGE_ID page_id, FRAME_ID frame_id);

    BCB *get_bcb(PAGE_ID page_id);

    void set_dirty(FRAME_ID frame_id);

    void unset_dirty(FRAME_ID frame_id);

    void inc_hit_count();

    int buffer_count[100000]{};
};
}  // namespace zns

#endif  // ADB_PROJECT_BUFFER_MANAGER_HPP