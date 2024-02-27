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
    zone_list = (std::list<int> *)malloc(sizeof(std::list<int>) * cluster_num);
    for (ZONE_ID i = 0; i < MAX_ZONE_NUM; i++) {
        free_zone_list.push_back(i);
    }
    io_count = 0;
}

ZNSController::~ZNSController() {
    for (ZONE_ID i = 0; i < zone_number; i++) {
        while (Zone[i].inGc == true)
            ;
    }
    free(meta_page);
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
    reset_all();
    for (ZONE_ID i = 0; i < zone_number; i++) {
        Zone[i].id = i;
        Zone[i].ofst = i * (off_st)zone_size;
        Zone[i].valid_page = 0;
        Zone[i].cond = ZBD_ZONE_COND_EMPTY;
        Zone[i].wp = Zone[i].ofst;
        Zone[i].cf = -1;
        zone_used[i] = false;
        Zone[i].inGc = false;
        Zone[i].inRe = false;
    }
    for (PAGE_ID i = 0; i < block_per_zone * zone_number; i++) meta_page[i] = 0;
}

ZONE_ID ZNSController::read_page(PAGE_ID page_id, char *frame) {
    off_st page_addr = get_page_addr(page_id);
    char *buf = reinterpret_cast<char *>(memalign(4096, PAGE_SIZE));
    assert(buf != nullptr);
    off_st ret = pread(dev_id, buf, PAGE_SIZE, page_addr);
    inc_io_count();
    assert(ret == PAGE_SIZE);
    memcpy(frame, buf, PAGE_SIZE);
    free(buf);
    return page_addr / zone_size;
}

void ZNSController::write_page(int cf, PAGE_ID page_id,
                               char const *write_buffer) {
    ZONE_ID zoneWid;
    if (covzs) {
        zoneWid = select_write_zone(cf, 1);
    } else {
        zoneWid = select_write_zone_ach(cf, 1);
    }
    Zone[zoneWid].wMutex.lock();
    if (Zone[zoneWid].cond != ZBD_ZONE_COND_EXP_OPEN) {
        Zone[zoneWid].wMutex.unlock();
        write_page(cf, page_id, write_buffer);
        return;
    }
    set_valid(get_page_addr(page_id), 0);
    set_page_addr(page_id, Zone[zoneWid].wp);
    off_st ret = pwrite64(dev_id, write_buffer, PAGE_SIZE, Zone[zoneWid].wp);
    assert(ret == PAGE_SIZE);
    Zone[zoneWid].wp += ret;
    set_valid(get_page_addr(page_id), 1);
    Zone[zoneWid].wMutex.unlock();
    write_count++;
    inc_io_count();
}

void ZNSController::write_page_gc(int cf, PAGE_ID page_id,
                                  char const *write_buffer) {
    ZONE_ID zoneWid;
    if (covzs) {
        zoneWid = select_write_zone(cf, 1);
    } else {
        zoneWid = select_write_zone_ach(cf, 1);
    }
    Zone[zoneWid].wMutex.lock();
    if (Zone[zoneWid].cond != ZBD_ZONE_COND_EXP_OPEN) {
        Zone[zoneWid].wMutex.unlock();
        write_page_gc(cf, page_id, write_buffer);
        return;
    }
    set_valid_gc(get_page_addr(page_id), 0);
    off_st ret = pwrite64(dev_id, write_buffer, PAGE_SIZE, Zone[zoneWid].wp);
    assert(ret == PAGE_SIZE);
    set_page_addr(page_id, Zone[zoneWid].wp);
    set_valid_gc(get_page_addr(page_id), 1);
    Zone[zoneWid].wp += ret;
    Zone[zoneWid].wMutex.unlock();
    inc_io_count();
    write_count++;
    return;
}

void ZNSController::write_cluster(int cf, char const *write_buffer,
                                  PAGE_ID *page_list, int cluster_size) {
    ZONE_ID zoneWid;
    if (covzs) {
        zoneWid = select_write_zone(cf, cluster_size);
    } else {
        zoneWid = select_write_zone_ach(cf, cluster_size);
    }
    Zone[zoneWid].wMutex.lock();
    if (Zone[zoneWid].cond != ZBD_ZONE_COND_EXP_OPEN) {
        Zone[zoneWid].wMutex.unlock();
        write_cluster(cf, write_buffer, page_list, cluster_size);
        return;
    }
    for (int i = 0; i < cluster_size; i++) {
        set_valid(get_page_addr(page_list[i]), 0);
        set_page_addr(page_list[i], Zone[zoneWid].wp + i * PAGE_SIZE);
        set_valid(get_page_addr(page_list[i]), 1);
    }
    for (int i = 0; i <= cluster_size / WRITE_BATCH; i++) {
        if (i != cluster_size / WRITE_BATCH) {
            off_st ret =
                pwrite64(dev_id, write_buffer + i * WRITE_BATCH * PAGE_SIZE,
                         PAGE_SIZE * WRITE_BATCH, Zone[zoneWid].wp);
            assert(ret == PAGE_SIZE * WRITE_BATCH);
            Zone[zoneWid].wp += ret;
            //inc_io_count();
            write_count += WRITE_BATCH;
        } else {
            off_st ret = pwrite64(
                dev_id, write_buffer + i * WRITE_BATCH * PAGE_SIZE,
                PAGE_SIZE * (cluster_size % WRITE_BATCH), Zone[zoneWid].wp);
            assert(ret == PAGE_SIZE * (cluster_size % WRITE_BATCH));
            Zone[zoneWid].wp += ret;
            //inc_io_count();
            write_count += cluster_size % WRITE_BATCH;
        }
    }
    inc_io_count();
    Zone[zoneWid].wMutex.unlock();
}

ZONE_ID ZNSController::select_write_zone(int cf, int cluster_size) {
    cf = cf % cluster_num;
    for (ZONE_ID i = 0; i < MAX_ZONE_NUM; i++) {
        if (Zone[i].cf == cf && Zone[i].cond == ZBD_ZONE_COND_EXP_OPEN &&
            Zone[i].wp + cluster_size * PAGE_SIZE <= Zone[i].ofst + cap)
            return i;
        else if (Zone[i].cf == cf && Zone[i].cond == ZBD_ZONE_COND_EXP_OPEN &&
                 Zone[i].wp + cluster_size * PAGE_SIZE > Zone[i].ofst + cap) {
            Zone[i].wMutex.lock();
            finish_zone(i);
            Zone[i].cond = ZBD_ZONE_COND_FULL;
            Zone[i].wMutex.unlock();
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

ZONE_ID ZNSController::select_write_zone_ach(int cf, int cluster_size) {
    if (cf == 0) {
        if (zone_list[0].size() == 0) {
            zone_list[0].push_front(free_zone_list.front());
            free_zone_list.pop_front();
            Zone[free_zone_list.front()].cf = 0;
            open_zone(free_zone_list.front());
            Zone[free_zone_list.front()].cond = ZBD_ZONE_COND_EXP_OPEN;
        } else {
            ;
        }
    }
}

void ZNSController::create_new_page(PAGE_ID page_id, char const *write_buffer) {
    int cf = 0;
    ZONE_ID zoneWid;
    if (covzs) {
        zoneWid = select_write_zone(cf, 1);
    } else {
        zoneWid = select_write_zone_ach(cf, 1);
    }
    Zone[zoneWid].wMutex.lock();
    if (Zone[zoneWid].cond != ZBD_ZONE_COND_EXP_OPEN) {
        Zone[zoneWid].wMutex.unlock();
        write_page(cf, page_id, write_buffer);
        return;
    }
    set_page_addr(page_id, Zone[zoneWid].wp);
    set_valid(get_page_addr(page_id), 1);
    off_st ret = pwrite64(dev_id, write_buffer, PAGE_SIZE, Zone[zoneWid].wp);
    assert(ret == PAGE_SIZE);
    Zone[zoneWid].wp += ret;
    Zone[zoneWid].wMutex.unlock();
    inc_io_count();
    write_count++;
}

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
    for (ZONE_ID i = 0; i < zone_number; i++) {
        if (Zone[i].idle_rate > 0) {
            if ((int)i % cluster_num == 0) op << std::endl;
            op << "[" << std::setw(3) << std::setfill(' ') << i << "]  "
               << Zone[i].cf << " " << std::setw(6) << std::setprecision(4)
               << Zone[i].idle_rate << "/" << std::setw(6)
               << std::setprecision(4) << Zone[i].gc_rate << "/" << std::setw(6)
               << std::setprecision(4) << Zone[i].gc_rate_last << "\t";
        }
    }

    used_zone = 0;
    for (ZONE_ID i = 0; i < zone_number; i++) used_zone += zone_used[i];
    op << std::endl << "cd_count: " << cd_count << std::endl;
    op << std::endl << "gc_count: " << gc_count << std::endl;
    op << std::endl << "gc_valid_count: " << gc_valid_count << std::endl;
    op << "space amplification: " << (double)real_data / valid_data
       << std::endl;
    op << "write amplification: " << (double)write_count / buffer_write_count
       << std::endl;
    op << "used zones:" << used_zone << std::endl;
    op.close();
}

void ZNSController::cd_detect() {
    while (Gc == 1)
        ;
    if (Cd == 0 && Gc == 0) {
        get_gc_info();
        std::list<ZONE_ID> restruct;
        for (ZONE_ID i = 0; i < zone_number; i++) {
            if (Zone[i].cond == ZBD_ZONE_COND_FULL &&
                Zone[i].gc_rate - Zone[i].gc_rate_last < 0.005 &&
                Zone[i].gc_rate - Zone[i].gc_rate_last > 0 &&
                Zone[i].idle_rate * (1 - Zone[i].gc_rate) < 0.6)
                restruct.push_back(i);
        }
        if (restruct.size() >= 3) {
            print_gc_info();
            std::thread Cdre(&ZNSController::cold_data_restruct, this,
                             restruct);
            Cdre.detach();
        }
        get_gc_info();
        for (ZONE_ID i = 0; i < zone_number; i++) {
            Zone[i].gc_rate_last = Zone[i].gc_rate;
        }
    }
}

void ZNSController::cold_data_restruct(std::list<ZONE_ID> restruct) {
    Cd = 1;
    cd_count++;

    for (auto &iter : restruct) {
        std::ofstream op(output, std::ios::app);
        op << "cold data restruct for zone " << iter << std::endl;
        op.close();
        for (off_st pos = Zone[iter].ofst; pos < Zone[iter].ofst + cap;
             pos += PAGE_SIZE) {
            if (get_valid(pos)) {
                off_st ret;
                char *buf;
                ret = posix_memalign((void **)&buf, MEM_ALIGN_SIZE, FRAME_SIZE);
                assert(ret == 0);
                ret = pread(dev_id, buf, PAGE_SIZE, pos);
                assert(ret == PAGE_SIZE);
                //inc_io_count();
                write_count++;
                char *read_id_ = (char *)malloc(sizeof(char) * sizeof(PAGE_ID));
                for (long unsigned int i = 0; i < sizeof(PAGE_ID); i++)
                    read_id_[i] = buf[i];
                PAGE_ID *read_id = (PAGE_ID *)malloc(sizeof(PAGE_ID));
                memcpy(read_id, read_id_, sizeof(PAGE_ID));
                write_page_gc(0, *read_id, reinterpret_cast<char const *>(buf));
                free(buf);
                free(read_id_);
                free(read_id);
            }
        }
        reset_zone(iter);
        Zone[iter].cond = ZBD_ZONE_COND_EMPTY;
        Zone[iter].cf = -1;
        Zone[iter].wp = Zone[iter].ofst;
        for (off_st pos = Zone[iter].ofst; pos < Zone[iter].ofst + cap;
             pos += PAGE_SIZE) {
            if (get_valid(pos)) set_valid(pos, 0);
        }
        Zone[iter].gc_rate = 0;
        Zone[iter].idle_rate = 0;
    }

    Cd = 0;
}

void ZNSController::garbage_collection_detect() {
    while (Cd == 1)
        ;
    get_gc_info();
    if (Gc == 0)
        for (ZONE_ID i = 0; i < zone_number; i++)
            if (Zone[i].cond == ZBD_ZONE_COND_FULL && Zone[i].gc_rate > 0.85 &&
                Zone[i].inGc == false) {
                Zone[i].inGc = true;
                std::thread BGgc(&ZNSController::garbage_collection, this, i);
                BGgc.detach();
            }
}

void ZNSController::garbage_collection(ZONE_ID id) {
    printf("%d ", id);
    Gc = 1;
    gc_count++;
    print_gc_info();
    std::ofstream op(output, std::ios::app);
    op << "garbage collection for zone " << id << std::endl;
    op.close();
    for (off_st pos = Zone[id].ofst; pos < Zone[id].ofst + cap;
         pos += PAGE_SIZE) {
        if (get_valid(pos)) {
            gc_valid_count++;
            off_st ret;
            char *buf;
            ret = posix_memalign((void **)&buf, MEM_ALIGN_SIZE, FRAME_SIZE);
            assert(ret == 0);
            ret = pread(dev_id, buf, PAGE_SIZE, pos);
            assert(ret == PAGE_SIZE);
            //inc_io_count();
            write_count++;
            char *read_id_ = (char *)malloc(sizeof(char) * sizeof(PAGE_ID));
            for (long unsigned int i = 0; i < sizeof(PAGE_ID); i++)
                read_id_[i] = buf[i];
            PAGE_ID *read_id = (PAGE_ID *)malloc(sizeof(PAGE_ID));
            memcpy(read_id, read_id_, sizeof(PAGE_ID));
            write_page_gc(Zone[id].cf, *read_id,
                          reinterpret_cast<char const *>(buf));
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
    Zone[id].inGc = false;
    Gc = 0;
}

void ZNSController::open_file() {
    info = (struct zbd_info *)malloc(sizeof(struct zbd_info));
    if ((dev_id = zbd_open(ZNS_PATH, O_RDWR | O_DIRECT, info)) == -1) {
        printf("can't open device!\n");
        exit(0);
    }
    printf("open device successfully!\n");
    print_zns_info();
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
    return Table.find(page_id);
    ;
}

void ZNSController::set_page_addr(PAGE_ID page_id, off_st addr) {
    off_st offt;
    if (Table.find(page_id, offt)) Table.erase(page_id);
    Table.insert(page_id, addr);
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

void ZNSController::set_valid_gc(off_st page_addr, bool is_valid) {
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