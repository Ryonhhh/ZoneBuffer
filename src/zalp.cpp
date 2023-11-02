#include "../include/zalp.h"

namespace zns {
ZALP::ZALP() {
    lru_list = new std::list<int>();
    free_list = new std::list<int>();
    for (int i = 0; i < CLUSTER_NUM; i++)
        dirty_cluster_list[i] = new std::list<int>();

    lru_map = new std::unordered_map<int, std::list<int>::iterator>();
    free_map = new std::unordered_map<int, std::list<int>::iterator>();
    dirty_map = new std::unordered_map<int, std::list<int>::iterator>();
    for (int i = FRAME_SIZE - 1; i >= 0; i--) {
        free_list->push_back(i);
        (*free_map)[i] = free_list->end();
    }
}

ZALP::~ZALP() {
    free(lru_list);
    free(free_list);
    free(lru_map);
    free(free_map);
    free(dirty_map);
}

FRAME_ID ZALP::get_free_frame() {
    FRAME_ID free_frame_id = free_list->back();
    free_list->pop_back();
    return free_frame_id;
}

void ZALP::get_victim(std::list<int>* victim_list) {
    int max_cluster = 0, max = 0;
    for (int i = 0; i < CLUSTER_NUM; i++) {
        if (max < dirty_cluster_list[i]->size()) {
            max = dirty_cluster_list[i]->size();
            max_cluster = i;
        }
    }
    if (max == 0) {
        victim_list->push_back(lru_list->back());
        free_list->push_back(lru_list->back());
        lru_list->pop_back();
    } else
        for (auto& iter : *dirty_cluster_list[max_cluster]) {
            victim_list->push_back(iter);
            free_list->push_back(iter);
            auto it = (*lru_map)[iter];
            lru_list->erase(it);
        }
}

int ZALP::set_dirty(FRAME_ID frame_id, int cf) {
    dirty_cluster_list[cf]->push_back(frame_id);
    if (dirty_cluster_list[cf]->size() >= cluster_gc_num) 
    return cf;
}

void ZALP::push(FRAME_ID id) {
    lru_list->push_front(id);
    (*lru_map)[id] = lru_list->begin();
}

void ZALP::update(FRAME_ID id) {
    auto iter = (*lru_map)[id];
    lru_list->erase(iter);
    push(id);
}
}  // namespace zns