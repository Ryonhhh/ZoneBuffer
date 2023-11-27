#include <cstring>
#include <iostream>
#include <memory>

#include "../include/zBuffer.h"

namespace zns {
BufferManager::BufferManager() {
    zdsm = new ZNSController();
    strategy = new ZALP;
    hit_count = 0;
}

BufferManager::~BufferManager() {
    clean_buffer();
    std::cout << "Total IO count: " << get_io_count() << std::endl;
    std::cout << "Total hit count: " << get_hit_count() << std::endl;
    delete zdsm;
    delete strategy;
}

Frame::sptr BufferManager::read_page(PAGE_ID page_id) {
    int frame_id = fix_page(false, page_id);
    //printf("read frm_id: %d", frame_id);
    auto frame = std::make_shared<Frame>();
    memcpy(frame->field, (buffer + frame_id)->field, FRAME_SIZE);
    return frame;
}

void BufferManager::write_page(PAGE_ID page_id, const Frame::sptr &frame) {
    FRAME_ID frame_id = fix_page(true, page_id);
    //printf("write frm_id: %d", frame_id);
    memcpy((buffer + frame_id)->field, frame->field, FRAME_SIZE);
    set_dirty(frame_id);
}

FRAME_ID BufferManager::fix_page(PAGE_ID page_id) {
    return fix_page(false, page_id);
}

FRAME_ID BufferManager::fix_page(bool is_write, PAGE_ID page_id) {
    BCB *bcb = get_bcb(page_id);
    // std::cout << "op: " << (is_write ? "w " : "r ");

    FRAME_ID frame_id;
    if (bcb == nullptr) {
        // if (zdsm->is_page_valid(page_id)) {
        if (strategy->is_evict()) evict_victim();
        frame_id = strategy->get_frame();

        insert_bcb(page_id, frame_id);
        strategy->push(frame_id, 0);
        set_page_id(frame_id, page_id);
        if (!is_write)
            (buffer + frame_id)->cluster_flag =
                zdsm->read_page_p(page_id, buffer + frame_id);
        //} else
        //    return -1;
    } else {
        frame_id = bcb->get_frame_id();
        strategy->update(frame_id, bcb->is_dirty());
        inc_hit_count();
    }
    return frame_id;
}

void BufferManager::fix_new_page(PAGE_ID page_id, const Frame::sptr &frame) {
    FRAME_ID frame_id;
    if (strategy->is_evict()) evict_victim();
    frame_id = strategy->get_frame();

    memcpy((buffer + frame_id)->field, frame->field, FRAME_SIZE);
    zdsm->create_new_page(page_id, buffer + frame_id);
    (buffer + frame_id)->cluster_flag = 0;
    std::cout << "\nnew page_id: " << page_id << std::endl;
    insert_bcb(page_id, frame_id);
    strategy->push(frame_id, 0);
    set_page_id(frame_id, page_id);
}

int BufferManager::hash_func(PAGE_ID page_id) { return page_id % DEF_BUF_SIZE; }

void BufferManager::evict_victim() {
    std::list<int> *candidate_list, *victim_list;
    candidate_list = new std::list<int>;
    victim_list = new std::list<int>;
    strategy->get_candidate(candidate_list);
    int dirty_cluster[CLUSTER_NUM];

    for (auto &iter : *candidate_list) {
        dirty_cluster[(buffer + iter)->cluster_flag]++;
    }

    int cf = -1, cfnum = -1;
    for (int i = 0; i < CLUSTER_NUM; i++) {
        cf = cfnum > dirty_cluster[i] ? cf : i;
        cfnum = cfnum > dirty_cluster[i] ? cfnum : dirty_cluster[i];
    }
    assert(cf!=-1);

    for (auto &iter : *candidate_list) {
        if((buffer + iter)->cluster_flag==cf) victim_list->push_back(iter);
    }
    strategy->update_dirty(victim_list);

    char *write_buffer =
        (char *)malloc(sizeof(char) * FRAME_SIZE * victim_list->size());
    PAGE_ID *page_list =
        (PAGE_ID *)malloc(sizeof(PAGE_ID) * victim_list->size());
    int i = 0;
    for (auto &iter : *victim_list) {
        memcpy(write_buffer + i * FRAME_SIZE, buffer + iter, FRAME_SIZE);
        page_list[i] = get_page_id(iter);
        i++;
    }
    zdsm->write_cluster_p(cf, write_buffer, page_list, i);
    if (i != 1) {
        printf("\nvictim cluster %d, flush %d pages\n", cf, i);
        buffer_count[cf]++;
    }
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
    strategy->print_list();
    std::cout << std::endl << "Cleaning buffer" << std::endl;
    for (const auto &bcb_list : page_to_frame)
        for (const auto &iter : bcb_list)
            if (iter.is_dirty())
                zdsm->write_page_p(iter.get_page_id(),
                                   buffer + iter.get_frame_id());

    for (int i = 0; i < CLUSTER_NUM; i++) printf("%d ", buffer_count[i]);
}

void BufferManager::set_dirty(int frame_id) {
    PAGE_ID page_id = get_page_id(frame_id);
    BCB *bcb = get_bcb(page_id);
    assert(bcb != nullptr);
    if (!bcb->is_dirty()) {
        bcb->set_dirty();
        strategy->set_dirty(frame_id);
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

PAGE_ID BufferManager::get_page_id(FRAME_ID frame_id) { return frame_to_page[frame_id]; }

void BufferManager::set_page_id(FRAME_ID frame_id, PAGE_ID page_id) {
    frame_to_page[frame_id] = page_id;
}

int BufferManager::get_free_frames_num() { return free_frames_num; }

int BufferManager::get_io_count() { return zdsm->get_io_count(); }

void BufferManager::inc_hit_count() { hit_count++; }

int BufferManager::get_hit_count() { return hit_count; }
}  // namespace zns