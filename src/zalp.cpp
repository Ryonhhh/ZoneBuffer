#include "../include/zalp.h"

namespace zns {
ZALP::ZALP(std::string out) {
    lList = new std::list<FRAME_ID>();
    fList = new std::list<FRAME_ID>();
    cList = new std::list<FRAME_ID>();
    dList = new std::list<FRAME_ID>();

    for (int i = 0; i < DEF_BUF_SIZE; i++) {
        fList->push_back(i);
    }
    output = out;
}

ZALP::~ZALP() {
    free(lList);
    free(fList);
    free(cList);
    free(dList);
}

bool ZALP::get_frame(FRAME_ID* rtframe) {
    if (fList->size() != 0) {
        *rtframe = fList->back();
        fList->pop_back();
        return 1;
    } else {
        assert(cList->size() != 0);
        *rtframe = cList->back();
        cList->pop_back();
        cMap.erase(*rtframe);
        LIST_ITER iterL = lMap.find(*rtframe);
        lList->erase(iterL);

        lMap.erase(*rtframe);
        //
        return 0;
    }
}

void ZALP::is_correct() {
    assert(lList->size() == dList->size() + cList->size());
    LIST_ITER pos = lList->begin(), p = cList->begin(), q = dList->begin();
    int posi = 0, pi = 0, qi = 0;
    for (; pos != lList->end(); pos++, posi++) {
        if (*pos == *p) {
            p++;
            pi++;
        } else if (*pos == *q) {
            q++;
            qi++;
        } else {
            std::cout << *pos << " " << posi << " " << *p << " " << pi << " "
                      << *q << " " << qi << std::endl;
            print_list();
            assert(0);
        }
    }
    assert(p == cList->end() && q == dList->end());
}

bool ZALP::is_evict() {
    if (fList->size() != 0 || lList->size() <= WORK_REG_SIZE ||
        cList->size() > WORK_REG_SIZE)
        return 0;

    auto iterC = cList->back();
    LIST_ITER p;
    if (!cMap.find(iterC, p)) return true;
    LIST_ITER iterL = lMap.find(iterC);

    return (std::distance(lList->begin(), iterL) < WORK_REG_SIZE);
}

void ZALP::get_candidate(std::list<int>* candidate_list) {
    LIST_ITER iterD = prev(dList->end());
    for (int i = 0; i < DEF_BUF_SIZE - WORK_REG_SIZE; i++) {
        candidate_list->push_front(*iterD);
        iterD = prev(iterD);
    }
}

void ZALP::get_candidate_cflru(std::list<int>* candidate_list) {
    candidate_list->push_back(dList->back());
}

void ZALP::set_dirty(FRAME_ID id) {
    LIST_ITER p;
    if (!dMap.find(id, p)) {
        dList->push_front(id);
        dMap.insert(id, dList->begin());
    }
    if (cMap.find(id, p)) {
        cList->erase(p);
        cMap.erase(id);
    }
}

void ZALP::update_dirty(std::list<int>* victim_list) {
    for (auto& iter : *victim_list) {
        LIST_ITER iterD = dMap.find(iter);
        dList->erase(iterD);
        dMap.erase(iter);
        LIST_ITER iterL = lMap.find(iter);
        lList->erase(iterL);
        lMap.erase(iter);
        fList->push_back(iter);
    }
}

void ZALP::dirty_to_clean(std::list<int>* victim_list) {
    for (auto& iter : *victim_list) {
        LIST_ITER iterD = dMap.find(iter);
        dList->erase(iterD);
        dMap.erase(iter);
        LIST_ITER p;
        assert((cMap.find(iter, p) == false));
        cList->push_back(iter);
        cMap.insert(iter, prev(cList->end()));
    }
}

void ZALP::push(FRAME_ID id, bool is_dirty) {
    lList->push_front(id);
    lMap.insert(id, lList->begin());

    if (!is_dirty) {
        cList->push_front(id);
        cMap.insert(id, cList->begin());
    } else {
        dList->push_front(id);
        dMap.insert(id, dList->begin());
    }
}

void ZALP::update(FRAME_ID id, bool is_dirty) {
    LIST_ITER iterL = lMap.find(id);
    lList->erase(iterL);
    lMap.erase(id);

    LIST_ITER p;
    if (cMap.find(id, p)) {
        cList->erase(p);
        cMap.erase(id);
    } else if (dMap.find(id, p)) {
        dList->erase(p);
        dMap.erase(id);
    } else {
        assert(0);
    }
    push(id, is_dirty);
}

void ZALP::print_list() {
    printf("\nlru_list: ");
    int i = 0;
    for (auto& iter : *lList) {
        if (i == WORK_REG_SIZE) printf("\n [");
        i++;
        std::cout << iter << "->" << std::flush;
    }
    printf("]\n\nfree_list: ");
    for (auto& iter : *fList) {
        std::cout << iter << "->" << std::flush;
    }

    printf("\n\ndirty_list: ");
    for (auto& iter : *dList) {
        std::cout << iter << "->" << std::flush;
    }
    printf("\n\nclean_list: ");
    for (auto& iter : *cList) {
        std::cout << iter << "->" << std::flush;
    }
    printf("\n\n\n");
    /*
    std::ofstream op(output, std::ios::app);
    op << std::endl << "lList: ";
    i = 0;
    for (auto iter = lList->begin(); iter != nullptr; iter = iter->next) {
        if (i == WORK_REG_SIZE) op << " [";
        i++;
        op << iter << "->";
    }
    op << std::endl << "fList: ";
    for (auto iter = fList->begin(); iter != nullptr; iter = iter->next)
        op << iter << "->";
    op << std::endl << "cList: ";
    for (auto iter = cList->begin(); iter != nullptr; iter = iter->next)
        op << iter << "->";
    op << std::endl << "dList: ";
    for (auto iter = dList->begin(); iter != nullptr; iter = iter->next)
        op << iter << "->";
    op.close();
    */
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