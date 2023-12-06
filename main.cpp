#include <iostream>
#include <ctime>

#include "include/zBuffer.h"
#include "link.h"

using namespace zns;
using namespace std;
clock_t start_t, end_t;

void db_bench(const BufferManager::sptr &bm, string bench_file) {
    auto ins_vector = Instruction::read_instructions(bench_file);
    int index = 0;
    for (auto &iter : *ins_vector) {
        // cout << std::endl << index++ << ": " << i << endl;
        if (index % (ins_vector->size() / 10) == 0) printf("\n");
        if (index % (ins_vector->size() / 100) == 0) printf("#");
        index++;
        iter.execute(bm);
    }
    printf("\n");
}

int main() {
    auto bm = make_shared<BufferManager>();
    db_bench(bm, load_db_file);
    bm->hit_count_clear();
    start_t = clock();
    db_bench(bm, test_db_file);
    end_t = clock();
    double t = (end_t - start_t) / CLOCKS_PER_SEC;
    std::ofstream op(bm->output,std::ios::app);
    op << std::endl << "running time" << t << "s" << std::endl;
    return 0;
}