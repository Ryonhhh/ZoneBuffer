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

FRAME_ID ZALP::get_frame() {
    FRAME_ID rtframe;
    if (free_list->size() != 0) {
        rtframe = free_list->back();
        free_list->pop_back();
    } else {
        rtframe = clean_list->back();
        clean_list->pop_back();
        auto iterL = (*lru_map)[rtframe];
        lru_list->erase(iterL);
    }
    return rtframe;
}

bool ZALP::is_evict() {
    if (free_list->size() != 0) return 0;
    FRAME_ID last_clean = clean_list->back();
    auto it = (*lru_map)[last_clean];
    return (std::distance(lru_list->begin(), it) > WORK_REG_SIZE);
}

void ZALP::get_candidate(std::list<int>* candidate_list) {
    for (auto& iter : *dirty_list) candidate_list->push_back(iter);
}

void ZALP::set_dirty(FRAME_ID id) {
    auto iterC = (*clean_map)[id];
    clean_list->erase(iterC);
    
    auto iterL = (*lru_map)[id];
    if (std::distance(lru_list->begin(), iterL) > WORK_REG_SIZE) {
        dirty_list->push_back(id);
        (*dirty_map)[id] = dirty_list->end();
    }
}

void ZALP::update_dirty(std::list<int>* victim_list) {
    for (auto& iter : *victim_list) {
        auto iterD = (*dirty_map)[iter];
        dirty_list->erase(iterD);
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
    if ((*clean_map).find(*it) == (*clean_map).end()) {
        dirty_list->push_front(*it);
        (*dirty_map)[id] = dirty_list->begin();
    }
}

void ZALP::update(FRAME_ID id, bool is_dirty) {
    auto iterL = (*lru_map)[id];
    lru_list->erase(iterL);

    if (!is_dirty) {
        auto iterC = (*clean_map)[id];
        clean_list->erase(iterC);
    }
    if ((*dirty_map).find(id) != (*dirty_map).end()) {
        auto iterD = (*dirty_map)[id];
        dirty_list->erase(iterD);
    }
    push(id, is_dirty);
}

void ZALP::print_list() {
    printf("\n\nlru_list: ");
    for (auto& iter : *lru_list) printf("%d->", iter);
    printf("\n\nfree_list: ");
    for (auto& iter : *free_list) printf("%d->", iter);
    printf("\n\nclean_list: ");
    for (auto& iter : *clean_list) printf("%d->", iter);
    printf("\n\ndirty_list: ");
    for (auto& iter : *dirty_list) printf("%d->", iter);
}
}  // namespace zns