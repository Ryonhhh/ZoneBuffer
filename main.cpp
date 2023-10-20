#include <iostream>

#include "include/zBuffer.h"
#include "link.h"

using namespace zns;
using namespace std;

void init_db(const BufferManager::sptr &bm) {
  for (unsigned int j = 1; j <= 1050; j++) {
    auto frame = generate_random_frame();
    bm->fix_new_page(frame);
  }
}

void run_test(const BufferManager::sptr &bm) {
  auto ins_vector = Instruction::read_instructions("./data-5w-50w-zipf.txt");
  int index = 0;
  for (auto &i : *ins_vector) {
    cout << index++ << ": " << i << endl;
    i.execute(bm);
  }
}

int main() {
  auto bm = make_shared<BufferManager>();
  init_db(bm);
  run_test(bm);
  return 0;
}