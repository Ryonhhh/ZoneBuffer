#include <iostream>
#include <ctime>

#include "include/zBuffer.h"
#include "link.h"

using namespace zns;
using namespace std;
clock_t start_t, end_t;

void db_bench(BufferManager *bm, string bench_file) {
    auto ins_vector = Instruction::read_instructions(bench_file);
    int index = 0;
    for (auto &iter : *ins_vector) {
        // cout << std::endl << index++ << ": " << i << endl;
        if (index % (ins_vector->size() / 10) == 0) {
            std::cout << std::endl;
            //bm->zdsm->print_gc_info();
        }
        if (index % (ins_vector->size() / 100) == 0) std::cout << "#";
        index++;
        iter.execute(bm);
    }
    printf("\n");
}

int main() {
    srand((unsigned)time(NULL));
    BufferManager *bm = new BufferManager();
    std::cout<<"start loading..."<<std::endl;
    db_bench(bm, load_db_file);
    bm->hit_count_clear();
    std::cout<<"bench start!"<<std::endl;
    start_t = clock();
    db_bench(bm, test_db_file);
    end_t = clock();
    double t = (end_t - start_t) / CLOCKS_PER_SEC;
    std::ofstream op(bm->output,std::ios::app);
    op << std::endl << "running time" << t << "s" << std::endl;
    return 0;
}