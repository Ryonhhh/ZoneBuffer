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
    zdsm->cluster_num = zalp_wc ? HC_LV : (zalp ? CLUSTER_NUM : 1);
    flush_count = (int *)malloc(sizeof(int) * zdsm->cluster_num);
    zdsm->zone_select_ptr = (int *)malloc(sizeof(int) * zdsm->cluster_num);
    for (int i = 0; i < zdsm->cluster_num; i++) {
        flush_count[i] = 0;
        zdsm->zone_select_ptr[i] = -1;
    }
    int ret;
    for (int i = 0; i < DEF_BUF_SIZE; i++) {
        buffer[i] = nullptr;
        ret = posix_memalign((void **)(buffer + i), MEM_ALIGN_SIZE, FRAME_SIZE);
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
    BCB *bcb = get_bcb(page_id);

    FRAME_ID frame_id;
    if (lru) {
        if (bcb == nullptr) {
            if (free_frames_num == 0)
                frame_id = select_victim();
            else {
                frame_id = DEF_BUF_SIZE - free_frames_num;
                free_frames_num--;
            }
            insert_bcb(page_id, frame_id);
            lrus->push(frame_id);
            set_page_id(frame_id, page_id);
            if (!is_write) {
                zdsm->read_page_p(page_id, buffer[frame_id]);
            }
            return frame_id;

        } else {
            frame_id = bcb->get_frame_id();
            lrus->update(frame_id);
            inc_hit_count();
            return frame_id;
        }
    } else {
        if (bcb == nullptr) {
            if (strategy->is_evict()) evict_victim();
            int is_free = strategy->get_frame(&frame_id);
            if (!is_free) {
                PAGE_ID victim_page_id = get_page_id(frame_id);
                int hash = hash_func(victim_page_id);
                auto bcb_list = page_to_frame + hash;
                for (auto it = bcb_list->begin(); it != bcb_list->end(); it++) {
                    if (it->get_page_id() == victim_page_id) {
                        bcb_list->erase(it);
                        break;
                    }
                }
            }
            insert_bcb(page_id, frame_id);
            strategy->push(frame_id, 0);
            set_page_id(frame_id, page_id);
            if (!is_write) {
                if (zalp_wc) {
                    cluster_flag[frame_id] =
                        zdsm->read_page_p(page_id, buffer[frame_id]) * 0;
                    write_count[frame_id] = 0;
                    read_count[frame_id] = 0;
                } else if (zalp) {
                    cluster_flag[frame_id] =
                        zdsm->read_page_p(page_id, buffer[frame_id]) %
                        zdsm->cluster_num;
                } else if (cflru) {
                    zdsm->read_page_p(page_id, buffer[frame_id]);
                }
            }
        } else {
            frame_id = bcb->get_frame_id();
            if (zalp_wc) cluster_flag[frame_id]++;
            strategy->update(frame_id, bcb->is_dirty());
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
        zdsm->create_new_page(page_id, reinterpret_cast<char const *>(write_buffer));
        insert_bcb(page_id, frame_id);
        lrus->push(frame_id);
        set_page_id(frame_id, page_id);
        free(write_buffer);
        write_buffer = nullptr;
    } else {
        if (strategy->is_evict()) evict_victim();
        int is_free = strategy->get_frame(&frame_id);
        if (!is_free) {
            PAGE_ID victim_page_id = get_page_id(frame_id);
            int hash = hash_func(victim_page_id);
            auto bcb_list = page_to_frame + hash;
            for (auto it = bcb_list->begin(); it != bcb_list->end(); it++) {
                if (it->get_page_id() == victim_page_id) {
                    bcb_list->erase(it);
                    break;
                }
            }
        }

        memcpy(buffer[frame_id], frame, FRAME_SIZE);
        int ret;
        char *write_buffer;
        ret =
            posix_memalign((void **)&write_buffer, MEM_ALIGN_SIZE, FRAME_SIZE);
        assert(ret == 0);
        memcpy(write_buffer, frame, FRAME_SIZE);
        zdsm->create_new_page(page_id, reinterpret_cast<char const *>(write_buffer));
        if (zalp_wc) {
            cluster_flag[frame_id] = 0;
            write_count[frame_id] = 0;
            read_count[frame_id] = 0;
        } else if (zalp) {
            cluster_flag[frame_id] = page_id % zdsm->cluster_num;
        }
        insert_bcb(page_id, frame_id);
        strategy->push(frame_id, 0);
        set_page_id(frame_id, page_id);
        free(write_buffer);
        write_buffer = nullptr;
    }
}

int BufferManager::select_victim() {
    int frame_id = lrus->get_victim();
    PAGE_ID victim_page_id = get_page_id(frame_id);

    int hash = hash_func(victim_page_id);
    auto bcb_list = page_to_frame + hash;
    for (auto iter = bcb_list->begin(); iter != bcb_list->end(); iter++) {
        if (iter->get_page_id() == victim_page_id) {
            if (iter->is_dirty()) {
                int ret;
                char *write_buffer;
                ret = posix_memalign((void **)&write_buffer, MEM_ALIGN_SIZE,
                                     FRAME_SIZE);
                assert(ret == 0);
                memcpy(write_buffer, buffer[frame_id], FRAME_SIZE);
                zdsm->write_page_p(
                    0, victim_page_id,
                    reinterpret_cast<char const *>(write_buffer));
                zdsm->garbage_collection_detect();
                free(write_buffer);
                write_buffer = nullptr;
            }
            bcb_list->erase(iter);
            break;
        }
    }
    flush_count[0]++;
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
    std::list<FRAME_ID> *candidate_list= new std::list<FRAME_ID>;
    std::list<FRAME_ID> *victim_list= new std::list<FRAME_ID>;

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
            strategy->update_dirty_readhot(victim_list);
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
        strategy->update_dirty(victim_list);
        free(dirty_cluster);
    } else if (cflru) {
        strategy->get_candidate_cflru(victim_list);
        cf = 0;
        strategy->update_dirty(victim_list);
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
    zdsm->write_cluster_p(cf, write_buffer, page_list, victim_list->size());
    free(write_buffer);
    write_buffer = nullptr;
    flush_count[cf] += victim_list->size();

    for (auto &iter : *victim_list) {
        PAGE_ID victim_page_id = get_page_id(iter);
        int hash = hash_func(victim_page_id);
        auto bcb_list = page_to_frame + hash;
        for (auto it = bcb_list->begin(); it != bcb_list->end(); it++) {
            if (it->get_page_id() == victim_page_id) {
                bcb_list->erase(it);
                break;
            }
        }
    }
    zdsm->garbage_collection_detect();
    victim_list->clear();
    candidate_list->clear();
    victim_list = nullptr;
    candidate_list = nullptr;
    delete(victim_list);
    delete(candidate_list);
    free(page_list);
}

void BufferManager::clean_buffer() {
    // strategy->print_list();
    std::ofstream op(output, std::ios::app);
    std::cout << std::endl << "Cleaning buffer" << std::endl;
    for (const auto &bcb_list : page_to_frame)
        for (const auto &iter : bcb_list)
            if (iter.is_dirty()) {
                int ret;
                char *write_buffer;
                ret = posix_memalign((void **)&write_buffer, MEM_ALIGN_SIZE,
                                     FRAME_SIZE);
                assert(ret == 0);
                memcpy(write_buffer, buffer[iter.get_frame_id()], FRAME_SIZE);
                zdsm->write_page_p(
                    0, iter.get_page_id(),
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
    op << "zalp_wc:" << zalp_wc << " zalp:" << zalp << " cflru:" << cflru
       << " lru:" << lru << std::endl;
    op.close();
    zdsm->print_gc_info();
}

void BufferManager::set_dirty(FRAME_ID frame_id) {
    PAGE_ID page_id = get_page_id(frame_id);
    BCB *bcb = get_bcb(page_id);
    assert(bcb != nullptr);
    if (lru) {
        bcb->set_dirty();
    } else {
        if (!bcb->is_dirty()) {
            bcb->set_dirty();
            strategy->set_dirty(frame_id);
        }
    }
}

void BufferManager::unset_dirty(int frame_id) {
    PAGE_ID page_id = get_page_id(frame_id);
    BCB *bcb = get_bcb(page_id);
    bcb->unset_dirty();
}

BCB *BufferManager::get_bcb(PAGE_ID page_id) {
    int hash = hash_func(page_id);
    auto bcb_list = page_to_frame + hash;
    for (auto &iter : *bcb_list)
        if (iter.get_page_id() == page_id) return &iter;

    return nullptr;
}

void BufferManager::insert_bcb(PAGE_ID page_id, FRAME_ID frame_id) {
    int hash = hash_func(page_id);
    auto bcb_list = page_to_frame + hash;
    bcb_list->emplace_back(page_id, frame_id);
}

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