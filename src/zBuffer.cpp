#include <cstring>
#include <iostream>
#include <memory>

#include "../include/zBuffer.h"

namespace zns {
BufferManager::BufferManager() {
    zdsm = new ZNSController();
    output=zdsm->output;
    strategy = new ZALP(output);
    lrus = new LRU;
    free_frames_num = DEF_BUF_SIZE;
    cluster_num = zalp_wc ? HC_LV : (zalp ? CLUSTER_NUM : 1);
    hit_count = 0;
}

BufferManager::~BufferManager() {
    clean_buffer();
    delete zdsm;
    delete strategy;
}

int BufferManager::hash_func(PAGE_ID page_id) { return page_id % DEF_BUF_SIZE; }

Frame::sptr BufferManager::read_page(PAGE_ID page_id) {
    int frame_id = fix_page(false, page_id);
    // printf("read frm_id: %d", frame_id);
    auto frame = std::make_shared<Frame>();
    memcpy(frame->field, (buffer + frame_id)->field, FRAME_SIZE);
    char read_id_[8];
    for (long unsigned int i = 0; i < sizeof(PAGE_ID); i++)
        read_id_[i] = (buffer + frame_id)->field[i];
    PAGE_ID read_id = atol(read_id_);
    assert(read_id == page_id);
    return frame;
}

void BufferManager::write_page(PAGE_ID page_id, const Frame::sptr &frame) {
    FRAME_ID frame_id = fix_page(true, page_id);
    // printf("write frm_id: %d", frame_id);
    memcpy((buffer + frame_id)->field, frame->field, FRAME_SIZE);
    set_dirty(frame_id);
}

FRAME_ID BufferManager::fix_page(PAGE_ID page_id) {
    return fix_page(false, page_id);
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
                zdsm->read_page_p(page_id, buffer + frame_id);
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
                        zdsm->read_page_p(page_id, buffer + frame_id) * 0;
                } else if (zalp) {
                    cluster_flag[frame_id] =
                        zdsm->read_page_p(page_id, buffer + frame_id) %
                        cluster_num;
                } else if (cflru) {
                    zdsm->read_page_p(page_id, buffer + frame_id);
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

void BufferManager::fix_new_page(PAGE_ID page_id, const Frame::sptr &frame) {
    FRAME_ID frame_id;
    if (lru) {
        if (free_frames_num == 0) {
            frame_id = select_victim();
        } else {
            frame_id = DEF_BUF_SIZE - free_frames_num;
            free_frames_num--;
        }
        memcpy((buffer + frame_id)->field, frame->field, FRAME_SIZE);
        zdsm->create_new_page(page_id, buffer + frame_id, cluster_num);
        insert_bcb(page_id, frame_id);
        lrus->push(frame_id);
        set_page_id(frame_id, page_id);
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

        memcpy((buffer + frame_id)->field, frame->field, FRAME_SIZE);
        zdsm->create_new_page(page_id, buffer + frame_id, cluster_num);
        if (zalp_wc) {
            cluster_flag[frame_id] = 0;
        } else if (zalp) {
            cluster_flag[frame_id] = page_id % CLUSTER_NUM;
        }
        insert_bcb(page_id, frame_id);
        strategy->push(frame_id, 0);
        set_page_id(frame_id, page_id);
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
                zdsm->write_page_p(victim_page_id, buffer + frame_id,
                                   cluster_num);
            }
            bcb_list->erase(iter);
            break;
        }
    }
    buffer_count[0]++;
    std::cout << std::endl;
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
    std::list<int> *candidate_list, *victim_list;
    candidate_list = new std::list<int>;
    victim_list = new std::list<int>;
    strategy->get_candidate(candidate_list);

    if (zalp_wc) {
        int dirty_cluster[HC_LV] = {};
        int max = 0;
        for (auto &iter : *candidate_list)
            if (cluster_flag[iter] > max)
                max = cluster_flag[iter];
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
    } else if (zalp) {
        int dirty_cluster[CLUSTER_NUM] = {};

        for (auto &iter : *candidate_list) {
            dirty_cluster[cluster_flag[iter]]++;
        }

        cf = -1;
        int cfnum = -1;
        for (int i = 0; i < CLUSTER_NUM; i++) {
            cf = cfnum > dirty_cluster[i] ? cf : i;
            cfnum = cfnum > dirty_cluster[i] ? cfnum : dirty_cluster[i];
        }
        assert(cf != -1);

        for (auto &iter : *candidate_list) {
            if (cluster_flag[iter] == cf)
                victim_list->push_back(iter);
        }
    } 
    else if (cflru) {
        cf = 0;
        for (auto &iter : *candidate_list) {
            victim_list->push_back(iter);
        }
    }
    strategy->update_dirty(victim_list);

    char *write_buffer = reinterpret_cast<char *>(
        memalign(4096, victim_list->size() * FRAME_SIZE));
    PAGE_ID *page_list =
        (PAGE_ID *)malloc(sizeof(PAGE_ID) * victim_list->size());
    int i = 0;
    for (auto &iter : *victim_list) {
        memcpy(write_buffer + i * FRAME_SIZE, (buffer + iter)->field,
               FRAME_SIZE);
        page_list[i] = get_page_id(iter);
        i++;
    }
    zdsm->write_cluster_p(cf, write_buffer, page_list, i, cluster_num);
    //printf("\nvictim cluster %d, flush %d pages\n", cf, i);
    //std::ofstream op(output, std::ios::app);
    //op << "\nvictim cluster " << cf << " flush pages " << i ;
    //op.close();
    buffer_count[cf]++;

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
}

void BufferManager::clean_buffer() {
    // strategy->print_list();
    std::ofstream op(output, std::ios::app);
    std::cout << std::endl << "Cleaning buffer" << std::endl;
    for (const auto &bcb_list : page_to_frame)
        for (const auto &iter : bcb_list)
            if (iter.is_dirty())
                zdsm->write_page_p(iter.get_page_id(),
                                   buffer + iter.get_frame_id(), cluster_num);

    op << std::endl;

    
        for (int i = 0; i < cluster_num; i++) {
            op << buffer_count[i] << " ";
        }
    
    op << std::endl;

    std::cout << "IO count: " << get_io_count() << std::endl;
    std::cout << "hit/query: " << get_hit_count() << std::endl;
    op << "IO count: " << get_io_count() << std::endl;
    op << "hit/query: " << get_hit_count() << std::endl;
    op << "zalp_wc:" << zalp_wc << " zalp:" << zalp << " cflru:" << cflru
       << " lru:" << lru << std::endl;
    op.close();
    zdsm->print_gc_info(cluster_num);
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