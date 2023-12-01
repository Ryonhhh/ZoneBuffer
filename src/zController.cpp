#include <iostream>

#include "../include/zController.h"

namespace zns {
ZNSController::ZNSController() {
    dev_id = 0;
    log_file = nullptr;
    open_file();
    init();
    time_t timep;
    struct tm *p;
    time(&timep);
    p = localtime(&timep);
    output = "./output/" + std::to_string(p->tm_mon) +
                         std::to_string(p->tm_mday) +
                         std::to_string(p->tm_hour) + std::to_string(p->tm_min);
    std::ofstream op(output);
    op.close();
    io_count = 0;
}

ZNSController::~ZNSController() {
    write_log();
    // close_file();
}

void ZNSController::init() {
    // private init
    // cap = (unsigned int)(CAPACITY * 512 / info->nr_zones);
    cap = (unsigned int)(CAPACITY * 512 / info->nr_zones) / 4;
    zone_size = info->zone_size;
    zone_number = MAX_ZONE_NUM;
    block_per_zone = zone_size / info->pblock_size;
    meta_page = (bool *)malloc(sizeof(bool) * block_per_zone * zone_number);
    pages_num = 0;
    pageid2addr =
        (off_st *)malloc(sizeof(off_st) * block_per_zone * zone_number);
    reset_all();
    for (ZONE_ID i = 0; i < zone_number; i++) {
        Zone[i].id = i;
        Zone[i].ofst = i * (off_st)zone_size;
        Zone[i].cond = ZBD_ZONE_COND_EMPTY;
        Zone[i].wp = get_zone_wp(i);
    }
    for (PAGE_ID i = 0; i < block_per_zone * zone_number; i++) meta_page[i] = 0;
}

void ZNSController::write_log() {
    log_file = fopen(LOG_PATH, "w+");
    fseek(log_file, 0, SEEK_SET);
    // fwrite(meta_page, sizeof(bool), MAX_PAGE_NUM, log_file);
    fclose(log_file);
}

// bool ZNSController::is_page_valid(PAGE_ID page_id) {
// return page_id > 0 && get_valid(page_id);
//}

ZONE_ID ZNSController::read_page_p(PAGE_ID page_id, Frame *frm) {
    // if (!is_page_valid(page_id)) {
    //     return -1;
    // }
    off_st page_addr = get_page_addr(page_id);
    char *buf = reinterpret_cast<char *>(memalign(4096, PAGE_SIZE));
    assert(buf != nullptr);
    int ret = pread(dev_id, buf, PAGE_SIZE, page_addr);
    assert(ret == PAGE_SIZE);
    strcpy(frm->field, buf);
    inc_io_count();
    return page_addr / zone_size;
}

int ZNSController::write_page_p(PAGE_ID page_id, Frame *frm) {
    set_valid(get_page_addr(page_id), 0);
    ZONE_ID zoneWid = select_write_zone(page_id, 1);
    set_page_addr(page_id, Zone[zoneWid].wp);
    set_valid(get_page_addr(page_id), 1);
    int ret = pwrite(dev_id, frm->field, PAGE_SIZE, Zone[zoneWid].wp);
    assert(ret == PAGE_SIZE);
    // printf("\nzone_id:%d wp:%lld\n", zoneWid, wp_write);
    Zone[zoneWid].wp += ret;
    inc_io_count();
    return 0;
}

int ZNSController::write_cluster_p(int cf, char *write_buffer,
                                   PAGE_ID *page_list, int cluster_size) {
    if (cf == -1) cf = (int)rand() % CLUSTER_NUM;
    ZONE_ID zoneWid = select_write_zone(cf, cluster_size);
    for (int i = 0; i < cluster_size; i++) {
        set_valid(get_page_addr(page_list[i]), 0);
        set_page_addr(page_list[i], Zone[zoneWid].wp + i * PAGE_SIZE);
        set_valid(get_page_addr(page_list[i]), 1);
    }
    int ret = pwrite(dev_id, write_buffer, PAGE_SIZE * cluster_size,
                     Zone[zoneWid].wp);
    assert(ret == PAGE_SIZE * cluster_size);
    Zone[zoneWid].wp += ret;
    inc_io_count();
    return 0;
}

int ZNSController::write_cluster_a(int cf, char *write_buffer,
                                   PAGE_ID *page_list, int cluster_size) {
    /*
    ZONE_ID zoneWid = select_write_zone(cf, cluster_size);
    for (int i = 0; i < cluster_size; i++) {
        set_valid(get_page_addr(page_list[i]), 0);
        set_page_addr(page_list[i], Zone[i].wp + i * PAGE_SIZE);
        set_valid(get_page_addr(page_list[i]), 1);
    }
     nvme_zns_append(nvme_write_buffer);
    inc_io_count();
     Zone[zoneWid].wp += ret;
     */
    return 0;
}

ZONE_ID ZNSController::select_write_zone(PAGE_ID page_id, int cluster_size) {
    ZONE_ID zone_ret = (ZONE_ID)(page_id % CLUSTER_NUM);
    while (Zone[zone_ret].cond == ZBD_ZONE_COND_FULL ||
           Zone[zone_ret].wp + cluster_size * PAGE_SIZE >=
               Zone[zone_ret].ofst + cap) {
        if (Zone[zone_ret].cond == ZBD_ZONE_COND_EXP_OPEN) {
            finish_zone(zone_ret);
            Zone[zone_ret].cond = ZBD_ZONE_COND_FULL;
        }
        zone_ret += CLUSTER_NUM;
    }
    if (Zone[zone_ret].cond == ZBD_ZONE_COND_EMPTY) {
        open_zone(zone_ret);
        Zone[zone_ret].cond = ZBD_ZONE_COND_EXP_OPEN;
    }
    return zone_ret;
}

void ZNSController::create_new_page(PAGE_ID page_id, Frame *frm) {
    ZONE_ID zoneWid = select_write_zone(page_id, 1);
    set_page_addr(page_id, Zone[zoneWid].wp);
    set_valid(get_page_addr(page_id), 1);
    int ret = pwrite(dev_id, frm->field, PAGE_SIZE, Zone[zoneWid].wp);
    assert(ret == PAGE_SIZE);
    // printf("\nzone_id:%d wp:%lld\n", zoneWid, wp_write);
    Zone[zoneWid].wp += ret;
    inc_io_count();
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
    int ret = zbd_open_zones(dev_id, Zone[zone_id].ofst, zone_size);
    assert(ret == 0);
}

void ZNSController::close_zone(ZONE_ID zone_id) {
    if (get_zone_cond(zone_id) != ZBD_ZONE_COND_CLOSED) {
        int ret = zbd_close_zones(dev_id, Zone[zone_id].ofst, zone_size);
        assert(ret == 0);
    }
}

void ZNSController::reset_zone(ZONE_ID zone_id) {
    int ret = zbd_reset_zones(dev_id, Zone[zone_id].ofst, zone_size);
    assert(ret == 0);
}

void ZNSController::reset_all() {
    int ret = zbd_reset_zones(dev_id, 0, 0);
    assert(ret == 0);
}

void ZNSController::finish_zone(ZONE_ID zone_id) {
    int ret = zbd_finish_zones(dev_id, Zone[zone_id].ofst, zone_size);
    assert(ret == 0);
}

unsigned int ZNSController::get_zone_cond(ZONE_ID zone_id) {
    struct zbd_zone info_zone;
    unsigned int nr_zones = 1;
    int ret = zbd_report_zones(dev_id, Zone[zone_id].ofst, zone_size,
                               ZBD_RO_ALL, &info_zone, &nr_zones);
    assert(ret == 0);
    return info_zone.cond;
}

off_st ZNSController::get_zone_wp(ZONE_ID zone_id) {
    struct zbd_zone info_zone;
    unsigned int nr_zones = 1;
    int ret = zbd_report_zones(dev_id, Zone[zone_id].ofst, zone_size,
                               ZBD_RO_ALL, &info_zone, &nr_zones);
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
    meta_page[page_id / PAGE_SIZE] = is_valid;
}

bool ZNSController::get_valid(PAGE_ID page_id) {
    return meta_page[page_id / PAGE_SIZE];
}

void ZNSController::io_count_clear() { io_count = 0; }

PAGE_ID ZNSController::get_pages_num() { return pages_num; }

void ZNSController::inc_io_count() { io_count++; }

int ZNSController::get_io_count() { return io_count; }

void ZNSController::inc_pages_num() { pages_num++; }

void ZNSController::get_gc_info() {
    for (ZONE_ID i = 0; i < zone_number; i++) {
        Zone[i].idle_rate = (double)(Zone[i].wp - Zone[i].ofst) / cap;
        Zone[i].gc_rate = 0;
        for (PAGE_ID j = 0; j < cap / PAGE_SIZE; j++)
            Zone[i].gc_rate += meta_page[i * block_per_zone + j];
        Zone[i].gc_rate /= (cap / PAGE_SIZE);
        if (i % CLUSTER_NUM == 0) printf("\n");
        printf("\n\tzone \t%d\t idle_rate \t%f\t garbage_rate: \t%f\t", i,
               Zone[i].idle_rate, Zone[i].gc_rate);
    }
}

void ZNSController::print_gc_info() {
    get_gc_info();
    std::ofstream op(output, std::ios::app);
    for (ZONE_ID i = 0; i < zone_number; i++) {
        if (i % CLUSTER_NUM == 0) op << std::endl;
        ;
        op << "\n\tzone \t" << i << "\t idle_rate \t" << std::setw(8)
           << Zone[i].idle_rate << "\t garbage_rate: \t" << std::setw(8)
           << Zone[i].gc_rate << "\t";
    }
    op.close();
}

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