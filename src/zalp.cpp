#include "../include/zalp.h"

namespace zns {
ZALP::ZALP(std::string out) {
    lList = new List();
    fList = new List();
    cList = new List();
    dList = new List();

    lMap = new HashTable();
    cMap = new HashTable();
    dMap = new HashTable();
    for (int i = 0; i < DEF_BUF_SIZE; i++) {
        fList->L_push_back(i);
    }
    output = out;
}

ZALP::~ZALP() {
    free(lList);
    free(fList);
    free(cList);
    free(dList);
    free(lMap);
    free(cMap);
}

bool ZALP::get_frame(FRAME_ID* rtframe) {
    if (fList->L_size() != 0) {
        *rtframe = fList->L_back();
        fList->L_pop_back();
        return 1;
    } else {
        *rtframe = cList->L_back();
        cList->L_pop_back();
        cMap->H_erase(*rtframe);
        auto iterL = lMap->H_find(*rtframe)->value;
        lList->L_erase(iterL);
        lMap->H_erase(*rtframe);
        return 0;
    }
}

bool ZALP::is_evict() {
    assert(lList->L_size() == dList->L_size() + cList->L_size());
    if (fList->L_size() != 0 || lList->L_size() <= WORK_REG_SIZE || cList->L_size() > WORK_REG_SIZE) return 0;
    auto iterL = lList->L_advance(lList->L_begin(), WORK_REG_SIZE);
    if (dMap->H_find(iterL->data) == nullptr) return 0;
    auto iterD = dMap->H_find(iterL->data)->value;
    return (dList->L_distance(iterD, dList->L_end()) ==
            DEF_BUF_SIZE - WORK_REG_SIZE - 1);
}

void ZALP::get_candidate(std::list<int>* candidate_list) {
    //print_list();
    auto iterL = lList->L_advance(lList->L_begin(), WORK_REG_SIZE);
    auto iterD = dMap->H_find(iterL->data)->value;
    for (; iterD != nullptr; iterD = iterD->next)
        candidate_list->push_back(iterD->data);
}

void ZALP::get_candidate_cflru(std::list<int>* candidate_list) {
    candidate_list->push_back(dList->L_back());
}

void ZALP::set_dirty(FRAME_ID id) {
    auto iterC = cMap->H_find(id)->value;
    cList->L_erase(iterC);
    cMap->H_erase(id);

    if (dMap->H_find(id) == nullptr) {
        dList->L_push_front(id);
        dMap->H_insert(id, dList->L_begin());
    }
}

void ZALP::update_dirty(std::list<int>* victim_list) {
    for (auto& iter : *victim_list) {
        auto iterD = dMap->H_find(iter)->value;
        dList->L_erase(iterD);
        dMap->H_erase(iter);
        auto iterL = lMap->H_find(iter)->value;
        lList->L_erase(iterL);
        lMap->H_erase(iter);
        fList->L_push_back(iter);
    }
}

void ZALP::update_dirty_readhot(std::list<int>* victim_list) {
    for (auto& iter : *victim_list) {
        auto iterD = dMap->H_find(iter)->value;
        dList->L_erase(iterD);
        dMap->H_erase(iter);
        auto iterL = lMap->H_find(iter)->value;
        lList->L_erase(iterL);
        lMap->H_erase(iter);
        fList->L_push_back(iter);
    }
}

void ZALP::push(FRAME_ID id, bool is_dirty) {
    // print_list();
    lList->L_push_front(id);
    lMap->H_insert(id, lList->L_begin());

    if (!is_dirty) {
        cList->L_push_front(id);
        cMap->H_insert(id, cList->L_begin());
    } else {
        dList->L_push_front(id);
        dMap->H_insert(id, dList->L_begin());
    }
}

void ZALP::update(FRAME_ID id, bool is_dirty) {
    //print_list();
    auto iterL = lMap->H_find(id)->value;
    lList->L_erase(iterL);
    lMap->H_erase(id);

    if (!is_dirty) {
        auto iterC = cMap->H_find(id)->value;
        cList->L_erase(iterC);
        cMap->H_erase(id);
    } else {
        auto iterD = dMap->H_find(id)->value;
        dList->L_erase(iterD);
        dMap->H_erase(id);
    }
    push(id, is_dirty);
}

void ZALP::print_list() {
    printf("\nlru_list: ");
    int i = 0;
    for (auto iter = lList->L_begin(); iter != nullptr; iter = iter->next) {
        if (i == WORK_REG_SIZE) printf(" [");
        i++;
        printf("%d->", iter->data);
    }
    printf("]\n\nfree_list: ");
    for (auto iter = fList->L_begin(); iter != nullptr; iter = iter->next)
        printf("%d->", iter->data);
    printf("\n\nclean_list: ");
    for (auto iter = cList->L_begin(); iter != nullptr; iter = iter->next)
        printf("%d->", iter->data);
    printf("\n\ndirty_list: ");
    for (auto iter = dList->L_begin(); iter != nullptr; iter = iter->next)
        printf("%d->", iter->data);
    printf("\n");
    std::ofstream op(output, std::ios::app);
    op << std::endl << "lList: ";
    i = 0;
    for (auto iter = lList->L_begin(); iter != nullptr; iter = iter->next) {
        if (i == WORK_REG_SIZE) op << " [";
        i++;
        op << iter->data << "->";
    }
    op << std::endl << "fList: ";
    for (auto iter = fList->L_begin(); iter != nullptr; iter = iter->next)
        op << iter->data << "->";
    op << std::endl << "cList: ";
    for (auto iter = cList->L_begin(); iter != nullptr; iter = iter->next)
        op << iter->data << "->";
    op << std::endl << "dList: ";
    for (auto iter = dList->L_begin(); iter != nullptr; iter = iter->next)
        op << iter->data << "->";
    op.close();
}

LRU::LRU() {
    lList = new std::list<int>();
    lMap = new std::unordered_map<int, std::list<int>::iterator>();
}

LRU::~LRU() {
    free(lList);
    free(lMap);
}

int LRU::get_victim() {
    int victim = lList->back();
    lList->pop_back();
    return victim;
}

void LRU::push(int id) {
    lList->push_front(id);
    (*lMap)[id] = lList->begin();
}

void LRU::update(int id) {
    auto iter = (*lMap)[id];
    lList->erase(iter);
    push(id);
}
}  // namespace zns