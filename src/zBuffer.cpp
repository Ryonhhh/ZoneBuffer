#include <cstring>
#include <iostream>
#include <memory>

#include "../include/zBuffer.h"

namespace zns {
BufferManager::BufferManager() {
    zdsm = new ZNSController();
    output = zdsm->output;
    strategy = new ZALP(output);
    lrus = new LRU;
    free_frames_num = DEF_BUF_SIZE;
    zdsm->cluster_num =
        ach_only ? 3 : (zalp_wc ? HC_LV : (zalp ? CLUSTER_NUM : 1));
    flush_count = (int *)malloc(sizeof(int) * zdsm->cluster_num);
    cd_interval = (zdsm->cap / FRAME_SIZE /2);
    zdsm->zone_select_ptr = (int *)malloc(sizeof(int) * zdsm->cluster_num);
    for (int i = 0; i < zdsm->cluster_num; i++) {
        flush_count[i] = 0;
        zdsm->zone_select_ptr[i] = -1;
    }
    int ret;
    for (int i = 0; i < DEF_BUF_SIZE; i++) {
        buffer[i] = nullptr;
        ret = posix_memalign((void **)(buffer + i), MEM_ALIGN_SIZE, FRAME_SIZE);
        frame_dirty[i] = false;
        assert(ret == 0);
    }
    hit_count = 0;
}

BufferManager::~BufferManager() {
    clean_buffer();
    delete zdsm;
    delete strategy;
}

int BufferManager::hash_func(PAGE_ID page_id) { return page_id % DEF_BUF_SIZE; }

char *BufferManager::read_page(PAGE_ID page_id) {
    int frame_id = fix_page(false, page_id);
    // printf("read frm_id: %d", frame_id);
    char *frame = reinterpret_cast<char *>(memalign(4096, PAGE_SIZE));
    memcpy(frame, buffer[frame_id], FRAME_SIZE);
    read_count[frame_id]++;
    char *read_id_ = (char *)malloc(sizeof(char) * sizeof(PAGE_ID));
    for (long unsigned int i = 0; i < sizeof(PAGE_ID); i++)
        read_id_[i] = *(buffer[frame_id] + i);
    PAGE_ID *read_id = (PAGE_ID *)malloc(sizeof(PAGE_ID));
    memcpy(read_id, read_id_, sizeof(PAGE_ID));
    assert(*read_id == page_id);
    free(read_id);
    return frame;
}

void BufferManager::write_page(PAGE_ID page_id, char *frame) {
    FRAME_ID frame_id = fix_page(true, page_id);
    memcpy(buffer[frame_id], frame, FRAME_SIZE);
    read_count[frame_id]++;
    set_dirty(frame_id);
}

FRAME_ID BufferManager::fix_page(bool is_write, PAGE_ID page_id) {
    FRAME_ID frame_id;
    if (lru) {
        if (!page2frame.find(page_id, frame_id)) {
            if (free_frames_num == 0)
                frame_id = select_victim();
            else {
                frame_id = DEF_BUF_SIZE - free_frames_num;
                free_frames_num--;
            }
            page2frame.insert(page_id, frame_id);
            lrus->push(frame_id);
            set_page_id(frame_id, page_id);
            if (!is_write) {
                zdsm->read_page(page_id, buffer[frame_id]);
            }
            return frame_id;

        } else {
            lrus->update(frame_id);
            inc_hit_count();
            return frame_id;
        }
    } else {
        if (!page2frame.find(page_id, frame_id)) {
            if (strategy->is_evict()) {
                if (!ach_only)
                    evict_victim();
                else
                    evict_victim_ach();
            }
            int is_free = strategy->get_frame(&frame_id);
            if (!is_free) {
                PAGE_ID victim_page_id = get_page_id(frame_id);
                page2frame.erase(victim_page_id);
            }
            page2frame.insert(page_id, frame_id);
            strategy->push(frame_id, 0);
            set_page_id(frame_id, page_id);
            if (!is_write) {
                if (zalp_wc) {
                    cluster_flag[frame_id] =
                        zdsm->read_page(page_id, buffer[frame_id]) * 0;
                    write_count[frame_id] = 0;
                    read_count[frame_id] = 0;
                } else if (ach_only) {
                    zdsm->read_page(page_id, buffer[frame_id]);
                } else if (zalp) {
                    cluster_flag[frame_id] =
                        zdsm->read_page(page_id, buffer[frame_id]) %
                        zdsm->cluster_num;
                } else if (cflru) {
                    zdsm->read_page(page_id, buffer[frame_id]);
                }
            }
        } else {
            if (zalp_wc) cluster_flag[frame_id]++;
            strategy->update(frame_id, frame_dirty[frame_id]);
            inc_hit_count();
        }
    }
    // strategy->print_list();
    return frame_id;
}

void BufferManager::fix_new_page(PAGE_ID page_id, char *frame) {
    FRAME_ID frame_id;
    if (lru) {
        if (free_frames_num == 0) {
            frame_id = select_victim();
        } else {
            frame_id = DEF_BUF_SIZE - free_frames_num;
            free_frames_num--;
        }
        memcpy(buffer[frame_id], frame, FRAME_SIZE);
        int ret;
        char *write_buffer;
        ret =
            posix_memalign((void **)&write_buffer, MEM_ALIGN_SIZE, FRAME_SIZE);
        assert(ret == 0);
        memcpy(write_buffer, frame, FRAME_SIZE);
        zdsm->create_new_page(page_id,
                              reinterpret_cast<char const *>(write_buffer));
        page2frame.insert(page_id, frame_id);
        lrus->push(frame_id);
        set_page_id(frame_id, page_id);
        free(write_buffer);
        write_buffer = nullptr;
    } else {
        if (strategy->is_evict()) {
            if (!ach_only)
                evict_victim();
            else
                evict_victim_ach();
        }
        int is_free = strategy->get_frame(&frame_id);
        if (!is_free) {
            PAGE_ID victim_page_id = get_page_id(frame_id);
            page2frame.erase(victim_page_id);
        }

        memcpy(buffer[frame_id], frame, FRAME_SIZE);
        int ret;
        char *write_buffer;
        ret =
            posix_memalign((void **)&write_buffer, MEM_ALIGN_SIZE, FRAME_SIZE);
        assert(ret == 0);
        memcpy(write_buffer, frame, FRAME_SIZE);
        zdsm->create_new_page(page_id,
                              reinterpret_cast<char const *>(write_buffer));
        insert_count++;
        cd_count++;
        if (zalp_wc) {
            cluster_flag[frame_id] = 0;
            write_count[frame_id] = 0;
            read_count[frame_id] = 0;
        } else if (zalp) {
            cluster_flag[frame_id] = page_id % zdsm->cluster_num;
        }
        page2frame.insert(page_id, frame_id);
        strategy->push(frame_id, false);
        set_page_id(frame_id, page_id);
        free(write_buffer);
        write_buffer = nullptr;
    }
}

int BufferManager::select_victim() {
    int frame_id = lrus->get_victim();
    PAGE_ID victim_page_id = get_page_id(frame_id);

    if (frame_dirty[frame_id]) {
        int ret;
        char *write_buffer;
        ret =
            posix_memalign((void **)&write_buffer, MEM_ALIGN_SIZE, FRAME_SIZE);
        assert(ret == 0);
        memcpy(write_buffer, buffer[frame_id], FRAME_SIZE);
        zdsm->write_page(0, victim_page_id,
                         reinterpret_cast<char const *>(write_buffer));
        flush_count[0]++;
        zdsm->buffer_write_count = evict_count + insert_count;
        zdsm->garbage_collection_detect();
        free(write_buffer);
        write_buffer = nullptr;
    }
    page2frame.erase(frame_to_page[frame_id]);

    return frame_id;
}

int BufferManager::HC_Lv(int cluster_flag, int max) {
    if (cluster_flag == 0)
        return cold;
    else if (cluster_flag == 1)
        return warm;
    else
        return hot;
}

void BufferManager::evict_victim() {
    int cf;
    std::list<FRAME_ID> *candidate_list = new std::list<FRAME_ID>;
    std::list<FRAME_ID> *victim_list = new std::list<FRAME_ID>;

    if (zalp_wc) {
        if (wh_only) {
            strategy->get_candidate(candidate_list);
            int dirty_cluster[HC_LV] = {};
            int max = 0;
            for (auto &iter : *candidate_list)
                if (cluster_flag[iter] > max) max = cluster_flag[iter];
            for (auto &iter : *candidate_list) {
                switch (HC_Lv(cluster_flag[iter], max)) {
                    case cold:
                        dirty_cluster[cold]++;
                        break;
                    case warm:
                        dirty_cluster[warm]++;
                        break;
                    case hot:
                        dirty_cluster[hot]++;
                        break;
                }
            }
            cf = hot;
            if (dirty_cluster[hot] < (DEF_BUF_SIZE - WORK_REG_SIZE) / 4) {
                int cfnum = -1;
                for (int i = 0; i < HC_LV; i++) {
                    cf = cfnum > dirty_cluster[i] ? cf : i;
                    cfnum = cfnum > dirty_cluster[i] ? cfnum : dirty_cluster[i];
                }
                assert(cf != -1);
            }

            for (auto &iter : *candidate_list) {
                if (HC_Lv(cluster_flag[iter], max) == cf)
                    victim_list->push_back(iter);
            }
            strategy->dirty_to_clean(victim_list);
            evict_count = (evict_count + victim_list->size());
            cd_count = cd_count + victim_list->size();
        } else {
            strategy->get_candidate(candidate_list);
            int dirty_cluster[HC_LV] = {};
            int max = 0;
            for (auto &iter : *candidate_list)
                if (cluster_flag[iter] > max) max = cluster_flag[iter];
            for (auto &iter : *candidate_list) {
                switch (HC_Lv(cluster_flag[iter], max)) {
                    case cold:
                        dirty_cluster[cold]++;
                        break;
                    case warm:
                        dirty_cluster[warm]++;
                        break;
                    case hot:
                        dirty_cluster[hot]++;
                        break;
                }
            }

            cf = -1;
            int cfnum = -1;
            for (int i = 0; i < HC_LV; i++) {
                cf = cfnum > dirty_cluster[i] ? cf : i;
                cfnum = cfnum > dirty_cluster[i] ? cfnum : dirty_cluster[i];
            }
            assert(cf != -1);

            for (auto &iter : *candidate_list) {
                if (HC_Lv(cluster_flag[iter], max) == cf)
                    victim_list->push_back(iter);
            }
            strategy->update_dirty(victim_list);
            evict_count = (evict_count + victim_list->size());
            cd_count = cd_count + victim_list->size();
        }
    } else if (zalp) {
        strategy->get_candidate(candidate_list);
        int *dirty_cluster = (int *)malloc(sizeof(int) * zdsm->cluster_num);
        for (int i = 0; i < zdsm->cluster_num; i++) dirty_cluster[i] = 0;

        for (auto &iter : *candidate_list) {
            dirty_cluster[cluster_flag[iter]]++;
        }

        cf = -1;
        int cfnum = -1;
        for (int i = 0; i < zdsm->cluster_num; i++) {
            cf = cfnum > dirty_cluster[i] ? cf : i;
            cfnum = cfnum > dirty_cluster[i] ? cfnum : dirty_cluster[i];
        }
        assert(cf != -1);

        for (auto &iter : *candidate_list) {
            if (cluster_flag[iter] == cf) victim_list->push_back(iter);
        }
        strategy->dirty_to_clean(victim_list);
        evict_count = (evict_count + victim_list->size());
        cd_count = cd_count + victim_list->size();
        free(dirty_cluster);
    } else if (cflru) {
        strategy->get_candidate_cflru(victim_list);
        cf = 0;
        strategy->update_dirty(victim_list);
        evict_count++;
        cd_count++;
    }
    int ret;
    char *write_buffer;
    ret = posix_memalign((void **)&write_buffer, MEM_ALIGN_SIZE,
                         victim_list->size() * FRAME_SIZE);
    assert(ret == 0);
    PAGE_ID *page_list =
        (PAGE_ID *)malloc(sizeof(PAGE_ID) * victim_list->size());
    int i = 0;
    for (auto &iter : *victim_list) {
        memcpy(write_buffer + i * FRAME_SIZE, buffer[iter], FRAME_SIZE);
        page_list[i] = get_page_id(iter);
        i++;
    }
    if (victim_list->size() == 1){
        zdsm->write_page(cf, *page_list, write_buffer);
        flush_count[cf]++;
    }
    else{
        zdsm->write_cluster(cf, write_buffer, page_list, victim_list->size());
        flush_count[cf] += victim_list->size();
    }
    free(write_buffer);
    write_buffer = nullptr;
    

    for (auto &iter : *victim_list) {
        PAGE_ID victim_page_id = get_page_id(iter);
        page2frame.erase(victim_page_id);
        unset_dirty(iter);
    }
    if (cd_detect == true && cd_count > cd_interval) {
        std::cout << "d" << std::flush;
        cd_count = cd_count % cd_interval;
        zdsm->buffer_write_count = evict_count + insert_count;
        zdsm->cd_detect();
    }
    zdsm->buffer_write_count = evict_count + insert_count;
    zdsm->garbage_collection_detect();
    delete victim_list;
    delete candidate_list;
    free(page_list);
}

bool BufferManager::ach_clutser(int lv, std::pair<ACCTIME, ACCTIME> acht) {
    switch (lv) {
        case 0:
            if (acht.first == -1)
                return 1;
            else
                return 0;
            break;
        case 1:
            if (acht.second - acht.first < 8 * (DEF_BUF_SIZE - WORK_REG_SIZE))
                return 1;
            else
                return 0;
            break;
        case 2:
            if (acht.second - acht.first >= 8 * (DEF_BUF_SIZE - WORK_REG_SIZE))
                return 1;
            else
                return 0;
            break;
        default:
            assert(0);
    }
}

void BufferManager::evict_victim_ach() {
    int cf;
    std::list<FRAME_ID> *candidate_list = new std::list<FRAME_ID>;

    strategy->get_candidate(candidate_list);
    for (auto &iter : *candidate_list) {
        PAGE_ID page_id_ = get_page_id(iter);
        std::pair<ACCTIME, ACCTIME> p;
        if (!accessH.find(page_id_, p)) {
            std::pair<ACCTIME, ACCTIME> ins;
            ins.first = -1;
            ins.second = evict_count + insert_count;
            accessH.insert(page_id_, ins);
        } else {
            std::pair<ACCTIME, ACCTIME> ins;
            ins.first = p.second;
            ins.second = evict_count + insert_count;
            accessH.insert_or_assign(page_id_, ins);
        }
    }

    for (int t = 0; t < 3; t++) {
        std::list<FRAME_ID> *victim_list = new std::list<FRAME_ID>;
        cf = t;
        for (auto &iter : *candidate_list) {
            if (ach_clutser(cf, accessH.find(get_page_id(iter)))) {
                victim_list->push_back(iter);
            }
        }
        int ret;
        char *write_buffer;
        ret = posix_memalign((void **)&write_buffer, MEM_ALIGN_SIZE,
                             victim_list->size() * FRAME_SIZE);
        assert(ret == 0);
        PAGE_ID *page_list =
            (PAGE_ID *)malloc(sizeof(PAGE_ID) * victim_list->size());
        int i = 0;
        for (auto &iter : *victim_list) {
            memcpy(write_buffer + i * FRAME_SIZE, buffer[iter], FRAME_SIZE);
            page_list[i] = get_page_id(iter);
            i++;
        }
        zdsm->write_cluster(cf, write_buffer, page_list, victim_list->size());
        free(write_buffer);
        write_buffer = nullptr;
        flush_count[cf] += victim_list->size();
        free(victim_list);
    }

    evict_count = (evict_count + candidate_list->size());
    cd_count = cd_count + candidate_list->size();
    for (auto &iter : *candidate_list) {
        PAGE_ID victim_page_id = get_page_id(iter);
        page2frame.erase(victim_page_id);
        unset_dirty(iter);
    }
    if (cd_detect == true && cd_count > cd_interval) {
        std::cout << "d" << std::flush;
        cd_count = cd_count % cd_interval;
        zdsm->buffer_write_count = evict_count + insert_count;
        zdsm->cd_detect();
    }
    strategy->dirty_to_clean(candidate_list);
    zdsm->buffer_write_count = evict_count + insert_count;
    zdsm->garbage_collection_detect();
    delete candidate_list;
}

void BufferManager::clean_buffer() {
    std::ofstream op(output, std::ios::app);
    std::cout << std::endl << "Cleaning buffer" << std::endl;
    for (int i = 0; i < DEF_BUF_SIZE; i++)
        if (frame_dirty[i]) {
            int ret;
            char *write_buffer;
            ret = posix_memalign((void **)&write_buffer, MEM_ALIGN_SIZE,
                                 FRAME_SIZE);
            assert(ret == 0);
            memcpy(write_buffer, buffer[i], FRAME_SIZE);
            zdsm->write_page(0, frame_to_page[i],
                             reinterpret_cast<char const *>(write_buffer));
            free(write_buffer);
            write_buffer = nullptr;
        }

    op << std::endl << "flush count: ";

    for (int i = 0; i < zdsm->cluster_num; i++) {
        op << flush_count[i] << " ";
    }

    op << std::endl;

    std::cout << "IO count: " << get_io_count() << std::endl;
    std::cout << "hit/query: " << get_hit_count() << std::endl;
    op << "IO count: " << get_io_count() << std::endl;
    op << "hit/query: " << get_hit_count() << std::endl;
    op << "ach_only:" << ach_only << " zalp_wc:" << zalp_wc << " zalp:" << zalp
       << " cflru:" << cflru << " lru:" << lru << std::endl;
    op.close();
    zdsm->print_gc_info();
}

void BufferManager::set_dirty(FRAME_ID frame_id) {
    frame_dirty[frame_id] = true;
    if (!lru) {
        strategy->set_dirty(frame_id);
    }
}

void BufferManager::unset_dirty(int frame_id) { frame_dirty[frame_id] = false; }

PAGE_ID BufferManager::get_page_id(FRAME_ID frame_id) {
    return frame_to_page[frame_id];
}

void BufferManager::set_page_id(FRAME_ID frame_id, PAGE_ID page_id) {
    frame_to_page[frame_id] = page_id;
}

void BufferManager::hit_count_clear() {
    hit_count = 0;
    zdsm->io_count_clear();
}

int BufferManager::get_free_frames_num() { return free_frames_num; }

int BufferManager::get_io_count() { return zdsm->get_io_count(); }

void BufferManager::inc_hit_count() { hit_count++; }

int BufferManager::get_hit_count() { return hit_count; }
}  // namespace zns