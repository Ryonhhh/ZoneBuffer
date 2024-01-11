#ifndef ADB_PROJECT_INSTRUCTION_HPP
#define ADB_PROJECT_INSTRUCTION_HPP

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "include/zBuffer.h"

#define load_db_file "/home/wht/YCSB-Gen/dataset.dat"
#define test_db_file "/home/wht/YCSB-Gen/query.dat"
#define INSERT 0
#define SCAN 1
#define UPDATE 2
#define READ 3

namespace zns {
class Instruction {
   public:
    typedef std::shared_ptr<std::vector<Instruction>> vector;

    explicit Instruction(const std::string &s) {
        int sp = s.find(' ');
        std::string a = s.substr(0, sp);
        std::string b = s.substr(sp + 1, s.length());

        if (a == std::string("INSERT")) operate = INSERT;
        if (a == std::string("SCAN")) operate = SCAN;
        if (a == std::string("UPDATE")) operate = UPDATE;
        if (a == std::string("READ")) operate = READ;

        std::stringstream strValue;
        strValue << b;
        strValue >> page_id;
    }

    Instruction(int operate, int page_id)
        : operate(operate), page_id(page_id){};

    friend std::ostream &operator<<(std::ostream &os, const Instruction &i) {
        std::string opt;
        switch (i.operate) {
            case INSERT:
                opt = "INSERT";
                break;
            case SCAN:
                opt = "SCAN";
                break;
            case UPDATE:
                opt = "UPDATE";
                break;
            case READ:
                opt = "READ";
                break;
        }
        // os << "Instruction(" << opt << ", " << i.page_id << ")";
        return os;
    }

    void execute(const BufferManager::sptr &bm) {
        if (operate == INSERT) {
            auto frame = generate_random_frame(page_id);
            bm->fix_new_page(page_id, frame);
        } else if (operate == UPDATE) {
            auto new_frame = generate_random_frame(page_id);
            bm->write_page(page_id, new_frame);
        } else if (operate == READ) {
            bm->read_page(page_id);
        }
    };

    static Instruction::vector read_instructions(const std::string &filename) {
        std::ifstream test_file(filename);
        if (!test_file) {
            test_file.close();
            test_file.clear();
            test_file.open(filename);
        }
        assert(test_file.is_open());
        std::string line;
        auto ins_vector = std::make_shared<std::vector<Instruction>>();
        while (getline(test_file, line)) {
            ins_vector->emplace_back(line);
        }
        test_file.close();
        return ins_vector;
    }

   private:
    int operate;
    PAGE_ID page_id;
};
}  // namespace zns

#endif  // ADB_PROJECT_INSTRUCTION_HPP