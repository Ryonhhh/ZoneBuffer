#include <iostream>

#include "include/zBuffer.h"
#include "link.h"

using namespace zns;
using namespace std;

void db_bench(const BufferManager::sptr &bm, string bench_file) {
    auto ins_vector = Instruction::read_instructions(bench_file);
    int index = 0;
    for (auto &i : *ins_vector) {
        cout << std::endl << index++ << ": " << i << endl;
        i.execute(bm);
    }
}

int main() {
    auto bm = make_shared<BufferManager>();
    db_bench(bm, load_db_file);
    db_bench(bm, test_db_file);
    return 0;
}