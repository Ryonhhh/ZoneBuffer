#include "../include/common.h"

namespace zns {

PAGE_ID BCB::get_page_id() const { return page_id; }

FRAME_ID BCB::get_frame_id() const { return frame_id; }

bool BCB::is_dirty() const { return dirty; }

void BCB::set_dirty() { dirty = true; }

void BCB::unset_dirty() { dirty = false; }

char *generate_random_frame(PAGE_ID page_id) {
    char *frame = reinterpret_cast<char*>(memalign(4096, PAGE_SIZE));
    long unsigned int i = sizeof(PAGE_ID);
    memcpy(frame, (char*)&page_id, sizeof(PAGE_ID));
    for (; i < FRAME_SIZE - 1; i++) {
        frame[i] = 'a';
    }
    frame[i] = 0;
    return frame;
}
}  // namespace zns