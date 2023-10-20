#include "../include/zalp.h"

namespace zns{
    ZALP::ZALP() {
        lru_list = new std::list<int>();
        lru_map = new std::unordered_map<int, std::list<int>::iterator>();
    }

    ZALP::~ZALP() {
        free(lru_list);
        free(lru_map);
    }

    int ZALP::get_victim() {
        int victim = lru_list->back();
        lru_list->pop_back();
        return victim;
    }

    void ZALP::push(int id) {
        lru_list->push_front(id);
        (*lru_map)[id] = lru_list->begin();
    }

    void ZALP::update(int id) {
        auto iter = (*lru_map)[id];
        lru_list->erase(iter);
        push(id);
    }
}