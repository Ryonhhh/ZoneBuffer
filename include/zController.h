#ifndef ZNS_CONTROLLER
#define ZNS_CONTROLLER

#include <string.h>
#include <fcntl.h>
#include <malloc.h>

#include "common.h"
#include "parameter.h"

namespace zns {
class ZNSController {
 public:
  ZNSController();

  ~ZNSController();

  //bool is_page_valid(PAGE_ID page_id);

  int read_page_p(PAGE_ID page_id, Frame *frm);

  ZONE_ID select_write_zone(PAGE_ID page_id);

  int write_page_p(PAGE_ID page_id, Frame *frm);

  int write_cluster_p(int cf,char *write_buffer, PAGE_ID *page_list, int cluster_size);
   
  int write_cluster_a(int cf,char *write_buffer, PAGE_ID *page_list, int cluster_size);

  void create_new_page(PAGE_ID page_id, Frame *frm);

  int get_io_count();

 private:
  unsigned int zone_number;

  int *zone_cond;

  unsigned int zone_size;

  unsigned int cap;

  off_st *ofst;

  //off_st *wp;

  long long block_per_zone;

  unsigned int *type;

  FILE *log_file;

  zbd_info *info;

  int dev_id;

  bool *meta_page;

  PAGE_ID pages_num;

  off_st *pageid2addr;

  int io_count;

  void open_file();

  void close_file();

  void init();

  void reset_all();

  void write_log();

  void open_zone(ZONE_ID zone_id);

  void close_zone(ZONE_ID zone_id);

  void reset_zone(ZONE_ID zone_id);

  void finish_zone(ZONE_ID zone_id);

  unsigned int get_zone_cond(ZONE_ID zone_id);

  off_st get_zone_wp(ZONE_ID zone_id);

  off_st get_page_addr(PAGE_ID page_id);

  void set_page_addr(PAGE_ID page_id, off_st addr);

  //void set_valid(PAGE_ID page_id, bool is_valid);

  //bool get_valid(PAGE_ID page_id);

  void inc_io_count();

  void inc_pages_num();

  PAGE_ID get_pages_num();

  void print_zns_info();
};
}  // namespace zns

#endif  // ZNS_CONTROLLER