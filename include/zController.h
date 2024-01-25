#ifndef ZNS_CONTROLLER
#define ZNS_CONTROLLER

#include <fcntl.h>
#include <malloc.h>
#include <string.h>
#include <time.h>
#include <thread>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <errno.h>
#include "../libcuckoo/cuckoohash_map.hh"

#include "common.h"
#include "parameter.h"

namespace zns {
class ZNSController {
   public:
    ZNSController();

    ~ZNSController();

    // bool is_page_valid(PAGE_ID page_id);

    std::string output;

    int gc_count = 0;

    int cluster_num;

    int *zone_select_ptr;

    ZONE_ID read_page_p(PAGE_ID page_id, char *frm);

    ZONE_ID select_write_zone(int cf, int cluster_size);

    int write_page_p(int cf, PAGE_ID page_id, char const *write_buffer);

    int write_cluster_p(int cf, char const *write_buffer, PAGE_ID *page_list,
                        int cluster_size);

    int write_page_gc(int cf, PAGE_ID page_id, char const *write_buffer);

    void create_new_page(PAGE_ID page_id, char const *write_buffer);

    void get_gc_info();

    void print_gc_info();

    void garbage_collection_detect();

    void io_count_clear();

    int get_io_count();

   private:
    typedef struct ZoneDesc {
        ZONE_ID id;
        off_st ofst;
        off_st wp;
        int cond;
        int valid_page;
        int cf;
        double idle_rate;
        double gc_rate;
        bool inGc;
    } ZoneDesc;

    unsigned int zone_number;

    ZoneDesc Zone[MAX_ZONE_NUM];

    unsigned int zone_size;

    unsigned int cap;

    long long block_per_zone;

    unsigned int *type;

    FILE *log_file;

    zbd_info *info;

    int dev_id;

    bool *meta_page;

    double *zone_idle_rate;

    double *zone_gc_rate;

    PAGE_ID pages_num;

    libcuckoo::cuckoohash_map<PAGE_ID, off_st> Table;

    int io_count;

    double valid_data = 0, real_data = 0;

    int used_zone = 0;

    bool zone_used[MAX_ZONE_NUM]{};

    void open_file();

    void close_file();

    void init();

    void reset_all();

    void open_zone(ZONE_ID zone_id);

    void close_zone(ZONE_ID zone_id);

    void reset_zone(ZONE_ID zone_id);

    void finish_zone(ZONE_ID zone_id);

    unsigned int get_zone_cond(ZONE_ID zone_id);

    off_st get_zone_wp(ZONE_ID zone_id);

    off_st get_page_addr(PAGE_ID page_id);

    void set_page_addr(PAGE_ID page_id, off_st addr);

    void set_valid(off_st page_addr, bool is_valid);

    void set_valid_gc(off_st page_addr, bool is_valid);

    bool get_valid(off_st page_addr);

    void garbage_collection(ZONE_ID id);

    void inc_io_count();

    void inc_pages_num();

    PAGE_ID get_pages_num();

    void print_zns_info();
};
}  // namespace zns

#endif  // ZNS_CONTROLLER