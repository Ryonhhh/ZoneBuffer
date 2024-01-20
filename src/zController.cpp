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
    output = "/home/wht/zalpBuffer/output/" + std::to_string(p->tm_mon) + "_" +
             std::to_string(p->tm_mday) + "_" + std::to_string(p->tm_hour) +
             "_" + std::to_string(p->tm_min);
    std::ofstream op(output);
    op.close();
    io_count = 0;
}

ZNSController::~ZNSController() {
    free(meta_page);
    free(pageid2addr);
    // close_file();
}

void ZNSController::init() {
    // private init
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
        Zone[i].valid_page = 0;
        Zone[i].cond = ZBD_ZONE_COND_EMPTY;
        Zone[i].wp = Zone[i].ofst;
        Zone[i].cf = -1;
        zone_used[i] = false;
    }
    for (PAGE_ID i = 0; i < block_per_zone * zone_number; i++) meta_page[i] = 0;
}

// bool ZNSController::is_page_valid(PAGE_ID page_id) {
// return page_id > 0 && get_valid(page_id);
//}

ZONE_ID ZNSController::read_page_p(PAGE_ID page_id, char *frame) {
    off_st page_addr = get_page_addr(page_id);
    char *buf = reinterpret_cast<char *>(memalign(4096, PAGE_SIZE));
    assert(buf != nullptr);
    off_st ret = pread(dev_id, buf, PAGE_SIZE, page_addr);
    assert(ret == PAGE_SIZE);
    memcpy(frame, buf, PAGE_SIZE);
    inc_io_count();
    free(buf);
    return page_addr / zone_size;
}

int ZNSController::write_page_p(int cf, PAGE_ID page_id,
                                char const *write_buffer) {
    set_valid(get_page_addr(page_id), 0);
    ZONE_ID zoneWid = select_write_zone(cf, 1);
    set_page_addr(page_id, Zone[zoneWid].wp);
    set_valid(get_page_addr(page_id), 1);
    off_st ret = pwrite64(dev_id, write_buffer, PAGE_SIZE, Zone[zoneWid].wp);
    assert(ret == PAGE_SIZE);
    // printf("\nzone_id:%d wp:%lld\n", zoneWid, wp_write);
    Zone[zoneWid].wp += ret;
    inc_io_count();
    return 0;
}

int ZNSController::write_cluster_p(int cf, char const *write_buffer,
                                   PAGE_ID *page_list, int cluster_size) {
    ZONE_ID zoneWid = select_write_zone(cf, cluster_size);
    for (int i = 0; i < cluster_size; i++) {
        set_valid(get_page_addr(page_list[i]), 0);
        set_page_addr(page_list[i], Zone[zoneWid].wp + i * PAGE_SIZE);
        set_valid(get_page_addr(page_list[i]), 1);
    }
    for (int i = 0; i <= cluster_size / WRITE_BATCH; i++) {
        // printf("\nbefore:%ld %ld\n", Zone[zoneWid].wp, get_zone_wp(zoneWid));
        if (i != cluster_size / WRITE_BATCH) {
            off_st ret =
                pwrite64(dev_id, write_buffer + i * WRITE_BATCH * PAGE_SIZE,
                         PAGE_SIZE * WRITE_BATCH, Zone[zoneWid].wp);
            assert(ret == PAGE_SIZE * WRITE_BATCH);
            Zone[zoneWid].wp += ret;
            inc_io_count();
        } else {
            off_st ret = pwrite64(
                dev_id, write_buffer + i * WRITE_BATCH * PAGE_SIZE,
                PAGE_SIZE * (cluster_size % WRITE_BATCH), Zone[zoneWid].wp);
            assert(ret == PAGE_SIZE * (cluster_size % WRITE_BATCH));
            Zone[zoneWid].wp += ret;
            inc_io_count();
        }
    }
    return 0;
}

ZONE_ID ZNSController::select_write_zone(int cf, int cluster_size) {
    /*
    ZONE_ID zone_ret = cf;
    for (ZONE_ID i = zone_ret; i < MAX_ZONE_NUM; i += cluster_num)
        if (Zone[i].cond == ZBD_ZONE_COND_EXP_OPEN &&
            Zone[i].wp + cluster_size * PAGE_SIZE <= Zone[i].ofst + cap)
            return i;
    while (Zone[zone_ret].cond == ZBD_ZONE_COND_FULL ||
           Zone[zone_ret].wp + cluster_size * PAGE_SIZE >
               Zone[zone_ret].ofst + cap) {
        if (Zone[zone_ret].cond == ZBD_ZONE_COND_EXP_OPEN) {
            finish_zone(zone_ret);
            Zone[zone_ret].cond = ZBD_ZONE_COND_FULL;
        }
        zone_ret += cluster_num;
    }
    if (Zone[zone_ret].cond == ZBD_ZONE_COND_EMPTY) {
        open_zone(zone_ret);
        zone_used[zone_ret] = true;
        Zone[zone_ret].cond = ZBD_ZONE_COND_EXP_OPEN;
    }
    return zone_ret;
    */
    cf = cf % cluster_num;
    for (ZONE_ID i = 0; i < MAX_ZONE_NUM; i++) {
        if (Zone[i].cf == cf && Zone[i].cond == ZBD_ZONE_COND_EXP_OPEN &&
            Zone[i].wp + cluster_size * PAGE_SIZE <= Zone[i].ofst + cap)
            return i;
        else if (Zone[i].cf == cf && Zone[i].cond == ZBD_ZONE_COND_EXP_OPEN &&
                 Zone[i].wp + cluster_size * PAGE_SIZE > Zone[i].ofst + cap) {
            finish_zone(i);
            Zone[i].cond = ZBD_ZONE_COND_FULL;
        }
    }
    for (ZONE_ID i = 0; i < MAX_ZONE_NUM; i++) {
        if (Zone[i].cf == -1 && Zone[i].cond == ZBD_ZONE_COND_EMPTY) {
            open_zone(i);
            Zone[i].cond = ZBD_ZONE_COND_EXP_OPEN;
            zone_used[i] = true;
            Zone[i].cf = cf;
            return i;
        }
    }
    assert(0);
}

void ZNSController::create_new_page(PAGE_ID page_id, char const *write_buffer) {
    int cf = page_id % cluster_num;
    ZONE_ID zoneWid = select_write_zone(cf, 1);
    set_page_addr(page_id, Zone[zoneWid].wp);
    set_valid(get_page_addr(page_id), 1);
    off_st ret = pwrite64(dev_id, write_buffer, PAGE_SIZE, Zone[zoneWid].wp);
    assert(ret == PAGE_SIZE);
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

void ZNSController::set_valid(off_st page_addr, bool is_valid) {
    if (meta_page[page_addr / PAGE_SIZE] == 0 && is_valid == 1)
        Zone[page_addr / zone_size].valid_page++;
    else if (meta_page[page_addr / PAGE_SIZE] == 1 && is_valid == 0)
        Zone[page_addr / zone_size].valid_page--;
    else
        assert(0);
    meta_page[page_addr / PAGE_SIZE] = is_valid;
}

bool ZNSController::get_valid(off_st page_addr) {
    return meta_page[page_addr / PAGE_SIZE];
}

void ZNSController::io_count_clear() { io_count = 0; }

PAGE_ID ZNSController::get_pages_num() { return pages_num; }

void ZNSController::inc_io_count() { io_count++; }

int ZNSController::get_io_count() { return io_count; }

void ZNSController::inc_pages_num() { pages_num++; }

void ZNSController::get_gc_info() {
    valid_data = 0, real_data = 0;
    for (ZONE_ID i = 0; i < zone_number; i++) {
        if (Zone[i].cond == ZBD_ZONE_COND_EXP_OPEN ||
            Zone[i].cond == ZBD_ZONE_COND_FULL) {
            Zone[i].idle_rate = (double)(Zone[i].wp - Zone[i].ofst) / cap;
            real_data += (double)(Zone[i].wp - Zone[i].ofst) / PAGE_SIZE;
            valid_data += (double)Zone[i].valid_page;
            Zone[i].gc_rate =
                1 - (double)Zone[i].valid_page / (cap / PAGE_SIZE);
        }
    }
}

void ZNSController::print_gc_info() {
    get_gc_info();
    std::ofstream op(output, std::ios::app);
    for (int j = 0; j < cluster_num; j++) {
        op << std::endl;
        for (ZONE_ID i = 0; i < zone_number; i++) {
            if ((int)i % cluster_num == j && Zone[i].idle_rate > 1e-9)
                op << "\n\tzone \t" << i << "\t idle_rate/garbage_rate: \t" << std::setw(8)
                   << Zone[i].gc_rate << std::setw(8)
                   << Zone[i].idle_rate << Zone[i].cond << "\t";
        }
    }
    used_zone = 0;
    for (ZONE_ID i = 0; i < zone_number; i++) used_zone += zone_used[i];
    op << std::endl << "gc_count: " << gc_count << std::endl;
    op << "space amplification: " << (double)real_data / valid_data << std::endl
       << "used zones:" << used_zone << std::endl;
    op.close();
}

void ZNSController::garbage_collection_detect() {
    get_gc_info();
    for (ZONE_ID i = 0; i < zone_number; i++)
        if (Zone[i].idle_rate > 0.80 && Zone[i].gc_rate > 0.80)
            garbage_collection(i);
}

void ZNSController::garbage_collection(ZONE_ID id) {
    printf("%d ", id);
    gc_count++;
    print_gc_info();
    std::ofstream op(output, std::ios::app);
    op << "garbage collection for zone " << id << std::endl;
    op.close();
    for (off_st pos = Zone[id].ofst; pos < Zone[id].ofst + cap;
         pos += PAGE_SIZE) {
        if (get_valid(pos)) {
            int rett;
            char *buf;
            rett = posix_memalign((void **)&buf, MEM_ALIGN_SIZE, FRAME_SIZE);
            assert(rett == 0);
            off_st ret = pread(dev_id, buf, PAGE_SIZE, pos);
            assert(ret == PAGE_SIZE);
            inc_io_count();
            char *read_id_ = (char *)malloc(sizeof(char) * sizeof(PAGE_ID));
            for (long unsigned int i = 0; i < sizeof(PAGE_ID); i++)
                read_id_[i] = buf[i];
            PAGE_ID *read_id = (PAGE_ID *)malloc(sizeof(PAGE_ID));
            memcpy(read_id, read_id_, sizeof(PAGE_ID));
            write_page_p(id, *read_id, reinterpret_cast<char const *>(buf));
            free(buf);
            free(read_id_);
            free(read_id);
        }
    }
    reset_zone(id);
    Zone[id].cond = ZBD_ZONE_COND_EMPTY;
    Zone[id].cf = -1;
    Zone[id].wp = Zone[id].ofst;
    for (off_st pos = Zone[id].ofst; pos < Zone[id].ofst + cap;
         pos += PAGE_SIZE) {
        if (get_valid(pos)) set_valid(pos, 0);
    }
    Zone[id].gc_rate = 0;
    Zone[id].idle_rate = 0;
}

void ZNSController::print_zns_info() {
    zbd_info *info_temp;
    info_temp = (struct zbd_info *)malloc(sizeof(struct zbd_info));
    zbd_get_info(dev_id, info_temp);
    std::cout << " vendor_id: \t" << info_temp->vendor_id << std::endl;
    std::cout << " nr_sectors: \t" << info_temp->nr_sectors << std::endl;
    std::cout << " nr_lblocks: \t" << info_temp->nr_lblocks << std::endl;
    std::cout << " nr_pblocks: \t" << info_temp->nr_pblocks << std::endl;
    std::cout << " zone_size: \t" << info_temp->zone_size << std::endl;
    std::cout << " zone_sectors: \t" << info_temp->zone_sectors << std::endl;
    std::cout << " lblock_size: \t" << info_temp->lblock_size << std::endl;
    std::cout << " pblock_size: \t" << info_temp->pblock_size << std::endl;
    std::cout << " nr_zones: \t" << info_temp->nr_zones << std::endl;
    std::cout << " max_nr_open_zones: \t" << info_temp->max_nr_open_zones
              << std::endl;
    std::cout << " max_nr_active_zones: \t" << info_temp->max_nr_active_zones
              << std::endl;
    std::cout << " model: \t" << info_temp->model << std::endl;
}
}  // namespace zns