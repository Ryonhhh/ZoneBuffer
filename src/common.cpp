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

    Frame::sptr generate_random_frame(PAGE_ID page_id) {
        auto frame = std::make_shared<Frame>();
        long unsigned int i = sizeof(PAGE_ID);
        memcpy(frame->field, (char*)&page_id, sizeof(PAGE_ID));
        for (; i < FRAME_SIZE - 1; i++) {
            frame->field[i] = 'a';
        }
        frame->field[i] = 0;
        return frame;
    }
    }  // namespace zns