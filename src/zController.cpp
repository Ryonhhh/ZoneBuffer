#include <iostream>

#include "../include/zController.h"

namespace zns {
ZNSController::ZNSController() {
    dev_id = 0;
    log_file = nullptr;
    open_file();
    init();
    io_count = 0;
}

ZNSController::~ZNSController() {
    write_log();
    close_file();
}

void ZNSController::init() {
    // private init
    cap = (unsigned int)(CAPACITY * 512);
    zone_size = info->zone_size;
    zone_number = info->nr_zones;
    ofst = (off_st *)malloc(sizeof(off_st) * zone_number);
    //wp = (off_st *)malloc(sizeof(off_st) * zone_number);
    block_per_zone = zone_size / info->pblock_size;
    meta_page = (bool *)malloc(sizeof(bool) * block_per_zone * zone_number);
    pages_num = 0;
    pageid2addr =
        (off_st *)malloc(sizeof(off_st) * block_per_zone * zone_number);
    reset_all();
    for (ZONE_ID i = 0; i < zone_number; i++) {
        ofst[i] = i * (off_st)zone_size;
        //wp[i] = get_zone_wp(i);
    }
    for (PAGE_ID i = 0; i < block_per_zone * zone_number; i++) meta_page[i] = 1;
}

void ZNSController::write_log() {
    log_file = fopen(LOG_PATH, "w+");
    fseek(log_file, 0, SEEK_SET);
    fwrite(meta_page, sizeof(bool), MAX_PAGE_NUM, log_file);
    fclose(log_file);
}

bool ZNSController::is_page_valid(PAGE_ID page_id) {
    return page_id > 0 && get_valid(page_id);
}

int ZNSController::read_page_p(PAGE_ID page_id, Frame *frm) {
    if (!is_page_valid(page_id)) {
        return -1;
    }
    off_st page_addr = get_page_addr(page_id);
    char *buf = reinterpret_cast<char *>(memalign(4096, PAGE_SIZE));
    assert(buf != nullptr);
    int ret = pread(dev_id, buf, PAGE_SIZE, page_addr);
    assert(ret == PAGE_SIZE);
    strcpy(frm->field, buf);
    inc_io_count();
    return 0;
}

int ZNSController::write_page_p(PAGE_ID page_id, Frame *frm) {
    // unsigned int page_addr = get_page_addr(page_id);
    ZONE_ID zone_write = select_write_zone(page_id);
    off_st wp_write = get_zone_wp(zone_write);
    set_page_addr(page_id, wp_write);
    int ret = pwrite(dev_id, frm->field, PAGE_SIZE, wp_write);
    assert(ret == PAGE_SIZE);
    //printf("\nzone_id:%d wp:%lld\n", zone_write, wp_write);
    inc_io_count();
    return 0;
}

/*int ZNSController::write_dirtyzone_p(unsigned long long page_id, Frame *frm) {
  unsigned int page_addr = get_page_addr(page_id);
  seek(page_addr);
  fwrite(frm->field, sizeof(char), FRAME_SIZE, zns_file);
  inc_io_count();
  return 0;
}*/

ZONE_ID ZNSController::select_write_zone(PAGE_ID page_id) {
    ZONE_ID zone_ret = (ZONE_ID)(page_id % 10);
    //if (get_zone_cond(zone_ret) != ZBD_ZONE_COND_EXP_OPEN ||
    //    get_zone_cond(zone_ret) != ZBD_ZONE_COND_IMP_OPEN)
    //    open_zone(zone_ret);
    return zone_ret;
}

PAGE_ID ZNSController::create_new_page(Frame *frm) {
    PAGE_ID page_id = get_pages_num();
    inc_pages_num();
    /*ZONE_ID zone_write = select_write_zone(page_id);
    set_page_addr(page_id, get_zone_wp(zone_write));
    int ret = pwrite(dev_id, frm->field, PAGE_SIZE, get_zone_wp(zone_write));
    assert(ret == PAGE_SIZE);
    printf("\nzone_id:%d wp:%lld\n", zone_write, get_zone_wp(zone_write));
    inc_io_count();
    */
    write_page_p(page_id, frm);
    return page_id;
}

void ZNSController::open_file() {
    info = (struct zbd_info *)malloc(sizeof(struct zbd_info));
    if ((dev_id = zbd_open(ZNS_PATH, O_RDWR | O_DIRECT, info)) == -1) {
        printf("can't open device!\n");
        exit(0);
    }
    printf("open device successfully!\n");
    print_zns_info();

    // read log
    /*if ((log_file = fopen(LOG_PATH, "rb+")) == nullptr) {
        log_file = fopen(ZNS_PATH, "wb+");
        char temp[META_PAGE_SIZE] = {0};
        fwrite(&temp, META_PAGE_SIZE * sizeof(char), 1, log_file);
        close_file();
        log_file = fopen(ZNS_PATH, "rb+");
        assert(log_file != nullptr);
    }
    fseek(log_file, 0, SEEK_SET);
    fread(meta_page, sizeof(int), MAX_PAGE_NUM, log_file);
    */
}

void ZNSController::close_file() {
    for (ZONE_ID i = 0; i < zone_number; i++) finish_zone(i);
}

void ZNSController::open_zone(ZONE_ID zone_id) {
    int ret = zbd_open_zones(dev_id, ofst[zone_id], zone_size);
    assert(ret == 0);
}

void ZNSController::close_zone(ZONE_ID zone_id) {
    if (get_zone_cond(zone_id) != ZBD_ZONE_COND_CLOSED) {
        int ret = zbd_close_zones(dev_id, ofst[zone_id], zone_size);
        assert(ret == 0);
    }
}

void ZNSController::reset_zone(ZONE_ID zone_id) {
    int ret = zbd_reset_zones(dev_id, ofst[zone_id], zone_size);
    assert(ret == 0);
}

void ZNSController::reset_all() {
    int ret = zbd_reset_zones(dev_id, 0, 0);
    assert(ret == 0);
}

void ZNSController::finish_zone(ZONE_ID zone_id) {
    int ret = zbd_finish_zones(dev_id, ofst[zone_id], zone_size);
    assert(ret == 0);
}

unsigned int ZNSController::get_zone_cond(ZONE_ID zone_id) {
    struct zbd_zone info_zone;
    unsigned int nr_zones = 1;
    int ret = zbd_report_zones(dev_id, ofst[zone_id], zone_size, ZBD_RO_ALL,
                               &info_zone, &nr_zones);
    assert(ret == 0);
    return info_zone.cond;
}

off_st ZNSController::get_zone_wp(ZONE_ID zone_id) {
    struct zbd_zone info_zone;
    unsigned int nr_zones = 1;
    int ret = zbd_report_zones(dev_id, ofst[zone_id], zone_size, ZBD_RO_ALL,
                               &info_zone, &nr_zones);
    assert(ret == 0);
    return info_zone.wp;
}

off_st ZNSController::get_page_addr(PAGE_ID page_id) {
    return pageid2addr[page_id];
}

void ZNSController::set_page_addr(PAGE_ID page_id, off_st addr) {
    pageid2addr[page_id] = addr;
}

void ZNSController::set_valid(PAGE_ID page_id, bool is_valid) {
    meta_page[page_id] = is_valid;
}

bool ZNSController::get_valid(PAGE_ID page_id) { return meta_page[page_id]; }

PAGE_ID ZNSController::get_pages_num() { return pages_num; }

void ZNSController::inc_io_count() { io_count++; }

int ZNSController::get_io_count() { return io_count; }

void ZNSController::inc_pages_num() { pages_num++; }

void ZNSController::print_zns_info() {
    zbd_info *info_temp;
    info_temp = (struct zbd_info *)malloc(sizeof(struct zbd_info));
    zbd_get_info(dev_id, info_temp);
    std::cout << " vendor_id: " << info_temp->vendor_id << std::endl;
    std::cout << " nr_sectors: " << info_temp->nr_sectors << std::endl;
    std::cout << " nr_lblocks: " << info_temp->nr_lblocks << std::endl;
    std::cout << " nr_pblocks: " << info_temp->nr_pblocks << std::endl;
    std::cout << " zone_size: " << info_temp->zone_size << std::endl;
    std::cout << " zone_sectors: " << info_temp->zone_sectors << std::endl;
    std::cout << " lblock_size: " << info_temp->lblock_size << std::endl;
    std::cout << " pblock_size: " << info_temp->pblock_size << std::endl;
    std::cout << " nr_zones: " << info_temp->nr_zones << std::endl;
    std::cout << " max_nr_open_zones: " << info_temp->max_nr_open_zones
              << std::endl;
    std::cout << " max_nr_active_zones: " << info_temp->max_nr_active_zones
              << std::endl;
    std::cout << " model: " << info_temp->model << std::endl;
}
}  // namespace zns