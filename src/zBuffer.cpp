#include <cstring>
#include <iostream>
#include <memory>

#include "../include/zBuffer.h"

namespace zns {
BufferManager::BufferManager() {
    zdsm = new ZNSController();
    strategy = new ZALP;
    free_frames_num = DEF_BUF_SIZE;
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
    auto frame = std::make_shared<Frame>();
    memcpy(frame->field, (buffer + frame_id)->field, FRAME_SIZE);
    return frame;
}

void BufferManager::write_page(PAGE_ID page_id, const Frame::sptr &frame) {
    int frame_id = fix_page(true, page_id);
    memcpy((buffer + frame_id)->field, frame->field, FRAME_SIZE);
    set_dirty(frame_id);
}

FRAME_ID BufferManager::fix_page(PAGE_ID page_id) {
    return fix_page(false, page_id);
}

FRAME_ID BufferManager::fix_page(bool is_write, PAGE_ID page_id) {
    BCB *bcb = get_bcb(page_id);
    std::cout << "    op: " << (is_write ? "w" : "r");

    if (bcb == nullptr) {
        if (zdsm->is_page_valid(page_id)) {
            FRAME_ID frame_id;
            if (free_frames_num == 0) evict_victim();

            frame_id = strategy->get_free_frame();
            free_frames_num--;

            insert_bcb(page_id, frame_id);
            strategy->push(frame_id);
            set_page_id(frame_id, page_id);
            if (!is_write) {
                zdsm->read_page_p(page_id, buffer + frame_id);
            }
            return frame_id;
        } else
            return -1;

    } else {
        FRAME_ID frame_id = bcb->get_frame_id();
        strategy->update(frame_id);
        inc_hit_count();
        return frame_id;
    }
}

void BufferManager::fix_new_page(const Frame::sptr &frame) {
    FRAME_ID frame_id;
    if (free_frames_num == 0) evict_victim();
    frame_id = strategy->get_free_frame();
    free_frames_num--;

    memcpy((buffer + frame_id)->field, frame->field, FRAME_SIZE);
    PAGE_ID page_id = zdsm->create_new_page(buffer + frame_id);
    std::cout << ", page_id: " << page_id << std::endl;
    insert_bcb(page_id, frame_id);
    strategy->push(frame_id);
    set_page_id(frame_id, page_id);
}

int BufferManager::hash_func(PAGE_ID page_id) { return page_id % DEF_BUF_SIZE; }

int BufferManager::evict_victim() {
    std::list<int> *victim_list;
    victim_list = new std::list<int>;
    int cf = strategy->get_victim(victim_list);

    write_pages(cf, victim_list);
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
    free_frames_num += victim_list->size();
}

void BufferManager::write_pages(int cf, std::list<int> *victim_list) {
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
}

void BufferManager::clean_buffer() {
    std::cout << "Cleaning buffer" << std::endl;
    for (const auto &bcb_list : page_to_frame) {
        for (const auto &iter : bcb_list) {
            if (iter.is_dirty()) {
                zdsm->write_page_p(iter.get_page_id(),
                                   buffer + iter.get_frame_id());
                std::cout << "    IO count: " << get_io_count() << std::endl;
            }
        }
    }
}

void BufferManager::set_dirty(int frame_id) {
    // wait for modify
    // add frame id to cluster
    PAGE_ID page_id = get_page_id(frame_id);
    BCB *bcb = get_bcb(page_id);
    bcb->set_dirty();
    int cluster = strategy->set_dirty(frame_id, page_id % 10);
    if (cluster > 0) evict_victim();
}

void BufferManager::unset_dirty(int frame_id) {
    PAGE_ID page_id = get_page_id(frame_id);
    BCB *bcb = get_bcb(page_id);
    bcb->unset_dirty();
}

BCB *BufferManager::get_bcb(PAGE_ID page_id) {
    int hash = hash_func(page_id);
    auto bcb_list = page_to_frame + hash;
    for (auto &iter : *bcb_list) {
        if (iter.get_page_id() == page_id) {
            return &iter;
        }
    }
    return nullptr;
}

void BufferManager::insert_bcb(PAGE_ID page_id, int frame_id) {
    int hash = hash_func(page_id);
    auto bcb_list = page_to_frame + hash;
    bcb_list->emplace_back(page_id, frame_id);
}

int BufferManager::get_page_id(int frame_id) { return frame_to_page[frame_id]; }

void BufferManager::set_page_id(int frame_id, PAGE_ID page_id) {
    frame_to_page[frame_id] = page_id;
}

int BufferManager::get_free_frames_num() { return free_frames_num; }

int BufferManager::get_io_count() { return zdsm->get_io_count(); }

void BufferManager::inc_hit_count() { hit_count++; }

int BufferManager::get_hit_count() { return hit_count; }
}  // namespace zns