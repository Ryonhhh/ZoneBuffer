#ifndef ZNS_COMMON
#define ZNS_COMMON

#include <assert.h>
#include <libnvme.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <libzbd/zbd.h>

#include <cstdlib>
#include <malloc.h>
#include <iostream>
#include <list>
#include <memory>
#include <unordered_map>

#include "parameter.h"

namespace zns {

class BCB {
   public:
    BCB(PAGE_ID page_id, FRAME_ID frame_id)
        : page_id(page_id), frame_id(frame_id), dirty(false){};

    PAGE_ID get_page_id() const;

    FRAME_ID get_frame_id() const;

    bool is_dirty() const;

    void set_dirty();

    void unset_dirty();

   private:
    PAGE_ID page_id;
    FRAME_ID frame_id;
    bool dirty;
};

char *generate_random_frame(PAGE_ID page_id);
}  // namespace zns

#endif  // ZNS_COMMON