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

    BufferManager();

    ~BufferManager();

    void clean_buffer();

    char *read_page(PAGE_ID page_id);

    void write_page(PAGE_ID page_id, char *frame);

    FRAME_ID fix_page(bool is_write, PAGE_ID page_id);

    void fix_new_page(PAGE_ID page_id, char *frame);

    int get_free_frames_num();

    void hit_count_clear();

    int get_io_count();

    int get_hit_count();

    std::string output; 

    ZNSController *zdsm;

   private:
    libcuckoo::cuckoohash_map<PAGE_ID, std::pair<ACCTIME, ACCTIME>> accessH;
    bool zalp_wc = false, wh_only = false;
    bool ach_only = false;
    bool zalp = false;
    bool cflru = false;
    bool lru = true;
    bool cd_detect = false;
    enum WC { cold, warm, hot };
    char *buffer[DEF_BUF_SIZE]{};
    int cd_interval;
    int cluster_flag[DEF_BUF_SIZE]{};
    int write_count[DEF_BUF_SIZE]{};
    int read_count[DEF_BUF_SIZE]{};
    int free_frames_num;
    unsigned int evict_count = 0;
    unsigned int insert_count = 0;
    int cd_count;
    // Hash Table
    PAGE_ID frame_to_page[DEF_BUF_SIZE]{};
    bool frame_dirty[DEF_BUF_SIZE]{};
    list<BCB> page_to_frame[DEF_BUF_SIZE];
    libcuckoo::cuckoohash_map<PAGE_ID, FRAME_ID> page2frame;
    // victim strategy
    ZALP *strategy;
    LRU *lrus;

    int hit_count;

    static int hash_func(PAGE_ID page_id);

    int select_victim();

    int HC_Lv(int cluster_flag, int max);

    void evict_victim();

    void evict_victim_ach();

    bool ach_clutser(int lv, std::pair<ACCTIME, ACCTIME> acht);

    PAGE_ID get_page_id(FRAME_ID frame_id);

    void set_page_id(FRAME_ID frame_id, PAGE_ID page_id);

    void set_dirty(FRAME_ID frame_id);

    void unset_dirty(FRAME_ID frame_id);

    void inc_hit_count();

    int *flush_count;
};
}  // namespace zns

#endif  // ADB_PROJECT_BUFFER_MANAGER_HPP