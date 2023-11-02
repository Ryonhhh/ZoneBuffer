#include <cstring>
#include <iostream>
#include <memory>

#include "../include/buffer.h"

namespace zns {
BufferManager::BufferManager() {
  zdsm = new ZNSController();
  strategy = new LRUCache(FRAME_SIZE);
  free_frames_num = DEF_BUF_SIZE;
  hit_count = 0;
}

BufferManager::~BufferManager() {
  clean_buffer();
  std::cout << "Total IO count: " << get_io_count() << std::endl;
  std::cout << "Total hit count: " << get_hit_count() << std::endl;
  delete zdsm;
  delete strategy;
}

Frame::sptr BufferManager::read_page(PAGE_ID page_id) {
    BCB *bcb = get_bcb(page_id);
    std::cout << "    op: "
              << "r";
    FRAME_ID frame_id;
    auto frame = std::make_shared<Frame>();
    if (bcb == nullptr) {
        if (zdsm->is_page_valid(page_id)) {
            frame_id = select_victim();
            zdsm->read_page_p(page_id, buffer + frame_id);
            insert_bcb(page_id, frame_id);
            set_page_id(frame_id, page_id);
            strategy->put(frame_id, buffer + frame_id);
        } else {
            std::cout << "not valid page id!" << std::endl;
            exit(0);
        }
    } else {
        frame = strategy->get(frame_id);
        inc_hit_count();
    }
    return frame;
}

void BufferManager::write_page(PAGE_ID page_id,
                               const Frame::sptr &frame) {
  int frame_id = fix_page(true, page_id);
  memcpy((buffer + frame_id)->field, frame->field, FRAME_SIZE);
  set_dirty(frame_id);
}

int BufferManager::hash_func(PAGE_ID page_id) {
  return page_id % DEF_BUF_SIZE;
}

int BufferManager::select_victim() {
  int frame_id = strategy->get_victim();
  PAGE_ID victim_page_id = get_page_id(frame_id);

  int hash = hash_func(victim_page_id);
  auto bcb_list = page_to_frame + hash;
  for (auto iter = bcb_list->begin(); iter != bcb_list->end(); iter++) {
    if (iter->get_page_id() == victim_page_id) {
      if (iter->is_dirty()) {
        zdsm->write_page_p(victim_page_id, buffer + frame_id);
      }
      bcb_list->erase(iter);
      break;
    }
  }
  std::cout << std::endl;
  return frame_id;
}

void BufferManager::clean_buffer() {
  std::cout << "Cleaning buffer" << std::endl;
  for (const auto &bcb_list : page_to_frame) {
    for (const auto &iter : bcb_list) {
      if (iter.is_dirty()) {
        zdsm->write_page_p(iter.get_page_id(), buffer + iter.get_frame_id());
        std::cout << "    IO count: " << get_io_count() << std::endl;
      }
    }
  }
}

void BufferManager::set_dirty(int frame_id) {
  //wait for modify
  //add frame id to cluster
  PAGE_ID page_id = get_page_id(frame_id);
  BCB *bcb = get_bcb(page_id);
  bcb->set_dirty();
}

void BufferManager::unset_dirty(int frame_id) {
  PAGE_ID page_id = get_page_id(frame_id);
  BCB *bcb = get_bcb(page_id);
  bcb->unset_dirty();
}

BCB *BufferManager::get_bcb(PAGE_ID page_id) {
  int hash = hash_func(page_id);
  auto bcb_list = page_to_frame + hash;
  for (auto &iter : *bcb_list) {
    if (iter.get_page_id() == page_id) {
      return &iter;
    }
  }
  return nullptr;
}

void BufferManager::insert_bcb(PAGE_ID page_id, int frame_id) {
  int hash = hash_func(page_id);
  auto bcb_list = page_to_frame + hash;
  bcb_list->emplace_back(page_id, frame_id);
}

int BufferManager::get_page_id(int frame_id) { return frame_to_page[frame_id]; }

void BufferManager::set_page_id(int frame_id, PAGE_ID page_id) {
  frame_to_page[frame_id] = page_id;
}

int BufferManager::get_free_frames_num() { return free_frames_num; }

int BufferManager::get_io_count() { return zdsm->get_io_count(); }

void BufferManager::inc_hit_count() { hit_count++; }

int BufferManager::get_hit_count() { return hit_count; }
}  // namespace zns