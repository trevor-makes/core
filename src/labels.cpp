// https://github.com/trevor-makes/uMon.git
// Copyright (c) 2022 Trevor Makes

#include "uMon/labels.hpp"

#include <stdlib.h>
#include <string.h>

namespace uMon {

char* Labels::get(uint8_t index, uint8_t& size) const {
  if (index >= entries_) {
    return nullptr;
  }

  // Where in the buffer is index?
  uint8_t offset = 0;
  for (; index > 0; --index) {
    offset += *(buffer_ + offset);
  }

  size = *(buffer_ + offset);
  return buffer_ + offset + sizeof(uint8_t);
}

char* Labels::insert(uint8_t index, uint8_t size) {
  if (index > entries_) {
    return nullptr;
  }

  // How many bytes need to be inserted?
  uint8_t entry_size = sizeof(uint8_t) + size;

  // Where in the buffer is index and where is the end?
  uint8_t offset = 0;
  uint8_t old_size = 0;
  for (uint8_t entry = 1; entry <= entries_; ++entry) {
    old_size += *(buffer_ + old_size);
    if (entry == index) {
      offset = old_size;
    }
  }

  // Abort if buffer is too full
  if (buf_size_ - old_size < entry_size) {
    return nullptr;
  }

  // Move following entries back to make room
  if (offset < old_size) {
    memmove(buffer_ + offset + entry_size, buffer_ + offset, old_size - offset);
  }

  // Insert entry at index
  ++entries_;
  *(buffer_ + offset) = entry_size;
  return buffer_ + offset + sizeof(uint8_t);
}

bool Labels::remove_range(uint8_t first, uint8_t last) {
  if (first > last || last >= entries_) {
    return false;
  }

  // Where in the buffer are the indices and where is the end?
  uint8_t offset_first = 0;
  uint8_t offset_next = 0;
  uint8_t old_size = 0;
  for (uint8_t entry = 0; entry < entries_; ++entry) {
    if (entry == first) {
      offset_first = old_size;
    }
    old_size += *(buffer_ + old_size);
    if (entry == last) {
      offset_next = old_size;
    }
  }

  // Move following entries forward to fill vacancy
  if (offset_next < old_size) {
    memmove(buffer_ + offset_first, buffer_ + offset_next, old_size - offset_next);
  }

  entries_ -= last - first + 1;
  return true;
}

bool Labels::get_index(uint8_t index, const char*& name, uint16_t& addr) const {
  uint8_t size;
  char* entry = get(index, size);
  if (entry != nullptr) {
    addr = *(uint16_t*)entry;
    name = entry + 2;
    return true;
  }
  return false;
}

// Result<bool, uint16_t> or something would be nice...
bool Labels::get_addr(const char* name, uint16_t& addr) const {
  for (uint8_t i = 0; i < entries(); ++i) {
    uint8_t size;
    char* entry = get(i, size);
    if (strcmp(entry + 2, name) == 0) {
      addr = *(uint16_t*)entry;
      return true;
    }
  }
  return false;
}

// Result<bool, const char*> or something would be nice...
bool Labels::get_name(uint16_t addr, const char*& name) const {
  for (uint8_t i = 0; i < entries(); ++i) {
    uint8_t size;
    char* entry = get(i, size);
    if (*(uint16_t*)entry == addr) {
      name = entry + 2;
      return true;
    }
  }
  return false;
}

bool Labels::remove_label(const char* name) {
  for (uint8_t i = 0; i < entries(); ++i) {
    uint8_t size;
    const char* entry = get(i, size);
    if (strcmp(entry + 2, name) == 0) {
      remove(i);
      return true;
    }
  }
  return false;
}

bool Labels::set_label(const char* name, uint16_t addr) {
  remove_label(name);
  char* entry = insert(entries(), strlen(name) + 3);
  if (entry == nullptr) {
    return false;
  }
  *(uint16_t*)entry = addr;
  strcpy(entry + sizeof(uint16_t), name);
  return true;
}

} // namespace uMon
