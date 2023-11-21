#include "../include/common.h"

namespace zns {

    PAGE_ID BCB::get_page_id() const {
        return page_id;
    }

    FRAME_ID BCB::get_frame_id() const {
        return frame_id;
    }

    bool BCB::is_dirty() const {
        return dirty;
    }

    void BCB::set_dirty() {
        dirty = true;
    }

    void BCB::unset_dirty() {
        dirty = false;
    }

    Frame::sptr generate_random_frame() {
        auto frame = std::make_shared<Frame>();
        srand((unsigned)time(NULL));
        for (unsigned int i = 0; i < FRAME_SIZE - 1; i++) {
            frame->field[i] = 'a' + ((int)rand())%26;
        }
        frame->field[FRAME_SIZE - 1] = 0;
        return frame;
    }
}