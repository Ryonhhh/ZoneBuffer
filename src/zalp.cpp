#include "../include/zalp.h"

namespace zns {
ZALP::ZALP() {
    lru_list = new std::list<int>();
    free_list = new std::list<int>();
    clean_list = new std::list<int>();
    dirty_list = new std::list<int>();

    lru_map = new std::unordered_map<int, std::list<int>::iterator>();
    clean_map = new std::unordered_map<int, std::list<int>::iterator>();
    dirty_map = new std::unordered_map<int, std::list<int>::iterator>();
    for (int i = 0; i < DEF_BUF_SIZE; i++) {
        free_list->push_back(i);
    }
}

ZALP::~ZALP() {
    free(lru_list);
    free(free_list);
    free(clean_list);
    free(dirty_list);
    free(lru_map);
    free(clean_map);
}

bool ZALP::get_frame(FRAME_ID* rtframe) {
    if (free_list->size() != 0) {
        *rtframe = free_list->back();
        free_list->pop_back();
        return 1;
    } else {
        *rtframe = clean_list->back();
        clean_list->pop_back();
        auto iterL = (*lru_map)[*rtframe];
        lru_list->erase(iterL);
        lru_map->erase(*rtframe);
        return 0;
    }
}

bool ZALP::is_evict() {
    if (free_list->size() != 0) return 0;
    FRAME_ID last_clean = clean_list->back();
    auto it = (*lru_map)[last_clean];
    return (std::distance(lru_list->begin(), it) < WORK_REG_SIZE);
}

void ZALP::get_candidate(std::list<int>* candidate_list) {
    for (auto& iter : *dirty_list) candidate_list->push_back(iter);
}

void ZALP::set_dirty(FRAME_ID id) {
    auto iterC = (*clean_map)[id];
    clean_list->erase(iterC);
    clean_map->erase(id);

    auto iterL = (*lru_map)[id];
    if (std::distance(lru_list->begin(), iterL) > WORK_REG_SIZE &&
        !lfind(dirty_list, id)) {
        dirty_list->push_front(id);
        (*dirty_map)[id] = dirty_list->begin();
    }
}

void ZALP::update_dirty(std::list<int>* victim_list) {
    for (auto& iter : *victim_list) {
        auto iterD = (*dirty_map)[iter];
        dirty_list->erase(iterD);
        dirty_map->erase(iter);
        auto iterL = (*lru_map)[iter];
        lru_list->erase(iterL);
        lru_map->erase(iter);
        free_list->push_back(iter);
    }
}

void ZALP::push(FRAME_ID id, bool is_dirty) {
    lru_list->push_front(id);
    (*lru_map)[id] = lru_list->begin();

    if (!is_dirty) {
        clean_list->push_front(id);
        (*clean_map)[id] = clean_list->begin();
    }

    auto it = lru_list->begin();
    std::advance(it, WORK_REG_SIZE);
    if (!lfind(clean_list, *it) && !lfind(dirty_list, *it)) {
        dirty_list->push_front(*it);
        assert((*dirty_map).find(id)==(*dirty_map).end());
        (*dirty_map)[*it] = dirty_list->begin();
    }
}

void ZALP::update(FRAME_ID id, bool is_dirty) {
    auto iterL = (*lru_map)[id];
    lru_list->erase(iterL);
    lru_map->erase(id);

    if (!is_dirty) {
        auto iterC = (*clean_map)[id];
        clean_list->erase(iterC);
        clean_map->erase(id);
    }
    if (lfind(dirty_list, id)) {
        auto iterD = (*dirty_map)[id];
        dirty_list->erase(iterD);
        dirty_map->erase(id);
    }
    push(id, is_dirty);
}

bool ZALP::lfind(std::list<FRAME_ID>* list, FRAME_ID frame_id) {
    for (auto& iter : *list)
        if (iter == frame_id) return 1;
    return 0;
}

void ZALP::print_list() {
    printf("\nlru_list: ");
    int i = 0;
    for (auto& iter : *lru_list) {
        if (i == WORK_REG_SIZE) printf(" [");
        i++;
        printf("%d->", iter);
    }
    printf("]\n\nfree_list: ");
    for (auto& iter : *free_list) printf("%d->", iter);
    printf("\n\nclean_list: ");
    for (auto& iter : *clean_list) printf("%d->", iter);
    printf("\n\ndirty_list: ");
    for (auto& iter : *dirty_list) printf("%d->", iter);
    //printf("\n\ndirty_map: ");
    //for (auto iter : *dirty_map) printf("[%d]=%d ", iter.first, *(iter.second));
}

 LRU::LRU() {
        lru_list = new std::list<int>();
        lru_map = new std::unordered_map<int, std::list<int>::iterator>();
    }

    LRU::~LRU() {
        free(lru_list);
        free(lru_map);
    }

    int LRU::get_victim() {
        int victim = lru_list->back();
        lru_list->pop_back();
        return victim;
    }

    void LRU::push(int id) {
        lru_list->push_front(id);
        (*lru_map)[id] = lru_list->begin();
    }

    void LRU::update(int id) {
        auto iter = (*lru_map)[id];
        lru_list->erase(iter);
        push(id);
    }
}  // namespace zns